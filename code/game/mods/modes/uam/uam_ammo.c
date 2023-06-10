/*
* UAM Ammo
* 
* By default, all weapons in UAM have fully unlimited ammo.
* 
* For consistency with Gladiator mod, the option g_mod_WeaponAmmoMode is supported
* to set weapons to ammo regenerating mode, although not well tested.
*/

#define MOD_NAME ModUAMAmmo

#include "mods/modes/uam/uam_local.h"

typedef struct {
	int regenFireDelay;
	int lastMoveTime;
	int timeToRegen[WP_NUM_WEAPONS];
} modClient_t;

static struct {
	modClient_t clients[MAX_CLIENTS];
	int ammoFlags;
} *MOD_STATE;

#define IS_REGEN_ENABLED( weapon ) ( MOD_STATE->ammoFlags & ( 1 << ( weapon - 1 ) ) )

static const struct {
	int maxAmmo;
	int regenAmount;
	int regenInterval;
} weaponStats[WP_NUM_WEAPONS] = {
	{ 0, 0, 0 },		// WP_NONE
	{ 100, 2, 100 },	// WP_PHASER
	{ 50, 2, 250 },		// WP_COMPRESSION_RIFLE
	{ 150, 2, 350 },	// WP_IMOD
	{ 50, 2, 100 },		// WP_SCAVENGER_RIFLE
	{ 70, 2, 700 },		// WP_STASIS
	{ 35, 2, 700 },		// WP_GRENADE_LAUNCHER
	{ 35, 2, 150 },		// WP_TETRION_DISRUPTOR
	{ 35, 2, 1200 },	// WP_QUANTUM_BURST
	{ 35, 2, 100 },		// WP_DREADNOUGHT
	{ 100, 0, 0 },		// WP_VOYAGER_HYPO
	{ 100, 2, 100 },	// WP_BORG_ASSIMILATOR
	{ 70, 2, 200 },		// WP_BORG_WEAPON
};

/*
==============
UpdateFireDelay
==============
*/
static void UpdateFireDelay( int clientNum, int oldEventSequence, int moveLength ) {
	int i;
	modClient_t *modClient = &MOD_STATE->clients[clientNum];
	gclient_t *client = &level.clients[clientNum];

	modClient->regenFireDelay -= moveLength;
	if ( modClient->regenFireDelay < 0 ) {
		modClient->regenFireDelay = 0;
	}

	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence; i < client->ps.eventSequence; i++ ) {
		int event = client->ps.events[i & ( MAX_PS_EVENTS - 1 )];
		if ( event == EV_FIRE_WEAPON || event == EV_ALT_FIRE || event == EV_FIRE_EMPTY_PHASER ) {
			modClient->regenFireDelay = 200;
		}
	}
}

/*
==============
(ModFN) PostPmoveActions
==============
*/
static void MOD_PREFIX(PostPmoveActions)( MODFN_CTV, pmove_t *pmove, int clientNum, int oldEventSequence ) {
	modClient_t *modClient = &MOD_STATE->clients[clientNum];
	gclient_t *client = &level.clients[clientNum];
	int moveLength;

	MODFN_NEXT( PostPmoveActions, ( MODFN_NC, pmove, clientNum, oldEventSequence ) );

	moveLength = client->ps.commandTime - modClient->lastMoveTime;
	modClient->lastMoveTime = client->ps.commandTime;

	// Don't regen if weapon was fired too recently.
	UpdateFireDelay( clientNum, oldEventSequence, moveLength );
	if ( modClient->regenFireDelay > 0 ) {
		return;
	}

	// Don't regen while currently firing.
	if ( client->ps.weaponTime > 0 ) {
		return;
	}

	if ( moveLength > 0 ) {
		int weapon;
		for ( weapon = WP_PHASER; weapon < WP_NUM_WEAPONS; ++weapon ) {
			if ( weaponStats[weapon].regenInterval > 0 ) {
				modClient->timeToRegen[weapon] -= moveLength;
				if ( modClient->timeToRegen[weapon] <= 0 ) {
					qboolean haveWeapon = client->ps.stats[STAT_WEAPONS] & ( 1 << weapon );
					if ( haveWeapon && client->ps.ammo[weapon] < weaponStats[weapon].maxAmmo ) {
						// Add ammo and time to counter.
						client->ps.ammo[weapon] += weaponStats[weapon].regenAmount;
						if ( client->ps.ammo[weapon] > weaponStats[weapon].maxAmmo ) {
							client->ps.ammo[weapon] = weaponStats[weapon].maxAmmo;
						}
						modClient->timeToRegen[weapon] += weaponStats[weapon].regenInterval;
					} else {
						// Weapon not currently regenerating.
						modClient->timeToRegen[weapon] = 0;
					}
				}
			}
		}
	}
}

/*
==============
(ModFN) ModifyAmmoUsage

Don't use up any ammo when firing.
==============
*/
static int MOD_PREFIX(ModifyAmmoUsage)( MODFN_CTV, int defaultValue, int weapon, qboolean alt ) {
	if ( !IS_REGEN_ENABLED( weapon ) ) {
		// Non regenerating weapons don't use ammo.
		return 0;
	}

	return MODFN_NEXT( ModifyAmmoUsage, ( MODFN_NC, defaultValue, weapon, alt ) );
}

/*
================
(ModFN) CheckItemSpawnDisabled

Disable ammo pickups.
================
*/
static qboolean MOD_PREFIX(CheckItemSpawnDisabled)( MODFN_CTV, gitem_t *item ) {
	if ( item->giType == IT_AMMO ) {
		return qtrue;
	}

	return MODFN_NEXT( CheckItemSpawnDisabled, ( MODFN_NC, item ) );
}

/*
================
ModUAMAmmo_Init
================
*/
void ModUAMAmmo_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		{
			vmCvar_t ammoFlagsCvar;
			trap_Cvar_Register( &ammoFlagsCvar, "g_mod_WeaponAmmoMode", "000000000000", CVAR_LATCH );
			MOD_STATE->ammoFlags = G_ModUtils_ReadGladiatorBitflags( ammoFlagsCvar.string );
		}

		MODFN_REGISTER( PostPmoveActions, ++modePriorityLevel );
		MODFN_REGISTER( ModifyAmmoUsage, ++modePriorityLevel );
		MODFN_REGISTER( CheckItemSpawnDisabled, ++modePriorityLevel );
	}
}
