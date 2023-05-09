/*
* Ping Compensation
* 
* This is the main module for ping compensation and player movement smoothing features.
* 
* It provides functions to check which features are enabled, but the actual implementation
* of each feaure is handled through submodules.
*/

#define MOD_NAME ModPingcomp

#include "mods/features/pingcomp/pc_local.h"

static struct {
	trackedCvar_t g_unlagged;	// 0 = disabled, 1 = instant hit only, 2 = all weapons
	trackedCvar_t g_smoothClients;	// 0 = disabled, 1 = enabled
} *MOD_STATE;

/*
================
ModPingcomp_Static_PingCompensationEnabled
================
*/
qboolean ModPingcomp_Static_PingCompensationEnabled( void ) {
	return MOD_STATE && MOD_STATE->g_unlagged.integer ? qtrue : qfalse;
}

/*
================
ModPingcomp_Static_ProjectileCompensationEnabled
================
*/
qboolean ModPingcomp_Static_ProjectileCompensationEnabled( void ) {
	return MOD_STATE && MOD_STATE->g_unlagged.integer >= 2 ? qtrue : qfalse;
}

/*
================
ModPingcomp_Static_SmoothingEnabled
================
*/
qboolean ModPingcomp_Static_SmoothingEnabled( void ) {
	return MOD_STATE && MOD_STATE->g_smoothClients.integer ? qtrue : qfalse;
}

/*
==============
ModPingcomp_Static_SmoothingEnabledForClient
==============
*/
qboolean ModPingcomp_Static_SmoothingEnabledForClient( int clientNum ) {
	if ( !ModPingcomp_Static_SmoothingEnabled() || !G_IsConnectedClient( clientNum ) ||
			level.clients[clientNum].pers.connected != CON_CONNECTED || clientNum >= MAX_SMOOTHING_CLIENTS ) {
		return qfalse;
	}
	return qtrue;
}

/*
================
ModPingcomp_Static_PositionShiftEnabled

Returns whether any position shifting functions are active. If not, storage of client frames can be
skipped for efficiency.
================
*/
qboolean ModPingcomp_Static_PositionShiftEnabled( void ) {
	return MOD_STATE && ( ModPingcomp_Static_PingCompensationEnabled() ||
			ModPingcomp_Static_SmoothingEnabled() ) ? qtrue : qfalse;
}

/*
================
ModPingcomp_CvarCallback

Make sure any configstrings for client prediction are updated.
================
*/
static void ModPingcomp_CvarCallback( trackedCvar_t *cvar ) {
	ModModcfgCS_Static_Update();
}

/*
================
ModPingcomp_Init
================
*/
void ModPingcomp_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		ModPCSmoothing_Init();
		ModPCInstantWeapons_Init();
		ModPCProjectileLaunch_Init();
		ModPCClientPredict_Init();

		G_RegisterTrackedCvar( &MOD_STATE->g_unlagged, "g_unlagged", "0", CVAR_SERVERINFO, qfalse );
		G_RegisterCvarCallback( &MOD_STATE->g_unlagged, ModPingcomp_CvarCallback, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_smoothClients, "g_smoothClients", "1", 0, qfalse );
		G_RegisterCvarCallback( &MOD_STATE->g_smoothClients, ModPingcomp_CvarCallback, qfalse );
	}
}
