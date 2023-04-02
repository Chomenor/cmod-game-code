/*
* Public Mod Header
* 
* Definitions shared with main game code.
*/

void G_ModsInit( void );

typedef enum {
	CJA_AUTOJOIN,
	CJA_SETTEAM,
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
} generalConstant_t;

typedef enum {
	WC_NONE,
	WC_CRIFLE_ALTDAMAGE,
	WC_CRIFLE_ALT_SPLASH_RADIUS,
	WC_CRIFLE_ALT_SPLASH_DMG,
	WC_USE_RANDOM_DAMAGE,
	WC_WELDER_SKIP_INITIAL_THINK,
	WC_USE_TRIPMINES,
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
