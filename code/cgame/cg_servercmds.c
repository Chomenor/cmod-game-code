// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"


/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores( void ) {
	int		i, powerups, eliminated;

	cg.numScores = atoi( CG_Argv( 1 ) );
	if ( cg.numScores > MAX_CLIENTS ) {
		cg.numScores = MAX_CLIENTS;
	}

	cg.teamScores[0] = atoi( CG_Argv( 2 ) );
	cg.teamScores[1] = atoi( CG_Argv( 3 ) );

	memset( cg.scores, 0, sizeof( cg.scores ) );
	for ( i = 0 ; i < cg.numScores ; i++ ) {
		cg.scores[i].client			= atoi( CG_Argv( i * 11+ 4 ) );
		cg.scores[i].score			= atoi( CG_Argv( i * 11+ 5 ) );
		cg.scores[i].ping			= atoi( CG_Argv( i * 11+ 6 ) );
		cg.scores[i].time			= atoi( CG_Argv( i * 11+ 7 ) );
		cg.scores[i].scoreFlags		= atoi( CG_Argv( i * 11+ 8 ) );
		powerups					= atoi( CG_Argv( i * 11+ 9 ) );
//		cg.scores[i].faveTarget		= atoi( CG_Argv( i * 12+ 10) );
//		cg.scores[i].faveTargetKills = atoi( CG_Argv( i * 12+ 11) );
		cg.scores[i].worstEnemy		= atoi( CG_Argv( i * 11+ 10) );
		cg.scores[i].worstEnemyKills= atoi( CG_Argv( i * 11+ 11) );
		cg.scores[i].faveWeapon		= atoi( CG_Argv( i * 11+ 12) );
		cg.scores[i].killedCnt		= atoi( CG_Argv( i * 11+ 13) );
		eliminated					= atoi( CG_Argv( i * 11+ 14) );

		if ( cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS ) {
			cg.scores[i].client = 0;
		}
		cgs.clientinfo[ cg.scores[i].client ].score = cg.scores[i].score;
		cgs.clientinfo[ cg.scores[i].client ].powerups = powerups;
		cgs.clientinfo[ cg.scores[i].client ].eliminated = eliminated;
	}

}

/*
=================
CG_ParseTeamInfo

=================
*/
static void CG_ParseTeamInfo( void ) {
	int		i;
	int		client;

	numSortedTeamPlayers = atoi( CG_Argv( 1 ) );

	for ( i = 0 ; i < numSortedTeamPlayers ; i++ ) {
		client = atoi( CG_Argv( i * 6 + 2 ) );

		sortedTeamPlayers[i] = client;

		cgs.clientinfo[ client ].location = atoi( CG_Argv( i * 6 + 3 ) );
		cgs.clientinfo[ client ].health = atoi( CG_Argv( i * 6 + 4 ) );
		cgs.clientinfo[ client ].armor = atoi( CG_Argv( i * 6 + 5 ) );
		cgs.clientinfo[ client ].curWeapon = atoi( CG_Argv( i * 6 + 6 ) );
		cgs.clientinfo[ client ].powerups = atoi( CG_Argv( i * 6 + 7 ) );
	}
}

/*
=================
CG_ParseHealthInfo

=================
*/
static void CG_ParseHealthInfo( void ) {
	int		i;
	int		client;
	int		numHealthInfoClients = 0;

	numHealthInfoClients = atoi( CG_Argv( 1 ) );

	for ( i = 0 ; i < numHealthInfoClients ; i++ ) {
		client = atoi( CG_Argv( i * 2 + 2 ) );

		cgs.clientinfo[ client ].health = atoi( CG_Argv( i * 2 + 3 ) );
	}
}

