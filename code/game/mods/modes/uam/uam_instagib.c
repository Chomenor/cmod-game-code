/*
* UAM Instagib
* 
* This is a variation of Disintegration mode with a few changes:
* 
* - Players spawn with an Imod as well as a rifle. The Imod works almost the same
* as the rifle and is basically just for aesthetic preference.
* - Players can weapon jump with either weapon.
* - Seeker and Detpack powerups are available.
* - Some effects are different.
*/

#define MOD_NAME ModUAMInstagib

#include "mods/modes/uam/uam_local.h"

static struct {
	// -1 = enabled except in ctf (default), 0 = disabled, 1 = enabled
	trackedCvar_t g_instagib_rifleJump;
} *MOD_STATE;

#define RIFLE_JUMP_ENABLED ( MOD_STATE->g_instagib_rifleJump.integer >= 0 ? \
		MOD_STATE->g_instagib_rifleJump.integer : g_gametype.integer != GT_CTF )

/*
======================
(ModFN) AdjustWeaponConstant

Adjust weapon configuration.
======================
*/
static int MOD_PREFIX(AdjustWeaponConstant)( MODFN_CTV, weaponConstant_t wcType, int defaultValue ) {
	// Enable hack which changes Imod to actually use the alt fire rifle mechanics, but keeps the imod fx event.
	if ( wcType == WC_USE_IMOD_RIFLE )
		return 1;

	// Set splash damage for rifle jumping.
	if ( RIFLE_JUMP_ENABLED ) {
		if ( wcType == WC_CRIFLE_ALT_SPLASH_RADIUS )
			return 170;
		if ( wcType == WC_CRIFLE_ALT_SPLASH_DMG )
			return 140;
	}

	return MODFN_NEXT( AdjustWeaponConstant, ( MODFN_NC, wcType, defaultValue ) );
}

/*
============
(ModFN) ModifyDamageFlags

Rifle splash does no damage, and does knockback to self only.
============
*/
static int MOD_PREFIX(ModifyDamageFlags)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	dflags = MODFN_NEXT( ModifyDamageFlags, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );

	if ( mod == MOD_CRIFLE_ALT_SPLASH ) {
		if ( targ == attacker ) {
			// Can take away armor, but not health.
			dflags |= DAMAGE_NO_HEALTH_DAMAGE;
			dflags &= ~DAMAGE_NO_ARMOR;
		} else {
			dflags |= DAMAGE_TRIGGERS_ONLY;
		}
	}

	return dflags;
}

/*
================
(ModFN) SpawnConfigureClient

Add Imod weapon to inventory.
================
*/
static void MOD_PREFIX(SpawnConfigureClient)( MODFN_CTV, int clientNum ) {
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( SpawnConfigureClient, ( MODFN_NC, clientNum ) );

	client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_IMOD );
	client->ps.ammo[WP_IMOD] = 150;
}

/*
==============
(ModFN) ModifyFireRate

Set Imod fire rate to same as rifle.
==============
*/
static int MOD_PREFIX(ModifyFireRate)( MODFN_CTV, int defaultInterval, int weapon, qboolean alt ) {
	if ( weapon == WP_IMOD ) {
		return 1200;
	}

	return MODFN_NEXT( ModifyFireRate, ( MODFN_NC, defaultInterval, weapon, alt ) );
}

/*
================
(ModFN) AltFireConfig

Set Imod to primary fire only.
================
*/
static char MOD_PREFIX(AltFireConfig)( MODFN_CTV, weapon_t weapon ) {
	if ( weapon == WP_IMOD ) {
		return 'f';
	}
	return MODFN_NEXT( AltFireConfig, ( MODFN_NC, weapon ) );
}

/*
==================
(ModFN) PlayerDeathEffect

Some death effect adjustments:
- Don't clear velocity, so killed player can be pushed/fly through the air.
- Don't play disintegration effect on rifle kills.
- Play borg effect on Imod kills.
==================
*/
static void MOD_PREFIX(PlayerDeathEffect)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int contents, int killer, int meansOfDeath ) {
	if ( meansOfDeath == MOD_CRIFLE_ALT ) {
		qboolean imod = attacker->client && attacker->client->ps.weapon == WP_IMOD;

		if ( imod ) {
			// Play borg effect
			self->client->ps.powerups[PW_BORG_ADAPT] = level.time + 250;
		} else {
			// Normal death sound
			G_AddEvent( self, irandom(EV_DEATH1, EV_DEATH3), killer );
		}

		self->takedamage = qfalse;
		self->health = 0;
	}

	else {
		MODFN_NEXT( PlayerDeathEffect, ( MODFN_NC, self, inflictor, attacker, contents, killer, meansOfDeath ) );
	}
}

/*
================
(ModFN) CheckItemSpawnDisabled

Restore a couple items normally disabled in Disintegration: Detpack and Seeker.
================
*/
static qboolean MOD_PREFIX(CheckItemSpawnDisabled)( MODFN_CTV, gitem_t *item ) {
	if ( ( item->giType == IT_HOLDABLE && item->giTag == HI_DETPACK ) ||
			( item->giType == IT_POWERUP && item->giTag == PW_SEEKER ) ) {
		return qfalse;
	}

	return MODFN_NEXT( CheckItemSpawnDisabled, ( MODFN_NC, item ) );
}

/*
================
ModUAMInstagib_Init
================
*/
void ModUAMInstagib_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_instagib_rifleJump, "g_instagib_rifleJump", "-1", 0, qfalse );

		// Use Disintegration mode as basis
		ModDisintegration_Init();

		// Load dependencies
		ModAltFireConfig_Init();
		ModFireRateCS_Init();

		// Support saving imod/rifle preference between round changes.
		ModUAMInstagibSaveWep_Init();

		RegisterItem( BG_FindItemForWeapon( WP_IMOD ) );

		MODFN_REGISTER( AdjustWeaponConstant, ++modePriorityLevel );
		MODFN_REGISTER( ModifyDamageFlags, ++modePriorityLevel );
		MODFN_REGISTER( SpawnConfigureClient, ++modePriorityLevel );
		MODFN_REGISTER( ModifyFireRate, ++modePriorityLevel );
		MODFN_REGISTER( AltFireConfig, ++modePriorityLevel );
		MODFN_REGISTER( PlayerDeathEffect, ++modePriorityLevel );
		MODFN_REGISTER( CheckItemSpawnDisabled, ++modePriorityLevel );
	}
}
