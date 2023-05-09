/*
* Spawn Protection
* 
* This module provides a workaround for players being killed immediately on spawn due to
* a lack of map spawn points. When an invalid spawn point is selected, it will be either
* changed to a different one or shifted to the side to ensure there is space for the player.
*/

#define MOD_NAME ModSpawnProtect

#include "mods/g_mod_local.h"

#define MAX_SPAWN_POINTS 128

#define SP_DEBUG MOD_STATE->g_debugSpawnProtect.integer

static vec3_t sp_playerMins = { -15, -15, -24 };
static vec3_t sp_playerMaxs = { 15, 15, 32 };

typedef struct {
	gentity_t *spots[MAX_SPAWN_POINTS];
	unsigned int count;
} spawnList_t;

static struct {
	trackedCvar_t g_debugSpawnProtect;
} *MOD_STATE;

/*
================
ModSpawnProtect_AddSpawnsToList

Populate list with spawns matching classname.
================
*/
static void ModSpawnProtect_AddSpawnsToList( const char *className, spawnList_t *list ) {
	gentity_t *spot = NULL;
	while ( list->count < MAX_SPAWN_POINTS && ( spot = G_Find( spot, FOFS( classname ), className ) ) != NULL ) {
		list->spots[list->count++] = spot;
	}
}

/*
================
ModSpawnProtect_ShuffleList

Randomize order of spawn list.
================
*/
static void ModSpawnProtect_ShuffleList( spawnList_t *list ) {
	if ( list->count > 1 ) {
		int i;
		for ( i = 0; i < list->count - 1; ++i ) {
			int pick = irandom( i, list->count - 1 );
			if ( pick != i && EF_WARN_ASSERT( pick >= i && pick <= list->count - 1 ) ) {
				gentity_t *temp = list->spots[i];
				list->spots[i] = list->spots[pick];
				list->spots[pick] = temp;
			}
		}
	}
}

/*
================
ModSpawnProtect_NonPlayerTrace

Trace with players ignored.
================
*/
static void ModSpawnProtect_NonPlayerTrace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int passEntityNum, int contentMask ) {
	int i;
	int oldContents[MAX_CLIENTS];

	for ( i = 0; i < level.maxclients; ++i ) {
		oldContents[i] = g_entities[i].r.contents;
		g_entities[i].r.contents = 0;
	}

	trap_Trace( results, start, mins, maxs, end, passEntityNum, contentMask );

	for ( i = 0; i < level.maxclients; ++i ) {
		g_entities[i].r.contents = oldContents[i];
	}
}

/*
================
ModSpawnProtect_TrySpawn

Test spawn at original origin. If valid, returns true and sets outputs.
================
*/
static qboolean ModSpawnProtect_TrySpawn( gentity_t *spawn, gentity_t **spawn_out, vec3_t origin_out, vec3_t angles_out ) {
	vec3_t adjusted;
	VectorCopy( spawn->s.origin, adjusted );
	adjusted[2] += 9.0f;

	if ( SpotWouldTelefrag( adjusted ) ) {
		return qfalse;
	}

	*spawn_out = spawn;
	VectorCopy( adjusted, origin_out );
	VectorCopy( spawn->s.angles, angles_out );
	return qtrue;
}

/*
================
ModSpawnProtect_TryShiftedSpawn

Tests spawn with origin randomly shifted to the side. If valid, returns true and sets outputs.
================
*/
static qboolean ModSpawnProtect_TryShiftedSpawn( gentity_t *spawn, gentity_t **spawn_out, vec3_t origin_out, vec3_t angles_out ) {
	trace_t tr;
	vec3_t start;
	vec3_t side;
	vec3_t bottom;

	// start = same as input, but sometimes shifted up
	VectorCopy( spawn->s.origin, start );
	start[2] += 9.0f;
	if ( rand() % 2 ) {
		start[2] += flrandom( 0.0f, 50.0f );
	}

	// side = random horizontal shift from start
	VectorCopy( start, side );
	side[0] += flrandom( 20.0f, 300.0f ) * ( rand() % 2 ? 1.0f : -1.0f );
	side[1] += flrandom( 20.0f, 300.0f ) * ( rand() % 2 ? 1.0f : -1.0f );

	// bottom = trace endpoint directly beneath side
	VectorCopy( side, bottom );
	bottom[2] -= 100.0f;

	// trace to the side, to make sure new location is player-accessible
	ModSpawnProtect_NonPlayerTrace( &tr, start, sp_playerMins, sp_playerMaxs, side, -1, MASK_PLAYERSOLID );
	if ( tr.startsolid || tr.fraction != 1.0f ) {
		return qfalse;
	}

	// trace to the ground, to make sure new location isn't off a ledge
	trap_Trace( &tr, side, sp_playerMins, sp_playerMaxs, bottom, -1, MASK_PLAYERSOLID );
	if ( tr.startsolid || tr.fraction >= 1.0f ) {
		return qfalse;
	}

	if ( SpotWouldTelefrag( tr.endpos ) ) {
		return qfalse;
	}

	*spawn_out = spawn;
	VectorCopy( tr.endpos, origin_out );
	VectorCopy( spawn->s.angles, angles_out );
	return qtrue;
}

