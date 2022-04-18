/*
* Local Mod Header
* 
* Definitions shared between mods, but not with main game code.
*/

#include "g_local.h"

// Utils
int G_ModUtils_GetLatchedValue( const char *cvar_name, const char *default_value, int flags );

// Note: By convention, functions labeled 'static' can be called even if the mod isn't loaded.

/* ************************************************************************* */
// Features - Modules loaded directly from G_ModsInit to add features
/* ************************************************************************* */

void ModAltSwapHandler_Init( void );
void ModPingcomp_Init( void );
void ModPlayerMove_Init( void );

//
// Ping Compensation (pc_main.c)
//

qboolean ModPingcomp_Static_PingCompensationEnabled( void );
