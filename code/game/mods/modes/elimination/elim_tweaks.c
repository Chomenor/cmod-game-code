/*
* Elimination Tweaks
* 
* Adds some extra elimination features and tweaks, mostly from Gladiator mod.
* 
* Enabled by g_mod_elimTweaks cvar.
*/

#define MOD_PREFIX( x ) ModElimTweaks##x

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

#ifdef FEATURE_INTERMISSION_READY_TWEAKS
	IntermissionReady_ConfigFunction_t prevIntermissionReadyConfig;
#endif

	// For mod function stacking
	ModFNType_AdjustGeneralConstant Prev_AdjustGeneralConstant;
	ModFNType_AdjustScoreboardAttributes Prev_AdjustScoreboardAttributes;
	ModFNType_CheckSuicideAllowed Prev_CheckSuicideAllowed;
	ModFNType_PostPlayerDie Prev_PostPlayerDie;
	ModFNType_PrePlayerLeaveTeam Prev_PrePlayerLeaveTeam;
	ModFNType_PostRunFrame Prev_PostRunFrame;
	ModFNType_MatchStateTransition Prev_MatchStateTransition;
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

	if ( g_gametype.integer >= GT_TEAM ) {
		int playersAlive = ModElimination_Shared_CountPlayersAliveTeam( oldTeam, clientNum );
		if ( playersAlive >= 1 ) {
			trap_SendServerCommand( -1, va( "cp \"%s ^7%s\n^%s%i player%s of team %s left\"",
					client->pers.netname, eliminated ? "died" : "left the game",
					oldTeam == TEAM_RED ? "1" : "4", playersAlive,
					playersAlive == 1 ? "" : "s", oldTeam == TEAM_RED ? "red" : "blue" ) );
			ModElimTweaks_RemainingPlayerSound( playersAlive );
		}
	
	} else {
		int playersAlive = ModElimination_Shared_CountPlayersAliveTeam( TEAM_FREE, clientNum );
		if ( playersAlive >= 2 ) {
			trap_SendServerCommand( -1, va( "cp \"%s ^7%s\n^4%i players in the arena left\"",
					client->pers.netname, eliminated ? "died" : "left the game", playersAlive ) );
			ModElimTweaks_RemainingPlayerSound( playersAlive );
		}
	}
}
#endif

#ifdef FEATURE_INTERMISSION_READY_TWEAKS
/*
================
ModElimTweaks_IntermissionConfigFunction
================
*/
static void ModElimTweaks_IntermissionConfigFunction( ModIntermissionReady_config_t *config ) {
	MOD_STATE->prevIntermissionReadyConfig( config );
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
static int MOD_PREFIX(AdjustGeneralConstant)( generalConstant_t gcType, int defaultValue ) {
#ifdef FEATURE_JOIN_MESSAGE_TWEAKS
	if ( gcType == GC_SKIP_ENTER_GAME_PRINT )
		return 1;
	if ( gcType == GC_JOIN_MESSAGE_CONSOLE_PRINT )
		return 1;
#endif

	return MOD_STATE->Prev_AdjustGeneralConstant( gcType, defaultValue );
}

/*
================
(ModFN) AdjustScoreboardAttributes
================
*/
static int MOD_PREFIX(AdjustScoreboardAttributes)( int clientNum, scoreboardAttribute_t saType, int defaultValue ) {
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

	return MOD_STATE->Prev_AdjustScoreboardAttributes( clientNum, saType, defaultValue );
}

/*
=================
(ModFN) CheckSuicideAllowed

Disable suicide during warmup.
=================
*/
LOGFUNCTION_SRET( qboolean, MOD_PREFIX(CheckSuicideAllowed), ( int clientNum ),
		( clientNum ), "G_MODFN_CHECKSUICIDEALLOWED" ) {
#ifdef FEATURE_RESTRICT_SUICIDE
	if ( level.matchState == MS_WARMUP || level.matchState >= MS_INTERMISSION_QUEUED ) {
		return qfalse;
	}
#endif

	return MOD_STATE->Prev_CheckSuicideAllowed( clientNum );
}

/*
================
(ModFN) PostPlayerDie
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostPlayerDie), ( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath, int *awardPoints ),
		( self, inflictor, attacker, meansOfDeath, awardPoints ), "G_MODFN_POSTPLAYERDIE" ) {
	int clientNum = self - g_entities;
	gclient_t *client = &level.clients[clientNum];
	elim_extras_client_t *modclient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_PostPlayerDie( self, inflictor, attacker, meansOfDeath, awardPoints );

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

Reset stats and eliminated state when player switches teams or becomes spectator.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PrePlayerLeaveTeam), ( int clientNum, team_t oldTeam ),
		( clientNum, oldTeam ), "G_MODFN_PREPLAYERLEAVETEAM" ) {
	MOD_STATE->Prev_PrePlayerLeaveTeam( clientNum, oldTeam );

#ifdef FEATURE_ELIMINATED_MESSAGES
	if ( level.matchState == MS_ACTIVE && oldTeam != TEAM_SPECTATOR && !ModElimination_Static_IsPlayerEliminated( clientNum ) ) {
		ModElimTweaks_EliminatedMessage( clientNum, oldTeam, qfalse );
	}
#endif
}

/*
================
(ModFN) PostRunFrame
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostRunFrame), (void), (), "G_MODFN_POSTRUNFRAME" ) {
	MOD_STATE->Prev_PostRunFrame();

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
LOGFUNCTION_SVOID( MOD_PREFIX(MatchStateTransition), ( matchState_t oldState, matchState_t newState ),
		( oldState, newState ), "G_MODFN_MATCHSTATETRANSITION" ) {
	MOD_STATE->Prev_MatchStateTransition( oldState, newState );

#ifdef FEATURE_SURVIVOR_MESSAGES
	if ( newState == MS_INTERMISSION_QUEUED ) {
		if ( g_gametype.integer >= GT_TEAM ) {
			trap_SendServerCommand( -1, va( "cp \"%s\n^2is the survivor!\n\"",
					level.winningTeam == TEAM_RED ? "^1TEAM RED" : "^4TEAM BLUE" ) );
		} else {
			if ( G_AssertConnectedClient( level.sortedClients[0] ) &&
					EF_WARN_ASSERT( !ModElimination_Static_IsPlayerEliminated( level.sortedClients[0] ) ) ) {
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
LOGFUNCTION_VOID( ModElimTweaks_Init, ( void ), (), "G_MOD_INIT G_ELIMINATION" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( AdjustGeneralConstant );
		INIT_FN_STACKABLE( AdjustScoreboardAttributes );
		INIT_FN_STACKABLE( CheckSuicideAllowed );
		INIT_FN_STACKABLE( PostPlayerDie );
		INIT_FN_STACKABLE( PrePlayerLeaveTeam );
		INIT_FN_STACKABLE( PostRunFrame );
		INIT_FN_STACKABLE( MatchStateTransition );

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
		MOD_STATE->prevIntermissionReadyConfig = modIntermissionReady_shared->configFunction;
		modIntermissionReady_shared->configFunction = ModElimTweaks_IntermissionConfigFunction;
		ModIntermissionReady_Shared_UpdateConfig();
#endif
	}
}
