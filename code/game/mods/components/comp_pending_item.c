/*
* Pending Items
* 
* This module is used to handle pending item countdowns, such as the borg
* teleporter in Assimilation and the demolitionist detpack in Specialties.
*/

#define MOD_NAME ModPendingItem

#include "mods/g_mod_local.h"

typedef struct {
	int pendingItemTime;
	holdable_t pendingItem;
} modClient_t;

static struct {
	modClient_t clients[MAX_CLIENTS];
} *MOD_STATE;

/*
================
(ModFN) PreClientSpawn

Reset pending item when client spawns.
================
*/
static void MOD_PREFIX(PreClientSpawn)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	modClient_t *modclient = &MOD_STATE->clients[clientNum];
	modclient->pendingItem = HI_NONE;
	modclient->pendingItemTime = 0;
	MODFN_NEXT( PreClientSpawn, ( MODFN_NC, clientNum, spawnType ) );
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	int i;
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

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
void ModPendingItem_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( PreClientSpawn, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
	}
}