/*
================
ModSpawnProtect_SelectSpawn

Tests all the spawns with the specified classnames, both with the original origin and randomly shifted origins.
If a valid origin is found, returns true and sets outputs.
================
*/
static qboolean ModSpawnProtect_SelectSpawn( const char **classNames, gentity_t **spawn_out, vec3_t origin_out, vec3_t angles_out ) {
	int i;
	spawnList_t list;

	// generate spawn list
	list.count = 0;
	while ( *classNames ) {
		int oldCount = list.count;
		ModSpawnProtect_AddSpawnsToList( *classNames, &list );
		if ( SP_DEBUG ) {
			G_Printf( "  loaded %i spawn candidates with classname '%s'\n", list.count - oldCount, *classNames );
		}
		++classNames;
	}
	ModSpawnProtect_ShuffleList( &list );

	// check if one is valid with the original origin
	for ( i = 0; i < list.count; ++i ) {
		if ( ModSpawnProtect_TrySpawn( list.spots[i], spawn_out, origin_out, angles_out ) ) {
			if ( SP_DEBUG ) {
				G_Printf( "  found valid spawn: classname(%s) origin(%f %f %f)\n",
						list.spots[i]->classname, EXPAND_VECTOR( list.spots[i]->s.origin ) );
			}
			return qtrue;
		}
	}

	// try to shift spawns sideways to get a valid spot
	if ( list.count > 0 ) {
		int index = 0;
		for ( i = 0; i < 256; ++i ) {
			if ( ModSpawnProtect_TryShiftedSpawn( list.spots[index], spawn_out, origin_out, angles_out ) ) {
				if ( SP_DEBUG ) {
					G_Printf( "  found adjusted spawn: pass(%i / 256) classname(%s) origin(%f %f %f) adjusted(%f %f %f)\n",
							i + 1, list.spots[index]->classname, EXPAND_VECTOR( list.spots[index]->s.origin ),
							EXPAND_VECTOR( origin_out ) );
				}
				return qtrue;
			}

			index = ( index + 1 ) % list.count;
		}
	}

	return qfalse;
}

/*
================
(ModFN) PatchClientSpawn

Checks if player spawn selected by the normal process would cause a telefrag, and if so, tries to
replace it with a different or shifted spawn point.
================
*/
static void MOD_PREFIX(PatchClientSpawn)( MODFN_CTV, int clientNum, gentity_t **spawn, vec3_t origin, vec3_t angles ) {
	if ( !*spawn || SpotWouldTelefrag( origin ) ) {
		if ( SP_DEBUG ) {
			if ( clientNum >= 0 ) {
				G_Printf( "spawn protect: Attempting to patch invalid spawn for client %i.\n", clientNum );
			} else {
				G_Printf( "spawn protect: Attempting to patch invalid spawn.\n" );
			}
		}
		if ( clientNum >= 0 && level.clients[clientNum].sess.sessionTeam == TEAM_RED ) {
			const char *list1[] = { "team_CTF_redplayer", "team_CTF_redspawn", NULL };
			const char *list2[] = { "info_player_deathmatch", NULL };
			if ( ModSpawnProtect_SelectSpawn( list1, spawn, origin, angles ) ) {
				return;
			}
			if ( ModSpawnProtect_SelectSpawn( list2, spawn, origin, angles ) ) {
				return;
			}
		} else if ( clientNum >= 0 && level.clients[clientNum].sess.sessionTeam == TEAM_BLUE ) {
			const char *list1[] = { "team_CTF_blueplayer", "team_CTF_bluespawn", NULL };
			const char *list2[] = { "info_player_deathmatch", NULL };
			if ( ModSpawnProtect_SelectSpawn( list1, spawn, origin, angles ) ) {
				return;
			}
			if ( ModSpawnProtect_SelectSpawn( list2, spawn, origin, angles ) ) {
				return;
			}
		} else {
			const char *list[] = { "info_player_deathmatch", "team_CTF_redplayer", "team_CTF_redspawn",
					"team_CTF_blueplayer", "team_CTF_bluespawn", NULL };
			if ( ModSpawnProtect_SelectSpawn( list, spawn, origin, angles ) ) {
				return;
			}
		}
	}
}

/*
================
ModSpawnProtect_Init
================
*/
void ModSpawnProtect_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_debugSpawnProtect, "g_debugSpawnProtect", "0", 0, qfalse );

		MODFN_REGISTER( PatchClientSpawn, MODPRIORITY_GENERAL );
	}
}
