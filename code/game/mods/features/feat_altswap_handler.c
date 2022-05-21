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

#include "mods/g_mod_local.h"

#define PREFIX( x ) ModAltSwapHandler_##x
#define MOD_STATE PREFIX( state )

typedef struct {
	int swapFlags;
} AltFireSwap_client_t;

static struct {
	qboolean enabled;
	AltFireSwap_client_t clients[MAX_CLIENTS];

	// For mod function stacking
	ModFNType_InitClientSession Prev_InitClientSession;
	ModFNType_GenerateClientSessionInfo Prev_GenerateClientSessionInfo;
	ModFNType_ModClientCommand Prev_ModClientCommand;
	ModFNType_PmoveInit Prev_PmoveInit;
	ModFNType_AddModConfigInfo Prev_AddModConfigInfo;
	ModFNType_PostRunFrame Prev_PostRunFrame;
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
		G_UpdateModConfigInfo();
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
LOGFUNCTION_SVOID( PREFIX(InitClientSession), ( int clientNum, qboolean initialConnect, const info_string_t *info ),
		( clientNum, initialConnect, info ), "G_MODFN_INITCLIENTSESSION" ) {
	AltFireSwap_client_t *modclient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_InitClientSession( clientNum, initialConnect, info );
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
LOGFUNCTION_SVOID( PREFIX(GenerateClientSessionInfo), ( int clientNum, info_string_t *info ),
		( clientNum, info ), "G_MODFN_GENERATECLIENTSESSIONINFO" ) {
	AltFireSwap_client_t *modclient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_GenerateClientSessionInfo( clientNum, info );

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
LOGFUNCTION_SRET( qboolean, PREFIX(ModClientCommand), ( int clientNum, const char *cmd ),
		( clientNum, cmd ), "G_MODFN_MODCLIENTCOMMAND" ) {
	if ( MOD_STATE->enabled && !Q_stricmp( cmd, "setAltSwap" ) ) {
		AltFireSwap_client_t *modclient = &MOD_STATE->clients[clientNum];
		char buffer[32];
		trap_Argv( 1, buffer, sizeof( buffer ) );
		modclient->swapFlags = atoi( buffer );
		return qtrue;
	}

	return MOD_STATE->Prev_ModClientCommand( clientNum, cmd );
}

/*
================
(ModFN) PmoveInit
================
*/
LOGFUNCTION_SVOID( PREFIX(PmoveInit), ( int clientNum, pmove_t *pmove ),
		( clientNum, pmove ), "G_MODFN_PMOVEINIT" ) {
	playerState_t *ps = &level.clients[clientNum].ps;

	MOD_STATE->Prev_PmoveInit( clientNum, pmove );

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
LOGFUNCTION_SVOID( PREFIX(AddModConfigInfo), ( char *info ), ( info ), "G_MODFN_ADDMODCONFIGINFO" ) {
	if ( MOD_STATE->enabled ) {
		Info_SetValueForKey( info, "altSwapSupport", "1" );
	}

	MOD_STATE->Prev_AddModConfigInfo( info );
}

/*
================
(ModFN) PostRunFrame
================
*/
LOGFUNCTION_SVOID( PREFIX(PostRunFrame), ( void ), (), "G_MODFN_POSTRUNFRAME" ) {
	MOD_STATE->Prev_PostRunFrame();
	ModAltSwapHandler_CheckEnabled();
}

/*
================
ModAltSwapHandler_Init
================
*/

#define INIT_FN_STACKABLE( name ) \
	MOD_STATE->Prev_##name = modfn.name; \
	modfn.name = PREFIX(name);

#define INIT_FN_OVERRIDE( name ) \
	modfn.name = PREFIX(name);

LOGFUNCTION_VOID( ModAltSwapHandler_Init, ( void ), (), "G_MOD_INIT" ) {
	// Don't enable if server engine has its own alt swap handler
	if ( VMExt_GVCommandInt( "sv_support_setAltSwap", 0 ) ) {
		return;
	}

	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( InitClientSession );
		INIT_FN_STACKABLE( GenerateClientSessionInfo );
		INIT_FN_STACKABLE( ModClientCommand );
		INIT_FN_STACKABLE( PmoveInit );
		INIT_FN_STACKABLE( AddModConfigInfo );
		INIT_FN_STACKABLE( PostRunFrame );

		ModAltSwapHandler_CheckEnabled();
	}
}
