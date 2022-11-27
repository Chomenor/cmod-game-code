/*
* Pending Items
* 
* This module is used to handle pending item countdowns, such as the borg
* teleporter in Assimilation and the demolitionist detpack in Specialties.
*/

#define MOD_PREFIX( x ) ModPendingItem_##x

#include "mods/g_mod_local.h"

typedef struct {
	int pendingItemTime;
	holdable_t pendingItem;
} modClient_t;

static struct {
	modClient_t clients[MAX_CLIENTS];

	// For mod function stacking
	ModFNType_PreClientSpawn Prev_PreClientSpawn;
	ModFNType_PostRunFrame Prev_PostRunFrame;
} *MOD_STATE;

/*
================
(ModFN) PreClientSpawn

Reset pending item when client spawns.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PreClientSpawn), ( int clientNum, clientSpawnType_t spawnType ),
		( clientNum, spawnType ), "G_MODFN_PRECLIENTSPAWN" ) {
	modClient_t *modclient = &MOD_STATE->clients[clientNum];
	modclient->pendingItem = HI_NONE;
	modclient->pendingItemTime = 0;
	MOD_STATE->Prev_PreClientSpawn( clientNum, spawnType );
}

/*
================
(ModFN) PostRunFrame
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostRunFrame), ( void ), (), "G_MODFN_POSTRUNFRAME" ) {
	int i;
	MOD_STATE->Prev_PostRunFrame();

	for ( i = 0; i < level.maxclients; i++ ) {
		gclient_t *client = &level.clients[i];
		modClient_t *modclient = &MOD_STATE->clients[i];

		if ( client->pers.connected != CON_CONNECTED || modfn.SpectatorClient( i ) ) {
			continue;
		}

		if ( modclient->pendingItemTime && EF_WARN_ASSERT( modclient->pendingItem ) && modclient->pendingItemTime < level.time ) {
			// Time has elapsed, give item to player
			G_GiveHoldable( client, modclient->pendingItem );
			client->ps.stats[STAT_USEABLE_PLACED] = 0;
			modclient->pendingItemTime = 0;
		}

		if ( modclient->pendingItemTime ) {
			// Time still remains, update countdown value for CG_DrawHoldableItem
			client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
			client->ps.stats[STAT_USEABLE_PLACED] = ( modclient->pendingItemTime - level.time + 999 ) / 1000;
		}
	}
}

/*
================
ModPendingItem_Shared_SchedulePendingItem
================
*/
void ModPendingItem_Shared_SchedulePendingItem( int clientNum, holdable_t item, int delay ) {
	modClient_t *modclient;
	EF_ERR_ASSERT( MOD_STATE );

	modclient = &MOD_STATE->clients[clientNum];
	modclient->pendingItem = item;
	modclient->pendingItemTime = level.time + delay;
}

/*
================
ModPendingItem_Init
================
*/
LOGFUNCTION_VOID( ModPendingItem_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( PreClientSpawn );
		INIT_FN_STACKABLE( PostRunFrame );
	}
}
