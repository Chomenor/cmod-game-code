/*
* Elimination Tweaks
* 
* Adds some extra elimination features and tweaks, mostly from Gladiator mod.
* 
* Enabled by g_mod_elimTweaks cvar.
*/

#define MOD_NAME ModElimTweaks

#include "mods/modes/elimination/elim_local.h"

// Restart game when timelimit expires.
#define FEATURE_TIMELIMIT_RESTART

// Support timelimit adjustment.
#define FEATURE_FINALIST_TIMELIMIT

// Enable click to join during warmup.
#define FEATURE_CLICKJOIN

// Changes team change announcement to console print instead of center print, and disables "entered the game" messages.
// Needed for some survivor messages to display properly.
#define FEATURE_JOIN_MESSAGE_TWEAKS

// Display message and play remaining player voice countdown when player is eliminated.
#define FEATURE_ELIMINATED_MESSAGES

// Display message identifying the surviving player/team when round ends.
#define FEATURE_SURVIVOR_MESSAGES

// Use gladiator-style time indicator which shows seconds instead of minutes and freezes counter when
// player is eliminated.
#define FEATURE_SCOREBOARD_TIME_INDICATOR

// Don't show eliminated 'X' during intermission scoreboard.
#define FEATURE_SCOREBOARD_NO_INTERMISSION_X

// Change behavior of intermission ready checks and indicators to be consistent with gladiator mod.
#define FEATURE_INTERMISSION_READY_TWEAKS

// Disable suicides during warmup or intermission.
#define FEATURE_RESTRICT_SUICIDE

// Disable follow spectators cycling to players who have just been eliminated.
#define FEATURE_NO_ELIMINATED_CYCLE

typedef struct {
	int _unused;
#ifdef FEATURE_SCOREBOARD_TIME_INDICATOR
	int scoreboardEliminatedTime;
#endif
} elim_extras_client_t;

static struct {
	elim_extras_client_t clients[MAX_CLIENTS];

#ifdef FEATURE_TIMELIMIT_ADJUST
	trackedCvar_t g_mod_finalistsTimelimit;
	trackedCvar_t g_mod_onlyBotsTimelimit;
	qboolean finalistTimelimitActive;
	qboolean onlyBotsTimelimitActive;
	int oldTimelimit;
#endif
} *MOD_STATE;

#ifdef FEATURE_ELIMINATED_MESSAGES
/*
================
ModElimTweaks_RemainingPlayerSound
================
*/
static void ModElimTweaks_RemainingPlayerSound( int numRemaining ) {
	if ( numRemaining >= 1 && numRemaining <= 10 ) {
		G_GlobalSound( G_SoundIndex( va( "sound/voice/computer/voy3/%isec.wav", numRemaining ) ) );
	}
}

/*
================
ModElimTweaks_EliminatedMessage
================
*/
static void ModElimTweaks_EliminatedMessage( int clientNum, team_t oldTeam, qboolean eliminated ) {
	gclient_t *client = &level.clients[clientNum];
	const char *leavemsg = modcfg.mods_enabled.uam ? "left the battle" : "left the game";
	const char *playerterm = modcfg.mods_enabled.uam ? "gladiator" : "player";

	if ( g_gametype.integer >= GT_TEAM ) {
		int playersAlive = ModElimination_Shared_CountPlayersAliveTeam( oldTeam, clientNum );
		if ( playersAlive >= 1 ) {
			trap_SendServerCommand( -1, va( "cp \"%s ^7%s\n^%s%i %s%s of team %s left\"",
					client->pers.netname, eliminated ? "died" : leavemsg,
					oldTeam == TEAM_RED ? "1" : "4", playersAlive, playerterm,
					playersAlive == 1 ? "" : "s", oldTeam == TEAM_RED ? "red" : "blue" ) );
			ModElimTweaks_RemainingPlayerSound( playersAlive );
		}
	
	} else {
		int playersAlive = ModElimination_Shared_CountPlayersAliveTeam( TEAM_FREE, clientNum );
		if ( playersAlive >= 2 ) {
			trap_SendServerCommand( -1, va( "cp \"%s ^7%s\n^4%i %ss in the arena left\"",
					client->pers.netname, eliminated ? "died" : leavemsg, playersAlive, playerterm ) );
			ModElimTweaks_RemainingPlayerSound( playersAlive );
		}
	}
}
#endif

