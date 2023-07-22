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

/*
================
G_ModUtils_GetMapName

Returns current map name.
================
*/
const char *G_ModUtils_GetMapName( void ) {
	static const char *name;
	if ( !name ) {
		char buffer[256];
		trap_Cvar_VariableStringBuffer( "mapname", buffer, sizeof( buffer ) );
		name = G_ModUtils_AllocateString( buffer );
	}
	return name;
}

/*
================
G_ModUtils_ReadGladiatorBitflags

Read boolean string in format from Gladiator mod.
Values of "1", "y", and "Y" are considered true.
================
*/
qboolean G_ModUtils_ReadGladiatorBoolean( const char *str ) {
	if ( str[0] == 'Y' || str[0] == 'y' || str[0] == '1' ) {
		return qtrue;
	}
	return qfalse;
}

/*
================
G_ModUtils_ReadGladiatorBitflags

Read a string of boolean values in format from Gladiator mod and convert it to bitmask.
Characters '1', 'y', and 'Y' are considered true.
================
*/
unsigned int G_ModUtils_ReadGladiatorBitflags( const char *str ) {
	int i;
	unsigned int result = 0;

	for ( i = 0; i < 32; ++i ) {
		char c = str[i];

		if ( !c ) {
			break;
		}

		if ( c == '1' || c == 'y' || c == 'Y' ) {
			result |= 1 << i;
		}
	}

	return result;
}
