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

// Ignore local defs
#define MOD_FUNCTION_LOCAL( name, returntype, parameters )

// Mod function types (ModFNType_*)
#define MOD_FUNCTION_DEF( name, returntype, parameters ) \
	typedef returntype ( *ModFNType_##name ) parameters;
#include "mods/g_mod_defs.h"
#undef MOD_FUNCTION_DEF

// Default mod function declarations (ModFNDefault_*)
#define MOD_FUNCTION_DEF( name, returntype, parameters ) \
	returntype ModFNDefault_##name parameters;
#include "mods/g_mod_defs.h"
#undef MOD_FUNCTION_DEF

// Mod functions structure
typedef struct {

#define MOD_FUNCTION_DEF( name, returntype, parameters ) \
	ModFNType_##name name;
#include "mods/g_mod_defs.h"
#undef MOD_FUNCTION_DEF

} mod_functions_t;

#undef MOD_FUNCTION_LOCAL

extern mod_functions_t modfn;
