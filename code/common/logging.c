#ifdef MODULE_GAME
#include "g_local.h"
#endif
#ifdef MODULE_CGAME
#include "cg_local.h"
#endif
#ifdef MODULE_UI
#include "ui_local.h"
#endif

#ifdef MODULE_GAME
#define TRAP_PRINT trap_Printf
#else
#define TRAP_PRINT trap_Print
#endif

/*
=================
Logging_LogMsg

Prints message to loggers matching conditions.
Message does not need trailing newline (it will be automatically added).
=================
*/
void QDECL Logging_LogMsg( const char *conditions, const char *fmt, ... ) {
	va_list argptr;
	char msg[16384];

	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

	VMExt_FN_LoggingPrintExt( LP_INFO, conditions, msg );
}

/*
=================
Logging_Printf

Prints message to console and loggers matching conditions.
Currently message string should contain trailing newline.
=================
*/
void QDECL Logging_Printf( const char *conditions, const char *fmt, ... ) {
	va_list argptr;
	char msg[16384];

	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

	if ( !VMExt_FN_LoggingPrintExt( LP_CONSOLE, conditions, msg ) ) {
		// No engine support - fall back to regular print
		TRAP_PRINT( msg );
	}
}

/*
=================
Logging_AssertionWarning
=================
*/
void Logging_AssertionWarning( const char *expression ) {
	static qboolean warned = qfalse;
	if ( !warned ) {
		Logging_Printf( "WARNINGS ASSERTION", "Assertion failed (warning): %s\n", expression );
		TRAP_PRINT( "NOTE: Further assertions this session will not be printed to console.\n" );
		warned = qtrue;
	} else {
		Logging_LogMsg( "WARNINGS ASSERTION", "Assertion failed (warning): %s\n", expression );
	}
}

/*
=================
Logging_AssertionError
=================
*/
static void Logging_AssertionError( const char *expression ) {
	trap_Error( va( "Assertion failed: %s", expression ) );
}

/*
=================
Logging_Assertion
=================
*/
int Logging_Assertion( intptr_t result, const char *expression, qboolean error ) {
	if ( !result ) {
		if ( error ) {
			Logging_AssertionError( expression );
		} else {
			Logging_AssertionWarning( expression );
		}
	}
	return result;
}
