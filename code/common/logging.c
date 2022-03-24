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
Logging_AssertionWarning

Current generate only one warning per session, to avoid potential console flooding if
warning is triggered repeatedly.
=================
*/
static void Logging_AssertionWarning( const char *expression ) {
	static qboolean warned = qfalse;
	if ( !warned ) {
		char buffer[1024];
		Com_sprintf( buffer, sizeof( buffer ), "Assertion warning: %s\n", expression );
		TRAP_PRINT( buffer );
		warned = qtrue;
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
