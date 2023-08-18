/*
* Disintegration Mode
* 
* All players spawn with a rifle which has unlimited ammo and kills with one hit.
*/

#define MOD_NAME ModDisintegration

#include "mods/g_mod_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
================
(ModFN) SpawnConfigureClient

Player starts with just a rifle.
================
*/
static void MOD_PREFIX(SpawnConfigureClient)( MODFN_CTV, int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( SpawnConfigureClient, ( MODFN_NC, clientNum ) );

	client->ps.stats[STAT_WEAPONS] = ( 1 << WP_COMPRESSION_RIFLE );
	client->ps.ammo[WP_COMPRESSION_RIFLE] = Max_Ammo[WP_COMPRESSION_RIFLE];
}

/*
======================
(ModFN) AdjustWeaponConstant

Rifle does enough damage to 1-hit, but no splash damage.
======================
*/
static int MOD_PREFIX(AdjustWeaponConstant)( MODFN_CTV, weaponConstant_t wcType, int defaultValue ) {
	if ( wcType == WC_USE_RANDOM_DAMAGE )
		return 0;
	if ( wcType == WC_CRIFLE_ALTDAMAGE )
		return 1000;
	if ( wcType == WC_CRIFLE_ALT_SPLASH_RADIUS )
		return 0;
	if ( wcType == WC_CRIFLE_ALT_SPLASH_DMG )
		return 0;

	return MODFN_NEXT( AdjustWeaponConstant, ( MODFN_NC, wcType, defaultValue ) );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_FORCE_BOTROAMSONLY )
		return 1;
	if ( gcType == GC_DISABLE_TACTICIAN )
		return 1;

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
==============
(ModFN) ModifyAmmoUsage

Don't use up any ammo when firing.
==============
*/
static int MOD_PREFIX(ModifyAmmoUsage)( MODFN_CTV, int defaultValue, int weapon, qboolean alt ) {
	return 0;
}

/*
============
(ModFN) CanItemBeDropped

Don't drop rifles on death.
============
*/
static qboolean MOD_PREFIX(CanItemBeDropped)( MODFN_CTV, gitem_t *item, int clientNum ) {
	if ( item->giType == IT_WEAPON ) {
		return qfalse;
	}

	return MODFN_NEXT( CanItemBeDropped, ( MODFN_NC, item, clientNum ) );
}

/*
============
(ModFN) ModifyDamageFlags

Adjust some damage flags for consistency with original implementation. Probably doesn't make much difference typically.
============
*/
static int MOD_PREFIX(ModifyDamageFlags)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	if ( mod == MOD_CRIFLE_ALT || mod == MOD_CRIFLE_ALT_SPLASH ) {
		dflags |= DAMAGE_NO_ARMOR | DAMAGE_NO_INVULNERABILITY;
	}

	return MODFN_NEXT( ModifyDamageFlags, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );
}

/*
================
(ModFN) CheckItemSpawnDisabled

Disable most item pickups.
================
*/
static qboolean MOD_PREFIX(CheckItemSpawnDisabled)( MODFN_CTV, gitem_t *item ) {
	switch ( item->giType ) {
		case IT_ARMOR: //useless
		case IT_WEAPON: //only compression rifle
		case IT_HEALTH: //useless
		case IT_AMMO: //only compression rifle ammo
			return qtrue;
		case IT_HOLDABLE:
			switch ( item->giTag ) {
				case HI_MEDKIT:
				case HI_DETPACK:
					return qtrue;
			}
			break;
		case IT_POWERUP:
			switch ( item->giTag ) {
				case PW_BATTLESUIT:
				case PW_QUAD:
				case PW_REGEN:
				case PW_SEEKER:
					return qtrue;
			}
	}

	return MODFN_NEXT( CheckItemSpawnDisabled, ( MODFN_NC, item ) );
}

/*
================
(ModFN) AltFireConfig

Set rifle to alt fire only.
================
*/
static char MOD_PREFIX(AltFireConfig)( MODFN_CTV, weapon_t weapon ) {
	if ( weapon == WP_COMPRESSION_RIFLE ) {
		return 'F';
	}
	return MODFN_NEXT( AltFireConfig, ( MODFN_NC, weapon ) );
}

/*
================
ModDisintegration_Init
================
*/
void ModDisintegration_Init( void ) {
	if ( EF_WARN_ASSERT( !MOD_STATE ) ) {
		modcfg.mods_enabled.disintegration = qtrue;
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModAltFireConfig_Init();

		MODFN_REGISTER( SpawnConfigureClient, ++modePriorityLevel );
		MODFN_REGISTER( AdjustWeaponConstant, ++modePriorityLevel );
		MODFN_REGISTER( AdjustGeneralConstant, ++modePriorityLevel );
		MODFN_REGISTER( ModifyAmmoUsage, ++modePriorityLevel );
		MODFN_REGISTER( CanItemBeDropped, ++modePriorityLevel );
		MODFN_REGISTER( ModifyDamageFlags, ++modePriorityLevel );
		MODFN_REGISTER( CheckItemSpawnDisabled, ++modePriorityLevel );
		MODFN_REGISTER( AltFireConfig, ++modePriorityLevel );
	}
}
