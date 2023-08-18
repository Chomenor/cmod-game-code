/*
* Mods Main
* 
* Handles mod state and initialization.
*/

#include "mods/g_mod_local.h"

mod_functions_t modfn;
mod_functions_local_t modfn_lcl;
mod_config_t modcfg;

modFunctionsBackend_t modFunctionsBackend;

/*
================
G_ModCallSort
================
*/
static int G_ModCallSort( const void *e1, const void *e2 ) {
	const modFunctionBackendCall_t *t1 = *( (const modFunctionBackendCall_t **)e1 );
	const modFunctionBackendCall_t *t2 = *( (const modFunctionBackendCall_t **)e2 );
	if ( t1->priority > t2->priority ) {
		return -1;
	}
	if ( t2->priority > t1->priority ) {
		return 1;
	}
	return Q_stricmp( t1->source, t2->source );
}

/*
================
G_GetModCallList

Generate a sorted list of calls for the mod function.
================
*/
void G_GetModCallList( modFunctionBackendEntry_t *backendEntry, modCallList_t *out ) {
	modFunctionBackendCall_t *curEntry = backendEntry->entries;
	out->count = 0;

	while ( curEntry ) {
		if ( out->count >= MAX_MOD_CALLS - 1 ) {
			trap_Error( "G_UpdateModCalls: MAX_MOD_CALLS overflow" );
		}
		out->entries[out->count++] = curEntry;
		curEntry = curEntry->next;
	}

	qsort( out->entries, out->count, sizeof( *out->entries ), G_ModCallSort );
}

/*
================
G_UpdateModCalls

Update call stack for mod function. Should be called during initialization and whenever
calls are added.
================
*/
static void G_UpdateModCalls( modFunctionBackendEntry_t *backendEntry ) {
	int i;
	modCallList_t callList;

	G_GetModCallList( backendEntry, &callList );

	for ( i = 0; i < callList.count; ++i ) {
		backendEntry->callStack[i] = callList.entries[i]->call;
	}
	backendEntry->callStack[callList.count] = backendEntry->defaultCall;
}

/*
================
G_RegisterModFunctionCall

Add a new call to a mod function's call list.
================
*/
void G_RegisterModFunctionCall( modFunctionBackendEntry_t *backendEntry, modCall_t call, const char *source, float priority ) {
	// verify no duplicate name
	modFunctionBackendCall_t *curEntry = backendEntry->entries;
	while ( curEntry ) {
		if ( !Q_stricmp( source, curEntry->source ) ) {
			trap_Error( va( "G_RegisterModFunctionCall: Duplicate source '%s'", source ) );
		}
		curEntry = curEntry->next;
	}

	// generate new entry
	curEntry = G_Alloc( sizeof( *curEntry ) );
	curEntry->call = call;
	curEntry->source = source;
	curEntry->priority = priority;
	curEntry->next = backendEntry->entries;
	backendEntry->entries = curEntry;

	// update call stack
	G_UpdateModCalls( backendEntry );
}

