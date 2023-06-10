/*
* Gladiator Item Enablement
* 
* This module provides cvar methods to enable and disable item spawns in the
* format of the Gladiator mod.
*/

#define MOD_NAME ModGladiatorItemEnable

#include "mods/modes/uam/uam_local.h"

static struct {
	trackedCvar_t g_mod_HealthAvailableFlag;
	trackedCvar_t g_mod_PowerupsAvailableFlags;
	trackedCvar_t g_mod_HoldableAvailableFlags;
	trackedCvar_t g_mod_ArmorAvailableFlag;
} *MOD_STATE;

/*
================
(ModFN) CheckItemSuppressed
================
*/
static int MOD_PREFIX(CheckItemSuppressed)( MODFN_CTV, gentity_t *item ) {
	if ( item->item->giType == IT_HEALTH ) {
		if ( !G_ModUtils_ReadGladiatorBoolean( MOD_STATE->g_mod_HealthAvailableFlag.string ) ) {
			return 5000;
		}
	} else if ( item->item->giType == IT_ARMOR ) {
		if ( !G_ModUtils_ReadGladiatorBoolean( MOD_STATE->g_mod_ArmorAvailableFlag.string ) ) {
			return 5000;
		}
	} else if ( item->item->giType == IT_POWERUP ) {
		if ( !( G_ModUtils_ReadGladiatorBitflags( MOD_STATE->g_mod_PowerupsAvailableFlags.string ) &
				( 1 << ( item->item->giTag - 1 ) ) ) ) {
			return 30000 + irandom( 0, 30000 );
		}
	} else if ( item->item->giType == IT_HOLDABLE ) {
		if ( !( G_ModUtils_ReadGladiatorBitflags( MOD_STATE->g_mod_HoldableAvailableFlags.string ) &
				( 1 << ( item->item->giTag - 1 ) ) ) ) {
			return 10000 + irandom( 0, 10000 );
		}
	}

	return MODFN_NEXT( CheckItemSuppressed, ( MODFN_NC, item ) );
}

/*
================
GladiatorItemEnable_Init
================
*/
void ModGladiatorItemEnable_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( CheckItemSuppressed, MODPRIORITY_GENERAL );

		G_RegisterTrackedCvar( &MOD_STATE->g_mod_HealthAvailableFlag, "g_mod_HealthAvailableFlag", "Y", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_mod_PowerupsAvailableFlags, "g_mod_PowerupsAvailableFlags", "YYYYYYY", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_mod_HoldableAvailableFlags, "g_mod_HoldableAvailableFlags", "YYYYY", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_mod_ArmorAvailableFlag, "g_mod_ArmorAvailableFlag", "Y", 0, qfalse );
	}
}
