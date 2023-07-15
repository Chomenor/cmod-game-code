/*
* UAM Weapons
* 
* Configure weapon behavior, spawns, and ammo for UAM mode.
*/

#define MOD_NAME ModUAMWeapons

#include "mods/modes/uam/uam_local.h"

static struct {
	trackedCvar_t g_mod_UseTripwiresMode;
} *MOD_STATE;

/*
======================
(ModFN) AdjustWeaponConstant
======================
*/
static int MOD_PREFIX(AdjustWeaponConstant)( MODFN_CTV, weaponConstant_t wcType, int defaultValue ) {
	switch ( wcType ) {
		case WC_USE_RANDOM_DAMAGE:
			return 0;		// orig: 1
		case WC_USE_TRIPMINES:
			return MOD_STATE->g_mod_UseTripwiresMode.integer ? 1 : 0;		// orig: 0
		case WC_IMPRESSIVE_REWARDS:
			// Disable reward for 2 rifle/imod hits in a row
			return 0;		// orig: 1
		case WC_DN_MAX_MOVES:
			// 100ms per move, so welder bursts stop after about 10 seconds.
			return 100;		// orig: 0
		case WC_ASSIM_NO_STRICT_TEAM_CHECK:
			return 1;

		case WC_PHASER_DAMAGE:
			return 7;		// orig: 6
		case WC_IMOD_ALT_DAMAGE:
			return 30;		// orig: 48
		case WC_SCAV_DAMAGE:
			return 5;		// orig: 8
		case WC_STASIS_DAMAGE:
			return 10;		// orig: 9
		case WC_DREADNOUGHT_DAMAGE:
			return 5;		// orig: 8
		case WC_DREADNOUGHT_ALTDAMAGE:
			return 20;		// orig: 35
		case WC_BORG_PROJ_DAMAGE:
			return 15;		// orig: 20
		case WC_BORG_TASER_DAMAGE:
			return 20;		// orig: 15

		case WC_SCAV_VELOCITY:
			return 3000;	// orig: 1500
		case WC_STASIS_VELOCITY:
			return 3000;	// orig: 1100
		case WC_DN_SEARCH_DIST:
			return 128;		// orig: 256
		case WC_DN_ALT_SIZE:
			return 10;		// orig: 12
		case WC_BORG_PROJ_VELOCITY:
			return 8000;	// orig: 1000
	}

	return MODFN_NEXT( AdjustWeaponConstant, ( MODFN_NC, wcType, defaultValue ) );
}

/*
==============
(ModFN) ModifyFireRate

Set fire rate constants consistent with Gladiator mod.
==============
*/
static int MOD_PREFIX(ModifyFireRate)( MODFN_CTV, int defaultInterval, int weapon, qboolean alt ) {
	switch ( weapon ) {
		case WP_PHASER:
			return alt ? 50 : 66;		// orig: 100 / 100
		case WP_COMPRESSION_RIFLE:
			return alt ? 600 : 166;		// orig: 1200 / 250
		case WP_IMOD:
			return alt ? 280 : 233;		// orig: 700 / 350
		case WP_SCAVENGER_RIFLE:
			return alt ? 350 : 66;		// orig: 700 / 100
		case WP_STASIS:
			return alt ? 350 : 466;		// orig: 700 / 700
		case WP_GRENADE_LAUNCHER:
			return alt ? 350 : 466;		// orig: 700 / 700
		case WP_TETRION_DISRUPTOR:
			return 100;					// orig: 200 / 100
		case WP_QUANTUM_BURST:
			return alt ? 1200 : 800;	// orig: 1600 / 1200
		case WP_DREADNOUGHT:
			return alt ? 250 : 66;		// orig: 500 / 100
		case WP_BORG_ASSIMILATOR:
			return 66;					// orig: 100 / 100
		case WP_BORG_WEAPON:
			return alt ? 150 : 125;		// orig: 300 / 500
	}

	return MODFN_NEXT( ModifyFireRate, ( MODFN_NC, defaultInterval, weapon, alt ) );
}

/*
================
(ModFN) AltFireConfig

Set alt fire swap default to primary for welder.
================
*/
static char MOD_PREFIX(AltFireConfig)( MODFN_CTV, weapon_t weapon ) {
	if ( weapon == WP_DREADNOUGHT ) {
		return 'n';
	}
	return MODFN_NEXT( AltFireConfig, ( MODFN_NC, weapon ) );
}

/*
================
(ModFN) ModifyDamageFlags

Don't allow assimilator weapon to penetrate shields.
================
*/
static int MOD_PREFIX(ModifyDamageFlags)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	if ( mod == MOD_ASSIMILATE ) {
		dflags &= ~( DAMAGE_NO_ARMOR | DAMAGE_NO_INVULNERABILITY );
	}
	return MODFN_NEXT( ModifyDamageFlags, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );
}

/*
================
ModUAMWeapons_Init
================
*/
void ModUAMWeapons_Init( qboolean allowWeaponCvarChanges ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		// Load starting weapons configuration
		ModUAMWeaponSpawn_Init( allowWeaponCvarChanges );

		// Load unlimited/regenerating ammo configuration
		ModUAMAmmo_Init();

		// Load dependencies
		ModAltFireConfig_Init();
		ModFireRateCS_Init();

		G_RegisterTrackedCvar( &MOD_STATE->g_mod_UseTripwiresMode, "g_mod_UseTripwiresMode", "1", 0, qfalse );

		MODFN_REGISTER( AdjustWeaponConstant, ++modePriorityLevel );
		MODFN_REGISTER( ModifyFireRate, ++modePriorityLevel );
		MODFN_REGISTER( AltFireConfig, ++modePriorityLevel );
		MODFN_REGISTER( ModifyDamageFlags, ++modePriorityLevel );
	}
}
