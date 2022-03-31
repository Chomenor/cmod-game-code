/*
* Mod Function Definitions
* 
* Mod functions defined here are added to the "modfn" structure and shared
* with all game code.
* 
* The default value for each mod function will be set to the corresponding
* "ModFNDefault_(name)" function, which must be defined somewhere in the game code.
* 
* The mod function can be changed from the default during mod initialization,
* but by convention should never change after that until the game is unloaded.
* 
* Format: MOD_FUNCTION_DEF( name, return_type, parameters )
*/

//////////////////////////
// general match related
//////////////////////////

// Called after G_InitGame completes.
MOD_FUNCTION_DEF( GeneralInit, void, ( void ) )

// Called after G_RunFrame completes.
MOD_FUNCTION_DEF( PostRunFrame, void, ( void ) )

//////////////////////////
// misc
//////////////////////////

// Allows mods to add values to the mod config configstring.
MOD_FUNCTION_DEF( AddModConfigInfo, void, ( char *info ) )
