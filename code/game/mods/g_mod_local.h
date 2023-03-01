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
void ModMiscFeatures_Init( void );
void ModPingcomp_Init( void );
void ModPlayerMove_Init( void );
void ModSpawnProtect_Init( void );
void ModStatusScores_Init( void );
void ModTeamGroups_Init( void );
void ModSpectPassThrough_Init( void );
void ModVoting_Init( void );

//
// Ping Compensation (pingcomp/*.c)
//

qboolean ModPingcomp_Static_PingCompensationEnabled( void );
void ModPCSmoothing_Static_RecordClientMove( int clientNum );

//
// Team Groups (feat_team_groups.c)
//

void ModTeamGroups_Shared_ForceConfigStrings( const char *redGroup, const char *blueGroup );

/* ************************************************************************* */
// Components - Modules generally only loaded by other modules
/* ************************************************************************* */

void ModClickToJoin_Init( void );
void ModHoldableTransporter_Init( void );
void ModIntermissionReady_Init( void );
void ModModcfgCS_Init( void );
void ModModelGroups_Init( void );
void ModModelSelection_Init( void );
void ModPendingItem_Init( void );
void ModWarmupSequence_Init();

//
// Click-to-Join support (comp_clicktojoin.c)
//

qboolean ModClickToJoin_Static_ActiveForClient( int clientNum );

//
// Intermission ready handling (comp_intermissionready.c)
//

typedef struct {
	qboolean readySound;		// play beep sound when players become ready
	qboolean ignoreSpectators;	// treat spectators as non-players for ready calculations (similar to bots)
	qboolean noPlayersExit;		// exit immediately (after minExitTime) if no valid players available to signal ready
	int anyReadyTime;			// time to exit when at least one player is ready
	int sustainedAnyReadyTime;	// time to exit after continuous period of at least one player being ready (resets if nobody is ready)
	int minExitTime;			// don't exit before this time regardless of players ready
	int maxExitTime;			// always exit at this time regardless of players ready
} modIntermissionReady_config_t;

void ModIntermissionReady_Shared_UpdateConfig( void );
void ModIntermissionReady_Shared_Suspend( void );
void ModIntermissionReady_Shared_Resume( void );

//
// Modcfg Configstring Handling (comp_modcfg_cs.c)
//

void ModModcfgCS_Static_Update( void );

//
// Player model groups (comp_model_groups.c)
//

char *ModModelGroups_Shared_SearchGroupList(const char *name);
qboolean ModModelGroups_Shared_ListContainsRace(const char *race_list, const char *race);
void ModModelGroups_Shared_RandomModelForRace( const char *race, char *model, unsigned int size );

//
// Pending item handling (comp_pending_item.c)
//

void ModPendingItem_Shared_SchedulePendingItem( int clientNum, holdable_t item, int delay );

//
// Warmup sequences (comp_warmupsequence.c)
//

#define MAX_INFO_SEQUENCE_EVENTS 20

typedef struct {
	int time;
	void ( *operation )( const char *msg );
	const char *msg;
} modWarmupSequenceEvent_t;

typedef struct {
	int duration;
	int eventCount;
	modWarmupSequenceEvent_t events[MAX_INFO_SEQUENCE_EVENTS];
} modWarmupSequence_t;

void ModWarmupSequence_Static_AddEventToSequence( modWarmupSequence_t *sequence, int time,
		void ( *operation )( const char *msg ), const char *msg );
qboolean ModWarmupSequence_Static_SequenceInProgressOrPending( void );

/* ************************************************************************* */
// Local mod functions (modfn_lcl.*) - For mod functions only called by mods
/* ************************************************************************* */

// Ignore public defs
#define MOD_FUNCTION_DEF( name, returntype, parameters )

// Mod function types (ModFNType_*)
#define MOD_FUNCTION_LOCAL( name, returntype, parameters ) \
	typedef returntype ( *ModFNType_##name ) parameters;
#include "mods/g_mod_defs.h"
#undef MOD_FUNCTION_LOCAL

// Default mod function declarations (ModFNDefault_*)
#define MOD_FUNCTION_LOCAL( name, returntype, parameters ) \
	returntype ModFNDefault_##name parameters;
#include "mods/g_mod_defs.h"
#undef MOD_FUNCTION_LOCAL

// Mod functions structure
typedef struct {

#define MOD_FUNCTION_LOCAL( name, returntype, parameters ) \
	ModFNType_##name name;
#include "mods/g_mod_defs.h"
#undef MOD_FUNCTION_LOCAL

} mod_functions_local_t;

#undef MOD_FUNCTION_DEF

extern mod_functions_local_t modfn_lcl;

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

#define INIT_FN_STACKABLE_LCL( name ) \
	MOD_STATE->Prev_##name = modfn_lcl.name; \
	modfn_lcl.name = MOD_PREFIX(name);

#define INIT_FN_OVERRIDE_LCL( name ) \
	modfn_lcl.name = MOD_PREFIX(name);

#define INIT_FN_BASE_LCL( name ) \
	EF_WARN_ASSERT( modfn_lcl.name == ModFNDefault_##name ); \
	modfn_lcl.name = MOD_PREFIX(name);
#endif
