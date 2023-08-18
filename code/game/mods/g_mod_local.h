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
	qboolean uam;
	qboolean razor;
	qboolean clanarena;
} mods_enabled_t;

typedef struct {
	mods_enabled_t mods_enabled;
} mod_config_t;

extern mod_config_t modcfg;

// Utils

int G_ModUtils_GetLatchedValue( const char *cvar_name, const char *default_value, int flags );
char *G_ModUtils_AllocateString( const char *string );
const char *G_ModUtils_GetMapName( void );
qboolean G_ModUtils_ReadGladiatorBoolean( const char *str );
unsigned int G_ModUtils_ReadGladiatorBitflags( const char *str );

// Mod Function Priority

#define MODPRIORITY_LOW 0	// should be called last
#define MODPRIORITY_GENERAL 1000	// for regular components or features
#define MODPRIORITY_MODES 2000		// for game type modifications
#define MODPRIORITY_MODIFICATIONS 3000		// for extra customizations
#define MODPRIORITY_HIGH 4000	// should be called first

// Priority counter to support last-registered-first-called model for modes
extern float modePriorityLevel;

// Constants for AdjustModConstant

typedef enum {
	MC_NONE,

	// feat_bot_adding.c / ModBotAdding_Init
	MC_BOTADD_PER_TEAM_COUNT,		// bot_minplayers represents number of bots on each team, not total
	MC_BOTADD_PER_TEAM_ADJUST,		// only add/remove bots from one team per cycle
	MC_BOTADD_IGNORE_SPECTATORS,	// spectators don't count as players for FFA bot count calculations

	// ping compensation
	MC_PINGCOMP_NO_TH_DEAD_MOVE,	// don't perform split body movement after trigger hurt death

	// comp_detpack.c / ModDetpack_Init
	MC_DETPACK_ORIGIN_PATCH,		// use detpack origin for blast, rather than origin of player who placed it
	MC_DETPACK_GLADIATOR_ANNOUNCEMENTS,		// gladiator-style audio announcements on detpack place/destroy
	MC_DETPACK_PING_SOUND,			// gladiator-style ping sound effect on detpack
	MC_DETPACK_DROP_ON_DEATH,		// drop placed detpack at placement location when player dies, instead of destroying
	MC_DETPACK_INVULNERABLE,		// detpack can't be destroyed by weapons fire

	// comp_seeker.c / ModSeeker_Init
	MC_SEEKER_SECONDS_PER_SHOT,		// 0 = disable firing altogether
	MC_SEEKER_MOD_TYPE,				// e.g. MOD_QUANTUM_ALT
	MC_SEEKER_ACCELERATOR_MODE,		// use Pinball-style projectile acceleration

	// comp_quad_effects.c / ModQuadEffects_Init
	MC_QUAD_EFFECTS_ENABLED,
	MC_QUAD_EFFECTS_PINBALL_STYLE,

	// comp_ghost_sparkle.c / ModGhostSparkle_Init
	MC_GHOST_SPARKLE_ENABLED,
} modConstant_t;

/* ************************************************************************* */
// Modes - Modules loaded directly from G_ModsInit to change game mechanics
/* ************************************************************************* */

void ModActionHero_Init( void );
void ModAssimilation_Init( void );
void ModClanArena_Init( void );
void ModDisintegration_Init( void );
void ModElimination_Init( void );
void ModRazor_Init( void );
void ModSpecialties_Init( void );
void ModTournament_Init( void );
void ModUAM_Init( void );

//
// Elimination
//

// elim_main.c
int ModElimination_Static_CountPlayersAlive( void );

// elim_multiround.c
qboolean ModElimMultiRound_Static_GetMultiRoundEnabled( void );
int ModElimMultiRound_Static_GetTotalRounds( void );
int ModElimMultiRound_Static_GetCurrentRound( void );
qboolean ModElimMultiRound_Static_GetIsTiebreakerRound( void );
qboolean ModElimMultiRound_Static_GetIsFinalScores( void );

//
// UAM
//

// uam_music.c
void UAMMusic_Static_PlayIntermissionMusic( void );

/* ************************************************************************* */
// Features - Modules loaded directly from G_ModsInit to add features
/* ************************************************************************* */

void ModAltSwapHandler_Init( void );
void ModBotAdding_Init( void );
void ModDebugModfn_Init( void );
void ModDelayRespawn_Init( void );
void ModFlagUndercap_Init( void );
void ModGameInfo_Init( void );
void ModGladiatorItemEnable_Init( void );
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

void ModAltFireConfig_Init( void );
void ModClickToJoin_Init( void );
void ModDetpack_Init( void );
void ModEndSound_Init( void );
void ModFireRateCS_Init( void );
void ModForcefield_Init( void );
void ModGhostSparkle_Init( void );
void ModHoldableTransporter_Init( void );
void ModIntermissionReady_Init( void );
void ModJoinLimit_Init( void );
void ModModcfgCS_Init( void );
void ModModelGroups_Init( void );
void ModModelSelection_Init( void );
void ModPendingItem_Init( void );
void ModSeeker_Init( qboolean registerPhoton );
void ModQuadEffects_Init( void );
void ModTimelimitCountdown_Init( void );
void ModWarmupSequence_Init( void );

//
// Click-to-Join support (comp_clicktojoin.c)
//

qboolean ModClickToJoin_Static_ActiveForClient( int clientNum );

//
// Portable Forcefields (comp_forcefield.c)
//

typedef enum {
	FFTR_BLOCK,			// don't let player through the forcefield
	FFTR_KILL,			// kill player who touched the forcefield
	FFTR_PASS,			// let player through forcefield
	FFTR_QUICKPASS,		// let player through forcefield without taking it down temporarily
} modForcefield_touchResponse_t;

