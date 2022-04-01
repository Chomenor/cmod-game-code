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
// player movement
//////////////////////////

// Support modifying pmove-related integer constants.
MOD_FUNCTION_DEF( AdjustPmoveConstant, int, ( pmoveConstant_t pmcType, int defaultValue ) )

// Initialize pmove_t structure ahead of player move.
MOD_FUNCTION_DEF( PmoveInit, void, ( int clientNum, pmove_t *pmove ) )

// Performs player movement corresponding to a single input usercmd from the client.
MOD_FUNCTION_DEF( RunPlayerMove, void, ( int clientNum ) )

// Process triggers and other operations after player move(s) have completed.
// This may be called 0, 1, or multiple times per input usercmd depending on move partitioning.
MOD_FUNCTION_DEF( PostPmoveActions, void, ( pmove_t *pmove, int clientNum, int oldEventSequence ) )

//////////////////////////
// misc
//////////////////////////

// Allows mods to add values to the mod config configstring.
MOD_FUNCTION_DEF( AddModConfigInfo, void, ( char *info ) )
