/*
* Powerup Modifiers
*/

#define MOD_NAME ModPowerupMods

#include "mods/g_mod_local.h"

static struct {
	// 0 = explosion damage at position of player who placed detpack when they placed it (original behavior)
	// 1 = explosion damage at position of detpack
	// -1 = default (0) or mod preference
	trackedCvar_t g_detExplodePos;

	// 0 = detpack does half damage to owner (original behavior)
	// 1 = detpack does full damage to owner
	// -1 = default (0) or mod preference
	trackedCvar_t g_detFullSelfDamage;

	// 0 = detpack damages targets in random order (similar to original behavior)
	// 1 = detpack damages nearest target first (harder to suicide det in end-of-match situations)
	// -1 = default (0) or mod preference
	trackedCvar_t g_detProxHitOrder;

	// 0 = disabled (original behavior)
	// 1 = enables proximity hit order, and if detpack owner dies, targets further away aren't damaged
	// -1 = default (0) or mod preference
	trackedCvar_t g_detAntiSuicide;
} *MOD_STATE;

/*
================
(ModFN) AdjustModConstant
================
*/
static int MOD_PREFIX(AdjustModConstant)( MODFN_CTV, modConstant_t mcType, int defaultValue ) {
	if ( mcType == MC_DETPACK_ORIGIN_PATCH && MOD_STATE->g_detExplodePos.integer >= 0 ) {
		return MOD_STATE->g_detExplodePos.integer;
	}
	if ( mcType == MC_DETPACK_FULL_SELF_DAMAGE && MOD_STATE->g_detFullSelfDamage.integer >= 0 ) {
		return MOD_STATE->g_detFullSelfDamage.integer;
	}
	if ( mcType == MC_DETPACK_PROXIMITY_HIT_ORDER ) {
		if ( MOD_STATE->g_detAntiSuicide.integer >= 1 ) {
			return 1;
		}
		if ( MOD_STATE->g_detProxHitOrder.integer >= 0 ) {
			return MOD_STATE->g_detProxHitOrder.integer;
		}
	}
	if ( mcType == MC_DETPACK_SKIP_DMG_IF_OWNER_DIES && MOD_STATE->g_detAntiSuicide.integer >= 0 ) {
		return MOD_STATE->g_detAntiSuicide.integer;
	}

	return MODFN_NEXT( AdjustModConstant, ( MODFN_NC, mcType, defaultValue ) );
}

/*
================
ModPowerupMods_Init
================
*/
void ModPowerupMods_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_detExplodePos, "g_detExplodePos", "-1", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_detFullSelfDamage, "g_detFullSelfDamage", "-1", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_detProxHitOrder, "g_detProxHitOrder", "-1", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_detAntiSuicide, "g_detAntiSuicide", "-1", 0, qfalse );

		MODFN_REGISTER( AdjustModConstant, MODPRIORITY_MODIFICATIONS );

		ModDetpack_Init();
	}
}
