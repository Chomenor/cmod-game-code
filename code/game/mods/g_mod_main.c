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
	int modsEnabled = G_ModUtils_GetLatchedValue( "g_modsEnabled", "2", 0 );

	// Initialize default mod functions
#define MOD_FUNCTION_DEF( name, returntype, parameters ) \
	modfn.name = ModFNDefault_##name;
#include "mods/g_mod_defs.h"
#undef MOD_FUNCTION_DEF

	if ( modsEnabled <= 0 ) {
		return;
	}

	// Initialize cvars
	trap_Cvar_Register( NULL, "g_gametype", "0", CVAR_SERVERINFO | CVAR_LATCH );
	trap_Cvar_Register( NULL, "g_pModAssimilation", "0", CVAR_SERVERINFO | CVAR_LATCH );
	trap_Cvar_Register( NULL, "g_pModDisintegration", "0", CVAR_SERVERINFO | CVAR_LATCH );
	trap_Cvar_Register( NULL, "g_pModActionHero", "0", CVAR_SERVERINFO | CVAR_LATCH );
	trap_Cvar_Register( NULL, "g_pModSpecialties", "0", CVAR_SERVERINFO | CVAR_LATCH );
	trap_Cvar_Register( NULL, "g_pModElimination", "0", CVAR_SERVERINFO | CVAR_LATCH );

	// Core mods
	ModTeamGroups_Init();
	ModVoting_Init();

	// Default mods
	if ( modsEnabled >= 2 ) {
		ModMiscFeatures_Init();
		ModPlayerMove_Init();
		ModAltSwapHandler_Init();
		ModPingcomp_Init();
		ModSpectPassThrough_Init();
		ModSpawnProtect_Init();
		ModBotAdding_Init();
		ModStatusScores_Init();
	}

	// Game modes
	if ( trap_Cvar_VariableIntegerValue( "g_gametype" ) == 1 ) {
		ModTournament_Init();
	} else if ( trap_Cvar_VariableIntegerValue( "g_pModElimination" ) ) {
		ModElimination_Init();
	} else if ( trap_Cvar_VariableIntegerValue( "g_pModAssimilation" ) ) {
		ModAssimilation_Init();
	} else if ( trap_Cvar_VariableIntegerValue( "g_pModActionHero" ) ) {
		ModActionHero_Init();
	} else if ( trap_Cvar_VariableIntegerValue( "g_pModDisintegration" ) ) {
		ModDisintegration_Init();
	} else if ( trap_Cvar_VariableIntegerValue( "g_pModSpecialties" ) ) {
		ModSpecialties_Init();
	}

	// Make sure info cvars are set accurately
	trap_Cvar_Set( "g_pModElimination", modcfg.mods_enabled.elimination ? "1" : "0" );
	trap_Cvar_Set( "g_pModAssimilation", modcfg.mods_enabled.assimilation ? "1" : "0" );
	trap_Cvar_Set( "g_pModActionHero", modcfg.mods_enabled.actionhero ? "1" : "0" );
	trap_Cvar_Set( "g_pModDisintegration", modcfg.mods_enabled.disintegration ? "1" : "0" );
	trap_Cvar_Set( "g_pModSpecialties", modcfg.mods_enabled.specialties ? "1" : "0" );

	modfn.PostModInit();
}
