/*
* Clan Arena Mode
* 
* This is an Elimination variant loosely inspired by Quake 3 mods of the same name.
* Players spawn with all weapons, but limited ammo and no ammo spawns, so you have
* to prioritize which weapons to use first.
*/

#define MOD_NAME ModClanArena

#include "mods/g_mod_local.h"

// Enables clan arena respawn time scaling, which increase the time between respawns for "defensive"
// powerups such as health and armor as the match progresses. This is intended to compensate for the
// diminishing ammo mechanic to Clan Arena, because otherwise a full complement of health and armor
// respawns combined with a lack of ammo for strong weapons could make it too hard to get kills late-game.
#define FEATURE_ITEM_RESPAWN_TIMING

static struct {
	int _unused;
} *MOD_STATE;

#ifdef FEATURE_ITEM_RESPAWN_TIMING
typedef enum {
	PC_NON_DEFENSIVE,
	PC_DEFENSIVE,			// items that can slow down the match slightly
	PC_HIGHLY_DEFENSIVE		// items that can slow down the match a lot
} powerupClassification_t;

/*
================
GetPowerupClassification
================
*/
static powerupClassification_t ModClanArena_GetPowerupClassification( const gitem_t *item ) {
	if ( item->giType == IT_ARMOR || item->giType == IT_HEALTH )
		return PC_DEFENSIVE;

	if ( item->giType == IT_HOLDABLE && item->giTag == HI_MEDKIT )
		return PC_DEFENSIVE;

	if ( item->giType == IT_POWERUP && item->giTag == PW_REGEN )
		return PC_HIGHLY_DEFENSIVE;

	if ( item->giType == IT_HOLDABLE && item->giTag == HI_TRANSPORTER )
		return PC_HIGHLY_DEFENSIVE;

	return PC_NON_DEFENSIVE;
}

/*
================
GetItemScalingFactor
================
*/
static float ModClanArena_GetItemScalingFactor( const gitem_t *item ) {
	powerupClassification_t classification = ModClanArena_GetPowerupClassification( item );
	if ( classification != PC_NON_DEFENSIVE ) {
		float minutes_elapsed = (float)( level.time - level.startTime ) / ( 1000.0f * 60.0f );

		if ( minutes_elapsed < 0 )
			minutes_elapsed = 0;
		if ( minutes_elapsed > 5 )
			minutes_elapsed = 5;

		return 1.0f + minutes_elapsed / ( classification == PC_HIGHLY_DEFENSIVE ? 2.5f : 3.5f );
	} else {
		return 1.0f;
	}
}

/*
================
(ModFN) AdjustItemRespawnTime
================
*/
static float MOD_PREFIX(AdjustItemRespawnTime)( MODFN_CTV, float time, const gentity_t *ent ) {
	return MODFN_NEXT( AdjustItemRespawnTime, ( MODFN_NC, time, ent ) ) *
			ModClanArena_GetItemScalingFactor( ent->item );
}
#endif

/*
===========
(ModFN) SpawnCenterPrintMessage

Override the elimination mode message (print "Clan Arena" instead).
============
*/
static void MOD_PREFIX(SpawnCenterPrintMessage)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionTeam == TEAM_SPECTATOR || spawnType != CST_RESPAWN ) {
		trap_SendServerCommand( clientNum, "cp \"Clan Arena\"" );
	}
}

/*
================
(ModFN) CheckItemSpawnDisabled

Disable weapon and ammo pickups.
================
*/
static qboolean MOD_PREFIX(CheckItemSpawnDisabled)( MODFN_CTV, gitem_t *item ) {
	if ( item->giType == IT_WEAPON || item->giType == IT_AMMO ) {
		return qtrue;
	}

	return MODFN_NEXT( CheckItemSpawnDisabled, ( MODFN_NC, item ) );
}

/*
============
(ModFN) CanItemBeDropped

Disable weapon drops.
============
*/
static qboolean MOD_PREFIX(CanItemBeDropped)( MODFN_CTV, gitem_t *item, int clientNum ) {
	if ( item->giType == IT_WEAPON ) {
		return qfalse;
	}

	return MODFN_NEXT( CanItemBeDropped, ( MODFN_NC, item, clientNum ) );
}

