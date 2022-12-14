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

// Called at beginning of G_RunFrame.
MOD_FUNCTION_DEF( PreRunFrame, void, ( void ) )

// Called after G_RunFrame completes.
MOD_FUNCTION_DEF( PostRunFrame, void, ( void ) )

// Called after G_ShutdownGame completes.
MOD_FUNCTION_DEF( PostGameShutdown, void, ( qboolean restart ) )

// Called when level.matchState has been updated.
MOD_FUNCTION_DEF( MatchStateTransition, void, ( matchState_t oldState, matchState_t newState ) )

// Returns length in milliseconds to use for warmup if enabled, or <= 0 if warmup disabled.
MOD_FUNCTION_DEF( WarmupLength, int, ( void ) )

// Checks for initiating match exit to intermission.
// Called every frame and after CalculateRanks, which is called on events like
//   player death, connect, disconnect, team change, etc.
// Not called if warmup or intermission is already in progress, but is typically called
//   before check to enter warmup, so events like a player quitting or being assimilated
//   can trigger intermission even if they make CheckEnoughPlayers false.
MOD_FUNCTION_DEF( CheckExitRules, void, ( void ) )

// Determines whether to display green 'ready' indicator next to player's name during intermission.
MOD_FUNCTION_DEF( IntermissionReadyIndicator, qboolean, ( int clientNum ) )

// Determine whether it is time to exit intermission.
MOD_FUNCTION_DEF( IntermissionReadyToExit, qboolean, ( void ) )

// Execute change to next round or map when intermission completes.
MOD_FUNCTION_DEF( ExitLevel, void, ( void ) )

//////////////////////////
// client events
// (connection, spawn, disconnect, etc.)
//////////////////////////

// Called at beginning of ClientConnect when a valid client is connecting.
// Note that client structures have not yet been initialized at this point.
MOD_FUNCTION_DEF( PreClientConnect, void, ( int clientNum, qboolean firstTime, qboolean isBot ) )

// Called when a player is ready to respawn.
MOD_FUNCTION_DEF( ClientRespawn, void, ( int clientNum ) )

// Called when a player is spawning.
MOD_FUNCTION_DEF( PreClientSpawn, void, ( int clientNum, clientSpawnType_t spawnType ) )

// Called after a player finishes spawning.
MOD_FUNCTION_DEF( PostClientSpawn, void, ( int clientNum, clientSpawnType_t spawnType ) )

// Called when a player is leaving a team either due to team change or disconnect.
MOD_FUNCTION_DEF( PrePlayerLeaveTeam, void, ( int clientNum, team_t oldTeam ) )

// Called near the end of player_die function.
MOD_FUNCTION_DEF( PostPlayerDie, void, ( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath, int *awardPoints ) )

//////////////////////////
// client accessors
//////////////////////////

// Returns true if client is currently spectating, including eliminated players.
// Note: This returns false for players who were eliminated but not yet respawned as a spectator.
MOD_FUNCTION_DEF( SpectatorClient, qboolean, ( int clientNum ) )

// Returns effective score values to use for client.
MOD_FUNCTION_DEF( EffectiveScore, int, ( int clientNum, effectiveScoreType_t type ) )

// Returns effective handicap values to use for client.
MOD_FUNCTION_DEF( EffectiveHandicap, int, ( int clientNum, effectiveHandicapType_t type ) )

// Returns player-selected team, even if active team is temporarily overriden by borg assimilation.
MOD_FUNCTION_DEF( RealSessionTeam, team_t, ( int clientNum ) )

// Returns player-selected class, even if active class is temporarily overridden by borg assimilation.
MOD_FUNCTION_DEF( RealSessionClass, pclass_t, ( int clientNum ) )

// Check if client is allowed to join game or change team/class.
// If join was blocked, sends appropriate notification message to client.
// targetTeam is only applicable if type is CJA_SETTEAM.
MOD_FUNCTION_DEF( CheckJoinAllowed, qboolean, ( int clientNum, join_allowed_type_t type, team_t targetTeam ) )

