// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_bot.c

#include "g_local.h"

#define ARENA_LIST_BUFFER ( 128 * 1024 )

static int		g_numBots;
static char		*g_botInfos[MAX_BOTS];

// Enables announcing each bot one by one in single player.
// Normally disabled in EF 1.20.
// #define USE_BOT_DELAY

#define BOT_BEGIN_DELAY_BASE		2000
#define BOT_BEGIN_DELAY_INCREMENT	2000

#define BOT_SPAWN_QUEUE_DEPTH	16

typedef struct {
	int		clientNum;
	int		spawnTime;
} botSpawnQueue_t;

static int				botBeginDelay;
static botSpawnQueue_t	botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];


/*
===============
G_GetArenaInfoByMapFromFile

Returns empty string if map not found in file.
===============
*/
static void G_GetArenaInfoByMapFromFile( const char *path, const char *map, char *buffer, unsigned int bufSize ) {
	int len;
	fileHandle_t f;
	char buf[MAX_ARENAS_TEXT];
	infoParseResult_t ipr;

	*buffer = '\0';

	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( !f ) {
		trap_Printf( va( S_COLOR_RED "file not found: %s\n", path ) );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap_Printf( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", path, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	if ( BG_ParseInfoForMap( buf, map, &ipr ) ) {
		Q_strncpyz( buffer, ipr.info, bufSize );
	}
}

/*
===============
G_GetArenaInfoByMap

Returns empty string if map not found.
===============
*/
static void G_GetArenaInfoByMap( const char *map, char *buffer, unsigned int bufSize ) {
	static char *dirlist = NULL;
	int numdirs;
	vmCvar_t arenasFile;
	char *dirptr;
	int i;
	int dirlen;

	*buffer = '\0';

	if ( !dirlist ) {
		// not very efficient, but should be adequate for solo match mode
		dirlist = G_Alloc( ARENA_LIST_BUFFER );
	}

	trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT | CVAR_ROM );
	if ( *arenasFile.string ) {
		G_GetArenaInfoByMapFromFile( arenasFile.string, map, buffer, bufSize );
	} else {
		G_GetArenaInfoByMapFromFile( "scripts/arenas.txt", map, buffer, bufSize );
	}
	if ( *buffer ) {
		return;
	}

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList( "scripts", ".arena", dirlist, ARENA_LIST_BUFFER );
	dirptr = dirlist;
	for ( i = 0; i < numdirs; i++, dirptr += dirlen + 1 ) {
		char fullPath[256];
		dirlen = strlen( dirptr );
		Com_sprintf( fullPath, sizeof( fullPath ), "scripts/%s", dirptr );
		G_GetArenaInfoByMapFromFile( fullPath, map, buffer, bufSize );
		if ( *buffer ) {
			return;
		}
	}
}


/*
=================
PlayerIntroSound
=================
*/
static void PlayerIntroSound( const char *modelAndSkin ) {
	char	model[MAX_QPATH];
	char	*skin;

	Q_strncpyz( model, modelAndSkin, sizeof(model) );
	skin = Q_strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	}
	else {
		skin = model;
	}

	if( Q_stricmp( skin, "default" ) == 0 ) {
		skin = model;
	}
	if( Q_stricmp( skin, "red" ) == 0 ) {
		skin = model;
	}
	if( Q_stricmp( skin, "blue" ) == 0 ) {
		skin = model;
	}
	//precached in cg_info.c, CG_LoadingClient
	trap_SendConsoleCommand( EXEC_APPEND, va( "play sound/voice/computer/misc/%s.wav\n", skin ) );
}

/*
===============
G_CountBotInstances

Check if a certain bot is already in the game.
===============
*/
static int G_CountBotInstances( const char *netname, int team ) {
	int i;
	gclient_t *cl;
	int count = 0;

	for ( i = 0; i < g_maxclients.integer; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_DISCONNECTED && ( g_entities[i].r.svFlags & SVF_BOT ) &&
				( team < 0 || modfn.RealSessionTeam( i ) == team ) && !Q_stricmp( cl->pers.netname, netname ) ) {
			++count;
		}
	}

	return count;
}

