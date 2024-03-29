/*
* UAM Weapon Spawn
* 
* In UAM weapons are given out when players spawn with them, rather than as pickups in
* the map. The weapons players may spawn with are controlled by 3 cvars:
* 
* g_mod_WeaponAvailableFlags: Every time a player spawns, they receive 1 weapon
* randomly selected from this list.
* 
* g_mod_WeaponStartingFlags: Every time a player spawns, they receive every weapon
* from this list.
* 
* g_mod_WeaponRoundFlags: 1 weapon is randomly selected from this list at the beginning of
* each round. Every player who spawns receives this weapon for the duration of the round.
*/

#define MOD_NAME ModUAMWeaponSpawn

#include "mods/modes/uam/uam_local.h"

static struct {
	// Cvars
	trackedCvar_t g_mod_WeaponAvailableFlags;
	trackedCvar_t g_mod_WeaponStartingFlags;
	trackedCvar_t g_mod_WeaponRoundFlags;

	// Weapon configuration generated by ModUAMWeaponSpawn_UpdateWeapons
	unsigned int roundWeaponSelection;
	unsigned int availableWeps;
	unsigned int startingWeps;

	// The weapon selection for each player is cached here and only cleared if the player dies.
	// This helps discourage players from reconnecting to cycle to a different weapon, because
	// they will tend to get the same weapon if they are assigned the same client slot.
	int weaponSelection[MAX_CLIENTS];
} *MOD_STATE;

#define WEAPON_MASK( weapon ) ( 1 << ( weapon - 1 ) )
#define VALID_WEAPONS ( ( WEAPON_MASK( WP_NUM_WEAPONS ) - 1 ) & ~WEAPON_MASK( WP_VOYAGER_HYPO ) )

/*
================
CountBits

Count the number of bits in a value.
================
*/
static int CountBits( unsigned int bits ) {
	int result = 0;
	while ( bits ) {
		if ( bits & 1 ) {
			++result;
		}
		bits >>= 1;
	}
	return result;
}

/*
================
PickRandomBit

Given a bitmask, returns the index (0-31) of 1 randomly selected bit.
Input should not be 0.
================
*/
static int PickRandomBit( unsigned int mask ) {
	int count = CountBits( mask );
	if ( count ) {
		int sel = irandom( 1, count );
		int i;
		for ( i = 0; i < 32; ++i ) {
			if ( ( 1 << i ) & mask ) {
				if ( !--sel ) {
					return i;
				}
			}
		}
	}
	return 0;
}

/*
================
ModUAMWeaponSpawn_UpdateWeapons

Generate weapon configuration for round.
================
*/
static void ModUAMWeaponSpawn_UpdateWeapons( void ) {
	unsigned int roundWeps;
	MOD_STATE->availableWeps = G_ModUtils_ReadGladiatorBitflags( MOD_STATE->g_mod_WeaponAvailableFlags.string ) & VALID_WEAPONS;
	MOD_STATE->startingWeps = G_ModUtils_ReadGladiatorBitflags( MOD_STATE->g_mod_WeaponStartingFlags.string ) & VALID_WEAPONS;
	roundWeps = G_ModUtils_ReadGladiatorBitflags( MOD_STATE->g_mod_WeaponRoundFlags.string ) & VALID_WEAPONS;

	// If only one weapon is in availableWeps or roundWeps, move it to startingWeps.
	if ( CountBits( MOD_STATE->availableWeps ) == 1 ) {
		MOD_STATE->startingWeps |= MOD_STATE->availableWeps;
		MOD_STATE->availableWeps = 0;
	}
	if ( CountBits( roundWeps ) == 1 ) {
		MOD_STATE->startingWeps |= roundWeps;
		roundWeps = 0;
	}

	// Clear anything from availableWeps and roundWeps that is in startingWeps.
	MOD_STATE->availableWeps &= ~MOD_STATE->startingWeps;
	roundWeps &= ~MOD_STATE->startingWeps;

	// Generate new round weapon selection if needed.
	if ( !( MOD_STATE->roundWeaponSelection & roundWeps ) ) {
		MOD_STATE->roundWeaponSelection = roundWeps ? 1 << PickRandomBit( roundWeps ) : 0;
	}

	// Merge round weapon for round into startingWeps, and remove from availableWeps.
	MOD_STATE->startingWeps |= MOD_STATE->roundWeaponSelection;
	MOD_STATE->availableWeps &= ~MOD_STATE->roundWeaponSelection;

	// Make sure there is at least 1 weapon available.
	if ( !MOD_STATE->availableWeps && !MOD_STATE->startingWeps ) {
		MOD_STATE->availableWeps = G_ModUtils_ReadGladiatorBitflags( "NYYYYYYYYNNY" ) & VALID_WEAPONS;
	}
}

