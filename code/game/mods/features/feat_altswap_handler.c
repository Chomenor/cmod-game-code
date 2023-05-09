/*
* Alt Fire Swap Handler
* 
* This module implements the "setAltSwap" command which is used by the cgame-based
* alt fire swapping system.
* 
* This is mainly intended as a backup option for cases where neither server or client
* engine support for alt fire swapping is available. It is only enabled in local games
* or if sv_floodProtect is disabled on the server, because otherwise the flood protection
* could cause the command to be dropped leading to an inconsistent state.
*/

#define MOD_NAME ModAltSwapHandler

#include "mods/g_mod_local.h"

typedef struct {
	int swapFlags;
} AltFireSwap_client_t;

static struct {
	qboolean enabled;
	AltFireSwap_client_t clients[MAX_CLIENTS];
} *MOD_STATE;

#define DEDICATED_SERVER ( trap_Cvar_VariableIntegerValue( "dedicated" ) || !trap_Cvar_VariableIntegerValue( "cl_running" ) )

/*
================
ModAltSwapHandler_CheckEnabled
================
*/
static void ModAltSwapHandler_CheckEnabled( void ) {
	qboolean enabled = qtrue;

	// Disable swap support if flood protection is enabled because it can cause commands to be dropped
	if ( trap_Cvar_VariableIntegerValue( "sv_floodProtect" ) && DEDICATED_SERVER ) {
		enabled = qfalse;
	}

	if ( enabled != MOD_STATE->enabled ) {
		MOD_STATE->enabled = enabled;
		ModModcfgCS_Static_Update();
		if ( !enabled ) {
			memset( MOD_STATE->clients, 0, sizeof( MOD_STATE->clients ) );
		}
	}
}

/*
================
(ModFN) InitClientSession
================
*/
static void MOD_PREFIX(InitClientSession)( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ) {
	AltFireSwap_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( InitClientSession, ( MODFN_NC, clientNum, initialConnect, info ) );
	memset( modclient, 0, sizeof( *modclient ) );

	if ( !initialConnect && level.hasRestarted ) {
		// Restore flags when coming back from a map restart
		modclient->swapFlags = atoi ( Info_ValueForKey( info->s, "altSwapFlags" ) );
	}
}

/*
================
(ModFN) GenerateClientSessionInfo
================
*/
static void MOD_PREFIX(GenerateClientSessionInfo)( MODFN_CTV, int clientNum, info_string_t *info ) {
	AltFireSwap_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( GenerateClientSessionInfo, ( MODFN_NC, clientNum, info ) );

	if ( modclient->swapFlags ) {
		Info_SetValueForKey_Big( info->s, "altSwapFlags", va( "%i", modclient->swapFlags ) );
	}
}

/*
================
(ModFN) ModClientCommand

Handle setAltSwap command.
================
*/
static qboolean MOD_PREFIX(ModClientCommand)( MODFN_CTV, int clientNum, const char *cmd ) {
	if ( MOD_STATE->enabled && !Q_stricmp( cmd, "setAltSwap" ) ) {
		AltFireSwap_client_t *modclient = &MOD_STATE->clients[clientNum];
		char buffer[32];
		trap_Argv( 1, buffer, sizeof( buffer ) );
		modclient->swapFlags = atoi( buffer );
		return qtrue;
	}

	return MODFN_NEXT( ModClientCommand, ( MODFN_NC, clientNum, cmd ) );
}

/*
================
(ModFN) PmoveInit
================
*/
static void MOD_PREFIX(PmoveInit)( MODFN_CTV, int clientNum, pmove_t *pmove ) {
	playerState_t *ps = &level.clients[clientNum].ps;

	MODFN_NEXT( PmoveInit, ( MODFN_NC, clientNum, pmove ) );

	if ( MOD_STATE->enabled && ps->weapon >= 1 && ps->weapon < WP_NUM_WEAPONS ) {
		AltFireSwap_client_t *modclient = &MOD_STATE->clients[clientNum];
		if ( modclient->swapFlags & ( 1 << ( ps->weapon - 1 ) ) ) {
			pmove->altFireMode = ALTMODE_SWAPPED;
		}
	}
}

/*
==============
(ModFN) AddModConfigInfo
==============
*/
static void MOD_PREFIX(AddModConfigInfo)( MODFN_CTV, char *info ) {
	if ( MOD_STATE->enabled ) {
		Info_SetValueForKey( info, "altSwapSupport", "1" );
	}

	MODFN_NEXT( AddModConfigInfo, ( MODFN_NC, info ) );
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );
	ModAltSwapHandler_CheckEnabled();
}

/*
================
ModAltSwapHandler_Init
================
*/
void ModAltSwapHandler_Init( void ) {
	// Don't enable if server engine has its own alt swap handler
	if ( VMExt_GVCommandInt( "sv_support_setAltSwap", 0 ) ) {
		return;
	}

	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( InitClientSession, MODPRIORITY_GENERAL );
		MODFN_REGISTER( GenerateClientSessionInfo, MODPRIORITY_GENERAL );
		MODFN_REGISTER( ModClientCommand, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PmoveInit, MODPRIORITY_GENERAL );
		MODFN_REGISTER( AddModConfigInfo, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );

		ModModcfgCS_Init();
		ModAltSwapHandler_CheckEnabled();
	}
}