// Returns true if time to respawn dead player has been reached. Called with voluntary true if player
// is pressing the respawn button, and voluntary false to check for forced respawns.
MOD_FUNCTION_DEF( CheckRespawnTime, qboolean, ( int clientNum, qboolean voluntary ) )

//////////////////////////
// client misc
//////////////////////////

// Attempt to correct missing or spawn killing player spawn point. *spawn may be NULL if the standard
// spawn selection failed to find any spawn points, otherwise the selected spawn point will be checked
// for spawn killing and potentially replaced. clientNum can be -1 if not specified.
MOD_FUNCTION_DEF( PatchClientSpawn, void, ( int clientNum, gentity_t **spawn, vec3_t origin, vec3_t angles ) )

// Checks if current player class is valid, and sets a new one if necessary.
// Called both during spawn and initial client connect when reading session info.
MOD_FUNCTION_DEF( UpdateSessionClass, void, ( int clientNum ) )

// Configures class and other client parameters for non-spectators during ClientSpawn.
MOD_FUNCTION_DEF( SpawnConfigureClient, void, ( int clientNum ) )

// Prints info messages to client during ClientSpawn.
MOD_FUNCTION_DEF( SpawnCenterPrintMessage, void, ( int clientNum, clientSpawnType_t spawnType ) )

// Play transporter effect when player spawns.
MOD_FUNCTION_DEF( SpawnTransporterEffect, void, ( int clientNum, clientSpawnType_t spawnType ) )

// Retrieves player model string for client, performing any mod conversions as needed.
MOD_FUNCTION_DEF( GetPlayerModel, void, ( int clientNum, const char *userinfo, char *output, unsigned int outputSize ) )

// Generates awards message string for specified client.
MOD_FUNCTION_DEF( CalculateAwards, void, ( int clientNum, char *msg ) )

//////////////////////////
// weapon related
//////////////////////////

// Support modifying weapon-related constants in g_weapon.c.
MOD_FUNCTION_DEF( AdjustWeaponConstant, int, ( weaponConstant_t wcType, int defaultValue ) )

// Called after weapon projectile has been created.
MOD_FUNCTION_DEF( PostFireProjectile, void, ( gentity_t *projectile ) )

// Generate random number for weapon shot variation that can be predicted on client.
MOD_FUNCTION_DEF( WeaponPredictableRNG, unsigned int, ( int clientNum ) )

//////////////////////////
// combat related
// (damage, knockback, death, scoring, etc.)
//////////////////////////

// Allows adding or modifying damage flags.
MOD_FUNCTION_DEF( ModifyDamageFlags, int, ( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) )

	// Returns mass for knockback calculations.
MOD_FUNCTION_DEF( KnockbackMass, float, ( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) )

// Deals damage from explosion-type sources.
MOD_FUNCTION_DEF( RadiusDamage, qboolean, ( vec3_t origin, gentity_t *attacker, float damage, float radius,
		gentity_t *ignore, int dflags, int mod ) )

// Creates a corpse entity for dead player. Returns body if created, null otherwise.
MOD_FUNCTION_DEF( CopyToBodyQue, gentity_t *, ( int clientNum ) )

//////////////////////////
// item pickup related
//////////////////////////

// Allows replacing an item with a different item during initial map spawn.
MOD_FUNCTION_DEF( CheckReplaceItem, gitem_t *, ( gitem_t *item ) )

// Allows permanently removing an item during initial spawn.
// Returns qtrue to disable item, qfalse to spawn item normally.
MOD_FUNCTION_DEF( CheckItemSpawnDisabled, qboolean, ( gitem_t *item ) )

// Returns the effective number of players to use for g_adaptRespawn calculation.
MOD_FUNCTION_DEF( AdaptRespawnNumPlayers, int, ( void ) )