/*
================
ModUAMWeaponSpawn_WeaponCvarCallback
================
*/
static void ModUAMWeaponSpawn_WeaponCvarCallback( trackedCvar_t *cvar ) {
	ModUAMWeaponSpawn_UpdateWeapons();
}

/*
================
ModUAMWeaponSpawn_StartingAmmo

Give each weapon a certain fixed amount of ammo consistent with Gladiator mod.
================
*/
static int ModUAMWeaponSpawn_StartingAmmo( weapon_t weapon ) {
	switch ( weapon ) {
		case WP_PHASER:
			return 100;
		case WP_COMPRESSION_RIFLE:
			return 50;
		case WP_IMOD:
			return 150;
		case WP_SCAVENGER_RIFLE:
			return 50;
		case WP_STASIS:
			return 70;
		case WP_GRENADE_LAUNCHER:
			return 35;
		case WP_TETRION_DISRUPTOR:
			return 35;
		case WP_QUANTUM_BURST:
			return 35;
		case WP_DREADNOUGHT:
			return 35;
		case WP_BORG_ASSIMILATOR:
			return 100;
		case WP_BORG_WEAPON:
			return 70;
	}

	return 0;
}

/*
================
(ModFN) SpawnConfigureClient

Add weapons to client inventory when spawning.
================
*/
static void MOD_PREFIX(SpawnConfigureClient)( MODFN_CTV, int clientNum ) {
	gclient_t *client = &level.clients[clientNum];
	int *weaponSelection = &MOD_STATE->weaponSelection[clientNum];
	int weapon;

	MODFN_NEXT( SpawnConfigureClient, ( MODFN_NC, clientNum ) );

	if ( !*weaponSelection ) {
		// Determine which weapons to give player.
		*weaponSelection = MOD_STATE->startingWeps;
		if ( MOD_STATE->availableWeps ) {
			// Pick one additional random weapon.
			*weaponSelection |= ( 1 << PickRandomBit( MOD_STATE->availableWeps ) );
		}
	}

	// Add the weapons and ammo.
	client->ps.stats[STAT_WEAPONS] = 0;
	for ( weapon = WP_PHASER; weapon < WP_NUM_WEAPONS; ++weapon ) {
		if ( WEAPON_MASK( weapon ) & *weaponSelection ) {
			client->ps.stats[STAT_WEAPONS] |= ( 1 << weapon );
			client->ps.ammo[weapon] = ModUAMWeaponSpawn_StartingAmmo( weapon );
		}
	}
}

/*
================
(ModFN) PostPlayerDie

Clear weapon selection so a new one will be picked on next spawn.
================
*/
static void MOD_PREFIX(PostPlayerDie)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int meansOfDeath, int *awardPoints ) {
	int clientNum = self - g_entities;

	MODFN_NEXT( PostPlayerDie, ( MODFN_NC, self, inflictor, attacker, meansOfDeath, awardPoints ) );

	MOD_STATE->weaponSelection[clientNum] = 0;
}

/*
============
(ModFN) CanItemBeDropped

Disable weapon drops if everybody is starting with the same weapons.
============
*/
static qboolean MOD_PREFIX(CanItemBeDropped)( MODFN_CTV, gitem_t *item, int clientNum ) {
	if ( item->giType == IT_WEAPON ) {
		// Drop all weapons including phaser, if the weapon is part of random selection pool
		// (i.e. not already given to all players at start).
		return ( MOD_STATE->availableWeps & WEAPON_MASK( item->giTag ) ) ? qtrue : qfalse;
	}

	return MODFN_NEXT( CanItemBeDropped, ( MODFN_NC, item, clientNum ) );
}

