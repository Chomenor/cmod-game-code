/*
* Flag Undercap Filtering
* 
* This module supports limiting how CTF flags can be captured to prevent certain
* tricks like jump + crouch to capture from below platforms.
*/

#define MOD_NAME ModFlagUndercap

#include "mods/g_mod_local.h"

static struct {
	// 0 = default (all undercaps allowed)
	// 1 = limited (block most undercaps, but allow some specific cases)
	// 2 = strict (disable all undercaps and side captures)
	trackedCvar_t g_undercapLimit;

	// For ctf_voy2 like maps, standard mode disables undercap and scav jump, but allows
	// jump from regen platform that hits the flag platform from the side without fully
	// landing on it. Strict mode also disables this jump.
	qboolean voy2;

	// For ctf_boom2, standard mode still allows undercap, while strict mode disables it.
	qboolean boom2;
} *MOD_STATE;

// On voy2 maps, require being above the flag platform unless Z velocity is sufficiently
// negative. This allows capturing from the edge when jumping from the regen platform.
#define VOY2_FORCE_STRICT( player ) ( MOD_STATE->voy2 && ( player )->client->ps.velocity[2] > -340.0f )

/*
================
ModFlagUndercap_AllowCapture

Returns qtrue if flag capture is allowed.
================
*/
static qboolean ModFlagUndercap_AllowCapture( const gentity_t *player, const gentity_t *flag ) {
	if ( MOD_STATE->g_undercapLimit.integer <= 0 ) {
		return qtrue;
	}

	if ( MOD_STATE->g_undercapLimit.integer < 2 && MOD_STATE->boom2 && !( flag->flags & FL_DROPPED_ITEM ) ) {
		return qtrue;
	}

	if ( ( MOD_STATE->g_undercapLimit.integer >= 2 || VOY2_FORCE_STRICT( player ) ) &&
			!( flag->flags & FL_DROPPED_ITEM ) ) {
		// Strict mode: check how far bottom of player is above bottom of flag.
		float offset = ( player->r.currentOrigin[2] + player->r.mins[2] ) - ( flag->r.currentOrigin[2] + flag->r.mins[2] );
		return offset > -5.0f ? qtrue : qfalse;
	}

	else {
		// Standard mode: check how far top of player is above bottom of flag.
		float offset = ( player->r.currentOrigin[2] + player->r.maxs[2] ) - ( flag->r.currentOrigin[2] + flag->r.mins[2] );
		return offset > 0.0f ? qtrue : qfalse;
	}
}

/*
================
(ModFN) CanItemBeGrabbed
================
*/
static qboolean MOD_PREFIX(CanItemBeGrabbed)( MODFN_CTV, gentity_t *item, int clientNum ) {
	if ( item->item->giType == IT_TEAM && !ModFlagUndercap_AllowCapture( &g_entities[clientNum], item ) ) {
		return qfalse;
	}

	return MODFN_NEXT( CanItemBeGrabbed, ( MODFN_NC, item, clientNum ) );
}

/*
================
ModFlagUndercap_Init
================
*/
void ModFlagUndercap_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		// Load special configuration for certain maps.
		if ( !Q_stricmp( G_ModUtils_GetMapName(), "ctf_voy2" ) ||
				!Q_stricmp( G_ModUtils_GetMapName(), "pro_voy2" ) ||
				!Q_stricmp( G_ModUtils_GetMapName(), "ctf_voy8" ) ) {
			MOD_STATE->voy2 = qtrue;
		}
		if ( !Q_stricmp( G_ModUtils_GetMapName(), "ctf_boom2" ) ) {
			MOD_STATE->boom2 = qtrue;
		}

		G_RegisterTrackedCvar( &MOD_STATE->g_undercapLimit, "g_undercapLimit", "0", 0, qfalse );

		MODFN_REGISTER( CanItemBeGrabbed, MODPRIORITY_GENERAL );
	}
}