/*
===============
G_RandomBotInfo

Selects random bot and returns bot info for it. Returns NULL on error.
===============
*/
static char *G_RandomBotInfo( int team ) {
	qboolean avoidExisting = qtrue;
	int		n, num;
	char	*value;

	if ( !g_numBots ) {
		return NULL;
	}

	// look for bots not already in the game
	num = 0;
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		if ( !G_CountBotInstances( value, team ) ) {
			num++;
		}
	}

	// if none found, select from all available bots
	if ( !num ) {
		num = g_numBots;
		avoidExisting = qfalse;
	}

	num = rand() % num + 1;
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		if ( !avoidExisting || !G_CountBotInstances( value, team ) ) {
			num--;
			if (num <= 0) {
				return g_botInfos[n];
			}
		}
	}

	return NULL;
}

/*
===============
G_AddRandomBot
===============
*/
void G_AddRandomBot( int team ) {
	int skill = trap_Cvar_VariableIntegerValue( "g_spSkill" );
	char *teamstr = "auto";

	if ( team == TEAM_RED ) {
		teamstr = "red";
	} else if ( team == TEAM_BLUE ) {
		teamstr = "blue";
	}

	trap_SendConsoleCommand( EXEC_INSERT, va( "addbot random %i %s\n", skill, teamstr ) );
}

/*
===============
G_RemoveRandomBot
===============
*/
int G_RemoveRandomBot( int team ) {
	int i;
	gclient_t	*cl;

	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && modfn.RealSessionTeam( i ) != team ) {
			continue;
		}
		trap_SendConsoleCommand( EXEC_INSERT, va("kick \"%i\"\n", i) );
		return qtrue;
	}
	return qfalse;
}

/*
===============
G_CountHumanPlayers
===============
*/
int G_CountHumanPlayers( int team ) {
	int i, num;
	gclient_t	*cl;

	num = 0;
	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}
		if ( team >= 0 && modfn.RealSessionTeam( i ) != team ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CountBotPlayers
===============
*/
int G_CountBotPlayers( int team ) {
	int i, num;
	gclient_t	*cl;

	num = 0;
	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		if ( team >= 0 && modfn.RealSessionTeam( i ) != team ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CheckMinimumPlayers
===============
*/
void G_CheckMinimumPlayers( void ) {
	int minplayers;
	int humanplayers, botplayers;
	static int checkminimumplayers_time;

	if (level.intermissiontime) return;
	//only check once each 10 seconds
	if (checkminimumplayers_time > level.time - 10000) {
		return;
	}
	if ( modfn.AdjustGeneralConstant( GC_DISABLE_BOT_ADDING, 0 ) ) {
		return;
	}
	checkminimumplayers_time = level.time;
	minplayers = bot_minplayers.integer;
	if (minplayers <= 0) return;

	if (g_gametype.integer >= GT_TEAM) {
		if (minplayers >= g_maxclients.integer / 2) {
			minplayers = (g_maxclients.integer / 2) -1;
		}

		humanplayers = G_CountHumanPlayers( TEAM_RED );
		botplayers = G_CountBotPlayers(	TEAM_RED );
		//
		if (humanplayers + botplayers + G_CountBotPlayers( TEAM_SPECTATOR ) < minplayers) {
			G_AddRandomBot( TEAM_RED );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_RED );
		}
		//
		humanplayers = G_CountHumanPlayers( TEAM_BLUE );
		botplayers = G_CountBotPlayers( TEAM_BLUE );
		//
		if (humanplayers + botplayers + G_CountBotPlayers( TEAM_SPECTATOR ) < minplayers) {
			G_AddRandomBot( TEAM_BLUE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_BLUE );
		}
	}
	else if (g_gametype.integer == GT_TOURNAMENT) {
		if (minplayers >= g_maxclients.integer) {
			minplayers = g_maxclients.integer-1;
		}
		humanplayers = G_CountHumanPlayers( -1 );
		botplayers = G_CountBotPlayers( -1 );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			// try to remove spectators first
			if (!G_RemoveRandomBot( TEAM_SPECTATOR )) {
				// just remove the bot that is playing
				G_RemoveRandomBot( -1 );
			}
		}
	}
	else if (g_gametype.integer == GT_FFA) {
		if (minplayers >= g_maxclients.integer) {
			minplayers = g_maxclients.integer-1;
		}
		humanplayers = G_CountHumanPlayers( TEAM_FREE );
		botplayers = G_CountBotPlayers( -1 );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_FREE );
		}
	}
}

