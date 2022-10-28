// Copyright (C) 1999-2000 Id Software, Inc.
//
//
// ui_gameinfo.c
//

#include "ui_local.h"

#define MAX_MAPS 4096
#define MAP_LIST_BUFFER ( MAX_MAPS * 30 )
#define ARENA_LIST_BUFFER ( MAX_MAPS * 30 )

#define GAMEINFO_DEBUG UI_GameInfoDebug()
#define GAMEINFO_DEBUG_PRINT( msg ) if ( GAMEINFO_DEBUG ) Com_Printf msg

/*
===============
UI_GameInfoDebug
===============
*/
static qboolean UI_GameInfoDebug( void ) {
	return trap_Cvar_VariableValue( "ui_gameInfoDebug" ) ? qtrue : qfalse;
}

/*
=======================================================================

INFO PARSING

=======================================================================
*/

/*
===============
UI_ReadInfoFile

Reads arena or bot file into memory with safe null termination.
===============
*/
static void UI_ReadInfoFile( const char *path, char *buffer, unsigned int bufSize ) {
	int len;
	fileHandle_t f;
	*buffer = '\0';

	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( !f ) {
		GAMEINFO_DEBUG_PRINT( ( "info file not found: %s\n", path ) );
		return;
	}
	if ( len < 0 || len >= bufSize ) {
		GAMEINFO_DEBUG_PRINT( ( "info file too large: %s is %i, max allowed is %i", path, len, bufSize ) );
		trap_FS_FCloseFile( f );
		return;
	}

	memset( buffer, 0, len );
	trap_FS_Read( buffer, len, f );
	buffer[len] = '\0';
	trap_FS_FCloseFile( f );
}

typedef struct {
	char info[MAX_INFO_STRING];
	char error[128];
} infoParseResult_t;

/*
===============
UI_ParseInfo

Parse a single entry out of an arena or bot file.

Returns qtrue on success. In this case, function can be called again to attempt parsing
another entry. If error string output is non-empty, it represents a warning message.

Returns qfalse on error or end of file. Error string output will be empty on normal
end of file, or non-empty if there is a specific error message.
===============
*/
qboolean UI_ParseInfo( char **ptr, infoParseResult_t *out ) {
	int		i;
	char	*token;
	char	key[MAX_TOKEN_CHARS];
	static const char badChars[] = { '"', '\\', ';' };

	out->info[0] = '\0';
	out->error[0] = '\0';

	token = COM_Parse( ptr );
	if ( !token[0] ) {
		return qfalse;
	}
	if ( strcmp( token, "{" ) ) {
		Q_strncpyz( out->error, "Missing { in info file", sizeof( out->error ) );
		return qfalse;
	}

	while ( 1 ) {
		token = COM_ParseExt( ptr, qtrue );
		if ( !token[0] ) {
			Q_strncpyz( out->error, "Unexpected end of info file", sizeof( out->error ) );
			return qfalse;
		}
		if ( !strcmp( token, "}" ) ) {
			break;
		}
		Q_strncpyz( key, token, sizeof( key ) );

		token = COM_ParseExt( ptr, qfalse );
		if ( !token[0] ) {
			strcpy( token, "<NULL>" );
		}

		// check for bad characters here to avoid console spam from Info_SetValueForKey
		for ( i = 0; i < sizeof( badChars ); ++i ) {
			if ( strchr( key, badChars[i] ) || strchr( token, badChars[i] ) ) {
				Com_sprintf( out->error, sizeof( out->error ), "Bad character '%c' in info file key/value", badChars[i] );
				break;
			}
		}
		if ( i != sizeof( badChars ) ) {
			continue;
		}

		Info_SetValueForKey( out->info, key, token );
	}

	return qtrue;
}

/*
===============
UI_ParseInfoForMap

Attempts to retrieve arena info for a specific map from arena file.
Returns qtrue on success, qfalse on error or map not found.
===============
*/
qboolean UI_ParseInfoForMap( const char *arenaData, const char *mapName, infoParseResult_t *out ) {
	while ( UI_ParseInfo( (char **)&arenaData, out ) ) {
		if ( !Q_stricmp( Info_ValueForKey( out->info, "map" ), mapName ) ) {
			return qtrue;
		}
	}
	
	return qfalse;
}

/*
=======================================================================

CTF SUPPORT CHECK

=======================================================================
*/

#define PARSE_BUFFER_SIZE 24000
#define PARSE_BUFFER_SIZE_NT ( PARSE_BUFFER_SIZE + 1 )

typedef struct {
	char s[PARSE_BUFFER_SIZE_NT];
} parseBuffer_t;

typedef struct {
	parseBuffer_t *buf;
	char *buf_p;
	fileHandle_t f;
	unsigned int fileRemaining;
} bufferedParse_t;

/*
===============
UI_InitBufferedParse

Initializes buffered parsed structure, which is used to read entities without allocating
memory for the entire entity block beforehand. Avoids the need for a large static buffer,
which would waste memory and increase the chance of com_hunkmegs errors on the original client.
===============
*/
static bufferedParse_t UI_InitBufferedParse( parseBuffer_t *buf, fileHandle_t f, unsigned int size ) {
	bufferedParse_t bp;
	bp.buf = buf;
	bp.buf_p = buf->s + PARSE_BUFFER_SIZE;	// start pointer at end of buffer to force read on first
											// UI_GetEntityToken call
	bp.f = f;
	bp.fileRemaining = size;
	*bp.buf_p = '\0';
	return bp;
}

