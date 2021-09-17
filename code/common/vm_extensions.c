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
