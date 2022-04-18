/*
* Mods Main
* 
* Handles mod state and initialization.
*/

#include "mods/g_mod_local.h"

mod_functions_t modfn;

/*
================
G_ModsInit
================
*/
LOGFUNCTION_VOID( G_ModsInit, ( void ), (), "G_MOD_INIT" ) {
	// 2 = enable all mods, 1 = only mods from original game, 0 = disable all mods
	int modsEnabled = G_ModUtils_GetLatchedValue( "g_mods_enabled", "2", 0 );

	// Initialize default mod functions
#define MOD_FUNCTION_DEF( name, returntype, parameters ) \
	modfn.name = ModFNDefault_##name;
#include "mods/g_mod_defs.h"
#undef MOD_FUNCTION_DEF

	if ( modsEnabled <= 0 ) {
		return;
	}

	if ( modsEnabled >= 2 ) {
		// Default mods
		ModPlayerMove_Init();
		ModAltSwapHandler_Init();
		ModPingcomp_Init();
	}
}
