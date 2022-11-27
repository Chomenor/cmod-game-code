/*
* Local Mod Header
* 
* Definitions shared between mods, but not with main game code.
*/

#include "g_local.h"

// Shared Variables

typedef struct {
	qboolean tournament;
	qboolean assimilation;
	qboolean specialties;
	qboolean elimination;
	qboolean disintegration;
	qboolean actionhero;
} mods_enabled_t;

typedef struct {
	mods_enabled_t mods_enabled;
} mod_config_t;

extern mod_config_t modcfg;

// Utils

int G_ModUtils_GetLatchedValue( const char *cvar_name, const char *default_value, int flags );
char *G_ModUtils_AllocateString( const char *string );

/* ************************************************************************* */
// Modes - Modules loaded directly from G_ModsInit to change game mechanics
/* ************************************************************************* */

void ModActionHero_Init( void );
void ModAssimilation_Init( void );
void ModDisintegration_Init( void );
void ModElimination_Init( void );
void ModSpecialties_Init( void );
void ModTournament_Init( void );

/* ************************************************************************* */
// Features - Modules loaded directly from G_ModsInit to add features
/* ************************************************************************* */

void ModAltSwapHandler_Init( void );
void ModBotAdding_Init( void );
void ModPingcomp_Init( void );
void ModPlayerMove_Init( void );
void ModSpawnProtect_Init( void );
void ModTeamGroups_Init( void );
void ModSpectPassThrough_Init( void );
void ModVoting_Init( void );

//
// Ping Compensation (pc_main.c)
//

qboolean ModPingcomp_Static_PingCompensationEnabled( void );

//
// Team Groups (feat_team_groups.c)
//

void ModTeamGroups_Shared_ForceConfigStrings( const char *redGroup, const char *blueGroup );

/* ************************************************************************* */
// Components - Modules generally only loaded by other modules
/* ************************************************************************* */

void ModHoldableTransporter_Init( void );
void ModModelGroups_Init( void );
void ModModelSelection_Init( void );
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
// Player model groups (comp_model_groups.c)
//

char *ModModelGroups_Shared_SearchGroupList(const char *name);
qboolean ModModelGroups_Shared_ListContainsRace(const char *race_list, const char *race);
void ModModelGroups_Shared_RandomModelForRace( const char *race, char *model, unsigned int size );

//
// Player model selection (comp_model_selection.c)
//

// Performs processing/conversion of player model to fit mod requirements. Writes empty string to use random model instead.
typedef void ( *PlayerModels_ConvertPlayerModel_t )( int clientNum, const char *userinfo, const char *source_model,
		char *output, unsigned int outputSize );

// Generates a random model which meets mod requirements. Called when convert function returns empty string.
typedef void ( *PlayerModels_RandomPlayerModel_t )( int clientNum, const char *userinfo, char *output, unsigned int outputSize );

typedef struct {
	PlayerModels_ConvertPlayerModel_t ConvertPlayerModel;
	PlayerModels_RandomPlayerModel_t RandomPlayerModel;
} ModModelSelection_shared_t;

extern ModModelSelection_shared_t *ModModelSelection_shared;

//
// Pending item handling (g_mod_pending_item.c)
//

void ModPendingItem_Shared_SchedulePendingItem( int clientNum, holdable_t item, int delay );

/* ************************************************************************* */
// Helper Macros
/* ************************************************************************* */

#ifdef MOD_PREFIX
#define MOD_STATE MOD_PREFIX( state )

#define INIT_FN_STACKABLE( name ) \
	MOD_STATE->Prev_##name = modfn.name; \
	modfn.name = MOD_PREFIX(name);

#define INIT_FN_OVERRIDE( name ) \
	modfn.name = MOD_PREFIX(name);

#define INIT_FN_BASE( name ) \
	EF_WARN_ASSERT( modfn.name == ModFNDefault_##name ); \
	modfn.name = MOD_PREFIX(name);
#endif
