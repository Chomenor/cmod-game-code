/*
* Mods Main
* 
* Handles mod state and initialization.
*/

#include "mods/g_mod_local.h"

mod_functions_t modfn;
mod_config_t modcfg;

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

	// Initialize cvars
	trap_Cvar_Register( NULL, "g_pModAssimilation", "0", CVAR_SERVERINFO );

	// Default mods
	if ( modsEnabled >= 2 ) {
		ModPlayerMove_Init();
		ModAltSwapHandler_Init();
		ModPingcomp_Init();
		ModSpectPassThrough_Init();
		ModSpawnProtect_Init();
	}

	// Game modes
	if ( trap_Cvar_VariableIntegerValue( "g_pModAssimilation" ) ) {
		ModAssimilation_Init();
	}
}