#ifdef FEATURE_INTERMISSION_READY_TWEAKS
/*
================
(ModFN) IntermissionReadyConfig
================
*/
static void MOD_PREFIX(IntermissionReadyConfig)( MODFN_CTV, modIntermissionReady_config_t *config ) {
	MODFN_NEXT( IntermissionReadyConfig, ( MODFN_NC, config ) );
	config->readySound = qtrue;
	config->ignoreSpectators = qtrue;
	config->noPlayersExit = qtrue;
	config->anyReadyTime = 30000;
	config->sustainedAnyReadyTime = 0;
	config->minExitTime = 10000;
	config->maxExitTime = 70000;
}
#endif

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
#ifdef FEATURE_JOIN_MESSAGE_TWEAKS
	if ( gcType == GC_SKIP_ENTER_GAME_PRINT )
		return 1;
	if ( gcType == GC_JOIN_MESSAGE_CONSOLE_PRINT )
		return 1;
#endif

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
================
(ModFN) AdjustScoreboardAttributes
================
*/
static int MOD_PREFIX(AdjustScoreboardAttributes)( MODFN_CTV, int clientNum, scoreboardAttribute_t saType, int defaultValue ) {
#ifdef FEATURE_SCOREBOARD_TIME_INDICATOR
	// Gladiator-style time indicator
	if ( saType == SA_PLAYERTIME ) {
		gclient_t *client = &level.clients[clientNum];
		elim_extras_client_t *modclient = &MOD_STATE->clients[clientNum];
		if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
			return 0;
		} else if ( ModElimination_Static_IsPlayerEliminated( clientNum ) ) {
			return modclient->scoreboardEliminatedTime;
		} else {
			return ( level.time - client->pers.enterTime ) / 1000;
		}
	}
#endif

#ifdef FEATURE_SCOREBOARD_NO_INTERMISSION_X
	if ( saType == SA_ELIMINATED && level.matchState >= MS_INTERMISSION_QUEUED ) {
		return 0;
	}
#endif

	return MODFN_NEXT( AdjustScoreboardAttributes, ( MODFN_NC, clientNum, saType, defaultValue ) );
}

/*
================
(ModFN) EnableCycleFollow
================
*/
static qboolean MOD_PREFIX(EnableCycleFollow)( MODFN_CTV, int clientNum ) {
#ifdef FEATURE_NO_ELIMINATED_CYCLE
	if ( ModElimination_Static_IsPlayerEliminated( clientNum ) ) {
		return qfalse;
	}
#endif

	return MODFN_NEXT( EnableCycleFollow, ( MODFN_NC, clientNum ) );
}

/*
=================
(ModFN) CheckSuicideAllowed

Disable suicide during warmup.
=================
*/
static qboolean MOD_PREFIX(CheckSuicideAllowed)( MODFN_CTV, int clientNum ) {
#ifdef FEATURE_RESTRICT_SUICIDE
	if ( level.matchState == MS_WARMUP || level.matchState >= MS_INTERMISSION_QUEUED ) {
		return qfalse;
	}
#endif

	return MODFN_NEXT( CheckSuicideAllowed, ( MODFN_NC, clientNum ) );
}

/*
================
(ModFN) PostPlayerDie
================
*/
static void MOD_PREFIX(PostPlayerDie)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int meansOfDeath, int *awardPoints ) {
	int clientNum = self - g_entities;
	gclient_t *client = &level.clients[clientNum];
	elim_extras_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( PostPlayerDie, ( MODFN_NC, self, inflictor, attacker, meansOfDeath, awardPoints ) );

	if ( ModElimination_Static_IsPlayerEliminated( clientNum ) ) {
#ifdef FEATURE_ELIMINATED_MESSAGES
		if ( level.matchState == MS_ACTIVE ) {
			ModElimTweaks_EliminatedMessage( clientNum, client->sess.sessionTeam, qtrue );
		}
#endif

#ifdef FEATURE_SCOREBOARD_TIME_INDICATOR
		modclient->scoreboardEliminatedTime = ( level.time - client->pers.enterTime ) / 1000;
#endif
	}
}

