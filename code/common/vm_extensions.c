#ifdef MODULE_GAME
#include "g_local.h"
#endif
#ifdef MODULE_CGAME
#include "cg_local.h"
#endif
#ifdef MODULE_UI
#include "ui_local.h"
#endif

// This system is used to access engine-specific features from game modules,
// based on a protocol from Quake3e.

// Use this hacky syscall define since VM doesn't support __VA_ARGS__
#ifdef Q3_VM
#define SYSCALL( callnum, arg_types, arg_values, callnum_plus_arg_values ) \
	( ( ( intptr_t (*) arg_types )( ~callnum ) ) arg_values )
#else
extern intptr_t ( QDECL *syscall )( intptr_t arg, ... );	// *_syscalls.c
#define SYSCALL( callnum, arg_types, arg_values, callnum_plus_arg_values ) \
	( syscall callnum_plus_arg_values )
#endif

/*
================
VMExt_GVCommand

Performs GetValue query to engine.
Returns qtrue on success, qfalse on error such as lack of engine support for the command.
Result can be assumed to be null terminated.
================
*/
qboolean VMExt_GVCommand( char *buffer, unsigned int bufferSize, const char *command ) {
	static qboolean getvalue_callnum_retrieved = qfalse;
	static int getvalue_callnum = 0;

	if ( !bufferSize ) {
		trap_Error( "VMExt_GVCommand with zero bufferSize" );
	}

	// Attempt to retrieve the trap number of the GetValue function.
	if ( !getvalue_callnum_retrieved ) {
		char index_buffer[256];
		trap_Cvar_VariableStringBuffer( "//trap_GetValue", index_buffer, sizeof( index_buffer ) );
		getvalue_callnum = atoi( index_buffer );
		getvalue_callnum_retrieved = qtrue;
	}

	// Call the function if available.
	if ( getvalue_callnum ) {
		if ( SYSCALL( getvalue_callnum,
			( char *buffer, int bufferSize, const char *command ),
			( buffer, bufferSize, command ),
			( getvalue_callnum, buffer, bufferSize, command ) ) )
		{
			buffer[bufferSize - 1] = '\0';
			return qtrue;
		}
	}

	buffer[0] = '\0';
	return qfalse;
}

/*
================
VMExt_GVCommandInt

Performs GetValue query to engine and converts the response to an integer.
Returns value on success, or defaultValue on error such as lack of engine support.
================
*/
int VMExt_GVCommandInt( const char *command, int defaultValue ) {
	char buffer[256];

	if ( VMExt_GVCommand( buffer, sizeof(buffer), command ) ) {
		return atoi( buffer );
	}

	return defaultValue;
}

typedef struct {
	qboolean queried;
	int value;		// null if invalid
} vmext_cached_value_t;

/*
================
VMExt_UpdateCachedValue

Updates cached value if not already queried.
================
*/
static void VMExt_UpdateCachedValue( const char *key, vmext_cached_value_t *output ) {
	if ( !output->queried ) {
		output->value = VMExt_GVCommandInt( key, 0 );
		output->queried = qtrue;
	}
}

#ifdef MODULE_UI
static vmext_cached_value_t trap_lan_serverstatus;

/*
================
VMExt_FNAvailable_LAN_ServerStatus_Ext

Returns qtrue if function is available.
================
*/
qboolean VMExt_FNAvailable_LAN_ServerStatus_Ext( void ) {
	VMExt_UpdateCachedValue( "trap_lan_serverstatus_ext", &trap_lan_serverstatus );
	return trap_lan_serverstatus.value ? qtrue : qfalse;
}

/*
================
VMExt_FN_LAN_ServerStatus_Ext
================
*/
int VMExt_FN_LAN_ServerStatus_Ext( const char *serverAddress, char *serverStatus, int maxLen, char *extString, int extLen ) {
	VMExt_UpdateCachedValue( "trap_lan_serverstatus_ext", &trap_lan_serverstatus );
	if ( EF_WARN_ASSERT( trap_lan_serverstatus.value ) ) {
		return SYSCALL( trap_lan_serverstatus.value,
			( const char *serverAddress, char *serverStatus, int maxLen, char *extString, int extLen ),
			( serverAddress, serverStatus, maxLen, extString, extLen ),
			( trap_lan_serverstatus.value, serverAddress, serverStatus, maxLen, extString, extLen )
		);
	}
	return 0;
}
#endif

#ifdef MODULE_CGAME
static vmext_cached_value_t trap_altswap_set_state;

/*
================
VMExt_FNAvailable_AltSwap_SetState

Returns qtrue if function is available.
================
*/
qboolean VMExt_FNAvailable_AltSwap_SetState( void ) {
	VMExt_UpdateCachedValue( "trap_altswap_set_state", &trap_altswap_set_state );
	return trap_altswap_set_state.value ? qtrue : qfalse;
}

/*
================
VMExt_FN_AltSwap_SetState

Sets whether the engine should swap the alt fire and primary fire buttons.
================
*/
void VMExt_FN_AltSwap_SetState( qboolean swapState ) {
	VMExt_UpdateCachedValue( "trap_altswap_set_state", &trap_altswap_set_state );
	if ( EF_WARN_ASSERT( trap_altswap_set_state.value ) ) {
		SYSCALL( trap_altswap_set_state.value,
			( qboolean swapState ),
			( swapState ),
			( trap_altswap_set_state.value, swapState )
		);
	}
}
#endif

#ifdef MODULE_GAME
static vmext_cached_value_t trap_status_scores_override_set_array;

/*
================
VMExt_FNAvailable_StatusScoresOverride_SetArray

Returns qtrue if function is available.
================
*/
qboolean VMExt_FNAvailable_StatusScoresOverride_SetArray( void ) {
	VMExt_UpdateCachedValue( "trap_status_scores_override_set_array", &trap_status_scores_override_set_array );
	return trap_status_scores_override_set_array.value ? qtrue : qfalse;
}

/*
================
VMExt_FN_StatusScoresOverride_SetArray

Sets buffer of score values for the engine to use for status queries instead of playerstate PERS_SCORE.
================
*/
void VMExt_FN_StatusScoresOverride_SetArray( int *sharedArray, int clientCount ) {
	VMExt_UpdateCachedValue( "trap_status_scores_override_set_array", &trap_status_scores_override_set_array );
	if ( EF_WARN_ASSERT( trap_status_scores_override_set_array.value ) ) {
		SYSCALL( trap_status_scores_override_set_array.value,
			( int *sharedArray, int clientCount ),
			( sharedArray, clientCount ),
			( trap_status_scores_override_set_array.value, sharedArray, clientCount )
		);
	}
}
#endif
