/*
* Mod Utiltiy Functions
* 
* Misc functions shared between mods.
*/

#include "mods/g_mod_local.h"

float modePriorityLevel = MODPRIORITY_MODES;

/*
================
G_ModUtils_GetLatchedValue

Unlatches and retrieves value of latched cvar.
================
*/
int G_ModUtils_GetLatchedValue( const char *cvar_name, const char *default_value, int flags ) {
	trap_Cvar_Register( NULL, cvar_name, default_value, CVAR_LATCH | flags );
	return trap_Cvar_VariableIntegerValue( cvar_name );
}

/*
================
G_ModUtils_AllocateString

Creates permanently allocated copy of a string.
================
*/
char *G_ModUtils_AllocateString( const char *string ) {
	int length = strlen( string );
	char *result = G_Alloc( length + 1 );
	memcpy( result, string, length );
	result[length] = '\0';
	return result;
}