// Define exec functions as a wrapper to add context parameters to initial modfn call
#define PREFIX2 MODFN_CTN,
#define VOID1 void
#define VOID2 MODFN_CTN
#define MOD_FUNCTION_DEF( name, returntype, parameters, parameters_untyped, returnkw ) \
	static returntype ModFNExec_##name parameters { \
		modCall_t *next = &modFunctionsBackend.name.callStack[1]; \
		returnkw ( ( ModFNType_##name )*modFunctionsBackend.name.callStack) parameters_untyped; }
#define MOD_FUNCTION_LOCAL( name, returntype, parameters, parameters_untyped, returnkw ) \
	static returntype ModFNExec_##name parameters { \
		modCall_t *next = &modFunctionsBackend.name.callStack[1]; \
		returnkw ( ( ModFNType_##name )*modFunctionsBackend.name.callStack) parameters_untyped; }
#include "mods/g_mod_defs.h"

// Define final functions as a wrapper to remove context parameters when calling the ModFNDefault_ function
#define PREFIX1 MODFN_CTV,
#define VOID1 MODFN_CTV
#define MOD_FUNCTION_DEF( name, returntype, parameters, parameters_untyped, returnkw ) \
	static returntype ModFNFinal_##name parameters { \
		returnkw ModFNDefault_##name parameters_untyped; }
#define MOD_FUNCTION_LOCAL( name, returntype, parameters, parameters_untyped, returnkw ) \
	static returntype ModFNFinal_##name parameters { \
		returnkw ModFNDefault_##name parameters_untyped; }
#include "mods/g_mod_defs.h"

/*
================
G_ModsInit
================
*/
LOGFUNCTION_VOID( G_ModsInit, ( void ), (), "G_MOD_INIT" ) {
	// 2 = enable all mods, 1 = only mods from original game, 0 = disable all mods
	int modsEnabled = G_ModUtils_GetLatchedValue( "g_modsEnabled", "2", 0 );

	// Initialize mod function backend
#define MOD_FUNCTION_DEF( modfnname, returntype, parameters, parameters_untyped, returnkw ) \
	modFunctionsBackend.modfnname.name = #modfnname; \
	modFunctionsBackend.modfnname.defaultCall = (modCall_t)ModFNFinal_##modfnname; \
	G_UpdateModCalls( &modFunctionsBackend.modfnname );
#define MOD_FUNCTION_LOCAL( modfnname, returntype, parameters, parameters_untyped, returnkw ) \
	modFunctionsBackend.modfnname.name = #modfnname; \
	modFunctionsBackend.modfnname.defaultCall = (modCall_t)ModFNFinal_##modfnname; \
	G_UpdateModCalls( &modFunctionsBackend.modfnname );
#include "mods/g_mod_defs.h"

	// Initialize modn.* and modfn_lcl.* call points
#define MOD_FUNCTION_DEF( modfnname, returntype, parameters, parameters_untyped, returnkw ) \
	modfn.modfnname = ModFNExec_##modfnname;
#define MOD_FUNCTION_LOCAL( modfnname, returntype, parameters, parameters_untyped, returnkw ) \
	modfn_lcl.modfnname = ModFNExec_##modfnname;
#include "mods/g_mod_defs.h"

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
	trap_Cvar_Register( NULL, "g_mod_uam", "0", CVAR_SERVERINFO | CVAR_LATCH );
	trap_Cvar_Register( NULL, "g_mod_instagib", "0", CVAR_SERVERINFO | CVAR_LATCH );
	trap_Cvar_Register( NULL, "g_mod_razor", "0", CVAR_SERVERINFO | CVAR_LATCH );
	trap_Cvar_Register( NULL, "g_mod_clanArena", "0", CVAR_SERVERINFO | CVAR_LATCH );

	// Core mods
	ModTeamGroups_Init();
	ModVoting_Init();
	ModDebugModfn_Init();

	// Default mods
	if ( modsEnabled >= 2 ) {
		ModMiscFeatures_Init();
		ModPlayerMove_Init();
		ModAltSwapHandler_Init();
		ModPingcomp_Init();
		ModSpectPassThrough_Init();
		ModSpawnProtect_Init();
		ModGameInfo_Init();
		ModStatusScores_Init();
		ModBotAdding_Init();
		ModGladiatorItemEnable_Init();
	}

	// Game modes
	if ( trap_Cvar_VariableIntegerValue( "g_gametype" ) == 1 ) {
		ModTournament_Init();
	} else if ( modsEnabled >= 2 && trap_Cvar_VariableIntegerValue( "g_mod_uam" ) ) {
		ModUAM_Init();
	} else if ( modsEnabled >= 2 && trap_Cvar_VariableIntegerValue( "g_mod_razor" ) ) {
		ModRazor_Init();
	} else if ( modsEnabled >= 2 && trap_Cvar_VariableIntegerValue( "g_mod_clanArena" ) ) {
		ModClanArena_Init();
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

	if ( modsEnabled >= 2 && trap_Cvar_VariableIntegerValue( "g_gametype" ) == GT_CTF ) {
		ModFlagUndercap_Init();
	}

	if ( modsEnabled >= 2 && !modcfg.mods_enabled.elimination ) {
		ModDelayRespawn_Init();
	}

	// Make sure info cvars are set accurately
	// Note: UAM handles these separately
	if ( !modcfg.mods_enabled.uam ) {
		trap_Cvar_Set( "g_pModElimination", modcfg.mods_enabled.elimination && !modcfg.mods_enabled.clanarena ? "1" : "0" );
		trap_Cvar_Set( "g_pModAssimilation", modcfg.mods_enabled.assimilation ? "1" : "0" );
		trap_Cvar_Set( "g_pModActionHero", modcfg.mods_enabled.actionhero ? "1" : "0" );
		trap_Cvar_Set( "g_pModDisintegration", modcfg.mods_enabled.disintegration ? "1" : "0" );
		trap_Cvar_Set( "g_pModSpecialties", modcfg.mods_enabled.specialties ? "1" : "0" );
		trap_Cvar_Set( "g_mod_instagib", "" );
		trap_Cvar_Set( "g_mod_uam", "" );
	}

	if ( !modcfg.mods_enabled.razor ) {
		trap_Cvar_Set( "difficulty", "" );
	}

	trap_Cvar_Set( "g_mod_razor", modcfg.mods_enabled.razor ? "1" : "" );
	trap_Cvar_Set( "g_mod_clanArena", modcfg.mods_enabled.clanarena ? "1" : "" );

	modfn_lcl.PostModInit();
}
