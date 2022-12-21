/*
* Player Move Features
* 
* This module provides some additional cvar options to customize player movement.
* 
* g_pMoveMsec - frame length for fps-neutral physics (0 = disabled, 8 = 125fps, 3 = 333fps, etc.)
* g_pMoveBotsMsec - frame length for bots (-1 = same as g_pMoveMsec, 0 = disabled, 8 = 125 fps, etc.)
*    This can be used to tweak jump pad landing points.
* g_pMoveTriggerMode - process touch triggers after every move fragment (0 = disabled, 1 = enabled)
*    This prevents lag tricks to drop through kill areas in certain maps.
* g_noJumpKeySlowdown - don't reduce acceleration when jump key is held midair (0 = disabled, 1 = enabled)
*    In the original game, holding the jump key for longer when jumping reduces strafe jumping
*    effectiveness. Enabling this options cancels this effect so strafe jumping has full
*    acceleration regardless of how long the jump key is held.
*/

#define MOD_PREFIX( x ) ModPlayerMove_##x

#include "mods/g_mod_local.h"

#define SNAPVECTOR_GRAV_LIMIT 100
#define SNAPVECTOR_GRAV_LIMIT_STR "100"

static struct {
	trackedCvar_t g_pMoveMsec;
	trackedCvar_t g_pMoveBotsMsec;
	trackedCvar_t g_pMoveTriggerMode;
	trackedCvar_t g_noJumpKeySlowdown;

	// For mod function stacking
	ModFNType_PmoveInit Prev_PmoveInit;
	ModFNType_AddModConfigInfo Prev_AddModConfigInfo;
} *MOD_STATE;

typedef struct {
	int clientNum;
	int oldEventSequence;
} postMoveContext_t;

/*
==============
ModPlayerMove_PostPmoveCallback
==============
*/
LOGFUNCTION_SVOID( ModPlayerMove_PostPmoveCallback, ( pmove_t *pmove, qboolean finalFragment, void *context ),
		( pmove, finalFragment, context ), "" ) {
	if ( finalFragment || MOD_STATE->g_pMoveTriggerMode.integer ) {
		postMoveContext_t *pmc = (postMoveContext_t *)context;
		modfn.PostPmoveActions( pmove, pmc->clientNum, pmc->oldEventSequence );
		pmc->oldEventSequence = level.clients[pmc->clientNum].ps.eventSequence;
	}
}

/*
==============
(ModFN) PmoveFixedLength

Returns fixed frame length to use for player move, or 0 for no fixed length.
==============
*/
int MOD_PREFIX(PmoveFixedLength)( qboolean isBot ) {
	int time = MOD_STATE->g_pMoveMsec.integer;
	if ( isBot && MOD_STATE->g_pMoveBotsMsec.integer >= 0 ) {
		time = MOD_STATE->g_pMoveBotsMsec.integer;
	}
	if ( time > 50 ) {
		time = 50;
	}
	if ( time > 0 ) {
		return time;
	}
	return 0;
}

/*
==============
(ModFN) RunPlayerMove

Performs player movement corresponding to a single input usercmd from the client.
==============
*/
LOGFUNCTION_VOID( MOD_PREFIX(RunPlayerMove), ( int clientNum ), ( clientNum ), "G_MODFN_RUNPLAYERMOVE" ) {
	gclient_t *client = &level.clients[clientNum];
	playerState_t *ps = &client->ps;
	pmove_t pmove;
	postMoveContext_t pmc = { 0 };
	qboolean isBot = ( g_entities[clientNum].r.svFlags & SVF_BOT ) ? qtrue : qfalse;
	int fixedLength = modfn.PmoveFixedLength( isBot );

	pmc.clientNum = clientNum;
	pmc.oldEventSequence = ps->eventSequence;

	if ( ps->pm_type == PM_SPECTATOR && client->ps.velocity[0] == 0.0f && client->ps.velocity[1] == 0.0f &&
			client->ps.velocity[2] == 0.0f && client->pers.cmd.forwardmove == 0 &&
			client->pers.cmd.rightmove == 0 && client->pers.cmd.upmove == 0 ) {
		// Skip full move for stationary spectators to save server cpu usage.
		PM_UpdateViewAngles( ps, &client->pers.cmd );
		ps->commandTime = client->pers.cmd.serverTime;
		if ( fixedLength > 0 ) {
			ps->commandTime -= ps->commandTime % fixedLength;
		}
		return;
	}

	modfn.PmoveInit( clientNum, &pmove );

	if ( isBot && fixedLength > 0 && client->ps.velocity[2] == 0.0f && pmove.cmd.upmove <= 0 ) {
		// For bots with no vertical movement, skip frame partitioning to save server cpu usage.
		// Since bots don't do things like circle jumping it shouldn't make a noticeable difference.
		pmove.cmd.serverTime -= pmove.cmd.serverTime % fixedLength;
		Pmove( &pmove, 0, ModPlayerMove_PostPmoveCallback, &pmc );
	} else {
		Pmove( &pmove, fixedLength, ModPlayerMove_PostPmoveCallback, &pmc );
	}
}