/*
===============
G_CheckBotSpawn
===============
*/
void G_CheckBotSpawn( void ) {
	int		n;
	char	userinfo[MAX_INFO_VALUE];

	G_CheckMinimumPlayers();

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		if ( level.clients[botSpawnQueue[n].clientNum].pers.connected != CON_CONNECTING ||
				!( g_entities[botSpawnQueue[n].clientNum].r.svFlags & SVF_BOT ) ) {
			// bot might have been kicked before being spawned
			botSpawnQueue[n].spawnTime = 0;
			continue;
		}
		ClientBegin( botSpawnQueue[n].clientNum );
		botSpawnQueue[n].spawnTime = 0;

		if( g_gametype.integer == GT_SINGLE_PLAYER ) {
			trap_GetUserinfo( botSpawnQueue[n].clientNum, userinfo, sizeof(userinfo) );
			PlayerIntroSound( Info_ValueForKey (userinfo, "model") );
		}
	}
}


/*
===============
AddBotToSpawnQueue
===============
*/
static void AddBotToSpawnQueue( int clientNum, int delay ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			botSpawnQueue[n].spawnTime = level.time + delay;
			botSpawnQueue[n].clientNum = clientNum;
			return;
		}
	}

	G_Printf( S_COLOR_YELLOW "Unable to delay spawn\n" );
	ClientBegin( clientNum );
}


/*
===============
G_QueueBotBegin
===============
*/
void G_QueueBotBegin( int clientNum ) {
	AddBotToSpawnQueue( clientNum, botBeginDelay );
	botBeginDelay += BOT_BEGIN_DELAY_INCREMENT;
}


/*
===============
G_BotConnect
===============
*/
qboolean G_BotConnect( int clientNum, qboolean restart ) {
	bot_settings_t	settings;
	char			userinfo[MAX_INFO_STRING];

	trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );

	Q_strncpyz( settings.characterfile, Info_ValueForKey( userinfo, "characterfile" ), sizeof(settings.characterfile) );
	settings.skill = atoi( Info_ValueForKey( userinfo, "skill" ) );
	Q_strncpyz( settings.team, Info_ValueForKey( userinfo, "team" ), sizeof(settings.team) );
	Q_strncpyz( settings.pclass, Info_ValueForKey( userinfo, "class" ), sizeof(settings.pclass) );
	
	if ( !settings.team[0] && level.clients[clientNum].sess.sessionTeam == TEAM_SPECTATOR ) {
		// If team is not set, and bot is currently spectator (after the session init which
		// should already have been done at this point), have the bot try to auto join.
		Q_strncpyz( settings.team, "auto", sizeof( settings.team ) );
	}

	if (!BotAISetupClient( clientNum, &settings )) {
		trap_DropClient( clientNum, "BotAISetupClient failed" );
		return qfalse;
	}

	if( restart && g_gametype.integer == GT_SINGLE_PLAYER ) {
		level.clients[clientNum].botDelayBegin = qtrue;
	}
	else {
		level.clients[clientNum].botDelayBegin = qfalse;
	}

	return qtrue;
}


