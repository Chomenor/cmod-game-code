/*
* Mod Configstring Support
* 
* This module implements the "modcfg" protocol, which allows various data to be passed
* to the client in key/value format via specially formatted configstrings.
* 
* Mods can add their own info to the modcfg data by implementing the "AddModConfigInfo"
* mod function. They should also call "ModModcfgCS_Static_Update" to trigger an update
* whenever their info values might have changed.
* 
* Modules making use of AddModConfigInfo should make sure this module is initialized
* by calling ModModcfgCS_Init during their own mod initialization.
*/

#define MOD_PREFIX( x ) ModModcfgCS##x

#include "mods/g_mod_local.h"

static struct {
	// Only set mod config after initialization is complete to avoid unnecessary configstring updates.
	qboolean modConfigReady;

	// For mod function stacking
	ModFNType_GeneralInit Prev_GeneralInit;
} *MOD_STATE;

/*
================
(ModFN) AddModConfigInfo
================
*/
LOGFUNCTION_VOID( ModFNDefault_AddModConfigInfo, ( char *info ), ( info ), "G_MODFN_ADDMODCONFIGINFO" ) {
}

/*
================
ModModcfgCS_Static_Update

Updates mod config configstring. Called on game startup and any time the value might have changed.
================
*/
void ModModcfgCS_Static_Update( void ) {
	if ( MOD_STATE && MOD_STATE->modConfigReady ) {
		char buffer[BIG_INFO_STRING + 8];
		char *info = buffer + 8;
		Q_strncpyz( buffer, "!modcfg ", sizeof( buffer ) );

		modfn_lcl.AddModConfigInfo( info );

		if ( *info ) {
			trap_SetConfigstring( CS_MOD_CONFIG, buffer );
		} else {
			trap_SetConfigstring( CS_MOD_CONFIG, "" );
		}
	}
}

/*
================
(ModFN) GeneralInit
================
*/
LOGFUNCTION_SVOID( MOD_PREFIX(GeneralInit), ( void ), (), "G_MODFN_GENERALINIT" ) {
	MOD_STATE->Prev_GeneralInit();
	MOD_STATE->modConfigReady = qtrue;
	ModModcfgCS_Static_Update();
}

/*
================
ModModcfgCS_Init
================
*/
LOGFUNCTION_VOID( ModModcfgCS_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( GeneralInit );
	}
}
