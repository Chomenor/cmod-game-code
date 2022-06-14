/*
* Local Mod Header
* 
* Definitions shared between mods, but not with main game code.
*/

#include "g_local.h"

// Shared Variables

typedef struct {
	qboolean assimilation;
	qboolean specialties;
	qboolean elimination;
	qboolean actionhero;
} mods_enabled_t;

typedef struct {
	mods_enabled_t mods_enabled;
} mod_config_t;

extern mod_config_t modcfg;

// Utils

int G_ModUtils_GetLatchedValue( const char *cvar_name, const char *default_value, int flags );

/* ************************************************************************* */
// Modes - Modules loaded directly from G_ModsInit to change game mechanics
/* ************************************************************************* */

void ModActionHero_Init( void );
void ModAssimilation_Init( void );
void ModElimination_Init( void );
void ModSpecialties_Init( void );

/* ************************************************************************* */
// Features - Modules loaded directly from G_ModsInit to add features
/* ************************************************************************* */

void ModAltSwapHandler_Init( void );
void ModPingcomp_Init( void );
void ModPlayerMove_Init( void );
void ModSpawnProtect_Init( void );
void ModSpectPassThrough_Init( void );

//
// Ping Compensation (pc_main.c)
//

qboolean ModPingcomp_Static_PingCompensationEnabled( void );

/* ************************************************************************* */
// Components - Modules generally only loaded by other modules
/* ************************************************************************* */

void ModHoldableTransporter_Init( void );
void ModPendingItem_Init( void );

//
// Portable transporter & borg teleport (comp_holdable_transporter.c)
//

typedef struct {
	// Determines whether to use borg teleporter for given client.
	qboolean ( *borgTeleportEnabled )( int clientNum );

	// Callback when borg teleport completes.
	void ( *postBorgTeleport )( int clientNum );
} ModHoldableTransporter_config_t;

extern ModHoldableTransporter_config_t *ModHoldableTransporter_config;

//
// Pending item handling (g_mod_pending_item.c)
//

void ModPendingItem_Shared_SchedulePendingItem( int clientNum, holdable_t item, int delay );