/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo( void ) {
	const char	*info;
	char	*mapname;

	info = CG_ConfigString( CS_SERVERINFO );
	cgs.gametype = atoi( Info_ValueForKey( info, "g_gametype" ) );
	cgs.pModAssimilation = atoi( Info_ValueForKey( info, "g_pModAssimilation" ) );
	cgs.pModDisintegration = atoi( Info_ValueForKey( info, "g_pModDisintegration" ) );
	cgs.pModActionHero = atoi( Info_ValueForKey( info, "g_pModActionHero" ) );
	cgs.pModSpecialties = atoi( Info_ValueForKey( info, "g_pModSpecialties" ) );
	cgs.pModElimination = atoi( Info_ValueForKey( info, "g_pModElimination" ) );
	cgs.dmflags = atoi( Info_ValueForKey( info, "dmflags" ) );
	cgs.teamflags = atoi( Info_ValueForKey( info, "teamflags" ) );
	cgs.fraglimit = atoi( Info_ValueForKey( info, "fraglimit" ) );
	cgs.capturelimit = atoi( Info_ValueForKey( info, "capturelimit" ) );
	cgs.timelimit = atoi( Info_ValueForKey( info, "timelimit" ) );
	cgs.maxclients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cgs.mapname, sizeof( cgs.mapname ), "maps/%s.bsp", mapname );

	CG_AltFire_UpdateServerPrefs();
}

/*
==================
CG_ParseWeaponValue
==================
*/
static void CG_ParseWeaponValue( const char *src, weaponValues_t *output ) {
	int weapon = WP_NONE;
	qboolean alt = qfalse;

	if ( src[0] >= '1' && src[0] <= '9' ) {
		weapon = WP_PHASER + ( src[0] - '1' );
	} else if ( src[0] == 'h' || src[0] == 'H' ) {
		weapon = WP_VOYAGER_HYPO;
	} else if ( src[0] == 'a' || src[0] == 'A' ) {
		weapon = WP_BORG_ASSIMILATOR;
	} else if ( src[0] == 'b' || src[0] == 'B' ) {
		weapon = WP_BORG_WEAPON;
	} else {
		return;
	}

	if ( src[1] == 'p' || src[1] == 'P' ) {
		alt = qfalse;
	} else if ( src[1] == 'a' || src[1] == 'A' ) {
		alt = qtrue;
	} else {
		return;
	}

	if ( src[2] != ':' ) {
		return;
	}

	if ( alt ) {
		output->alt[weapon] = atoi( &src[3] );
	} else {
		output->primary[weapon] = atoi( &src[3] );
	}
}

/*
==================
CG_ParseWeaponValues

Parse string of weapon values such as fire rates or ammo usage.
Format example: "1p:50 2a:100" represents a value of 50 for phaser primary, 100 for rifle alt, and 0 for others.
==================
*/
static void CG_ParseWeaponValues( const char *src, weaponValues_t *output ) {
	char srcBuffer[256];
	char *current = srcBuffer;
	Q_strncpyz( srcBuffer, src, sizeof( srcBuffer ) );

	memset( output, 0, sizeof( *output ) );

	while ( current ) {
		char *next = strchr( current, ' ' );
		if ( next ) {
			*next = '\0';
			++next;
		}

		CG_ParseWeaponValue( current, output );
		current = next;
	}
}

