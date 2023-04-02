/*
* Holdable Transporter
* 
* This module implements support for the borg teleporter, which replaces the normal
* point to point transport with a temporary flight mode.
*/

#define MOD_NAME ModHoldableTransporter

#include "mods/g_mod_local.h"

typedef struct {
	int endTeleportTime;
} holdableTransporter_client_t;

static struct {
	holdableTransporter_client_t clients[MAX_CLIENTS];
	qboolean borgTeleportActive;	// any client is currently borg teleporting
} *MOD_STATE;

/*
================
(ModFN) BorgTeleportEnabled
================
*/
LOGFUNCTION_RET( qboolean, ModFNDefault_BorgTeleportEnabled, ( int clientNum ), ( clientNum ), "G_MODFN_BORGTELEPORTENABLED" ) {
	return qfalse;
}

/*
================
(ModFN) PostBorgTeleport
================
*/
LOGFUNCTION_VOID( ModFNDefault_PostBorgTeleport, ( int clientNum ), ( clientNum ), "G_MODFN_POSTBORGTELEPORT" ) {
}

/*
================
ModHoldableTransporter_Rematerialize

Take player out of borg teleport mode.
================
*/
LOGFUNCTION_SVOID( ModHoldableTransporter_Rematerialize, ( int clientNum ), ( clientNum ), "" ) {
	holdableTransporter_client_t *modclient = &MOD_STATE->clients[clientNum];
	gentity_t *ent = &g_entities[clientNum];

	ent->flags &= ~FL_NOTARGET;
	ent->takedamage = qtrue;
	ent->r.contents = MASK_PLAYERSOLID;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->client->ps.eFlags &= ~EF_NODRAW;
	TeleportPlayer( ent, ent->client->ps.origin, ent->client->ps.viewangles, TP_BORG );
	
	// take it away
	modclient->endTeleportTime = 0;
	ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	ent->client->ps.stats[STAT_USEABLE_PLACED] = 0;

	modfn_lcl.PostBorgTeleport( clientNum );
}

/*
================
(ModFN) PortableTransporterActivate

Called when player triggers the holdable transporter powerup.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PortableTransporterActivate), ( MODFN_CTV, int clientNum ),
		( MODFN_CTN, clientNum ), "G_MODFN_PORTABLETRANSPORTERACTIVATE" ) {
	holdableTransporter_client_t *modclient = &MOD_STATE->clients[clientNum];
	gclient_t *client = &level.clients[clientNum];
	gentity_t *ent = &g_entities[clientNum];

	if ( BG_BorgTransporting( &client->ps ) ) {
		ModHoldableTransporter_Rematerialize( clientNum );
	}

	else if ( modfn_lcl.BorgTeleportEnabled( clientNum ) ) {
		// go into free-roaming mode
		gentity_t	*tent;
		ent->flags |= FL_NOTARGET;
		ent->takedamage = qfalse;
		ent->r.contents = CONTENTS_PLAYERCLIP;
		ent->s.eFlags |= EF_NODRAW;
		ent->client->ps.eFlags |= EF_NODRAW;
		ent->client->ps.stats[STAT_USEABLE_PLACED] = 2;

		modclient->endTeleportTime = level.time + 10000;
		MOD_STATE->borgTeleportActive = qtrue;

		tent = G_TempEntity( ent->client->ps.origin, EV_BORG_TELEPORT );
		tent->s.clientNum = ent->s.clientNum;
	}

	else {
		MODFN_NEXT( PortableTransporterActivate, ( MODFN_NC, clientNum ) );
	}
}

/*
================
(ModFN) PreRunFrame

Check for expiring borg teleports.
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(PreRunFrame), ( MODFN_CTV ), ( MODFN_CTN ), "G_MODFN_PRERUNFRAME" ) {
	MODFN_NEXT( PreRunFrame, ( MODFN_NC ) );

	if ( MOD_STATE->borgTeleportActive ) {
		int i;
		MOD_STATE->borgTeleportActive = qfalse;

		for ( i = 0; i < level.maxclients; ++i ) {
			gclient_t *client = &level.clients[i];
			holdableTransporter_client_t *modclient = &MOD_STATE->clients[i];

			if ( client->pers.connected >= CON_CONNECTING && BG_BorgTransporting( &client->ps ) ) {
				MOD_STATE->borgTeleportActive = qtrue;
				if ( level.time > modclient->endTeleportTime ) {
					ModHoldableTransporter_Rematerialize( i );
				}
			}

			else {
				modclient->endTeleportTime = 0;
			}
		}
	}
}

/*
================
ModHoldableTransporter_Init
================
*/
LOGFUNCTION_VOID( ModHoldableTransporter_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( PortableTransporterActivate );
		MODFN_REGISTER( PreRunFrame );
	}
}
