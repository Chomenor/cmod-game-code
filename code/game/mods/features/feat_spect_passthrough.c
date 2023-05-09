/*
* Spectator Pass-Through
* 
* This module changes free-roaming spectator behavior to allow clipping through all dynamic
* entities. It replaces the standard method of auto-teleporting through doors, which is very
* unreliable and prone to getting stuck.
*/

#define MOD_NAME ModSpectPassThrough

#include "mods/g_mod_local.h"

static struct {
	void ( *Prev_Trace )( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
			const vec3_t end, int passEntityNum, int contentMask );
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
static void MOD_PREFIX(PmoveInit)( MODFN_CTV, int clientNum, pmove_t *pmove ) {
	MODFN_NEXT( PmoveInit, ( MODFN_NC, clientNum, pmove ) );

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
int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_SKIP_SPECTATOR_DOOR_TELEPORT ) {
		return 1;
	}

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
================
ModSpectPassThrough_Init
================
*/
void ModSpectPassThrough_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( PmoveInit, MODPRIORITY_GENERAL );
		MODFN_REGISTER( AdjustGeneralConstant, MODPRIORITY_GENERAL );
	}
}
