/*
* Assimilation Borg Teleport
* 
* Borg players spawn with a borg teleporter, which when used, starts a countdown
* to receive another one.
* 
* The Portable Transporter module is used to handle the actual borg teleport,
* and Pending Item module is used to handle the countdown.
*/

#include "mods/modes/assimilation/assim_local.h"

#define PREFIX( x ) ModAssimBorgTeleport_##x
#define MOD_STATE PREFIX( state )

static struct {
	// For mod function stacking
	ModFNType_SpawnConfigureClient Prev_SpawnConfigureClient;
} *MOD_STATE;

/*
================
(ModFN) SpawnConfigureClient
================
*/
LOGFUNCTION_SVOID( PREFIX(SpawnConfigureClient), ( int clientNum ), ( clientNum ), "G_MODFN_SPAWNCONFIGURECLIENT" ) {
	gclient_t *client = &level.clients[clientNum];

	MOD_STATE->Prev_SpawnConfigureClient( clientNum );

	if ( client->sess.sessionClass == PC_BORG ) {
		// Initial teleporter
		G_GiveHoldable( client, HI_TRANSPORTER );
		client->ps.stats[STAT_USEABLE_PLACED] = 0;
	}
}

/*
================
ModAssimBorgTeleport_BorgTeleportEnabled
================
*/
LOGFUNCTION_SRET( qboolean, ModAssimBorgTeleport_BorgTeleportEnabled, ( int clientNum ), ( clientNum ), "" ) {
	gclient_t *client = &level.clients[clientNum];
	return client->sess.sessionClass == PC_BORG;
}

/*
================
ModAssimBorgTeleport_PostBorgTeleport

Called after a borg teleport has completed. Start countdown to get new teleporter.
================
*/
LOGFUNCTION_SVOID( ModAssimBorgTeleport_PostBorgTeleport, ( int clientNum ), ( clientNum ), "" ) {
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

#define INIT_FN_STACKABLE( name ) \
	MOD_STATE->Prev_##name = modfn.name; \
	modfn.name = PREFIX(name);

#define INIT_FN_OVERRIDE( name ) \
	modfn.name = PREFIX(name);

#define INIT_FN_BASE( name ) \
	EF_WARN_ASSERT( modfn.name == ModFNDefault_##name ); \
	modfn.name = PREFIX(name);

LOGFUNCTION_VOID( ModAssimBorgTeleport_Init, ( void ), (), "G_MOD_INIT G_ASSIMILATION" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		// Register mod functions
		INIT_FN_STACKABLE( SpawnConfigureClient );

		// Configure borg teleporters
		ModHoldableTransporter_Init();
		ModHoldableTransporter_config->borgTeleportEnabled = ModAssimBorgTeleport_BorgTeleportEnabled;
		ModHoldableTransporter_config->postBorgTeleport = ModAssimBorgTeleport_PostBorgTeleport;

		// Pending item support for transporter cooldown
		ModPendingItem_Init();
	}
}