/*
==================
CG_ParseModConfig

The mod config format provides a standard method for servers to send mod configuration
data to clients. Any configstring index can be used to send mod config values, to allow
the server more flexibility to avoid conflicts or optimize. In the case of duplicate
keys the numerically higher configstring index takes precedence.

Configstrings containing mod config data begin with the prefix "!modcfg ", followed by
a standard slash-separated info string similar to serverinfo or systeminfo.
==================
*/
void CG_ParseModConfig( void ) {
	int i;
	char key[BIG_INFO_STRING];
	char value[BIG_INFO_STRING];

	memset( &cgs.modConfig, 0, sizeof( cgs.modConfig ) );
	memset( cgs.modConfigSet, 0, sizeof( cgs.modConfigSet ) );

	CG_WeaponPredict_ResetConfig();

	// look for any configstring matching "!modcfg " prefix
	for ( i = 0; i < MAX_CONFIGSTRINGS; ++i ) {
		const char *str = CG_ConfigString( i );

		if ( str[0] == '!' && !Q_stricmpn( str, "!modcfg ", 8 ) && EF_WARN_ASSERT( strlen( str ) < BIG_INFO_STRING ) ) {
			const char *cur = &str[8];

			// load values
			while ( 1 ) {
				Info_NextPair( &cur, key, value );
				if ( !key[0] )
					break;
				if ( !Q_stricmp( key, "pMoveFixed" ) )
					cgs.modConfig.pMoveFixed = atoi( value );
				if ( !Q_stricmp( key, "pMoveTriggerMode" ) )
					cgs.modConfig.pMoveTriggerMode = atoi( value );
				if ( !Q_stricmp( key, "noJumpKeySlowdown" ) )
					cgs.modConfig.noJumpKeySlowdown = atoi( value ) ? qtrue : qfalse;
				if ( !Q_stricmp( key, "bounceFix" ) )
					cgs.modConfig.bounceFix = atoi( value ) ? qtrue : qfalse;
				if ( !Q_stricmp( key, "snapVectorGravLimit" ) )
					cgs.modConfig.snapVectorGravLimit = atoi( value );
				if ( !Q_stricmp( key, "noFlyingDrift" ) )
					cgs.modConfig.noFlyingDrift = atoi( value ) ? qtrue : qfalse;
				if ( !Q_stricmp( key, "infilJumpFactor" ) )
					cgs.modConfig.infilJumpFactor = atof( value );
				if ( !Q_stricmp( key, "infilAirAccelFactor" ) )
					cgs.modConfig.infilAirAccelFactor = atof( value );
				if ( !Q_stricmp( key, "altSwapSupport" ) )
					cgs.modConfig.altSwapSupport = atoi( value ) ? qtrue : qfalse;
				if ( !Q_stricmp( key, "altSwapPrefs" ) )
					Q_strncpyz( cgs.modConfig.altSwapPrefs, value, sizeof( cgs.modConfig.altSwapPrefs ) );
				if ( !Q_stricmp( key, "fireRate" ) )
					CG_ParseWeaponValues( value, &cgs.modConfig.fireRates );
				if ( !Q_stricmp( key, "weaponPredict" ) )
					CG_WeaponPredict_LoadConfig( value );
#ifdef ADDON_RESPAWN_TIMER
				if ( !Q_stricmp( key, "respawnTimerStatIndex" ) ) {
					cgs.modConfig.respawnTimerStatIndex = atoi( value );
				}
#endif
			}

			cgs.modConfigSet[i] = qtrue;
		}
	}

	CG_AltFire_UpdateServerPrefs();
}

/*
==================
CG_ParseWarmup
==================
*/
static void CG_ParseWarmup( void ) {
	const char	*info;
	int			warmup;

	info = CG_ConfigString( CS_WARMUP );

	warmup = atoi( info );
	cg.warmupCount = -1;

	if ( warmup == 0 && cg.warmup ) {

	} else if ( warmup > 0 && cg.warmup <= 0 ) {
		trap_S_StartLocalSound( cgs.media.countPrepareSound, CHAN_ANNOUNCER );
	}

	cg.warmup = warmup;
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues( void ) {
	const char *s;

	cgs.scores1 = atoi( CG_ConfigString( CS_SCORES1 ) );
	cgs.scores2 = atoi( CG_ConfigString( CS_SCORES2 ) );
	cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );
	s = CG_ConfigString( CS_FLAGSTATUS );
	cgs.redflag = s[0] - '0';
	cgs.blueflag = s[1] - '0';
	cg.warmup = atoi( CG_ConfigString( CS_WARMUP ) );
}

