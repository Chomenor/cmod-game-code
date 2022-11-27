/*
* Spectator Pass-Through
* 
* This module changes free-roaming spectator behavior to allow clipping through all dynamic
* entities. It replaces the standard method of auto-teleporting through doors, which is very
* unreliable and prone to getting stuck.
*/

#define MOD_PREFIX( x ) ModSpectPassThrough_##x

#include "mods/g_mod_local.h"

static struct {
	void ( *Prev_Trace )( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
			const vec3_t end, int passEntityNum, int contentMask );

	// For mod function stacking
	ModFNType_PmoveInit Prev_PmoveInit;
	ModFNType_AdjustGeneralConstant Prev_AdjustGeneralConstant;
} *MOD_STATE;

/*
================
ModSpectPassThrough_Trace

Trace with entities ignored so everything except the main map is pass-through.
================
*/
static void ModSpectPassThrough_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int passEntityNum, int contentMask ) {
	int entityNum;
	MOD_STATE->Prev_Trace( results, start, mins, maxs, end, passEntityNum, contentMask );
	entityNum = results->entityNum;

	if ( entityNum >= 0 && entityNum < ENTITYNUM_MAX_NORMAL && EF_WARN_ASSERT( g_entities[entityNum].r.contents ) ) {
		// Repeat trace with contacted entity temporarily set to empty contents.
		int oldContents = g_entities[entityNum].r.contents;
		g_entities[entityNum].r.contents = 0;
		ModSpectPassThrough_Trace( results, start, mins, maxs, end, passEntityNum, contentMask );
		g_entities[entityNum].r.contents = oldContents;
	}
}

/*
================
(ModFN) PmoveInit

Override trace function for spectators.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PmoveInit), ( int clientNum, pmove_t *pmove ),
		( clientNum, pmove ), "G_MODFN_PMOVEINIT" ) {
	MOD_STATE->Prev_PmoveInit( clientNum, pmove );

	if ( modfn.SpectatorClient( clientNum ) ) {
		MOD_STATE->Prev_Trace = pmove->trace;
		pmove->trace = ModSpectPassThrough_Trace;
	}
}

/*
================
(ModFN) AdjustGeneralConstant
================
*/
int MOD_PREFIX(AdjustGeneralConstant)( generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_SKIP_SPECTATOR_DOOR_TELEPORT ) {
		return 1;
	}

	return MOD_STATE->Prev_AdjustGeneralConstant( gcType, defaultValue );
}

/*
================
ModSpectPassThrough_Init
================
*/
LOGFUNCTION_VOID( ModSpectPassThrough_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( PmoveInit );
		INIT_FN_STACKABLE( AdjustGeneralConstant );
	}
}
