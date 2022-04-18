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

int ModFNDefault_AdjustWeaponConstant( weaponConstant_t wcType, int defaultValue ) {
	return defaultValue;
}

LOGFUNCTION_VOID( ModFNDefault_PostFireProjectile, ( gentity_t *projectile ), ( projectile ), "G_MODFN_POSTFIREPROJECTILE" ) {
}

LOGFUNCTION_RET( int, ModFNDefault_AdjustPmoveConstant, ( pmoveConstant_t pmcType, int defaultValue ),
		( pmcType, defaultValue ), "G_MODFN_ADJUSTPMOVECONSTANT" ) {
	return defaultValue;
}

int ModFNDefault_AdjustGeneralConstant( generalConstant_t gcType, int defaultValue ) {
	return defaultValue;
}

LOGFUNCTION_RET( qboolean, ModFNDefault_ModConsoleCommand, ( const char *cmd ), ( cmd ), "G_MODFN_MODCONSOLECOMMAND" ) {
	return qfalse;
}

LOGFUNCTION_RET( qboolean, ModFNDefault_ModClientCommand, ( int clientNum, const char *cmd ),
		( clientNum, cmd ), "G_MODFN_MODCLIENTCOMMAND" ) {
	return qfalse;
}

LOGFUNCTION_VOID( ModFNDefault_AddModConfigInfo, ( char *info ),
		( info ), "G_MODFN_ADDMODCONFIGINFO" ) {
}

LOGFUNCTION_VOID( ModFNDefault_TrapTrace, ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int passEntityNum, int contentmask, int modFlags ),
		( results, start, mins, maxs, end, passEntityNum, contentmask, modFlags ), "G_MODFN_TRAPTRACE" ) {
	trap_Trace( results, start, mins, maxs, end, passEntityNum, contentmask );
}