typedef struct {
	// Damage options
	int health;					// amount of damage forcefields can take
	int duration;				// length in seconds forcefields last (if no damage taken)
	qboolean invulnerable;		// don't allow forcefields to be damaged by weapons fire

	// Sound effects
	const char *loopSound;				// give forcefields a background sound (null for no sound)
	qboolean alternateExpireSound;		// play alternate sound when forcefield expires (gladiator style when killing ff enabled)
	qboolean alternatePassSound;		// play alternate sound when temporarily removing forcefield (gladiator style when killing ff disabled)
	qboolean gladiatorAnnounce;			// play announcements when forcefields are added or removed (gladiator style when killing ff enabled)

	// Misc options
	qboolean bounceProjectiles;			// when invulnerable is set, grenades/tetrion alt bounce off rather than collide
	qboolean activateDelayMode;			// add extra delay and sound effect when starting forcefield (gladiator style with g_modUseKillingForcefield 1)

	// Applies to killing forcefields (response type FFTR_KILL)
	qboolean killForcefieldFlicker;		// play sparkle and sound effect on forcefield when touched (gladiator style)
	qboolean killPlayerSizzle;			// play sizzling effect on player killed by forcefield (gladiator style)
} modForcefield_config_t;

qboolean ModForcefield_Static_KillingForcefieldEnabled( void );

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
// Join limit handling (comp_join_limit.c)
//

qboolean ModJoinLimit_Static_MatchLocked( void );
void ModJoinLimit_Static_StartMatchLock( void );

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

void ModWarmupSequence_Static_ServerCommand( const char *msg );
void ModWarmupSequence_Static_AddEventToSequence( modWarmupSequence_t *sequence, int time,
		void ( *operation )( const char *msg ), const char *msg );

/* ************************************************************************* */
// Mod Functions
/* ************************************************************************* */

#define MAX_MOD_CALLS 30

typedef struct modFunctionBackendCall_s {
	const char *source;
	float priority;
	modCall_t call;
	struct modFunctionBackendCall_s *next;
} modFunctionBackendCall_t;

typedef struct {
	const char *name;
	modCall_t defaultCall;
	modFunctionBackendCall_t *entries;
	modCall_t callStack[MAX_MOD_CALLS + 1];		// +1 for default function
} modFunctionBackendEntry_t;

typedef struct {
	modFunctionBackendCall_t *entries[MAX_MOD_CALLS];
	int count;
} modCallList_t;

// Mod function backend
// Backend stores the full mod function data including call priorities, debug information, etc.
// which is used to generate the call stack.
typedef struct {
#define PREFIX2 next,
#define VOID2 next
#define MOD_FUNCTION_DEF( name, returntype, parameters, parameters_untyped, returnkw ) \
	modFunctionBackendEntry_t name;
#define MOD_FUNCTION_LOCAL( name, returntype, parameters, parameters_untyped, returnkw ) \
	modFunctionBackendEntry_t name;
#include "mods/g_mod_defs.h"
} modFunctionsBackend_t;

// Mod function types (ModFNType_*)
// Generate the mod function signatures with context parameters included.
#define PREFIX1 MODFN_CTV,
#define VOID1 MODFN_CTV
#define MOD_FUNCTION_DEF( modfnname, returntype, parameters, parameters_untyped, returnkw ) \
	typedef returntype ( *ModFNType_##modfnname ) parameters;
#define MOD_FUNCTION_LOCAL( modfnname, returntype, parameters, parameters_untyped, returnkw ) \
	typedef returntype ( *ModFNType_##modfnname ) parameters;
#include "mods/g_mod_defs.h"

#define MODFN_NC next + 1	// "next context"
#define MODFN_NEXT( type, parameters ) ((ModFNType_##type)*next) parameters		// call next modfn in sequence

extern modFunctionsBackend_t modFunctionsBackend;
void G_RegisterModFunctionCall( modFunctionBackendEntry_t *backendEntry, modCall_t call, const char *source, float priority );
void G_GetModCallList( modFunctionBackendEntry_t *backendEntry, modCallList_t *out );

/* ************************************************************************* */
// Local mod functions (modfn_lcl.*) - For mod functions only called by mods
/* ************************************************************************* */

// Default mod function declarations (ModFNDefault_*)
#define VOID1 void
#define MOD_FUNCTION_LOCAL( modfnname, returntype, parameters, parameters_untyped, returnkw ) \
	returntype ModFNDefault_##modfnname parameters;
#include "mods/g_mod_defs.h"

// modfn_lcl.* structure
typedef struct {
#define VOID1 void
#define MOD_FUNCTION_LOCAL( modfnname, returntype, parameters, parameters_untyped, returnkw ) \
	returntype (*modfnname) parameters;
#include "mods/g_mod_defs.h"
} mod_functions_local_t;

extern mod_functions_local_t modfn_lcl;

/* ************************************************************************* */
// Helper Macros
/* ************************************************************************* */

#ifdef MOD_NAME
#define MOD_PREFIX3( x, y ) x##_##y
#define MOD_PREFIX2( x, y ) MOD_PREFIX3( x, y )
#define MOD_PREFIX( x ) MOD_PREFIX2( MOD_NAME, x )

#define MOD_NAME_STR3( x ) #x
#define MOD_NAME_STR2( x ) MOD_NAME_STR3( x )
#define MOD_NAME_STR MOD_NAME_STR2( MOD_NAME )

#define MOD_STATE MOD_PREFIX( state )

#define MODFN_REGISTER( name, priority ) { \
	ModFNType_##name modfn = MOD_PREFIX(name); \
	G_RegisterModFunctionCall( &modFunctionsBackend.name, (modCall_t)modfn, MOD_NAME_STR, priority ); }
#endif