/*
======================
(ModFN) AdjustWeaponConstant

Enable tripmines with standard grenade damage.
======================
*/
static int MOD_PREFIX(AdjustWeaponConstant)( MODFN_CTV, weaponConstant_t wcType, int defaultValue ) {
	switch ( wcType ) {
		case WC_USE_TRIPMINES:
			return 1;		// orig: 0
	}

	return MODFN_NEXT( AdjustWeaponConstant, ( MODFN_NC, wcType, defaultValue ) );
}

/*
==============
(ModFN) ModifyAmmoUsage

Tripmines use 2 ammo instead of 1.
==============
*/
static int MOD_PREFIX(ModifyAmmoUsage)( MODFN_CTV, int defaultValue, int weapon, qboolean alt ) {
	if ( weapon == WP_GRENADE_LAUNCHER && alt ) {
		return 2;
	}

	return MODFN_NEXT( ModifyAmmoUsage, ( MODFN_NC, defaultValue, weapon, alt ) );
}

/*
============
(ModFN) GetDamageMult

Reduce damage by half so weapons aren't too overpowered given the strong arsenal players spawn with.
============
*/
static float MOD_PREFIX(GetDamageMult)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod, qboolean knockbackMode ) {
	if ( !knockbackMode ) {
		return MODFN_NEXT( GetDamageMult, ( MODFN_NC, targ, inflictor, attacker, dir,
				point, damage, dflags, mod, knockbackMode ) ) * 0.5f;
	}

	return MODFN_NEXT( GetDamageMult, ( MODFN_NC, targ, inflictor, attacker, dir,
			point, damage, dflags, mod, knockbackMode ) );
}

/*
================
(ModFN) SpawnConfigureClient

Add weapons to client inventory when spawning.
================
*/
static void MOD_PREFIX(SpawnConfigureClient)( MODFN_CTV, int clientNum ) {
	gclient_t *client = &level.clients[clientNum];
	weapon_t weapon;

	MODFN_NEXT( SpawnConfigureClient, ( MODFN_NC, clientNum ) );

	// Add the weapons and ammo
	client->ps.stats[STAT_WEAPONS] = 0;
	for ( weapon = WP_PHASER; weapon <= WP_DREADNOUGHT; ++weapon ) {
		client->ps.stats[STAT_WEAPONS] |= ( 1 << weapon );
		client->ps.ammo[weapon] = Max_Ammo[weapon];
	}

	client->ps.ammo[WP_DREADNOUGHT] = 60;	// 120 default
}

/*
================
(ModFN) AddRegisteredItems

Register spawn weapons to avoid loading glitches.
================
*/
static void MOD_PREFIX(AddRegisteredItems)( MODFN_CTV ) {
	weapon_t weapon;
	MODFN_NEXT( AddRegisteredItems, ( MODFN_NC ) );
	for ( weapon = WP_PHASER; weapon <= WP_DREADNOUGHT; ++weapon ) {
		RegisterItem( BG_FindItemForWeapon( weapon ) );
	}
}

/*
================
ModClanArena_Init
================
*/
void ModClanArena_Init( void ) {
	if ( !MOD_STATE ) {
		modcfg.mods_enabled.clanarena = qtrue;
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModElimination_Init();

		MODFN_REGISTER( SpawnCenterPrintMessage, ++modePriorityLevel );
		MODFN_REGISTER( CheckItemSpawnDisabled, ++modePriorityLevel );
		MODFN_REGISTER( CanItemBeDropped, ++modePriorityLevel );
		MODFN_REGISTER( AdjustWeaponConstant, ++modePriorityLevel );
		MODFN_REGISTER( ModifyAmmoUsage, ++modePriorityLevel );
		MODFN_REGISTER( GetDamageMult, ++modePriorityLevel );
		MODFN_REGISTER( SpawnConfigureClient, ++modePriorityLevel );
		MODFN_REGISTER( AddRegisteredItems, ++modePriorityLevel );

#ifdef FEATURE_ITEM_RESPAWN_TIMING
		MODFN_REGISTER( AdjustItemRespawnTime, ++modePriorityLevel );
#endif

		// Set serverinfo description
		trap_Cvar_Set( "gamename", "Clan Arena" );
	}
}