/*
================
(ModFN) PrePlayerLeaveTeam
================
*/
static void MOD_PREFIX(PrePlayerLeaveTeam)( MODFN_CTV, int clientNum, team_t oldTeam ) {
	gclient_t *client = &level.clients[clientNum];

#ifdef FEATURE_ELIMINATED_MESSAGES
	if ( level.matchState == MS_ACTIVE && client->pers.connected == CON_CONNECTED &&
			oldTeam != TEAM_SPECTATOR && !ModElimination_Static_IsPlayerEliminated( clientNum ) &&
			// don't display if being dropped via join limit connecting check
			client->pers.enterTime != level.time ) {
		ModElimTweaks_EliminatedMessage( clientNum, oldTeam, qfalse );
	}
#endif

	// Call this last, since it may reset eliminated status in elim_main.c
	// and break ModElimination_Static_IsPlayerEliminated.
	MODFN_NEXT( PrePlayerLeaveTeam, ( MODFN_NC, clientNum, oldTeam ) );
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

#ifdef FEATURE_TIMELIMIT_RESTART
	// Check for restart.
	if ( g_timelimit.integer && level.matchState == MS_ACTIVE && !level.exiting &&
			level.time - level.startTime >= g_timelimit.integer * 60000 ) {
		trap_SendServerCommand( -1, "print \"Timelimit hit, game restart...\n\"");
		level.exiting = qtrue;
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
	}
#endif
}

/*
================
(ModFN) MatchStateTransition
================
*/
static void MOD_PREFIX(MatchStateTransition)( MODFN_CTV, matchState_t oldState, matchState_t newState ) {
	MODFN_NEXT( MatchStateTransition, ( MODFN_NC, oldState, newState ) );

#ifdef FEATURE_SURVIVOR_MESSAGES
	if ( newState == MS_INTERMISSION_QUEUED ) {
		if ( g_gametype.integer >= GT_TEAM ) {
			trap_SendServerCommand( -1, va( "cp \"%s\n^2is the survivor!\n\"",
					level.winningTeam == TEAM_RED ? "^1TEAM RED" : "^4TEAM BLUE" ) );
		} else {
			int survivor = level.sortedClients[0];
			if ( G_AssertConnectedClient( survivor ) && !ModElimination_Static_IsPlayerEliminated( survivor ) ) {
				trap_SendServerCommand( -1, va( "cp \"%s\n^2is the survivor!\n\"",
						level.clients[level.sortedClients[0]].pers.netname ) );
			}
		}
	}
#endif
}

/*
================
ModElimTweaks_Init
================
*/
void ModElimTweaks_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( AdjustGeneralConstant, ++modePriorityLevel );
		MODFN_REGISTER( AdjustScoreboardAttributes, ++modePriorityLevel );
		MODFN_REGISTER( EnableCycleFollow, ++modePriorityLevel );
		MODFN_REGISTER( CheckSuicideAllowed, ++modePriorityLevel );
		MODFN_REGISTER( PostPlayerDie, ++modePriorityLevel );
		MODFN_REGISTER( PrePlayerLeaveTeam, ++modePriorityLevel );
		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );
		MODFN_REGISTER( MatchStateTransition, ++modePriorityLevel );

#ifdef FEATURE_FINALIST_TIMELIMIT
		ModElimTimelimit_Init();
#endif

#ifdef FEATURE_CLICKJOIN
		if ( !modcfg.mods_enabled.tournament ) {
			ModClickToJoin_Init();
		}
#endif

#ifdef FEATURE_INTERMISSION_READY_TWEAKS
		ModIntermissionReady_Init();
		MODFN_REGISTER( IntermissionReadyConfig, ++modePriorityLevel );
		ModIntermissionReady_Shared_UpdateConfig();
#endif
	}
}
