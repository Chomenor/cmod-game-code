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
ModfnCountSamePriority

Returns number of calls attached to the mod function with the same priority value as another call.

Used to highlight cases where the order is ambiguous (determined alphabetically by the name of the
registering module). In some cases this is fine, but in other cases it may need additional scrutiny.
===================
*/
static int ModfnCountSamePriority( const modCallList_t *callList ) {
	int count = 0;
	int i, j;
	for ( i = 0; i < callList->count; ++i ) {
		for ( j = 0; j < callList->count; ++j ) {
			if ( i != j && callList->entries[i]->priority == callList->entries[j]->priority ) {
				++count;
				break;
			}
		}
	}
	return count;
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
			int indeterminate;
			G_GetModCallList( entry, &callList );
			indeterminate = ModfnCountSamePriority( &callList );
			G_Printf( "%s: %i calls%s\n", entry->name, callList.count,
					indeterminate ? va( " (%i same priority)", indeterminate ) : "" );
		}

		G_Printf( "\nTo show call list:\nmodfn_debug <mod function name>\n" );
	}
}

/*
===================
(ModFN) ModConsoleCommand
===================
*/
static qboolean MOD_PREFIX(ModConsoleCommand)( MODFN_CTV, const char *cmd ) {
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
void ModDebugModfn_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( ModConsoleCommand, MODPRIORITY_GENERAL );
	}
}
