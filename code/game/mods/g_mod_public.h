/*
* Public Mod Header
* 
* Definitions shared with main game code.
*/

void G_ModsInit( void );

typedef enum {
	CJA_AUTOJOIN,
	CJA_SETTEAM,
	CJA_FORCETEAM,
	CJA_SETCLASS,
} join_allowed_type_t;

typedef enum {
	SA_PLAYERTIME,
	SA_NUM_DEATHS,
	SA_ELIMINATED,
} scoreboardAttribute_t;

typedef enum {
	EST_REALSCORE,
	EST_SCOREBOARD,
	EST_STATUS_QUERY,
} effectiveScoreType_t;

typedef enum {
	EH_VISIBLE,
	EH_DAMAGE,
	EH_MAXHEALTH,
} effectiveHandicapType_t;

typedef enum {
	FSE_CREATE,
	FSE_ACTIVATE,
	FSE_TEMP_DEACTIVATE,
	FSE_TEMP_REACTIVATE,
	FSE_REMOVE
} forcefieldEvent_t;

typedef enum {
	WE_IMOD_END,
	WE_IMOD_RIFLE,
	WE_RIFLE_ALT,
	WE_GRENADE_PRIMARY,
	WE_GRENADE_ALT,
} weaponEffect_t;

typedef enum {
	GC_NONE,
	GC_SKIP_RUN_MISSILE,
	GC_EVENT_TIME_OFFSET,
	GC_SKIP_SPECTATOR_DOOR_TELEPORT,
	GC_DISABLE_TEAM_FORCE_BALANCE,
	GC_INTERMISSION_DELAY_TIME,
	GC_DISABLE_ADDSCORE,
	GC_ALLOW_BOT_CLASS_SPECIFIER,
	GC_FORCE_BOTROAMSONLY,
	GC_DISABLE_TACTICIAN,
	GC_SKIP_ENTER_GAME_PRINT,
	GC_DISABLE_BOT_ADDING,			// Skip normal bot_minplayers handling
	GC_JOIN_MESSAGE_CONSOLE_PRINT,	// Use console print for team join messages instead of center print
	GC_FORCEFIELD_HEALTH_TOTAL,
	GC_FORCEFIELD_HEALTH_DECREMENT,
	GC_FORCEFIELD_ACTIVATE_DELAY,
	GC_FORCEFIELD_TAKE_DAMAGE,
	GC_DETPACK_NO_SHOCKWAVE,
	GC_SEEKER_PICKUP_SOUND,			// Extra sound effect when picking up Seeker powerup consistent with Pinball mod
	GC_DECOY_SOUND_EFFECTS,			// Sound effects when decoy is placed/removed
	GC_CHAT_HIT_WARNING,			// Warning sound when hitting a player without being blocked by shields who is currently chatting
	GC_FORCE_TEAM_PODIUM,			// Use team podium with only single winner model displayed even in FFA mode
	GC_TOSS_ITEMS_ON_SUICIDE,
} generalConstant_t;

typedef enum {
	WC_NONE,
	WC_PHASER_DAMAGE,
	WC_PHASER_ALT_RADIUS,
	WC_CRIFLE_DAMAGE,
	WC_CRIFLE_MAIN_SPLASH_RADIUS,
	WC_CRIFLE_MAIN_SPLASH_DMG,
	WC_CRIFLE_ALTDAMAGE,
	WC_CRIFLE_ALT_SPLASH_RADIUS,
	WC_CRIFLE_ALT_SPLASH_DMG,
	WC_IMOD_DAMAGE,
	WC_IMOD_ALT_DAMAGE,
	WC_SCAV_DAMAGE,
	WC_SCAV_ALT_DAMAGE,
	WC_SCAV_ALT_SPLASH_RAD,
	WC_SCAV_ALT_SPLASH_DAM,
	WC_STASIS_DAMAGE,
	WC_STASIS_ALT_DAMAGE,
	WC_STASIS_ALT_DAMAGE2,
	WC_GRENADE_DAMAGE,
	WC_GRENADE_SPLASH_RAD,
	WC_GRENADE_SPLASH_DAM,
	WC_TETRION_DAMAGE,
	WC_TETRION_ALT_DAMAGE,
	WC_QUANTUM_DAMAGE,
	WC_QUANTUM_SPLASH_DAM,
	WC_QUANTUM_SPLASH_RAD,
	WC_QUANTUM_ALT_DAMAGE,
	WC_QUANTUM_ALT_SPLASH_DAM,
	WC_QUANTUM_ALT_SPLASH_RAD,
	WC_DREADNOUGHT_DAMAGE,
	WC_DREADNOUGHT_WIDTH,
	WC_DREADNOUGHT_ALTDAMAGE,
	WC_BORG_PROJ_DAMAGE,
	WC_BORG_TASER_DAMAGE,

	WC_SCAV_VELOCITY,
	WC_STASIS_VELOCITY,
	WC_DN_SEARCH_DIST,
	WC_DN_ALT_SIZE,
	WC_BORG_PROJ_VELOCITY,

	// Special options
	WC_USE_RANDOM_DAMAGE,
	WC_USE_TRIPMINES,
	WC_USE_IMOD_RIFLE,
	WC_DN_MAX_MOVES,
	WC_WELDER_SKIP_INITIAL_THINK,
	WC_IMPRESSIVE_REWARDS,
	WC_CRIFLE_SPLASH_USE_QUADFACTOR,
	WC_ASSIM_NO_STRICT_TEAM_CHECK,		// allow assimilator to work in FFA
} weaponConstant_t;

// Mod trace flags
#define MOD_TRACE_WEAPON 1

//
// mod functions (modfn.*)
//

// Generic function pointer type, needed to avoid compiler warnings
typedef void (*modCall_t)(void);

// Context parameters included in modfn calls
#define MODFN_CTV modCall_t *next	// "context variables"
#define MODFN_CTN next		// "context names"

// Default mod function declarations (ModFNDefault_*)
#define VOID1 void
#define MOD_FUNCTION_DEF( modfnname, returntype, parameters, parameters_untyped, returnkw ) \
	returntype ModFNDefault_##modfnname parameters;
#include "mods/g_mod_defs.h"

// modfn.* structure
typedef struct {
#define VOID1 void
#define MOD_FUNCTION_DEF( modfnname, returntype, parameters, parameters_untyped, returnkw ) \
	returntype (*modfnname) parameters;
#include "mods/g_mod_defs.h"
} mod_functions_t;

extern mod_functions_t modfn;
