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
LOGFUNCTION_SVOID( MOD_PREFIX(InitClientSession), ( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ),
		( MODFN_CTN, clientNum, initialConnect, info ), "G_MODFN_INITCLIENTSESSION" ) {
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
LOGFUNCTION_SVOID( MOD_PREFIX(GenerateClientSessionInfo), ( MODFN_CTV, int clientNum, info_string_t *info ),
		( MODFN_CTN, clientNum, info ), "G_MODFN_GENERATECLIENTSESSIONINFO" ) {
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
LOGFUNCTION_SRET( qboolean, MOD_PREFIX(ModClientCommand), ( MODFN_CTV, int clientNum, const char *cmd ),
		( MODFN_CTN, clientNum, cmd ), "G_MODFN_MODCLIENTCOMMAND" ) {
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
LOGFUNCTION_SVOID( MOD_PREFIX(PmoveInit), ( MODFN_CTV, int clientNum, pmove_t *pmove ),
		( MODFN_CTN, clientNum, pmove ), "G_MODFN_PMOVEINIT" ) {
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
LOGFUNCTION_SVOID( MOD_PREFIX(AddModConfigInfo), ( MODFN_CTV, char *info ), ( MODFN_CTN, info ), "G_MODFN_ADDMODCONFIGINFO" ) {
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
LOGFUNCTION_SVOID( MOD_PREFIX(PostRunFrame), ( MODFN_CTV ), ( MODFN_CTN ), "G_MODFN_POSTRUNFRAME" ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );
	ModAltSwapHandler_CheckEnabled();
}

/*
================
ModAltSwapHandler_Init
================
*/
LOGFUNCTION_VOID( ModAltSwapHandler_Init, ( void ), (), "G_MOD_INIT" ) {
	// Don't enable if server engine has its own alt swap handler
	if ( VMExt_GVCommandInt( "sv_support_setAltSwap", 0 ) ) {
		return;
	}

	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( InitClientSession );
		MODFN_REGISTER( GenerateClientSessionInfo );
		MODFN_REGISTER( ModClientCommand );
		MODFN_REGISTER( PmoveInit );
		MODFN_REGISTER( AddModConfigInfo );
		MODFN_REGISTER( PostRunFrame );

		ModModcfgCS_Init();
		ModAltSwapHandler_CheckEnabled();
	}
}