/*
===============
UI_GetEntityToken

Substitute for trap_GetEntityToken in the game module.
===============
*/
static qboolean UI_GetEntityToken( bufferedParse_t *bp, char *buffer, int bufferSize ) {
	char *token;

	// check for reading new data
	if ( bp->buf_p && bp->fileRemaining ) {
		unsigned int readSize = bp->buf_p - bp->buf->s;		// amount already read from buffer,
				// which is also the amount of space available for new data
		unsigned int unreadSize = PARSE_BUFFER_SIZE - readSize;	// amount of data left in buffer

		if ( unreadSize < 2000 ) {
			if ( readSize > bp->fileRemaining ) {
				readSize = bp->fileRemaining;
			}

			memcpy( bp->buf->s, bp->buf_p, unreadSize );
			memset( bp->buf->s + unreadSize, 0, readSize + 1 );
			trap_FS_Read( bp->buf->s + unreadSize, readSize, bp->f );
			bp->fileRemaining -= readSize;
			bp->buf_p = bp->buf->s;
		}
	}

	// read token
	token = COM_Parse( &bp->buf_p );
	Q_strncpyz( buffer, token, bufferSize );
	if ( !bp->buf_p && !token[0] ) {
		return qfalse;
	} else {
		return qtrue;
	}
}

/*
===============
UI_ParseSpawnVars

Parses a single entity looking for CTF flags, incrementing counts if needed.

Returns qfalse if reached the end of entities, or on error. Returns qtrue if valid
to call again to attempt parsing more entities.

Based on G_ParseSpawnVars.
===============
*/
static qboolean UI_ParseSpawnVars( bufferedParse_t *bp, int *redFlags, int *blueFlags ) {
	char keyname[MAX_TOKEN_CHARS];
	char token[MAX_TOKEN_CHARS];
	char classname[MAX_TOKEN_CHARS];
	classname[0] = '\0';

	// parse the opening brace
	if ( !UI_GetEntityToken( bp, token, sizeof( token ) ) ) {
		// end of entities
		return qfalse;
	}
	if ( token[0] != '{' ) {
		GAMEINFO_DEBUG_PRINT( ( "UI_ParseSpawnVars: found %s when expecting {\n", token ) );
		return qfalse;
	}

	// go through all the key / value pairs
	while ( 1 ) {
		// parse key
		if ( !UI_GetEntityToken( bp, keyname, sizeof( keyname ) ) ) {
			GAMEINFO_DEBUG_PRINT( ( "UI_ParseSpawnVars: EOF without closing brace\n" ) );
			return qfalse;
		}

		if ( keyname[0] == '}' ) {
			break;
		}

		// parse value
		if ( !UI_GetEntityToken( bp, token, sizeof( token ) ) ) {
			GAMEINFO_DEBUG_PRINT( ( "UI_ParseSpawnVars: EOF without closing brace\n" ) );
			return qfalse;
		}
		if ( token[0] == '}' ) {
			GAMEINFO_DEBUG_PRINT( ( "UI_ParseSpawnVars: closing brace without data\n" ) );
			return qfalse;
		}

		if ( !Q_stricmp( keyname, "classname" ) ) {
			Q_strncpyz( classname, token, sizeof( classname ) );
		}
	}

	// if this is a flag, increment counts
	if ( !strcmp( classname, "team_CTF_redflag" ) ) {
		++*redFlags;
	} else if ( !strcmp( classname, "team_CTF_blueflag" ) ) {
		++*blueFlags;
	}

	return qtrue;
}

/*
===============
UI_DecodeInt

Read values from bsp in an endian-safe way.
===============
*/
static unsigned int UI_DecodeInt( const char *buffer ) {
	const unsigned char *b = (unsigned char *)buffer;
	return (unsigned)b[0] + ( (unsigned)b[1] << 8 ) + ( (unsigned)b[2] << 16 ) + ( (unsigned)b[3] << 24 );
}

