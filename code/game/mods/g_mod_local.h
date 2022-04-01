/*
* Local Mod Header
* 
* Definitions shared between mods, but not with main game code.
*/

#include "g_local.h"

// Utils
int G_ModUtils_GetLatchedValue( const char *cvar_name, const char *default_value, int flags );

// Feature Initialization
void ModPlayerMove_Init( void );
