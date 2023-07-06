/*
* Razor Greeting
* 
* Handle introduction messages displayed at the beginning of the game.
*/

#define MOD_NAME ModRazorGreeting

#include "mods/modes/razor/razor_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
================
ModRazorGreeting_GametypeString
================
*/
static const char *ModRazorGreeting_GametypeString( void ) {
	if ( modcfg.mods_enabled.elimination ) {
		return "Elimination";
	}

	switch ( g_gametype.integer ) {
		default:
			return "Free For All";
		case GT_TOURNAMENT:
			return "Tournament";
		case GT_TEAM:
			return "Team Holomatch";
		case GT_CTF:
			return "Capture the Flag";
	}
}

/*
================
(ModFN) SpawnCenterPrintMessage
================
*/
static void MOD_PREFIX(SpawnCenterPrintMessage)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	if ( spawnType != CST_RESPAWN ) {
		trap_SendServerCommand( clientNum, va( "cp \"%s\n\ngameplay level: %s%s\"",
				ModRazorGreeting_GametypeString(), ModRazorDifficulty_Static_DifficultyString(),
				ModPingcomp_Static_PingCompensationEnabled() ? "\n\nunlagged" : "" ) );
	}
}

/*
===========
(ModFN) PostClientSpawn

Add information print similar to original Pinball mod.
============
*/
static void MOD_PREFIX(PostClientSpawn)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( PostClientSpawn, ( MODFN_NC, clientNum, spawnType ) );

	if ( spawnType != CST_RESPAWN ) {
		char message[1024];
		message[0] = '\0';
		if ( spawnType != CST_FIRSTTIME ) {
			Q_strcat( message, sizeof( message ), va( "This server is Unlagged: full lag compensation is %s!\n",
					ModPingcomp_Static_PingCompensationEnabled() ? "ON" : "OFF" ) );
		}
		Q_strcat( message, sizeof(message), "^7Welcome to ^1Razor Arena^7! ^0(cMod " __DATE__ ")\n"
				"^7based on PiNBALL by Lt. Cmdr. Salinga\n" );

		trap_SendServerCommand( clientNum, va( "print \"%s\"", message ) );
	}
}

/*
==================
(ModFN) SetSpawnCS

If map message field is empty, add Razor message in place of it.
==================
*/
static void MOD_PREFIX(SetSpawnCS)( MODFN_CTV, int num, const char *defaultValue ) {
	if ( num == CS_MESSAGE && !*defaultValue ) {
		trap_SetConfigstring( num, va( "^1RaZor^4%s^7 - get your ass kicked!",
				ModPingcomp_Static_PingCompensationEnabled() ? " unlagged" : "" ) );
	} else {
		MODFN_NEXT( SetSpawnCS, ( MODFN_NC, num, defaultValue ) );
	}
}

/*
================
ModRazorGreeting_Init
================
*/
void ModRazorGreeting_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( SpawnCenterPrintMessage, ++modePriorityLevel );
		MODFN_REGISTER( PostClientSpawn, ++modePriorityLevel );
		MODFN_REGISTER( SetSpawnCS, ++modePriorityLevel );
	}
}