/*
===============
UI_CheckCTFSupportHandle

Given a handle to an open bsp file, returns CTF support status.
===============
*/
static qboolean UI_CheckCTFSupportHandle( unsigned int fileLength, fileHandle_t f ) {
	parseBuffer_t buf;
	unsigned int position;
	unsigned int entityOffset;
	unsigned int entityLength;

	// read bsp header
	if ( fileLength < 144 ) {
		GAMEINFO_DEBUG_PRINT( ( "UI_CheckCTFSupport: invalid bsp file length\n" ) );
		return qfalse;
	}
	memset( buf.s, 0, 144 );
	trap_FS_Read( buf.s, 144, f );
	entityOffset = UI_DecodeInt( &buf.s[8] );
	entityLength = UI_DecodeInt( &buf.s[12] );

	if ( entityOffset > 250000000 || entityLength > 2500000 || entityOffset + entityLength > fileLength ) {
		GAMEINFO_DEBUG_PRINT( ( "UI_CheckCTFSupport: invalid bsp entity lump values\n" ) );
		return qfalse;
	}

	// skip to entity segment
	position = 144;
	while ( position < entityOffset ) {
		int increment = entityOffset - position;
		if ( increment > sizeof( buf.s ) ) {
			increment = sizeof( buf.s );
		}
		trap_FS_Read( buf.s, increment, f );
		position += increment;
	}

	// parse entities to determine number of flags
	{
		int redFlags = 0;
		int blueFlags = 0;
		bufferedParse_t bp = UI_InitBufferedParse( &buf, f, entityLength );
		while ( UI_ParseSpawnVars( &bp, &redFlags, &blueFlags ) ) {
		}

		GAMEINFO_DEBUG_PRINT( ( "UI_CheckCTFSupport: fileLength(%i) entityOffset(%i) entityLength(%i) "
					"redFlags(%i) blueFlags(%i)\n", fileLength, entityOffset, entityLength, redFlags, blueFlags ) );

		if ( redFlags > 0 && blueFlags > 0 ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
===============
UI_CheckCTFSupport

Given a path to a bsp file, returns ctf support status.
===============
*/
static qboolean UI_CheckCTFSupport( const char *path ) {
	qboolean result;
	fileHandle_t f = 0;
	int length = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( !f ) {
		GAMEINFO_DEBUG_PRINT( ( "UI_CheckCTFSupport: failed to open bsp\n" ) );
		trap_FS_FCloseFile( f );
		return qfalse;
	}

	result = UI_CheckCTFSupportHandle( length, f );
	trap_FS_FCloseFile( f );
	return result;
}

/*
=======================================================================

MAP INDEXING

=======================================================================
*/

#define MAPFLAG_RAW_ARENA_PATH 1			// don't prepend "scripts/" to the arena file path
#define MAPFLAG_SINGLE_PLAYER_SUPPORT 2		// display map in single player menu
#define MAPFLAG_CTF_ARENA 4					// map has ctf support according to arena file
#define MAPFLAG_CTF_CHECKED 8				// performed entity-based ctf check
#define MAPFLAG_CTF_SUPPORTED 16			// map has ctf support according to entity check

typedef struct {
	const char *name;
	const char *arenaPath;
	char flags;
	char sortValue;
	unsigned char recommendedPlayers;
} mapEntry_t;

static struct {
	int mapCount;
	int singlePlayerMapCount;
	mapEntry_t mapList[MAX_MAPS];
	char mapListBuffer[MAP_LIST_BUFFER];
} ui_maps;

/*
===============
UI_CheckStockMap

Returns:
-1 for single player map which should be excluded from listings.
0 for custom map.
> 0 for stock maps, which should be listed ahead of other maps. Value indicates listing order.

For consistency, stock maps are listed ahead of other maps, in the same order as the original game.
===============
*/
static int UI_CheckStockMap( const char *name ) {
	if ( !Q_stricmp( name, "holodeck" ) ||
			!Q_stricmp( name, "tutorial" ) ) {
		return -1;
	}
	if ( name[0] == '_' ) {
		if ( !Q_stricmp( name, "_brig" ) ||
				!Q_stricmp( name, "_holodeck_camelot" ) ||
				!Q_stricmp( name, "_holodeck_firingrange" ) ||
				!Q_stricmp( name, "_holodeck_garden" ) ||
				!Q_stricmp( name, "_holodeck_highnoon" ) ||
				!Q_stricmp( name, "_holodeck_minigame" ) ||
				!Q_stricmp( name, "_holodeck_proton" ) ||
				!Q_stricmp( name, "_holodeck_proton2" ) ||
				!Q_stricmp( name, "_holodeck_temple" ) ||
				!Q_stricmp( name, "_holodeck_warlord" ) ) {
			return -1;
		}
	}
	if ( !Q_stricmpn( name, "borg", 4 ) ) {
		if ( !Q_stricmp( name, "borg1" ) ||
				!Q_stricmp( name, "borg2" ) ||
				!Q_stricmp( name, "borg3" ) ||
				!Q_stricmp( name, "borg4" ) ||
				!Q_stricmp( name, "borg5" ) ||
				!Q_stricmp( name, "borg6" ) ) {
			return -1;
		}
	}
	if ( !Q_stricmpn( name, "dn", 2 ) ) {
		if ( !Q_stricmp( name, "dn1" ) ||
				!Q_stricmp( name, "dn2" ) ||
				!Q_stricmp( name, "dn3" ) ||
				!Q_stricmp( name, "dn4" ) ||
				!Q_stricmp( name, "dn5" ) ||
				!Q_stricmp( name, "dn6" ) ||
				!Q_stricmp( name, "dn8" ) ) {
			return -1;
		}
	}
	if ( !Q_stricmpn( name, "forge", 5 ) ) {
		if ( !Q_stricmp( name, "forge1" ) ||
				!Q_stricmp( name, "forge2" ) ||
				!Q_stricmp( name, "forge3" ) ||
				!Q_stricmp( name, "forge4" ) ||
				!Q_stricmp( name, "forge5" ) ||
				!Q_stricmp( name, "forgeboss" ) ) {
			return -1;
		}
	}
	if ( !Q_stricmpn( name, "scav", 4 ) ) {
		if ( !Q_stricmp( name, "scav1" ) ||
				!Q_stricmp( name, "scav2" ) ||
				!Q_stricmp( name, "scav3" ) ||
				!Q_stricmp( name, "scav3b" ) ||
				!Q_stricmp( name, "scav4" ) ||
				!Q_stricmp( name, "scav5" ) ||
				!Q_stricmp( name, "scavboss" ) ) {
			return -1;
		}
	}
	if ( !Q_stricmpn( name, "stasis", 6 ) ) {
		if ( !Q_stricmp( name, "stasis1" ) ||
				!Q_stricmp( name, "stasis2" ) ||
				!Q_stricmp( name, "stasis3" ) ) {
			return -1;
		}
	}
	if ( !Q_stricmpn( name, "tour/deck", 9 ) ) {
		if ( !Q_stricmp( name, "tour/deck01" ) ||
				!Q_stricmp( name, "tour/deck02" ) ||
				!Q_stricmp( name, "tour/deck03" ) ||
				!Q_stricmp( name, "tour/deck04" ) ||
				!Q_stricmp( name, "tour/deck05" ) ||
				!Q_stricmp( name, "tour/deck08" ) ||
				!Q_stricmp( name, "tour/deck09" ) ||
				!Q_stricmp( name, "tour/deck10" ) ||
				!Q_stricmp( name, "tour/deck11" ) ||
				!Q_stricmp( name, "tour/deck15" ) ) {
			return -1;
		}
	}
	if ( !Q_stricmpn( name, "voy", 3 ) ) {
		if ( !Q_stricmp( name, "voy1" ) ||
				!Q_stricmp( name, "voy2" ) ||
				!Q_stricmp( name, "voy3" ) ||
				!Q_stricmp( name, "voy4" ) ||
				!Q_stricmp( name, "voy5" ) ||
				!Q_stricmp( name, "voy6" ) ||
				!Q_stricmp( name, "voy7" ) ||
				!Q_stricmp( name, "voy8" ) ||
				!Q_stricmp( name, "voy9" ) ||
				!Q_stricmp( name, "voy13" ) ||
				!Q_stricmp( name, "voy14" ) ||
				!Q_stricmp( name, "voy15" ) ||
				!Q_stricmp( name, "voy16" ) ||
				!Q_stricmp( name, "voy17" ) ||
				!Q_stricmp( name, "voy20" ) ) {
			return -1;
		}
	}
	if ( !Q_stricmpn( name, "hm_", 3 ) ) {
		if ( !Q_stricmp( name, "hm_borg1" ) ) return 33;
		if ( !Q_stricmp( name, "hm_kln1" ) ) return 32;
		if ( !Q_stricmp( name, "hm_for1" ) ) return 31;
		if ( !Q_stricmp( name, "hm_noon" ) ) return 30;
		if ( !Q_stricmp( name, "hm_voy2" ) ) return 29;
		if ( !Q_stricmp( name, "hm_dn1" ) ) return 28;
		if ( !Q_stricmp( name, "hm_scav1" ) ) return 27;
		if ( !Q_stricmp( name, "hm_voy1" ) ) return 26;
		if ( !Q_stricmp( name, "hm_borg2" ) ) return 25;
		if ( !Q_stricmp( name, "hm_dn2" ) ) return 24;
		if ( !Q_stricmp( name, "hm_cam" ) ) return 23;
		if ( !Q_stricmp( name, "hm_borg3" ) ) return 22;
		if ( !Q_stricmp( name, "hm_altar" ) ) return 12;
		if ( !Q_stricmp( name, "hm_blastradius" ) ) return 11;
		if ( !Q_stricmp( name, "hm_borgattack" ) ) return 10;
		if ( !Q_stricmp( name, "hm_for2" ) ) return 9;
		if ( !Q_stricmp( name, "hm_raven" ) ) return 8;
		if ( !Q_stricmp( name, "hm_temple" ) ) return 7;
		if ( !Q_stricmp( name, "hm_voy3" ) ) return 6;
	}
	if ( !Q_stricmpn( name, "ctf_", 4 ) ) {
		if ( !Q_stricmp( name, "ctf_breach" ) ) return 21;
		if ( !Q_stricmp( name, "ctf_for1" ) ) return 20;
		if ( !Q_stricmp( name, "ctf_neptune" ) ) return 19;
		if ( !Q_stricmp( name, "ctf_oldwest" ) ) return 18;
		if ( !Q_stricmp( name, "ctf_reservoir" ) ) return 17;
		if ( !Q_stricmp( name, "ctf_singularity" ) ) return 16;
		if ( !Q_stricmp( name, "ctf_dn1" ) ) return 15;
		if ( !Q_stricmp( name, "ctf_spyglass2" ) ) return 14;
		if ( !Q_stricmp( name, "ctf_stasis" ) ) return 13;
		if ( !Q_stricmp( name, "ctf_kln1" ) ) return 5;
		if ( !Q_stricmp( name, "ctf_kln2" ) ) return 4;
		if ( !Q_stricmp( name, "ctf_and1" ) ) return 3;
		if ( !Q_stricmp( name, "ctf_voy1" ) ) return 2;
		if ( !Q_stricmp( name, "ctf_voy2" ) ) return 1;
	}
	return qfalse;
}

/*
===============
UI_MapListCompare

Main map list sort function.
===============
*/
static int UI_MapListCompare( const void *e1, const void *e2 ) {
	const mapEntry_t *m1 = (const mapEntry_t *)e1;
	const mapEntry_t *m2 = (const mapEntry_t *)e2;
	if ( m1->sortValue < m2->sortValue ) {
		return -1;
	}
	if ( m2->sortValue < m1->sortValue ) {
		return 1;
	}
	return Q_stricmp( m1->name, m2->name );
}

/*
===============
UI_PopulateMapList

Generate map list. Should only be called once during UI session.
===============
*/
static void UI_PopulateMapList( void ) {
	int i;
	int listCount;
	char *dirptr;
	int dirlen;
	int stock;

	if ( !EF_WARN_ASSERT( !ui_maps.mapCount ) ) {
		return;
	}

	listCount = trap_FS_GetFileList( "maps", ".bsp", ui_maps.mapListBuffer, sizeof( ui_maps.mapListBuffer ) );

	dirptr = ui_maps.mapListBuffer;
	for ( i = 0; i < listCount; ++i, dirptr += dirlen + 1 ) {
		dirlen = strlen(dirptr);

		if ( EF_WARN_ASSERT( dirlen >= 4 && !Q_stricmp( &dirptr[dirlen - 4], ".bsp" ) ) ) {
			dirptr[dirlen - 4] = '\0';
		}
		stock = UI_CheckStockMap( dirptr );
		if ( stock < 0 ) {
			continue;
		}
		if ( ui_maps.mapCount >= MAX_MAPS ) {
			break;
		}
		ui_maps.mapList[ui_maps.mapCount].name = dirptr;
		ui_maps.mapList[ui_maps.mapCount].sortValue = -stock;
		ui_maps.mapCount++;
	}

	qsort( ui_maps.mapList, ui_maps.mapCount, sizeof( *ui_maps.mapList ), UI_MapListCompare );

	Com_Printf( "Indexed %i maps\n", ui_maps.mapCount );
}

static char arenaListBuffer[ARENA_LIST_BUFFER];

typedef struct {
	mapEntry_t *maps[MAX_MAPS];
} sortedMapList_t;

/*
===============
UI_SortedMapListCompare
===============
*/
static int UI_SortedMapListCompare( const void *e1, const void *e2 ) {
	return Q_stricmp( ( *(const mapEntry_t **)e1 )->name, ( *(const mapEntry_t **)e2 )->name );
}

/*
===============
UI_GetSortedMapList

Generates list of maps sorted by name. Currently just used by UI_FindMapByName to do a
binary search.
===============
*/
static void UI_GetSortedMapList( sortedMapList_t *out ) {
	int i;
	for ( i = 0; i < ui_maps.mapCount; ++i ) {
		out->maps[i] = &ui_maps.mapList[i];
	}
	qsort( out->maps, ui_maps.mapCount, sizeof( *out->maps ), UI_SortedMapListCompare );
}

/*
===============
UI_FindMapByName
===============
*/
static mapEntry_t *UI_FindMapByName( const sortedMapList_t *sml, const char *name ) {
	int low = 0;
	int high = ui_maps.mapCount - 1;
	int mid;
	int cmp;

	while ( low <= high ) {
		mid = low + ( high - low ) / 2;
		cmp = Q_stricmp( sml->maps[mid]->name, name );

		if ( !cmp ) {
			return sml->maps[mid];
		} else if ( cmp < 0 ) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	return NULL;
}

typedef struct {
	int indexed;
	int duplicate;
	int unmatched;
} arenaStats_t;

/*
===============
UI_IndexArenasFromFile

Path should be a statically allocated constant.
===============
*/
static void UI_IndexArenasFromFile( const char *path, qboolean rawPath, sortedMapList_t *sml, arenaStats_t *stats ) {
	char arenaFile[MAX_ARENAS_TEXT];
	char *parsePtr = arenaFile;
	infoParseResult_t info;

	if ( rawPath ) {
		UI_ReadInfoFile( path, arenaFile, MAX_ARENAS_TEXT );
	} else {
		char fullPath[256];
		Com_sprintf( fullPath, sizeof( fullPath ), "scripts/%s", path );
		UI_ReadInfoFile( fullPath, arenaFile, MAX_ARENAS_TEXT );
	}

	if ( !*arenaFile ) {
		return;
	}

	while ( UI_ParseInfo( &parsePtr, &info ) ) {
		const char *mapName = Info_ValueForKey( info.info, "map" );
		mapEntry_t *mapEntry = UI_FindMapByName( sml, mapName );
		if ( mapEntry ) {
			if ( mapEntry->arenaPath ) {
				GAMEINFO_DEBUG_PRINT( ( "skipping definition for map '%s' from '%s',"
						" because it was already defined in '%s'\n", mapName, path, mapEntry->arenaPath ) );
				++stats->duplicate;
			} else {
				mapEntry->arenaPath = path;
				if ( rawPath ) {
					mapEntry->flags |= MAPFLAG_RAW_ARENA_PATH;
				}

				// register single player support
				if ( strstr( Info_ValueForKey( info.info, "type" ), "single" ) ) {
					
					mapEntry->flags |= MAPFLAG_SINGLE_PLAYER_SUPPORT;
					ui_maps.singlePlayerMapCount++;
				}

				// register ctf support
				{
					char *type = Info_ValueForKey( info.info, "type" );
					char *token;
					while ( 1 ) {
						token = COM_ParseExt( &type, qfalse );
						if ( token[0] == '\0' ) {
							break;
						}
						if ( !Q_stricmp( token, "ctf" ) ) {
							mapEntry->flags |= MAPFLAG_CTF_ARENA;
							break;
						}
					}
				}

				// register recommended players number
				{
					int recommended = atoi( Info_ValueForKey( info.info, "recommended" ) );
					if ( recommended < 0 ) {
						recommended = 0;
					} else if ( recommended > 128 ) {
						recommended = 128;
					}
					mapEntry->recommendedPlayers = recommended;
				}

				++stats->indexed;
			}
			if ( *info.error ) {
				GAMEINFO_DEBUG_PRINT( ( "parse warning in arena file '%s': %s\n", path, info.error ) );
			}
		} else {
			GAMEINFO_DEBUG_PRINT( ( "skipping definition for map '%s' from '%s', because the associated"
					" bsp file was not found\n", mapName, path ) );
			++stats->unmatched;
		}
	}

	if ( *info.error ) {
		GAMEINFO_DEBUG_PRINT( ( "parse error in arena file '%s': %s\n", path, info.error ) );
	}
}

/*
===============
UI_IndexArenas

Load data from arena files into map list. Should only be called once during UI session.
===============
*/
static void UI_IndexArenas( void ) {
	int i;
	int listCount;
	char *dirptr;
	int dirlen;
	static vmCvar_t arenasFile;
	sortedMapList_t sml;
	arenaStats_t stats = { 0 };

	UI_GetSortedMapList( &sml );

	trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT | CVAR_ROM );
	if ( *arenasFile.string ) {
		UI_IndexArenasFromFile( arenasFile.string, qtrue, &sml, &stats );
	} else {
		UI_IndexArenasFromFile( "scripts/arenas.txt", qtrue, &sml, &stats );
	}

	listCount = trap_FS_GetFileList( "scripts", ".arena", arenaListBuffer, sizeof( arenaListBuffer ) );
	dirptr = arenaListBuffer;
	for ( i = 0; i < listCount; ++i, dirptr += dirlen + 1 ) {
		dirlen = strlen(dirptr);
		UI_IndexArenasFromFile( dirptr, qfalse, &sml, &stats );
	}

	Com_Printf( "Indexed arena data for %i maps (%i duplicate, %i missing bsp)\n",
			stats.indexed, stats.duplicate, stats.unmatched );
	Com_Printf( "Have %i maps with single player support\n", ui_maps.singlePlayerMapCount );
}

/*
===============
UI_IndexMapsIfNeeded
===============
*/
static void UI_IndexMapsIfNeeded( void ) {
	static qboolean complete = qfalse;
	if ( !complete ) {
		UI_PopulateMapList();
		UI_IndexArenas();
		complete = qtrue;
	}
}

/*
===============
UI_UpdateMapEntryCTFSupport

Checks CTF support for map entry and updates flags.
===============
*/
static void UI_UpdateMapEntryCTFSupport( mapEntry_t *entry ) {
	if ( !( entry->flags & MAPFLAG_CTF_CHECKED ) ) {
		if ( UI_CheckCTFSupport( va( "maps/%s.bsp", entry->name ) ) ) {
			entry->flags |= MAPFLAG_CTF_SUPPORTED;
		}
		entry->flags |= MAPFLAG_CTF_CHECKED;
	}
}

/*
=======================================================================

MAP INDEX ACCESSORS

=======================================================================
*/

/*
===============
UI_GetNumMaps
===============
*/
int UI_GetNumMaps( void ) {
	UI_IndexMapsIfNeeded();
	return ui_maps.mapCount;
}

/*
===============
UI_GetMapName
===============
*/
const char *UI_GetMapName( int num ) {
	if ( EF_WARN_ASSERT( num >= 0 && num < ui_maps.mapCount ) ) {
		return ui_maps.mapList[num].name;
	}
	return "<error>";
}

/*
===============
UI_GetMapRecommendedPlayers
===============
*/
int UI_GetMapRecommendedPlayers( int num ) {
	if ( EF_WARN_ASSERT( num >= 0 && num < ui_maps.mapCount ) ) {
		return ui_maps.mapList[num].recommendedPlayers;
	}
	return 0;
}

/*
===============
UI_GetArenaInfo

Returns empty string on error.
===============
*/
void UI_GetArenaInfo( int num, char *buffer, unsigned int bufSize ) {
	*buffer = '\0';

	if ( EF_WARN_ASSERT( num >= 0 && num < ui_maps.mapCount ) ) {
		mapEntry_t *mapEntry = &ui_maps.mapList[num];
		if ( mapEntry->arenaPath ) {
			char arenaFile[MAX_ARENAS_TEXT];
			char *parsePtr = arenaFile;
			infoParseResult_t info;

			if ( mapEntry->flags & MAPFLAG_RAW_ARENA_PATH ) {
				UI_ReadInfoFile( mapEntry->arenaPath, arenaFile, MAX_ARENAS_TEXT );
			} else {
				char fullPath[256];
				Com_sprintf( fullPath, sizeof( fullPath ), "scripts/%s", mapEntry->arenaPath );
				UI_ReadInfoFile( fullPath, arenaFile, MAX_ARENAS_TEXT );
			}

			if ( *arenaFile && UI_ParseInfoForMap( arenaFile, mapEntry->name, &info ) ) {
				Q_strncpyz( buffer, info.info, bufSize );
			}
		}
	}
}

/*
===============
UI_GetMapPage

Generates a single page of map results for the map selection menu.

Parameters:
- filter and type specify text search criteria.
- ctf limits results to ctf-supported maps.
- startIndex indicates index of first map to display on the page. Assuming 4 maps
	per page, this would be 0 for page 1, 4 for page 2, 8 for page 3, etc.
- mapsOut is the buffer the output maps are written to. Maps are output in
	terms of the "map number" which can be passed to functions such as
	UI_GetArenaInfo to get name and other information about the map.
- maxCount indicates max number of maps to output, which is the number of maps
	displayed on each page, typically 4.

Result:
- count is the number of maps written to mapNumsOut, in the range from 0
    to maxCount. It should equal maxCount except on the last page.
- totalMaps is the total number of maps matching the filter criteria.
- totalMapsEstimated indicates that the value of totalMaps is actually an estimate,
	and the true number of maps might be lower. This is only returned in ctf filter
	mode. When this is returned, it will have been checked that there is at least 1
	more valid map for the next page, so advancing to the next page is valid.
===============
*/
mapSearchResult_t UI_GetMapPage( const char *filter, mapSearchType_t type, qboolean ctf,
		int startIndex, int *mapsOut, int maxCount ) {
	int i;
	int swLen = 0;
	mapSearchResult_t out;
	qboolean deferCTFCheck = qfalse;

	memset( &out, 0, sizeof( out ) );
	UI_IndexMapsIfNeeded();

	// prepare filter
	if ( !filter || !*filter ) {
		type = MST_UNFILTERED;
	}
	if ( type == MST_STARTS_WITH ) {
		swLen = strlen( filter );
	}

	for ( i = 0; i < ui_maps.mapCount; ++i ) {
		mapEntry_t *mapEntry = &ui_maps.mapList[i];

		// check filter
		if ( type == MST_STARTS_WITH && Q_stricmpn( filter, mapEntry->name, swLen ) ) {
			continue;
		}
		if ( type == MST_CONTAINS && !Q_strstr( mapEntry->name, filter ) ) {
			continue;
		}

		// check CTF support
		if ( ctf ) {
			if ( !deferCTFCheck ) {
				UI_UpdateMapEntryCTFSupport( mapEntry );
			}

			if ( !( mapEntry->flags & MAPFLAG_CTF_SUPPORTED ) ) {
				if ( mapEntry->flags & MAPFLAG_CTF_CHECKED ) {
					// no ctf support
					continue;
				} else {
					// check deferred, so determine support based on arena but set estimated marker
					out.totalMapsEstimated = qtrue;
					if ( !( mapEntry->flags & MAPFLAG_CTF_ARENA ) ) {
						continue;
					}
				}

			} else if ( out.count >= maxCount ) {
				// we now have the verified output number of maps plus one, so we can start
				// deferring the ctf check
				deferCTFCheck = qtrue;
			}
		}

		if ( out.totalMaps >= startIndex && out.count < maxCount ) {
			mapsOut[out.count++] = i;
		}

		++out.totalMaps;
	}

	return out;
}

/*
=======================================================================

SINGLE PLAYER FUNCTIONS

These functions use their own index number scheme in which the map numbers
range from 0 to UI_GetNumSPArenas()-1, and every index corresponds to a
valid single player-compatible map.

=======================================================================
*/

/*
===============
UI_GetNumSPArenas

Returns total number of maps to display in solo match menu.
===============
*/
int UI_GetNumSPArenas( void ) {
	int i;
	int spCount = 0;

	UI_IndexMapsIfNeeded();

	for ( i = 0; i < ui_maps.mapCount; ++i ) {
		if ( ui_maps.mapList[i].flags & MAPFLAG_SINGLE_PLAYER_SUPPORT ) {
			++spCount;
		}
	}
	return spCount;
}



/*
===============
UI_GetSPArenaNumByMap

Returns single player arena index for given map name, or -1 if not found.
===============
*/
int UI_GetSPArenaNumByMap( const char *mapName ) {
	int i;
	int spCount = 0;

	UI_IndexMapsIfNeeded();

	for ( i = 0; i < ui_maps.mapCount; ++i ) {
		if ( ui_maps.mapList[i].flags & MAPFLAG_SINGLE_PLAYER_SUPPORT ) {
			if ( !Q_stricmp( ui_maps.mapList[i].name, mapName ) ) {
				return spCount;
			}
			++spCount;
		}
	}
	return -1;
}

/*
===============
UI_GetSPArenaInfo

Returns arena info for specified single player index.

Returns empty string on error or invalid number.
===============
*/
void UI_GetSPArenaInfo( int spNum, char *buffer, unsigned int bufSize ) {
	int i;
	int spCount = 0;

	EF_ERR_ASSERT( bufSize >= MAX_INFO_STRING );
	*buffer = '\0';

	for ( i = 0; i < ui_maps.mapCount; ++i ) {
		if ( ui_maps.mapList[i].flags & MAPFLAG_SINGLE_PLAYER_SUPPORT ) {
			if ( spCount == spNum ) {
				UI_GetArenaInfo( i, buffer, bufSize );
				if ( *buffer ) {
					Info_SetValueForKey( buffer, "num", va( "%i", spNum ) );
				}
				return;
			}
			++spCount;
		}
	}
}

/*
===============
UI_GetInitialLevel

Returns single player index of the first level the player has not won.
===============
*/
int UI_GetInitialLevel( void ) {
	int i;
	int spCount = 0;

	UI_IndexMapsIfNeeded();

	for ( i = 0; i < ui_maps.mapCount; ++i ) {
		if ( ui_maps.mapList[i].flags & MAPFLAG_SINGLE_PLAYER_SUPPORT ) {
			if ( UI_ReadMapCompletionSkill( ui_maps.mapList[i].name ) <= 0 ) {
				return spCount;
			}
			++spCount;
		}
	}

	return 0;
}

/*
=======================================================================

BOT INDEXING

=======================================================================
*/

#define POOLSIZE	128 * 1024

int				ui_numBots;
static char		*ui_botInfos[MAX_BOTS];

static char		memoryPool[POOLSIZE];
static int		allocPoint, outOfMemory;


/*
===============
UI_Alloc
===============
*/
void *UI_Alloc( int size ) {
	char	*p;

	if ( allocPoint + size > POOLSIZE ) {
		outOfMemory = qtrue;
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 31 ) & ~31;

	return p;
}


/*
===============
UI_LoadBotsFromFile
===============
*/
static void UI_LoadBotsFromFile( char *filename ) {
	char buf[MAX_BOTS_TEXT];
	char *parsePtr = buf;
	infoParseResult_t info;

	UI_ReadInfoFile( filename, buf, sizeof( buf ) );
	if ( !*buf ) {
		return;
	}

	while ( UI_ParseInfo( &parsePtr, &info ) ) {
		if ( *info.error ) {
			GAMEINFO_DEBUG_PRINT( ( "parse warning in bot file '%s': %s\n", filename, info.error ) );
		}
		if ( ui_numBots >= MAX_BOTS ) {
			GAMEINFO_DEBUG_PRINT( ( "MAX_BOTS exceeded\n" ) );
			return;
		}
		ui_botInfos[ui_numBots] = UI_Alloc( strlen( info.info ) + 1 );
		if ( ui_botInfos[ui_numBots] ) {
			strcpy( ui_botInfos[ui_numBots], info.info );
			ui_numBots++;
		}
	}

	if ( *info.error ) {
		GAMEINFO_DEBUG_PRINT( ( "parse error in bot file '%s': %s\n", filename, info.error ) );
	}
}

/*
===============
UI_LoadBots
===============
*/
static void UI_LoadBots( void ) {
	vmCvar_t	botsFile;
	int			numdirs;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	ui_numBots = 0;

	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM );
	if( *botsFile.string ) {
		UI_LoadBotsFromFile(botsFile.string);
	}
	else {
		UI_LoadBotsFromFile("scripts/bots.txt");
	}

	// get all bots from .bot files
	numdirs = trap_FS_GetFileList("scripts", ".bot", dirlist, sizeof( dirlist ) );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadBotsFromFile(filename);
	}

	if (outOfMemory) trap_Print(S_COLOR_YELLOW"WARNING: not enough memory in pool to load all bots\n");
	trap_Print( va( "%i bots parsed\n", ui_numBots ) );
}

/*
===============
UI_LoadBotsIfNeeded
===============
*/
static void UI_LoadBotsIfNeeded( void ) {
	static qboolean complete = qfalse;
	if ( !complete ) {
		UI_LoadBots();
		complete = qtrue;
	}
}


/*
===============
UI_GetBotInfoByNumber
===============
*/
char *UI_GetBotInfoByNumber( int num ) {
	if( num < 0 || num >= ui_numBots ) {
		trap_Print( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}
	return ui_botInfos[num];
}


/*
===============
UI_GetBotInfoByName
===============
*/
char *UI_GetBotInfoByName( const char *name ) {
	int		n;
	char	*value;
	UI_LoadBotsIfNeeded();

	for ( n = 0; n < ui_numBots ; n++ ) {
		value = Info_ValueForKey( ui_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return ui_botInfos[n];
		}
	}

	return NULL;
}

/*
===============
UI_GetNumBots
===============
*/
int UI_GetNumBots( void ) {
	UI_LoadBotsIfNeeded();
	return ui_numBots;
}

/*
=======================================================================

SINGLE PLAYER GAME INFO

This section handles logging map completion progress, which is used to
display an icon on maps completed at 1st place in the solo match menu.

=======================================================================
*/

#define SCORE_FILENAME "spscores.dat"

static qboolean ui_mapCompletionLoaded = qfalse;
static char ui_mapCompletion[BIG_INFO_STRING];

/*
===============
UI_ReadMapCompletionIfNeeded

Reads completed map record from file.
===============
*/
static void UI_ReadMapCompletionIfNeeded( void ) {
	if ( !ui_mapCompletionLoaded ) {
		UI_ReadInfoFile( SCORE_FILENAME, ui_mapCompletion, sizeof( ui_mapCompletion ) );
		ui_mapCompletionLoaded = qtrue;
	}
}

/*
===============
UI_WriteMapCompletion

Writes completed map record to file.
===============
*/
static void UI_WriteMapCompletion( void ) {
	fileHandle_t f = 0;
	trap_FS_FOpenFile( SCORE_FILENAME, &f, FS_WRITE );
	if ( f ) {
		trap_FS_Write( ui_mapCompletion, strlen( ui_mapCompletion ), f );
		trap_FS_FCloseFile( f );
	}
}

/*
===============
UI_ReadMapCompletionSkill

Returns the highest difficulty beaten on a level, from 1 to 5, or 0 if the level
hasn't been completed.
===============
*/
int UI_ReadMapCompletionSkill( const char *mapName ) {
	int result;
	UI_ReadMapCompletionIfNeeded();
	result = atoi( Info_ValueForKey( ui_mapCompletion, mapName ) );
	if ( result < 0 ) {
		return 0;
	}
	if ( result > 5 ) { 
		return 5;
	}
	return result;
}

/*
===============
UI_WriteMapCompletionSkill

Logs the difficulty level when a map has been completed at 1st place.
===============
*/
void UI_WriteMapCompletionSkill( const char *mapName, int skill ) {
	if ( skill < 1 ) {
		skill = 1;
	}
	if ( skill > 5 ) {
		skill = 5;
	}
	UI_ReadMapCompletionIfNeeded();
	if ( skill > UI_ReadMapCompletionSkill( mapName ) ) {
		Info_SetValueForKey_Big( ui_mapCompletion, mapName, va( "%i", skill ) );
		UI_WriteMapCompletion();
	}
}

/*
===============
UI_ResetMapCompletion

Clears map completion record.
===============
*/
void UI_ResetMapCompletion( void ) {
	ui_mapCompletion[0] = '\0';
	ui_mapCompletionLoaded = qtrue;
	UI_WriteMapCompletion();
}


/*
===============
UI_InitGameinfo
===============
*/
void UI_InitGameinfo( void ) {
	if( trap_Cvar_VariableValue( "fs_restrict" ) ) {
		uis.demoversion = qtrue;
	}
	else {
		uis.demoversion = qfalse;
	}
}