// Check if item can be tossed on death/disconnect.
MOD_FUNCTION_DEF( CanItemBeDropped, qboolean, ( gitem_t *item, int clientNum ) )

// Can be used to add additional registered items, which are added to a configstring for
// client caching purposes.
MOD_FUNCTION_DEF( AddRegisteredItems, void, ( void ) )

//////////////////////////
// holdable item related
//////////////////////////

// Called when player triggers the holdable transporter powerup.
MOD_FUNCTION_DEF( PortableTransporterActivate, void, ( int clientNum ) )

//////////////////////////
// session related
//////////////////////////

// Called to initialize client state when client is initially connecting, or to load saved client data
// when client is being reconnected from a map change/restart. Info string will be empty when initialConnect is true.
MOD_FUNCTION_DEF( InitClientSession, void, ( int clientNum, qboolean initialConnect, const info_string_t *info ) )

// Generates client info string data to be written to cvar on match exit.
MOD_FUNCTION_DEF( GenerateClientSessionInfo, void, ( int clientNum, info_string_t *info ) )

// Generates session structure written to cvar on match exit. This data may be used by other mods
// on servers that support mod switching, so compatibility should be taken into account.
MOD_FUNCTION_DEF( GenerateClientSessionStructure, void, ( int clientNum, clientSession_t *sess ) )

// Generates global info string data to be written to cvar on match exit.
MOD_FUNCTION_DEF( GenerateGlobalSessionInfo, void, ( info_string_t *info ) )

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

// Adjust weapon ammo usage.
MOD_FUNCTION_DEF( ModifyAmmoUsage, int, ( int defaultValue, int weapon, qboolean alt ) )

//////////////////////////
// misc
//////////////////////////

// Support modifying integer constants anywhere in the game code.
// Intended for simple cases that don't justify a separate mod function.
MOD_FUNCTION_DEF( AdjustGeneralConstant, int, ( generalConstant_t gcType, int defaultValue ) )

// Allow mods to handle custom console commands. Returns qtrue to suppress normal handling of command.
MOD_FUNCTION_DEF( ModConsoleCommand, qboolean, ( const char *cmd ) )

// Allows mods to handle client commands. Returns qtrue to suppress normal handling of command.
// Can be called for connecting clients and during intermission.
MOD_FUNCTION_DEF( ModClientCommand, qboolean, ( int clientNum, const char *cmd ) )

// Allows mods to add values to the mod config configstring.
MOD_FUNCTION_DEF( AddModConfigInfo, void, ( char *info ) )

// Allows mods to adjust attributes sent to clients via "scores" command.
MOD_FUNCTION_DEF( AdjustScoreboardAttributes, int, ( int clientNum, scoreboardAttribute_t saType, int defaultValue ) )

// Check if suicide is allowed. If not, prints notification to client.
MOD_FUNCTION_DEF( CheckSuicideAllowed, qboolean, ( int clientNum ) )

// Set CS_SCORES* configstrings during CalculateRanks.
MOD_FUNCTION_DEF( SetScoresConfigStrings, void, ( void ) )

// Wrapper to trap_Trace function allowing mod overrides.
MOD_FUNCTION_DEF( TrapTrace, void, ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int passEntityNum, int contentmask, int modFlags ) )

//////////////////////////
// mod-specific
//////////////////////////

// Returns whether borg adaptive shields have blocked damage.
// Also sets PW_BORG_ADAPT to play effect on target if needed.
MOD_FUNCTION_DEF( CheckBorgAdapt, qboolean, ( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) )

// Checks whether specific client is the borg queen. Always returns false if not in Assimilation mode.
// Also returns false if not a valid client number, so is valid to call for any entity.
MOD_FUNCTION_DEF( IsBorgQueen, qboolean, ( int clientNum ) )

//////////////////////////
// internal
// (only called from mod code)
//////////////////////////

// Called after main mod initialization is complete.
MOD_FUNCTION_DEF( PostModInit, void, ( void ) )
