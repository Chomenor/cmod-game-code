/*
* Mod Function Stubs
* 
* Default mod functions with minimal functionality can be defined here,
* to reduce clutter in the rest of the game code.
*/

#include "g_local.h"

LOGFUNCTION_VOID( ModFNDefault_GeneralInit, ( void ),
		(), "G_MODFN_GENERALINIT" ) {
}

LOGFUNCTION_VOID( ModFNDefault_PostRunFrame, ( void ),
		(), "G_MODFN_POSTRUNFRAME" ) {
}

LOGFUNCTION_RET( int, ModFNDefault_AdjustPmoveConstant, ( pmoveConstant_t pmcType, int defaultValue ),
		( pmcType, defaultValue ), "G_MODFN_ADJUSTPMOVECONSTANT" ) {
	return defaultValue;
}

LOGFUNCTION_RET( qboolean, ModFNDefault_ModClientCommand, ( int clientNum, const char *cmd ),
		( clientNum, cmd ), "G_MODFN_MODCLIENTCOMMAND" ) {
	return qfalse;
}

LOGFUNCTION_VOID( ModFNDefault_AddModConfigInfo, ( char *info ),
		( info ), "G_MODFN_ADDMODCONFIGINFO" ) {
}
