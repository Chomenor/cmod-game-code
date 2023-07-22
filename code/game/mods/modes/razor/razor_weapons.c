/*
* Razor Weapons
* 
* Players spawn with rifle, grenades, and photon. Weapons do high knockback rather
* than direct damage.
*/

#define MOD_NAME ModRazorWeapons

#include "mods/modes/razor/razor_local.h"

static struct {
	qboolean getHandicap;
} *MOD_STATE;

/*
================
(ModFN) EffectiveHandicap
================
*/
static int MOD_PREFIX(EffectiveHandicap)( MODFN_CTV, int clientNum, effectiveHandicapType_t type ) {
	gclient_t *client = &level.clients[clientNum];

	if ( type == EH_MAXHEALTH ) {
		return 100;
	}

	// Disable handicap during normal G_Damage calculation. Handicap will be applied later
	// during ApplyKnockback below, when getHandicap is set to true.
	if ( type == EH_DAMAGE && !MOD_STATE->getHandicap ) {
		return 100;
	}

	return MODFN_NEXT( EffectiveHandicap, ( MODFN_NC, clientNum, type ) );
}

/*
============
(ModFN) ModifyDamageFlags
============
*/
static int MOD_PREFIX(ModifyDamageFlags)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	dflags = MODFN_NEXT( ModifyDamageFlags, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );

	if ( dir && targ->client ) {
		if ( !g_friendlyFire.integer && targ != attacker && OnSameTeam( targ, attacker ) ) {
			// no friendly fire knockback
			dflags |= DAMAGE_TRIGGERS_ONLY;
		} else {
			dflags |= DAMAGE_NO_HEALTH_DAMAGE | DAMAGE_LIMIT_EFFECTS | DAMAGE_NO_KNOCKBACK_CAP | DAMAGE_NO_HALF_SELF;
		}
	}

	return dflags;
}

