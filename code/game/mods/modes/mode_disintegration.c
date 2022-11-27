/*
* Disintegration Mode
* 
* All players spawn with a rifle which has unlimited ammo and kills with one hit.
*/

#define MOD_PREFIX( x ) ModDisintegration_##x

#include "mods/g_mod_local.h"

static struct {
	// For mod function stacking
	ModFNType_SpawnConfigureClient Prev_SpawnConfigureClient;
	ModFNType_AdjustWeaponConstant Prev_AdjustWeaponConstant;
	ModFNType_AdjustGeneralConstant Prev_AdjustGeneralConstant;
	ModFNType_PmoveInit Prev_PmoveInit;
	ModFNType_CanItemBeDropped Prev_CanItemBeDropped;
	ModFNType_ModifyDamageFlags Prev_ModifyDamageFlags;
	ModFNType_CheckItemSpawnDisabled Prev_CheckItemSpawnDisabled;
} *MOD_STATE;

/*
================
(ModFN) SpawnConfigureClient

Player starts with just a rifle.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(SpawnConfigureClient), ( int clientNum ), ( clientNum ), "G_MODFN_SPAWNCONFIGURECLIENT" ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];

	MOD_STATE->Prev_SpawnConfigureClient( clientNum );

	client->ps.stats[STAT_WEAPONS] = ( 1 << WP_COMPRESSION_RIFLE );
	client->ps.ammo[WP_COMPRESSION_RIFLE] = Max_Ammo[WP_COMPRESSION_RIFLE];
}

/*
======================
(ModFN) AdjustWeaponConstant

Rifle does enough damage to 1-hit, but no splash damage.
======================
*/
static int MOD_PREFIX(AdjustWeaponConstant)( weaponConstant_t wcType, int defaultValue ) {
	if ( wcType == WC_USE_RANDOM_DAMAGE )
		return 0;
	if ( wcType == WC_CRIFLE_ALTDAMAGE )
		return 1000;
	if ( wcType == WC_CRIFLE_ALT_SPLASH_RADIUS )
		return 0;
	if ( wcType == WC_CRIFLE_ALT_SPLASH_DMG )
		return 0;

	return MOD_STATE->Prev_AdjustWeaponConstant( wcType, defaultValue );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_FORCE_BOTROAMSONLY )
		return 1;
	if ( gcType == GC_DISABLE_TACTICIAN )
		return 1;

	return MOD_STATE->Prev_AdjustGeneralConstant( gcType, defaultValue );
}

/*
==============
(ModFN) ModifyAmmoUsage

Don't use up any ammo when firing.
==============
*/
LOGFUNCTION_SRET( int, MOD_PREFIX(ModifyAmmoUsage), ( int defaultValue, int weapon, qboolean alt ),
		( defaultValue, weapon, alt ), "G_MODFN_MODIFYAMMOUSAGE" ) {
	return 0;
}

/*
================
(ModFN) PmoveInit

Enable alt attack mode.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PmoveInit), ( int clientNum, pmove_t *pmove ),
		( clientNum, pmove ), "G_MODFN_PMOVEINIT" ) {
	MOD_STATE->Prev_PmoveInit( clientNum, pmove );

	pmove->pModDisintegration = qtrue;
}

/*
============
(ModFN) CanItemBeDropped

Don't drop rifles on death.
============
*/
LOGFUNCTION_SRET( qboolean, MOD_PREFIX(CanItemBeDropped), ( gitem_t *item, int clientNum ),
		( item, clientNum ), "G_MODFN_CANITEMBEDROPPED" ) {
	if ( item->giType == IT_WEAPON ) {
		return qfalse;
	}

	return MOD_STATE->Prev_CanItemBeDropped( item, clientNum );
}

/*
============
(ModFN) ModifyDamageFlags

Adjust some damage flags for consistency with original implementation. Probably doesn't make much difference typically.
============
*/
LOGFUNCTION_SRET( int, MOD_PREFIX(ModifyDamageFlags), ( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ),
		( targ, inflictor, attacker, dir, point, damage, dflags, mod ), "G_MODFN_MODIFYDAMAGEFLAGS" ) {
	if ( mod == MOD_CRIFLE_ALT || mod == MOD_CRIFLE_ALT_SPLASH ) {
		return DAMAGE_NO_ARMOR | DAMAGE_NO_INVULNERABILITY;
	}

	return MOD_STATE->Prev_ModifyDamageFlags( targ, inflictor, attacker, dir, point, damage, dflags, mod );
}

/*
================
(ModFN) CheckItemSpawnDisabled

Disable most item pickups.
================
*/
LOGFUNCTION_SRET( qboolean, MOD_PREFIX(CheckItemSpawnDisabled), ( gitem_t *item ), ( item ), "G_MODFN_CHECKITEMSPAWNDISABLED" ) {
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

	return MOD_STATE->Prev_CheckItemSpawnDisabled( item );
}

/*
================
ModDisintegration_Init
================
*/
LOGFUNCTION_VOID( ModDisintegration_Init, ( void ), (), "G_MOD_INIT G_DISINTEGRATION" ) {
	if ( EF_WARN_ASSERT( !MOD_STATE ) ) {
		modcfg.mods_enabled.disintegration = qtrue;
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( SpawnConfigureClient );
		INIT_FN_STACKABLE( AdjustWeaponConstant );
		INIT_FN_STACKABLE( AdjustGeneralConstant );
		INIT_FN_BASE( ModifyAmmoUsage );
		INIT_FN_STACKABLE( PmoveInit );
		INIT_FN_STACKABLE( CanItemBeDropped );
		INIT_FN_STACKABLE( ModifyDamageFlags );
		INIT_FN_STACKABLE( CheckItemSpawnDisabled );
	}
}
