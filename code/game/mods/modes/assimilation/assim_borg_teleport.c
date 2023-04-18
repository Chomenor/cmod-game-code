/*
* Assimilation Borg Teleport
* 
* Borg players spawn with a borg teleporter, which when used, starts a countdown
* to receive another one.
* 
* The Portable Transporter module is used to handle the actual borg teleport,
* and Pending Item module is used to handle the countdown.
*/

#define MOD_NAME ModAssimBorgTeleport

#include "mods/modes/assimilation/assim_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
================
(ModFN) SpawnConfigureClient
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(SpawnConfigureClient), ( MODFN_CTV, int clientNum ), ( MODFN_CTN, clientNum ), "G_MODFN_SPAWNCONFIGURECLIENT" ) {
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( SpawnConfigureClient, ( MODFN_NC, clientNum ) );

	if ( client->sess.sessionClass == PC_BORG ) {
		// Initial teleporter
		G_GiveHoldable( client, HI_TRANSPORTER );
		client->ps.stats[STAT_USEABLE_PLACED] = 0;
	}
}

/*
================
(ModFN) BorgTeleportEnabled

Enable borg teleport for borg class.
================
*/
LOGFUNCTION_SRET( qboolean, MOD_PREFIX(BorgTeleportEnabled), ( MODFN_CTV, int clientNum ), ( MODFN_CTN, clientNum ), "G_MODFN_BORGTELEPORTENABLED" ) {
	gclient_t *client = &level.clients[clientNum];
	return client->sess.sessionClass == PC_BORG;
}

/*
================
(ModFN) PostBorgTeleport

Called after a borg teleport has completed. Start countdown to get new teleporter.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PostBorgTeleport), ( MODFN_CTV, int clientNum ), ( MODFN_CTN, clientNum ), "G_MODFN_POSTBORGTELEPORT" ) {
	int delay = 15000;
	if ( modfn.IsBorgQueen( clientNum ) ) {
		delay = 60000;
	}

	ModPendingItem_Shared_SchedulePendingItem( clientNum, HI_TRANSPORTER, delay );
}

/*
================
ModAssimBorgTeleport_Init
================
*/
LOGFUNCTION_VOID( ModAssimBorgTeleport_Init, ( void ), (), "G_MOD_INIT G_ASSIMILATION" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		// Register mod functions
		MODFN_REGISTER( SpawnConfigureClient, ++modePriorityLevel );
		MODFN_REGISTER( BorgTeleportEnabled, ++modePriorityLevel );
		MODFN_REGISTER( PostBorgTeleport, ++modePriorityLevel );

		// Support borg teleporters
		ModHoldableTransporter_Init();

		// Pending item support for transporter cooldown
		ModPendingItem_Init();
	}
}