/*
============
(ModFN) ApplyKnockback
============
*/
static void MOD_PREFIX(ApplyKnockback)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod, float knockback ) {
	vec3_t	kvel;
	float	mass = modfn.KnockbackMass( targ, inflictor, attacker, dir, point, damage, dflags, mod );

	// flying targets get pushed around a lot more.
	if ( targ != attacker && targ->client->ps.powerups[PW_FLIGHT] ) {
		mass *= 0.15f;
	}

	if ( targ != attacker ) {
		// perform some additional scaling from Pinball mod
		// knockbacks from other players round down to multiple of 100, while non-player
		// knockbacks (like turrets) apparently get a much bigger reduction
		knockback = ( attacker && attacker->client ? 100 : 6 ) * ( (int)tonextint( knockback ) / 100 );

		// apply handicap
		if ( attacker && attacker->client ) {
			MOD_STATE->getHandicap = qtrue;
			knockback *= modfn.EffectiveHandicap( attacker - g_entities, EH_DAMAGE ) / 100.0f;
			MOD_STATE->getHandicap = qfalse;
		}
	}

	VectorScale (dir, 3.0f * g_knockback.value * (float)knockback / mass, kvel);
	VectorAdd (targ->client->ps.velocity, kvel, targ->client->ps.velocity);

	// set the timer so that the other client can't cancel
	// out the movement immediately
	if ( !targ->client->ps.pm_time ) {
		int		t;

		t = knockback * 2;
		if ( t < 50 ) {
			t = 50;
		}
		if ( t > 200 ) {
			t = 200;
		}
		targ->client->ps.pm_time = t;
		targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}

/*
======================
(ModFN) AdjustWeaponConstant
======================
*/
static int MOD_PREFIX(AdjustWeaponConstant)( MODFN_CTV, weaponConstant_t wcType, int defaultValue ) {
	switch ( wcType ) {
		case WC_CRIFLE_ALTDAMAGE:
			return 350;
		case WC_CRIFLE_ALT_SPLASH_RADIUS:
			return 100;
		case WC_CRIFLE_ALT_SPLASH_DMG:
			return 150;
		case WC_CRIFLE_SPLASH_USE_QUADFACTOR:
			return 1;
		case WC_GRENADE_DAMAGE:
			return 300;
		case WC_GRENADE_SPLASH_RAD:
			return 300;
		case WC_GRENADE_SPLASH_DAM:
			return 200;
		case WC_QUANTUM_DAMAGE:
			return 350;
		case WC_QUANTUM_SPLASH_DAM:
			return 250;
		case WC_QUANTUM_SPLASH_RAD:
			return 250;
	}

	return MODFN_NEXT( AdjustWeaponConstant, ( MODFN_NC, wcType, defaultValue ) );
}

/*
================
(ModFN) AltFireConfig

Set rifle to alt fire only and photon to primary fire only.
================
*/
static char MOD_PREFIX(AltFireConfig)( MODFN_CTV, weapon_t weapon ) {
	if ( weapon == WP_COMPRESSION_RIFLE ) {
		return 'F';
	}
	if ( weapon == WP_QUANTUM_BURST && !modcfg.mods_enabled.disintegration ) {
		return 'f';
	}
	return MODFN_NEXT( AltFireConfig, ( MODFN_NC, weapon ) );
}

/*
==============
(ModFN) ModifyAmmoUsage

Don't use ammo in Disintegration mode.
==============
*/
static int MOD_PREFIX(ModifyAmmoUsage)( MODFN_CTV, int defaultValue, int weapon, qboolean alt ) {
	if ( modcfg.mods_enabled.disintegration ) {
		return 0;
	}
	return MODFN_NEXT( ModifyAmmoUsage, ( MODFN_NC, defaultValue, weapon, alt ) );
}

/*
================
(ModFN) SpawnConfigureClient

Add weapons to inventory when spawning.
================
*/
static void MOD_PREFIX(SpawnConfigureClient)( MODFN_CTV, int clientNum ) {
	gclient_t *client = &level.clients[clientNum];
	MODFN_NEXT( SpawnConfigureClient, ( MODFN_NC, clientNum ) );

	if ( modcfg.mods_enabled.disintegration ) {
		client->ps.stats[STAT_WEAPONS] = 1 << WP_COMPRESSION_RIFLE;
		client->ps.ammo[WP_COMPRESSION_RIFLE] = 128;
	} else {
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_COMPRESSION_RIFLE ) |
				( 1 << WP_GRENADE_LAUNCHER ) | ( 1 << WP_QUANTUM_BURST );
		client->ps.ammo[WP_COMPRESSION_RIFLE] = 80;
		client->ps.ammo[WP_GRENADE_LAUNCHER] = 10;
		client->ps.ammo[WP_QUANTUM_BURST] = 10;
	}
}

/*
================
(ModFN) AddRegisteredItems

Register spawn weapons to avoid loading glitches.
================
*/
static void MOD_PREFIX(AddRegisteredItems)( MODFN_CTV ) {
	MODFN_NEXT( AddRegisteredItems, ( MODFN_NC ) );
	RegisterItem( BG_FindItemForWeapon( WP_COMPRESSION_RIFLE ) );
	RegisterItem( BG_FindItemForWeapon( WP_GRENADE_LAUNCHER ) );
	RegisterItem( BG_FindItemForWeapon( WP_QUANTUM_BURST ) );
}

/*
================
ModRazorWeapons_Init
================
*/
void ModRazorWeapons_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModAltFireConfig_Init();

		MODFN_REGISTER( EffectiveHandicap, MODPRIORITY_HIGH );
		MODFN_REGISTER( ModifyDamageFlags, ++modePriorityLevel );
		MODFN_REGISTER( ApplyKnockback, MODPRIORITY_LOW );
		MODFN_REGISTER( AdjustWeaponConstant, ++modePriorityLevel );
		MODFN_REGISTER( AltFireConfig, ++modePriorityLevel );
		MODFN_REGISTER( ModifyAmmoUsage, ++modePriorityLevel );
		MODFN_REGISTER( SpawnConfigureClient, ++modePriorityLevel );
		MODFN_REGISTER( AddRegisteredItems, ++modePriorityLevel );
	}
}