/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified( void ) {
	const char	*str;
	int		num;

	num = atoi( CG_Argv( 1 ) );

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState( &cgs.gameState );

	// look up the individual string that was modified
	str = CG_ConfigString( num );

	// check if a mod config string was changed
	if ( !Q_stricmpn( str, "!modcfg ", 8 ) || cgs.modConfigSet[num] ) {
		CG_ParseModConfig();
	}

	// do something with it if necessary
	if ( num == CS_MUSIC ) {
		CG_StartMusic();
	} else if ( num == CS_SERVERINFO ) {
		CG_ParseServerinfo();
	} else if ( num == CS_WARMUP ) {
		CG_ParseWarmup();
	} else if ( num == CS_SCORES1 ) {
		cgs.scores1 = atoi( str );
	} else if ( num == CS_SCORES2 ) {
		cgs.scores2 = atoi( str );
	} else if ( num == CS_WARMUP ) {
		CG_ParseWarmup();
	} else if ( num == CS_LEVEL_START_TIME ) {
		cgs.levelStartTime = atoi( str );
	} else if ( num == CS_VOTE_TIME ) {
		cgs.voteTime = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_YES ) {
		cgs.voteYes = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_NO ) {
		cgs.voteNo = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_STRING ) {
		Q_strncpyz( cgs.voteString, str, sizeof( cgs.voteString ) );
	} else if ( num == CS_INTERMISSION ) {
		cg.intermissionStarted = atoi( str );
	} else if ( num >= CS_MODELS && num < CS_MODELS+MAX_MODELS ) {
		cgs.gameModels[ num-CS_MODELS ] = trap_R_RegisterModel( str );
	} else if ( num >= CS_SOUNDS && num < CS_SOUNDS+MAX_SOUNDS ) {
		if ( str[0] != '*' ) {	// player specific sounds don't register here
			cgs.gameSounds[ num-CS_SOUNDS] = trap_S_RegisterSound( str );
		}
	} else if ( num >= CS_PLAYERS && num < CS_PLAYERS+MAX_CLIENTS ) {
		CG_NewClientInfo( num - CS_PLAYERS );
	} else if ( num == CS_FLAGSTATUS ) {
		// format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped
		cgs.redflag = str[0] - '0';
		cgs.blueflag = str[1] - '0';
	}

}