/*
===============
G_AddBot
===============
*/
static void G_AddBot( const char *name, int skill, const char *team, const char *pclass, int delay, char *altname) {
	int				clientNum;
	char			*botinfo;
	gentity_t		*bot;
	char			*key;
	char			*s;
	char			*botname;
	char			*model;
	char			userinfo[MAX_INFO_STRING];

	if ( !Q_stricmp( name, "random" ) ) {
		// if team is specified, try to avoid picking a character already on that team
		team_t teamVal = TEAM_FREE;
		if ( !Q_stricmp( team, "red" ) ) {
			teamVal = TEAM_RED;
		} else if ( !Q_stricmp( team, "blue" ) ) {
			teamVal = TEAM_BLUE;
		}
		botinfo = G_RandomBotInfo( teamVal );
	} else {
		botinfo = G_GetBotInfoByName( name );
	}
	if ( !botinfo ) {
		G_Printf( S_COLOR_RED "Error: Bot '%s' not defined\n", name );
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	botname = Info_ValueForKey( botinfo, "funname" );
	if( !botname[0] ) {
		botname = Info_ValueForKey( botinfo, "name" );
	}
	// check for an alternative name
	if (altname && altname[0]) {
		botname = altname;
	}
	Info_SetValueForKey( userinfo, "name", botname );
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );
	Info_SetValueForKey( userinfo, "skill", va("%i", skill) );

	if ( skill == 1 ) {
		Info_SetValueForKey( userinfo, "handicap", "50" );
	}
	else if ( skill == 2 ) {
		Info_SetValueForKey( userinfo, "handicap", "75" );
	}

	key = "model";
	model = Info_ValueForKey( botinfo, key );
	if ( !*model ) {
		model = "munro/default";
	}
	Info_SetValueForKey( userinfo, key, model );

	key = "gender";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "male";
	}
	Info_SetValueForKey( userinfo, "sex", s );

	key = "color";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "4";
	}
	Info_SetValueForKey( userinfo, key, s );

	s = Info_ValueForKey(botinfo, "aifile");
	if (!*s ) {
		trap_Printf( S_COLOR_RED "Error: bot has no aifile specified\n" );
		return;
	}

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if ( clientNum == -1 ) {
		G_Printf( S_COLOR_RED "Unable to add bot.  All player slots are in use.\n" );
		G_Printf( S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n" );
		return;
	}

	// initialize the bot settings
	Info_SetValueForKey( userinfo, "characterfile", Info_ValueForKey( botinfo, "aifile" ) );
	Info_SetValueForKey( userinfo, "skill", va( "%i", skill ) );
	if( team && *team ) {
		Info_SetValueForKey( userinfo, "team", team );
	}
	if( pclass && *pclass && modfn.AdjustGeneralConstant( GC_ALLOW_BOT_CLASS_SPECIFIER, 0 ) ) {
		Info_SetValueForKey( userinfo, "class", pclass );
	}

	bot = &g_entities[ clientNum ];
	bot->r.svFlags |= SVF_BOT;
	bot->inuse = qtrue;

	// register the userinfo
	trap_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if ( ClientConnect( clientNum, qtrue, qtrue ) ) {
		return;
	}

	if( delay == 0 ) {
		ClientBegin( clientNum );
		return;
	}

	AddBotToSpawnQueue( clientNum, delay );
}


