/*
* Mod Function Debugging
* 
* This module implements the "modfn_debug" command, which lists the active calls
* registered for a given mod function, in order from first to last called.
*/

#define MOD_NAME ModDebugModfn

#include "mods/g_mod_local.h"

static struct {
	int _unused;
} *MOD_STATE;

#define MODFN_COUNT ( sizeof( modFunctionsBackend ) / sizeof( modFunctionBackendEntry_t ) )

/*
===================
ModfnDebugPrintEntry
===================
*/
static void ModfnDebugPrintEntry( modFunctionBackendEntry_t *entry ) {
	int i;
	modCallList_t callList;

	G_GetModCallList( entry, &callList );
	G_Printf( "%i mod calls registered for '%s'.\n", callList.count, entry->name );

	for ( i = 0; i < callList.count; ++i ) {
		G_Printf( "Call %i: source(%s) priority(%f)\n", i + 1, callList.entries[i]->source, callList.entries[i]->priority );
	}
}

/*
===================
ModfnDebugCmd
===================
*/
static void ModfnDebugCmd( void ) {
	int i;

	if ( trap_Argc() >= 2 ) {
		char name[MAX_TOKEN_CHARS];
		trap_Argv( 1, name, sizeof( name ) );

		for ( i = 0; i < MODFN_COUNT; ++i ) {
			modFunctionBackendEntry_t *entry = (modFunctionBackendEntry_t *)&modFunctionsBackend + i;
			if ( !Q_stricmp( name, entry->name ) ) {
				ModfnDebugPrintEntry( entry );
				return;
			}
		}

		G_Printf( "Modfn with name '%s' not found.\n", name );
	}

	else {
		modCallList_t callList;
		for ( i = 0; i < MODFN_COUNT; ++i ) {
			modFunctionBackendEntry_t *entry = (modFunctionBackendEntry_t *)&modFunctionsBackend + i;
			G_GetModCallList( entry, &callList );
			G_Printf( "%s: %i calls\n", entry->name, callList.count );
		}

		G_Printf( "\nTo show call list:\nmodfn_debug <mod function name>\n" );
	}
}

/*
===================
(ModFN) ModConsoleCommand
===================
*/
LOGFUNCTION_SRET( qboolean, MOD_PREFIX(ModConsoleCommand), ( MODFN_CTV, const char *cmd ),
		( MODFN_CTN, cmd ), "G_MODFN_MODCONSOLECOMMAND" ) {
	if ( !Q_stricmp( cmd, "modfn_debug" ) ) {
		ModfnDebugCmd();
		return qtrue;
	}

	return MODFN_NEXT( ModConsoleCommand, ( MODFN_NC, cmd ) );
}

/*
================
ModDebugModfn_Init
================
*/
LOGFUNCTION_VOID( ModDebugModfn_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( ModConsoleCommand );
	}
}
