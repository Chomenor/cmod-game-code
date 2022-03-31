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

LOGFUNCTION_VOID( ModFNDefault_AddModConfigInfo, ( char *info ),
		( info ), "G_MODFN_ADDMODCONFIGINFO" ) {
}