/*
================
(ModFN) AddAmmoForItem

Set the standard ammo amount when player picks up a weapon (presumably dropped by another player).

Probably would be fine to preset the ammo for all weapons, but this way saves a few bytes in playerstate
ammo fields when only a few weapons are loaded.
================
*/
static void MOD_PREFIX(AddAmmoForItem)( MODFN_CTV, int clientNum, gentity_t *item, int weapon, int count ) {
	gclient_t *client = &level.clients[clientNum];
	client->ps.ammo[weapon] = ModUAMWeaponSpawn_StartingAmmo( weapon );
}

/*
================
(ModFN) CheckItemSpawnDisabled

Disable weapon item spawns, since weapons are distributed automatically.
================
*/
static qboolean MOD_PREFIX(CheckItemSpawnDisabled)( MODFN_CTV, gitem_t *item ) {
	if ( item->giType == IT_WEAPON ) {
		return qtrue;
	}

	return MODFN_NEXT( CheckItemSpawnDisabled, ( MODFN_NC, item ) );
}

/*
================
(ModFN) PodiumWeapon

On intermission podium, display weapon given to player at start of round, rather than most
recently held weapon.
================
*/
static weapon_t MOD_PREFIX(PodiumWeapon)( MODFN_CTV, int clientNum ) {
	weapon_t weapon;
	int randomSelected = MOD_STATE->weaponSelection[clientNum] & MOD_STATE->availableWeps;

	for ( weapon = WP_NUM_WEAPONS - 1; weapon >= WP_PHASER; --weapon ) {
		if ( WEAPON_MASK( weapon ) & randomSelected ) {
			return weapon;
		}
	}

	return MODFN_NEXT( PodiumWeapon, ( MODFN_NC, clientNum ) );
}

/*
================
(ModFN) AddRegisteredItems

Register spawn weapons to avoid loading glitches. All weapons are registered even if they
are temporarily disabled, because they might be enabled in later rounds. The welder also
needs to be registered for full forcefield kill sound effects on the client.
================
*/
static void MOD_PREFIX(AddRegisteredItems)( MODFN_CTV ) {
	weapon_t weapon;
	MODFN_NEXT( AddRegisteredItems, ( MODFN_NC ) );
	for ( weapon = WP_PHASER; weapon <= WP_DREADNOUGHT; ++weapon ) {
		RegisterItem( BG_FindItemForWeapon( weapon ) );
	}

	RegisterItem( BG_FindItemForWeapon( WP_BORG_WEAPON ) );
	RegisterItem( BG_FindItemForWeapon( WP_BORG_ASSIMILATOR ) );
}

/*
================
ModUAMWeaponSpawn_Init
================
*/
void ModUAMWeaponSpawn_Init( qboolean allowWeaponCvarChanges ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		// Load cvar values
		G_RegisterTrackedCvar( &MOD_STATE->g_mod_WeaponAvailableFlags, "g_mod_WeaponAvailableFlags", "NYYYYYYYYNNY", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_mod_WeaponStartingFlags, "g_mod_WeaponStartingFlags", "NNNNNNNNNNNN", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_mod_WeaponRoundFlags, "g_mod_WeaponRoundFlags", "NNNNNNNNNNNN", 0, qfalse );

		// Generate weapon configuration
		ModUAMWeaponSpawn_UpdateWeapons();

		// Regenerate weapon configuration when cvars are changed, if enabled.
		if ( allowWeaponCvarChanges ) {
			G_RegisterCvarCallback( &MOD_STATE->g_mod_WeaponAvailableFlags, ModUAMWeaponSpawn_WeaponCvarCallback, qfalse );
			G_RegisterCvarCallback( &MOD_STATE->g_mod_WeaponStartingFlags, ModUAMWeaponSpawn_WeaponCvarCallback, qfalse );
			G_RegisterCvarCallback( &MOD_STATE->g_mod_WeaponRoundFlags, ModUAMWeaponSpawn_WeaponCvarCallback, qfalse );
		}

		MODFN_REGISTER( SpawnConfigureClient, ++modePriorityLevel );
		MODFN_REGISTER( PostPlayerDie, ++modePriorityLevel );
		MODFN_REGISTER( AddAmmoForItem, ++modePriorityLevel );
		MODFN_REGISTER( CanItemBeDropped, ++modePriorityLevel );
		MODFN_REGISTER( CheckItemSpawnDisabled, ++modePriorityLevel );
		MODFN_REGISTER( PodiumWeapon, ++modePriorityLevel );
		MODFN_REGISTER( AddRegisteredItems, ++modePriorityLevel );
	}
}
