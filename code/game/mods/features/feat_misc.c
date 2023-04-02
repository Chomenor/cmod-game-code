/*
* Misc Features
* 
* This module can be used to add minor tweaks or cvar-controlled options that
* don't fit anywhere else and are too minor to justify their own mod file.
*/

#define MOD_NAME ModMiscFeatures

#include "mods/g_mod_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
==============
(ModFN) AddModConfigInfo
==============
*/
LOGFUNCTION_SVOID( MOD_PREFIX(AddModConfigInfo), ( MODFN_CTV, char *info ),
		( MODFN_CTN, info ), "G_MODFN_ADDMODCONFIGINFO" ) {
	// Indicate to cMod client to use preferred default UI module if possible, to avoid
	// potential settings reset on disconnect or other engine incompatibilities.
	// This should be disabled if creating a mod that has a need for a customized UI.
	Info_SetValueForKey( info, "nativeUI", "1" );

	MODFN_NEXT( AddModConfigInfo, ( MODFN_NC, info ) );
}

/*
================
ModMiscFeatures_Init
================
*/
LOGFUNCTION_VOID( ModMiscFeatures_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( AddModConfigInfo );

		ModModcfgCS_Init();
	}
}
