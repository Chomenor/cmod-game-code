/*
* Join Limit Handling
* 
* This module is used to restrict players from joining mid-match in Elimination
* and Assimilation modes. The match is considered locked after any players have
* been eliminated or assimilated, or after a certain time limit has been reached.
*/

#define MOD_NAME ModJoinLimit

#include "mods/g_mod_local.h"

static struct {
	trackedCvar_t g_noJoinTimeout;
	int joinLimitTime;	// time the match started
	qboolean locked;	// locked due to eliminated or assimilated players
} *MOD_STATE;

/*
================
ModJoinLimit_Static_MatchLocked

Returns whether join limits are currently active.
================
*/
qboolean ModJoinLimit_Static_MatchLocked( void ) {
	if ( MOD_STATE ) {
		if ( MOD_STATE->locked ) {
			return qtrue;
		}

		if ( level.matchState >= MS_ACTIVE && MOD_STATE->g_noJoinTimeout.integer > 0 &&
				level.time >= MOD_STATE->joinLimitTime + ( MOD_STATE->g_noJoinTimeout.integer * 1000 ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
ModJoinLimit_Static_StartMatchLock

Start match lock regardless of timer (e.g. when a player is eliminated or assimilated).
================
*/
void ModJoinLimit_Static_StartMatchLock( void ) {
	if ( MOD_STATE ) {
		MOD_STATE->locked = qtrue;
	}
}

/*
================
(ModFN Default) JoinLimitMessage
================
*/
void ModFNDefault_JoinLimitMessage( int clientNum, join_allowed_type_t type, team_t targetTeam ) {
	const gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( clientNum, "cp \"Wait until next round to join\"" );
	} else if ( type == CJA_SETCLASS ) {
		trap_SendServerCommand( clientNum, "cp \"Wait until next round to change classes\"" );
	} else {
		trap_SendServerCommand( clientNum, "cp \"Wait until next round to change teams\"" );
	}
}

/*
================
(ModFN) CheckJoinAllowed
================
*/
static qboolean MOD_PREFIX(CheckJoinAllowed)( MODFN_CTV, int clientNum, join_allowed_type_t type, team_t targetTeam ) {
	gclient_t *client = &level.clients[clientNum];

	if ( type != CJA_FORCETEAM && ModJoinLimit_Static_MatchLocked() ) {
		if ( type == CJA_SETTEAM || type == CJA_SETCLASS ) {
			modfn_lcl.JoinLimitMessage( clientNum, type, targetTeam );
		}

		return qfalse;
	}

	return MODFN_NEXT( CheckJoinAllowed, ( MODFN_NC, clientNum, type, targetTeam ) );
}

/*
================
(ModFN) PreClientSpawn

If a player was placed on a team when they connected, but the match become locked before
they finished loading, move them to spectator, consistent with Gladiator mod.
================
*/
static void MOD_PREFIX(PreClientSpawn)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	// This is a bit hacky, since normally team changes are always followed by a call
	// to ClientBegin, but with these spawn type values we should be being called through
	// ClientBegin already and the correct handling should be done.
	if ( ( spawnType == CST_FIRSTTIME || spawnType == CST_MAPCHANGE || spawnType == CST_MAPRESTART ) &&
			client->sess.sessionTeam != TEAM_SPECTATOR && ModJoinLimit_Static_MatchLocked() ) {
		modfn.PrePlayerLeaveTeam( clientNum, client->sess.sessionTeam );
		client->sess.sessionTeam = TEAM_SPECTATOR;
	}

	MODFN_NEXT( PreClientSpawn, ( MODFN_NC, clientNum, spawnType ) );
}

/*
================
(ModFN) MatchStateTransition
================
*/
static void MOD_PREFIX(MatchStateTransition)( MODFN_CTV, matchState_t oldState, matchState_t newState ) {
	MODFN_NEXT( MatchStateTransition, ( MODFN_NC, oldState, newState ) );

	if ( newState == MS_ACTIVE ) {
		G_DedPrintf( "join limit: Starting countdown due to sufficient players.\n" );
		MOD_STATE->joinLimitTime = level.time;
	} else if ( newState < MS_ACTIVE ) {
		if ( MOD_STATE->joinLimitTime ) {
			G_DedPrintf( "join limit: Resetting time due to insufficient players.\n" );
		}
		if ( MOD_STATE->locked ) {
			G_DedPrintf( "join limit: Resetting lock due to insufficient players.\n" );
		}
		MOD_STATE->joinLimitTime = 0;
		MOD_STATE->locked = qfalse;
	}
}

/*
================
ModJoinLimit_Init
================
*/
void ModJoinLimit_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_noJoinTimeout, "g_noJoinTimeout", "120", CVAR_ARCHIVE, qfalse );

		MODFN_REGISTER( CheckJoinAllowed, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PreClientSpawn, MODPRIORITY_GENERAL );
		MODFN_REGISTER( MatchStateTransition, MODPRIORITY_GENERAL );
	}
}