/*
================
(ModFN) PmoveInit
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PmoveInit), ( int clientNum, pmove_t *pmove ),
		( clientNum, pmove ), "G_MODFN_PMOVEINIT" ) {
	MOD_STATE->Prev_PmoveInit( clientNum, pmove );

	if ( MOD_STATE->g_noJumpKeySlowdown.integer ) {
		pmove->noJumpKeySlowdown = qtrue;
	}

	// general stuff that is just automatically enabled
	pmove->bounceFix = qtrue;
	pmove->snapVectorGravLimit = SNAPVECTOR_GRAV_LIMIT;
	pmove->noSpectatorDrift = qtrue; // currently no client prediction, as it isn't really needed
	pmove->noFlyingDrift = qtrue;
}

/*
==============
(ModFN) AddModConfigInfo
==============
*/
LOGFUNCTION_SVOID( MOD_PREFIX(AddModConfigInfo), ( char *info ), ( info ), "G_MODFN_ADDMODCONFIGINFO" ) {
	int pMoveFixed = modfn.PmoveFixedLength( qfalse );

	if ( pMoveFixed ) {
		Info_SetValueForKey( info, "pMoveFixed", va( "%i", pMoveFixed ) );
	}

	if ( MOD_STATE->g_pMoveTriggerMode.integer ) {
		Info_SetValueForKey( info, "pMoveTriggerMode", "1" );
	}

	if ( MOD_STATE->g_noJumpKeySlowdown.integer ) {
		Info_SetValueForKey( info, "noJumpKeySlowdown", "1" );
	}

	// general stuff that is just automatically enabled
	Info_SetValueForKey( info, "bounceFix", "1" );
	Info_SetValueForKey( info, "snapVectorGravLimit", SNAPVECTOR_GRAV_LIMIT_STR );
	Info_SetValueForKey( info, "noFlyingDrift", "1" );

	MOD_STATE->Prev_AddModConfigInfo( info );
}

/*
==============
ModPlayerMove_ModcfgCvarChanged

Called when any of the cvars potentially affecting the content of the mod configstring have changed.
==============
*/
static void ModPlayerMove_ModcfgCvarChanged( trackedCvar_t *cvar ) {
	G_UpdateModConfigInfo();
}

/*
================
ModPlayerMove_Init
================
*/
LOGFUNCTION_VOID( ModPlayerMove_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_BASE( PmoveFixedLength );
		INIT_FN_BASE( RunPlayerMove );
		INIT_FN_STACKABLE( PmoveInit );
		INIT_FN_STACKABLE( AddModConfigInfo );

		G_RegisterTrackedCvar( &MOD_STATE->g_pMoveMsec, "g_pMoveMsec", "8", CVAR_ARCHIVE, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_pMoveBotsMsec, "g_pMoveBotsMsec", "-1", CVAR_ARCHIVE, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_pMoveTriggerMode, "g_pMoveTriggerMode", "1", CVAR_ARCHIVE, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_noJumpKeySlowdown, "g_noJumpKeySlowdown", "1", CVAR_ARCHIVE, qfalse );

		G_RegisterCvarCallback( &MOD_STATE->g_pMoveMsec, ModPlayerMove_ModcfgCvarChanged, qfalse );
		G_RegisterCvarCallback( &MOD_STATE->g_pMoveTriggerMode, ModPlayerMove_ModcfgCvarChanged, qfalse );
		G_RegisterCvarCallback( &MOD_STATE->g_noJumpKeySlowdown, ModPlayerMove_ModcfgCvarChanged, qfalse );
	}
}
