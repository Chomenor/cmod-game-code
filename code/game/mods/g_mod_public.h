/*
* Public Mod Header
* 
* Definitions shared with main game code.
*/

void G_ModCoreInit( void );
void G_ModsInit( void );

typedef enum {
	GC_NONE,
	GC_SKIP_RUN_MISSILE,
	GC_EVENT_TIME_OFFSET,
	GC_SKIP_SPECTATOR_DOOR_TELEPORT,
} generalConstant_t;

typedef enum {
	WC_NONE,
	WC_WELDER_SKIP_INITIAL_THINK,
} weaponConstant_t;

typedef enum {
	PMC_NONE,
	PMC_FIXED_LENGTH,	// subdivide moves into this frame length (0 = disabled)
	PMC_PARTIAL_MOVE_TRIGGERS,	// if 1, process triggers after each subdivided move fragment
} pmoveConstant_t;

// Mod trace flags
#define MOD_TRACE_WEAPON 1

//
// mod functions (modfn.*)
//

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

extern mod_functions_t modfn;
