/*
* Misc Features
* 
* This module can be used to add minor tweaks or cvar-controlled options that
* don't fit anywhere else and are too minor to justify their own mod file.
*/

#define MOD_PREFIX( x ) ModMiscFeatures_##x

#include "mods/g_mod_local.h"

static struct {
	// For mod function stacking
	ModFNType_AddModConfigInfo Prev_AddModConfigInfo;
} *MOD_STATE;

/*
==============
(ModFN) AddModConfigInfo
==============
*/
LOGFUNCTION_VOID( MOD_PREFIX(AddModConfigInfo), ( char *info ), ( info ), "G_MODFN_ADDMODCONFIGINFO" ) {
	// Indicate to cMod client to use preferred default UI module if possible, to avoid
	// potential settings reset on disconnect or other engine incompatibilities.
	// This should be disabled if creating a mod that has a need for a customized UI.
	Info_SetValueForKey( info, "nativeUI", "1" );

	MOD_STATE->Prev_AddModConfigInfo( info );
}

/*
================
ModMiscFeatures_Init
================
*/
LOGFUNCTION_VOID( ModMiscFeatures_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		INIT_FN_STACKABLE( AddModConfigInfo );
	}
}