/*
=======================
CG_AddToTeamChat

=======================
*/
static void CG_AddToTeamChat( const char *str ) {
	int len;
	char *p, *ls;
	int lastcolor;
	int chatHeight;

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT) {
		chatHeight = cg_teamChatHeight.integer;
	} else {
		chatHeight = TEAMCHAT_HEIGHT;
	}

	if (chatHeight <= 0 || cg_teamChatTime.integer <= 0) {
		// team chat disabled, dump into normal chat
		cgs.teamChatPos = cgs.teamLastChatPos = 0;
		return;
	}

	len = 0;

	p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while (*str) {
		if (len > TEAMCHAT_WIDTH - 1) {
			if (ls) {
				str -= (p - ls);
				str++;
				p -= (p - ls);
			}
			*p = 0;

			cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;

			cgs.teamChatPos++;
			p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = NULL;
		}

		if ( Q_IsColorString( str ) ) {
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if (*str == ' ') {
			ls = p;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;

	cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
	cgs.teamChatPos++;

	if (cgs.teamChatPos - cgs.teamLastChatPos > chatHeight)
		cgs.teamLastChatPos = cgs.teamChatPos - chatHeight;
}



/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart( void ) {
	if ( cg_showmiss.integer ) {
		CG_Printf( "CG_MapRestart\n" );
	}

	CG_InitLocalEntities();
	CG_InitMarkPolys();

	// make sure the "3 frags left" warnings play again
	cg.fraglimitWarnings = 0;

	cg.timelimitWarnings = 0;

	cg.intermissionStarted = qfalse;

	cgs.voteTime = 0;

	cg.mapRestartUsercmd = trap_GetCurrentCmdNumber();

	CG_StartMusic();

	// we really should clear more parts of cg here and stop sounds

	// play the "fight" sound if this is a restart without warmup
	if ( cg.warmup == 0 /* && cgs.gametype == GT_TOURNAMENT */)
	{
		trap_S_StartLocalSound( cgs.media.countFightSound, CHAN_ANNOUNCER );
	}
}

/*
=================
CG_PrintChat

Print a chat message but check for ignored players.
=================
*/

static void CG_PrintChat(const char *chatstring, int chattype)
{
	clientInfo_t *curclient;
	const char *chatptr = chatstring;
	int namelen;

	for(curclient = cgs.clientinfo; (curclient - cgs.clientinfo) < MAX_CLIENTS; curclient++)
	{
		if(curclient->infoValid && curclient->ignore)
		{
			namelen = strlen(curclient->name);

			if(chattype)
			{
				if(chatptr[0] == '(' &&
					!Q_stricmpn(chatptr+1, curclient->name, namelen) &&
					chatptr[namelen+1] == '^' && chatptr[namelen+2] == '7' &&
					chatptr[namelen+3] == ')' &&
					((chatptr[namelen+4] == ':' && chatptr[namelen+5] == ' ') ||
					(chatptr[namelen+4] == ' ' && chatptr[namelen+5] == '('))
					)
				{
					// This user is being ingored.
					return;
				}
			}
			else
			{
				if(
					(!Q_stricmpn(chatptr, curclient->name, namelen) &&
					chatptr[namelen] == '^' && chatptr[namelen+1] == '7' &&
					chatptr[namelen+2] == ':' && chatptr[namelen+3] == ' ')
					||
					(chatptr[0] == '[' &&
					!Q_stricmpn(chatptr+1, curclient->name, namelen) &&
					chatptr[namelen+1] == '^' && chatptr[namelen+2] == '7' &&
					chatptr[namelen+3] == ']' &&
					((chatptr[namelen+4] == ':' && chatptr[namelen+5] == ' ') ||
					(chatptr[namelen+4] == ' ' && chatptr[namelen+5] == '('))
					)
					)
				{
					// This user is being ignored.
					return;
				}
			}
		}

	}

	trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);

	if(chattype)
		CG_AddToTeamChat(chatstring);

	CG_Printf("%s\n", chatstring);
}

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
static void CG_ServerCommand( void ) {
	const char	*cmd;

	cmd = CG_Argv(0);

	if ( !cmd[0] ) {
		// server claimed the command
		return;
	}

	if ( !strcmp( cmd, "cp" ) ) {
		CG_CenterPrint( CG_Argv(1), SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH );
		return;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CG_ConfigStringModified();
		return;
	}

	if ( !strcmp( cmd, "print" ) ) {
		CG_Printf( "%s", CG_Argv(1) );
		return;
	}

	if ( !strcmp( cmd, "chat" ) ) {
		CG_PrintChat(CG_Argv(1), 0);
		return;
	}

	if ( !strcmp( cmd, "pc" ) ) {
		trap_Cvar_Set("ui_playerclass", CG_Argv(1));
		return;
	}

	if ( !strcmp( cmd, "tchat" ) ) {
		CG_PrintChat(CG_Argv(1), 1);
		return;
	}

	if ( !strcmp( cmd, "scores" ) ) {
		CG_ParseScores();
		return;
	}

	if ( !strcmp( cmd, "awards" ) ) {
		AW_SPPostgameMenu_f();
		return;
	}

	if ( !strcmp( cmd, "tinfo" ) ) {
		CG_ParseTeamInfo();
		return;
	}

	if ( !strcmp( cmd, "hinfo" ) ) {
		CG_ParseHealthInfo();
		return;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		CG_MapRestart();
		return;
	}

	// loaddeferred can be both a servercmd and a consolecmd
	if ( !strcmp( cmd, "loaddefered" ) ) {	// FIXME: spelled wrong, but not changing for demo
		CG_LoadDeferredPlayers();
		return;
	}

	// clientLevelShot is sent before taking a special screenshot for
	// the menu system during development
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		cg.levelShot = qtrue;
		return;
	}

	CG_Printf( "Unknown client game command: %s\n", cmd );
}


/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands( int latestSequence ) {
	while ( cgs.serverCommandSequence < latestSequence ) {
		if ( trap_GetServerCommand( ++cgs.serverCommandSequence ) ) {
			CG_ServerCommand();
		}
	}
}
