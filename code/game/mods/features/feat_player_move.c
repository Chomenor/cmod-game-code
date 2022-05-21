/*
* Player Move Features
* 
* This module provides some additional cvar options to customize player movement.
* 
* g_pMoveFixed - enables fps-neutral physics (0 = disabled, 1 = enabled)
* g_pMoveMsec - frame length for fps-neutral physics (e.g. 8 = 125fps, 3 = 333fps)
* g_pMoveTriggerMode - process touch triggers after every move fragment (0 = disabled, 1 = enabled)
*    This prevents lag tricks to drop through kill areas in certain maps.
* g_noJumpKeySlowdown - don't reduce acceleration when jump key is held midair (0 = disabled, 1 = enabled)
*    In the original game, holding the jump key for longer when jumping reduces stafe jumping
*    effectiveness. Enabling this options cancels this effect so strafe jumping has full
*    acceleration regardless of how long the jump key is held.
*/

#include "mods/g_mod_local.h"

#define PREFIX( x ) ModPlayerMove_##x
#define MOD_STATE PREFIX( state )

#define SNAPVECTOR_GRAV_LIMIT 100
#define SNAPVECTOR_GRAV_LIMIT_STR "100"

static struct {
	trackedCvar_t g_pMoveFixed;
	trackedCvar_t g_pMoveMsec;
	trackedCvar_t g_pMoveTriggerMode;
	trackedCvar_t g_noJumpKeySlowdown;

	// For mod function stacking
	ModFNType_AdjustPmoveConstant Prev_AdjustPmoveConstant;
	ModFNType_PmoveInit Prev_PmoveInit;
	ModFNType_AddModConfigInfo Prev_AddModConfigInfo;
} *MOD_STATE;

/*
==================
(ModFN) AdjustPmoveConstant
==================
*/
LOGFUNCTION_SRET( int, PREFIX(AdjustPmoveConstant), ( pmoveConstant_t pmcType, int defaultValue ),
		( pmcType, defaultValue ), "G_MODFN_ADJUSTPMOVECONSTANT" ) {
	if ( pmcType == PMC_FIXED_LENGTH && MOD_STATE->g_pMoveFixed.integer &&
			MOD_STATE->g_pMoveMsec.integer > 0 && MOD_STATE->g_pMoveMsec.integer < 35 ) {
		return MOD_STATE->g_pMoveMsec.integer;
	}
	if ( pmcType == PMC_PARTIAL_MOVE_TRIGGERS ) {
		return MOD_STATE->g_pMoveTriggerMode.integer ? 1 : 0;
	}

	return MOD_STATE->Prev_AdjustPmoveConstant( pmcType, defaultValue );
}

/*
================
(ModFN) PmoveInit
================
*/
LOGFUNCTION_SVOID( PREFIX(PmoveInit), ( int clientNum, pmove_t *pmove ),
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
LOGFUNCTION_SVOID( PREFIX(AddModConfigInfo), ( char *info ), ( info ), "G_MODFN_ADDMODCONFIGINFO" ) {
	int pMoveFixed = modfn.AdjustPmoveConstant( PMC_FIXED_LENGTH, 0 );

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

#define INIT_FN_STACKABLE( name ) \
	MOD_STATE->Prev_##name = modfn.name; \
	modfn.name = PREFIX(name);

#define INIT_FN_OVERRIDE( name ) \
	modfn.name = PREFIX(name);

LOGFUNCTION_VOID( ModPlayerMove_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( AdjustPmoveConstant );
		INIT_FN_STACKABLE( PmoveInit );
		INIT_FN_STACKABLE( AddModConfigInfo );

		G_RegisterTrackedCvar( &MOD_STATE->g_pMoveFixed, "g_pMoveFixed", "1", CVAR_ARCHIVE, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_pMoveMsec, "g_pMoveMsec", "8", CVAR_ARCHIVE, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_pMoveTriggerMode, "g_pMoveTriggerMode", "1", CVAR_ARCHIVE, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_noJumpKeySlowdown, "g_noJumpKeySlowdown", "1", CVAR_ARCHIVE, qfalse );

		G_RegisterCvarCallback( &MOD_STATE->g_pMoveFixed, ModPlayerMove_ModcfgCvarChanged, qfalse );
		G_RegisterCvarCallback( &MOD_STATE->g_pMoveMsec, ModPlayerMove_ModcfgCvarChanged, qfalse );
		G_RegisterCvarCallback( &MOD_STATE->g_pMoveTriggerMode, ModPlayerMove_ModcfgCvarChanged, qfalse );
		G_RegisterCvarCallback( &MOD_STATE->g_noJumpKeySlowdown, ModPlayerMove_ModcfgCvarChanged, qfalse );
	}
}