/*
===============
Svcmd_AddBot_f
===============
*/
void Svcmd_AddBot_f( void ) {
	int				skill;
	int				delay;
	char			name[MAX_TOKEN_CHARS];
	char			altname[MAX_TOKEN_CHARS];
	char			string[MAX_TOKEN_CHARS];
	char			team[MAX_TOKEN_CHARS];
	char			pclass[MAX_TOKEN_CHARS];

	// are bots enabled?
	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	// name
	trap_Argv( 1, name, sizeof( name ) );
	if ( !name[0] ) {
		trap_Printf( "Usage: Addbot <botname> [skill 1-4] [team] [class] [msec delay] [altname]\n" );
		return;
	}

	// skill
	trap_Argv( 2, string, sizeof( string ) );
	if ( !string[0] ) {
		skill = 4;
	}
	else {
		skill = atoi( string );
	}

	// team
	trap_Argv( 3, team, sizeof( team ) );

	// class
	trap_Argv( 4, pclass, sizeof( pclass ) );

	// delay
	trap_Argv( 5, string, sizeof( string ) );
	if ( !string[0] ) {
		delay = 0;
	}
	else {
		delay = atoi( string );
	}

	// alternative name
	trap_Argv( 6, altname, sizeof( altname ) );

	G_AddBot( name, skill, team, pclass, delay, altname );

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if ( level.time - level.startTime > 1000 &&
		trap_Cvar_VariableIntegerValue( "cl_running" ) ) {
		trap_SendServerCommand( -1, "loaddefered\n" );	// FIXME: spelled wrong, but not changing for demo
	}
}

/*
===============
Svcmd_BotList_f
===============
*/
void Svcmd_BotList_f( void ) {
	int i;
	char name[MAX_TOKEN_CHARS];
	char funname[MAX_TOKEN_CHARS];
	char model[MAX_TOKEN_CHARS];
	char aifile[MAX_TOKEN_CHARS];

	trap_Printf("^1name             model            aifile              funname\n");
	for (i = 0; i < g_numBots; i++) {
		strcpy(name, Info_ValueForKey( g_botInfos[i], "name" ));
		if ( !*name ) {
			strcpy(name, "RedShirt");
		}
		strcpy(funname, Info_ValueForKey( g_botInfos[i], "funname" ));
		if ( !*funname ) {
			strcpy(funname, "");
		}
		strcpy(model, Info_ValueForKey( g_botInfos[i], "model" ));
		if ( !*model ) {
			strcpy(model, "munro/default");
		}
		strcpy(aifile, Info_ValueForKey( g_botInfos[i], "aifile"));
		if (!*aifile ) {
			strcpy(aifile, "bots/default_c.c");
		}
		trap_Printf(va("%-16s %-16s %-20s %-20s\n", name, model, aifile, funname));
	}
}


/*
===============
G_SpawnBots
===============
*/
static void G_SpawnBots( char *botList, int baseDelay ) {
	char		*bot;
	char		*p;
	int			skill;
	int			delay;
	char		bots[MAX_INFO_VALUE];

	skill = trap_Cvar_VariableIntegerValue( "g_spSkill" );
	if( skill < 1 || skill > 5 ) {
		trap_Cvar_Set( "g_spSkill", "2" );
		skill = 2;
	}

	Q_strncpyz( bots, botList, sizeof(bots) );
	p = &bots[0];
	delay = baseDelay;
	while( *p ) {
		//skip spaces
		while( *p && *p == ' ' ) {
			p++;
		}
		if( !p ) {
			break;
		}

		// mark start of bot name
		bot = p;

		// skip until space of null
		while( *p && *p != ' ' ) {
			p++;
		}
		if( *p ) {
			*p++ = 0;
		}

		// we must add the bot this way, calling G_AddBot directly at this stage
		// does "Bad Things"
#ifdef USE_BOT_DELAY
		trap_SendConsoleCommand( EXEC_INSERT, va("addbot %s %i free \"\" %i\n", bot, skill, delay) );
#else
		trap_SendConsoleCommand( EXEC_INSERT, va("addbot %s %i free \"\" 0\n", bot, skill) );
#endif

		delay += BOT_BEGIN_DELAY_INCREMENT;
	}
}


