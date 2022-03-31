/*
* Public Mod Header
* 
* Definitions shared with main game code.
*/

void G_ModCoreInit( void );
void G_ModsInit( void );

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