/*
===============
G_LoadBotsFromFile
===============
*/
static void G_LoadBotsFromFile( char *filename ) {
	int len;
	fileHandle_t f;
	char buf[MAX_BOTS_TEXT];
	char *parsePtr = buf;
	infoParseResult_t info;

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Printf( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_BOTS_TEXT ) {
		trap_Printf( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	while ( BG_ParseInfo( &parsePtr, &info ) ) {
		if ( g_numBots >= MAX_BOTS ) {
			return;
		}
		g_botInfos[g_numBots] = G_Alloc( strlen( info.info ) + 1 );
		if ( g_botInfos[g_numBots] ) {
			strcpy( g_botInfos[g_numBots], info.info );
			g_numBots++;
		}
	}
}

/*
===============
G_LoadBots
===============
*/
static void G_LoadBots( void ) {
	vmCvar_t	botsFile;
	int			numdirs;
	char		filename[128];
	char		dirlist[16384];
	char*		dirptr;
	int			i;
	int			dirlen;

	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	g_numBots = 0;

	// If g_botsFile is set, load bots exclusively from this file, like Gladiator mod.
	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_LATCH );
	if( *botsFile.string ) {
		G_LoadBotsFromFile(botsFile.string);
	}

	if ( !g_numBots ) {
		G_LoadBotsFromFile("scripts/bots.txt");

		// get all bots from .bot files
		numdirs = trap_FS_GetFileList("scripts", ".bot", dirlist, sizeof( dirlist ) );
		dirptr  = dirlist;
		for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
			dirlen = strlen(dirptr);
			Com_sprintf( filename, sizeof( filename ), "scripts/%s", dirptr );
			G_LoadBotsFromFile(filename);
			if ( g_numBots >= MAX_BOTS ) {
				break;
			}
		}
	}

	trap_Printf( va( "%i bots parsed\n", g_numBots ) );
}



/*
===============
G_GetBotInfoByName
===============
*/
char *G_GetBotInfoByName( const char *name ) {
	int		n;
	char	*value;

	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return g_botInfos[n];
		}
	}

	return NULL;
}

/*
===============
G_InitBots
===============
*/
void G_InitBots( qboolean restart ) {
	int			fragLimit;
	int			timeLimit;
	char		arenainfo[MAX_INFO_STRING];
	char		*strValue;
	int			basedelay;
	char		map[MAX_QPATH];
	char		serverinfo[MAX_INFO_STRING];

	G_LoadBots();

	if( g_gametype.integer == GT_SINGLE_PLAYER ) {
		trap_GetServerinfo( serverinfo, sizeof(serverinfo) );
		Q_strncpyz( map, Info_ValueForKey( serverinfo, "mapname" ), sizeof(map) );
		G_GetArenaInfoByMap( map, arenainfo, sizeof( arenainfo ) );
		if ( !*arenainfo ) {
			return;
		}

		strValue = Info_ValueForKey( arenainfo, "fraglimit" );
		fragLimit = atoi( strValue );
		if ( fragLimit ) {
			trap_Cvar_Set( "fraglimit", strValue );
		}
		else {
			trap_Cvar_Set( "fraglimit", "0" );
		}

		strValue = Info_ValueForKey( arenainfo, "timelimit" );
		timeLimit = atoi( strValue );
		if ( timeLimit ) {
			trap_Cvar_Set( "timelimit", strValue );
		}
		else {
			trap_Cvar_Set( "timelimit", "0" );
		}

		if ( !fragLimit && !timeLimit ) {
			trap_Cvar_Set( "fraglimit", "10" );
			trap_Cvar_Set( "timelimit", "0" );
		}

		if (g_holoIntro.integer)
		{	// The player will be looking at the holodeck doors for the first five seconds, so take that into account.
			basedelay = BOT_BEGIN_DELAY_BASE + TIME_INTRO;
		}
		else
		{
			basedelay = BOT_BEGIN_DELAY_BASE;
		}
		strValue = Info_ValueForKey( arenainfo, "special" );
		if( Q_stricmp( strValue, "training" ) == 0 ) {
			basedelay += 10000;
		}

		if( !restart ) {
			G_SpawnBots( Info_ValueForKey( arenainfo, "bots" ), basedelay );
		}
	}
}
