/*
* Mod Function Definitions
* 
* Mod functions declared here are added to the "modfn" structure and shared
* with all game code.
* 
* The default value for each mod function will be set to the corresponding
* "ModFNDefault_(name)" function, which must be defined somewhere in the game code.
* 
* Modules can register to handle mod functions via the MODFN_REGISTER macro.
* Each handler can call the next handler in the sequence, and the final handler
* will call the default mod function.
* 
* The essential format of each mod declaration is:
* MOD_FUNCTION_DEF( name, return_type, parameters, ... )
* with some additional elements added to meet C implementation requirements.
*/

#ifndef PREFIX1
#define PREFIX1
#endif
#ifndef PREFIX2
#define PREFIX2
#endif
#ifndef VOID1
#define VOID1
#endif
#ifndef VOID2
#define VOID2
#endif
#ifndef MOD_FUNCTION_DEF
#define MOD_FUNCTION_DEF( name, returntype, parameters, parameters_untyped, returnkw )
#endif
#ifndef MOD_FUNCTION_LOCAL
#define MOD_FUNCTION_LOCAL( name, returntype, parameters, parameters_untyped, returnkw )
#endif

//////////////////////////
// general match related
//////////////////////////

// Called after G_InitGame completes.
MOD_FUNCTION_DEF( GeneralInit, void, ( VOID1 ),
		( VOID2 ), )

// Called at beginning of G_RunFrame.
MOD_FUNCTION_DEF( PreRunFrame, void, ( VOID1 ),
		( VOID2 ), )

// Called after G_RunFrame completes.
MOD_FUNCTION_DEF( PostRunFrame, void, ( VOID1 ),
		( VOID2 ), )

// Called after G_ShutdownGame completes.
MOD_FUNCTION_DEF( PostGameShutdown, void, ( PREFIX1 qboolean restart ),
		( PREFIX2 restart ), )

// Called after CalculateRanks completes.
MOD_FUNCTION_DEF( PostCalculateRanks, void, ( VOID1 ),
		( VOID2 ), )

// Called when level.matchState has been updated.
MOD_FUNCTION_DEF( MatchStateTransition, void, ( PREFIX1 matchState_t oldState, matchState_t newState ),
		( PREFIX2 oldState, newState ), )

// Returns length in milliseconds to use for warmup if enabled, or <= 0 if warmup disabled.
MOD_FUNCTION_DEF( WarmupLength, int, ( VOID1 ),
		( VOID2 ), return )

// Checks for initiating match exit to intermission.
// Called every frame and after CalculateRanks, which is called on events like
//   player death, connect, disconnect, team change, etc.
// Not called if warmup or intermission is already in progress, but is typically called
//   before check to enter warmup, so events like a player quitting or being assimilated
//   can trigger intermission even if they make CheckEnoughPlayers false.
MOD_FUNCTION_DEF( CheckExitRules, void, ( VOID1 ),
		( VOID2 ), )

// Determines whether to display green 'ready' indicator next to player's name during intermission.
MOD_FUNCTION_DEF( IntermissionReadyIndicator, qboolean, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

// Determine whether it is time to exit intermission.
MOD_FUNCTION_DEF( IntermissionReadyToExit, qboolean, ( VOID1 ),
		( VOID2 ), return )

// Execute change to next round or map when intermission completes.
MOD_FUNCTION_DEF( ExitLevel, void, ( VOID1 ),
		( VOID2 ), )

//////////////////////////
// client events
// (connection, spawn, disconnect, etc.)
//////////////////////////

// Called after a player is added to the game.
MOD_FUNCTION_DEF( PostClientConnect, void, ( PREFIX1 int clientNum, qboolean firstTime, qboolean isBot ),
		( PREFIX2 clientNum, firstTime, isBot ), )

// Called when a player is ready to respawn.
MOD_FUNCTION_DEF( ClientRespawn, void, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), )

// Called when a player is spawning.
MOD_FUNCTION_DEF( PreClientSpawn, void, ( PREFIX1 int clientNum, clientSpawnType_t spawnType ),
		( PREFIX2 clientNum, spawnType ), )

// Called after a player finishes spawning.
MOD_FUNCTION_DEF( PostClientSpawn, void, ( PREFIX1 int clientNum, clientSpawnType_t spawnType ),
		( PREFIX2 clientNum, spawnType ), )

// Called when a player is leaving a team either due to team change or disconnect.
MOD_FUNCTION_DEF( PrePlayerLeaveTeam, void, ( PREFIX1 int clientNum, team_t oldTeam ),
		( PREFIX2 clientNum, oldTeam ), )

// Called near the end of player_die function.
MOD_FUNCTION_DEF( PostPlayerDie, void, ( PREFIX1 gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath, int *awardPoints ),
		( PREFIX2 self, inflictor, attacker, meansOfDeath, awardPoints ), )

//////////////////////////
// client accessors
//////////////////////////

// Returns true if client is currently spectating, including eliminated players.
// Note: This returns false for players who were eliminated but not yet respawned as a spectator.
MOD_FUNCTION_DEF( SpectatorClient, qboolean, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

// Returns effective score values to use for client.
MOD_FUNCTION_DEF( EffectiveScore, int, ( PREFIX1 int clientNum, effectiveScoreType_t type ),
		( PREFIX2 clientNum, type ), return )

// Returns effective handicap values to use for client.
MOD_FUNCTION_DEF( EffectiveHandicap, int, ( PREFIX1 int clientNum, effectiveHandicapType_t type ),
		( PREFIX2 clientNum, type ), return )

// Returns player-selected team, even if active team is temporarily overriden by borg assimilation.
MOD_FUNCTION_DEF( RealSessionTeam, team_t, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

// Returns player-selected class, even if active class is temporarily overridden by borg assimilation.
MOD_FUNCTION_DEF( RealSessionClass, pclass_t, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

// Check if client is allowed to join game or change team/class.
// If join was blocked, sends appropriate notification message to client.
// targetTeam is only applicable if type is CJA_SETTEAM.
MOD_FUNCTION_DEF( CheckJoinAllowed, qboolean, ( PREFIX1 int clientNum, join_allowed_type_t type, team_t targetTeam ),
		( PREFIX2 clientNum, type, targetTeam ), return )

// Returns true if time to respawn dead player has been reached. Called with voluntary true if player
// is pressing the respawn button, and voluntary false to check for forced respawns.
MOD_FUNCTION_DEF( CheckRespawnTime, qboolean, ( PREFIX1 int clientNum, qboolean voluntary ),
		( PREFIX2 clientNum, voluntary ), return )

// Returns true if follow spectators will cycle to this client by default.
MOD_FUNCTION_DEF( EnableCycleFollow, qboolean, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

//////////////////////////
// client misc
//////////////////////////

// Attempt to correct missing or spawn killing player spawn point. *spawn may be NULL if the standard
// spawn selection failed to find any spawn points, otherwise the selected spawn point will be checked
// for spawn killing and potentially replaced. clientNum can be -1 if not specified.
MOD_FUNCTION_DEF( PatchClientSpawn, void, ( PREFIX1 int clientNum, gentity_t **spawn, vec3_t origin, vec3_t angles ),
		( PREFIX2 clientNum, spawn, origin, angles ), )

// Checks if current player class is valid, and sets a new one if necessary.
// Called both during spawn and initial client connect when reading session info.
MOD_FUNCTION_DEF( UpdateSessionClass, void, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), )

// Configures class and other client parameters for non-spectators during ClientSpawn.
MOD_FUNCTION_DEF( SpawnConfigureClient, void, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), )

// Prints info messages to client during ClientSpawn.
MOD_FUNCTION_DEF( SpawnCenterPrintMessage, void, ( PREFIX1 int clientNum, clientSpawnType_t spawnType ),
		( PREFIX2 clientNum, spawnType ), )

// Play transporter effect when player spawns.
MOD_FUNCTION_DEF( SpawnTransporterEffect, void, ( PREFIX1 int clientNum, clientSpawnType_t spawnType ),
		( PREFIX2 clientNum, spawnType ), )

// Activate or deactivate spawn invulnerability on client.
MOD_FUNCTION_DEF( SetClientGhosting, void, ( PREFIX1 int clientNum, qboolean active ),
		( PREFIX2 clientNum, active ), )

// Retrieves player model string for client, performing any mod conversions as needed.
MOD_FUNCTION_DEF( GetPlayerModel, void, ( PREFIX1 int clientNum, const char *userinfo, char *output, unsigned int outputSize ),
		( PREFIX2 clientNum, userinfo, output, outputSize ), )

// Generates awards message string for specified client.
MOD_FUNCTION_DEF( CalculateAwards, void, ( PREFIX1 int clientNum, char *msg ),
		( PREFIX2 clientNum, msg ), )

// Determines which weapon to display for player on the winner podium.
MOD_FUNCTION_DEF( PodiumWeapon, weapon_t, ( PREFIX1 int clientNum ), ( PREFIX2 clientNum ), return )

//////////////////////////
// weapon related
//////////////////////////

// Support modifying weapon-related constants in g_weapon.c.
MOD_FUNCTION_DEF( AdjustWeaponConstant, int, ( PREFIX1 weaponConstant_t wcType, int defaultValue ),
		( PREFIX2 wcType, defaultValue ), return )

// Called after weapon projectile has been created.
MOD_FUNCTION_DEF( PostFireProjectile, void, ( PREFIX1 gentity_t *projectile ),
		( PREFIX2 projectile ), )

// Called when missile hits something.
MOD_FUNCTION_DEF( MissileImpact, void, ( PREFIX1 gentity_t *ent, trace_t *trace ),
		( PREFIX2 ent, trace ), )

// Generate random number for weapon shot variation that can be predicted on client.
MOD_FUNCTION_DEF( WeaponPredictableRNG, unsigned int, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

// Support adding effects to certain weapon events.
MOD_FUNCTION_DEF( AddWeaponEffect, void, ( PREFIX1 weaponEffect_t weType, gentity_t *ent, trace_t *trace ),
		( PREFIX2 weType, ent, trace ), )

//////////////////////////
// combat related
// (damage, knockback, death, scoring, etc.)
//////////////////////////

// Called when a player or entity takes damage.
MOD_FUNCTION_DEF( Damage, void, ( PREFIX1 gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ),
		( PREFIX2 targ, inflictor, attacker, dir, point, damage, dflags, mod ), )

// Allows adding or modifying damage flags.
MOD_FUNCTION_DEF( ModifyDamageFlags, int, ( PREFIX1 gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ),
		( PREFIX2 targ, inflictor, attacker, dir, point, damage, dflags, mod ), return )

// Returns effective g_dmgmult value.
// Called separately for both damage and knockback calculation, differentiated by knockbackMode parameter.
MOD_FUNCTION_DEF( GetDamageMult, float, ( PREFIX1 gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod, qboolean knockbackMode ),
		( PREFIX2 targ, inflictor, attacker, dir, point, damage, dflags, mod, knockbackMode ), return )

// Returns mass for knockback calculations.
MOD_FUNCTION_DEF( KnockbackMass, float, ( PREFIX1 gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ),
		( PREFIX2 targ, inflictor, attacker, dir, point, damage, dflags, mod ), return )

// Apply knockback to players when taking damage.
MOD_FUNCTION_DEF( ApplyKnockback, void, ( PREFIX1 gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod, float knockback ),
		( PREFIX2 targ, inflictor, attacker, dir, point, damage, dflags, mod, knockback ), )

// Deals damage from explosion-type sources.
MOD_FUNCTION_DEF( RadiusDamage, qboolean, ( PREFIX1 vec3_t origin, gentity_t *attacker, float damage, float radius,
		gentity_t *ignore, int dflags, int mod ),
		( PREFIX2 origin, attacker, damage, radius, ignore, dflags, mod ), return )

// Play some weapon-specific effects when player is killed.
MOD_FUNCTION_DEF( PlayerDeathEffect, void, ( PREFIX1 gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int contents, int killer, int meansOfDeath ),
		( PREFIX2 self, inflictor, attacker, contents, killer, meansOfDeath ), )

// Creates a corpse entity for dead player. Returns body if created, null otherwise.
MOD_FUNCTION_DEF( CopyToBodyQue, gentity_t *, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

//////////////////////////
// item pickup related
//////////////////////////

// Allows replacing an item with a different item during initial map spawn.
MOD_FUNCTION_DEF( CheckReplaceItem, gitem_t *, ( PREFIX1 gitem_t *item ),
		( PREFIX2 item ), return )

// Allows permanently removing an item during initial spawn.
// Returns qtrue to disable item, qfalse to spawn item normally.
MOD_FUNCTION_DEF( CheckItemSpawnDisabled, qboolean, ( PREFIX1 gitem_t *item ),
		( PREFIX2 item ), return )

// Temporarily suppresses item from spawning, but doesn't permanently remove it.
// If suppressed, returns number of milliseconds until next check, otherwise returns 0.
MOD_FUNCTION_DEF( CheckItemSuppressed, int, ( PREFIX1 gentity_t *item ),
		( PREFIX2 item ), return )

// Returns number of milliseconds to delay spawning item at start of level,
// or 0 for no delay.
MOD_FUNCTION_DEF( ItemInitialSpawnDelay, int, ( PREFIX1 gentity_t *ent ),
		( PREFIX2 ent ), return )

// Allow mods to change item respawn time. Called ahead of g_adaptRespawn adjustment.
MOD_FUNCTION_DEF( AdjustItemRespawnTime, float, ( PREFIX1 float time, const gentity_t *ent ),
		( PREFIX2 time, ent ), return )

// Returns the effective number of players to use for g_adaptRespawn calculation.
MOD_FUNCTION_DEF( AdaptRespawnNumPlayers, int, ( VOID1 ),
		( VOID2 ), return )

// Check if player is allowed to pick up item.
MOD_FUNCTION_DEF( CanItemBeGrabbed, qboolean, ( PREFIX1 gentity_t *item, int clientNum ),
		( PREFIX2 item, clientNum ), return )

// Check if item can be tossed on death/disconnect.
MOD_FUNCTION_DEF( CanItemBeDropped, qboolean, ( PREFIX1 gitem_t *item, int clientNum ),
		( PREFIX2 item, clientNum ), return )

// Set the client origin to use when tossing items.
MOD_FUNCTION_DEF( ItemTossOrigin, void, ( PREFIX1 int clientNum, vec3_t originOut ),
		( PREFIX2 clientNum, originOut ), )

// Returns number of milliseconds until dropped item expires (0 = never expires).
MOD_FUNCTION_DEF( ItemDropExpireTime, int, ( PREFIX1 const gitem_t *item ),
		( PREFIX2 item ), return )

// Adds ammo when player picks up either a weapon or ammo item.
MOD_FUNCTION_DEF( AddAmmoForItem, void, ( PREFIX1 int clientNum, gentity_t *item, int weapon, int count ),
		( PREFIX2 clientNum, item, weapon, count ), )

// Can be used to add additional registered items, which are added to a configstring for
// client caching purposes.
MOD_FUNCTION_DEF( AddRegisteredItems, void, ( VOID1 ),
		( VOID2 ), )

//////////////////////////
// holdable item related
//////////////////////////

// Called when player triggers the holdable transporter powerup.
MOD_FUNCTION_DEF( PortableTransporterActivate, void, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), )

// Called when player triggers the portable forcefield powerup.
// Returns entity if forcefield successfully placed, NULL if location invalid.
MOD_FUNCTION_DEF( ForcefieldPlace, gentity_t *, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

// Play sound effects on forcefield events such as creation and removal.
MOD_FUNCTION_DEF( ForcefieldSoundEvent, void, ( PREFIX1 gentity_t *ent, forcefieldEvent_t event ),
		( PREFIX2 ent, event ), )

// Play informational messages or sounds on forcefield events.
MOD_FUNCTION_DEF( ForcefieldAnnounce, void, ( PREFIX1 gentity_t *ent, forcefieldEvent_t event ),
		( PREFIX2 ent, event ), )

// Called when player triggers the detpack powerup.
// Returns entity if detpack successfully placed, NULL if location invalid.
MOD_FUNCTION_DEF( DetpackPlace, gentity_t *, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

// Called when a detpack is being destroyed due to weapons fire or timeout.
// Normally causes a small explosion but not full detonation.
MOD_FUNCTION_DEF( DetpackShot, void, ( PREFIX1 gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ),
		( PREFIX2 self, inflictor, attacker, damage, meansOfDeath ), )

// Play additional sounds and effects to accompany detpack explosion.
MOD_FUNCTION_DEF( DetpackExplodeEffects, void, ( PREFIX1 gentity_t *detpack ),
		( PREFIX2 detpack ), )

// Remove any placed detpack owned by a player who was just killed.
MOD_FUNCTION_DEF( PlayerDeathDiscardDetpack, void, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), )

// Check for firing seeker powerup. Called once per second in ClientTimerActions.
MOD_FUNCTION_DEF( CheckFireSeeker, void, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), )

//////////////////////////
// session related
//////////////////////////

// Called to initialize client state when client is initially connecting, or to load saved client data
// when client is being reconnected from a map change/restart. Info string will be empty when initialConnect is true.
MOD_FUNCTION_DEF( InitClientSession, void, ( PREFIX1 int clientNum, qboolean initialConnect, const info_string_t *info ),
		( PREFIX2 clientNum, initialConnect, info ), )

// Generates client info string data to be written to cvar on match exit.
MOD_FUNCTION_DEF( GenerateClientSessionInfo, void, ( PREFIX1 int clientNum, info_string_t *info ),
		( PREFIX2 clientNum, info ), )

// Generates session structure written to cvar on match exit. This data may be used by other mods
// on servers that support mod switching, so compatibility should be taken into account.
MOD_FUNCTION_DEF( GenerateClientSessionStructure, void, ( PREFIX1 int clientNum, clientSession_t *sess ),
		( PREFIX2 clientNum, sess ), )

// Generates global info string data to be written to cvar on match exit.
MOD_FUNCTION_DEF( GenerateGlobalSessionInfo, void, ( PREFIX1 info_string_t *info ),
		( PREFIX2 info ), )

//////////////////////////
// player movement
//////////////////////////

// Returns fixed frame length to use for player move, or 0 for no fixed length.
MOD_FUNCTION_DEF( PmoveFixedLength, int, ( PREFIX1 qboolean isBot ),
		( PREFIX2 isBot ), return )

// Initialize pmove_t structure ahead of player move.
MOD_FUNCTION_DEF( PmoveInit, void, ( PREFIX1 int clientNum, pmove_t *pmove ),
		( PREFIX2 clientNum, pmove ), )

// Performs player movement corresponding to a single input usercmd from the client.
// Called for both active players and free moving spectators, but not follow spectators.
MOD_FUNCTION_DEF( RunPlayerMove, void, ( PREFIX1 int clientNum, qboolean spectator ),
		( PREFIX2 clientNum, spectator ), )

// Called in place of RunPlayerMove for follow spectators.
MOD_FUNCTION_DEF( FollowSpectatorThink, void, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), )

// Process triggers and other operations after player move(s) have completed.
// This may be called 0, 1, or multiple times per input usercmd depending on move partitioning.
MOD_FUNCTION_DEF( PostPmoveActions, void, ( PREFIX1 pmove_t *pmove, int clientNum, int oldEventSequence ),
		( PREFIX2 pmove, clientNum, oldEventSequence ), )

// Adjust weapon fire rate.
MOD_FUNCTION_DEF( ModifyFireRate, int, ( PREFIX1 int defaultInterval, int weapon, qboolean alt ),
		( PREFIX2 defaultInterval, weapon, alt ), return )

// Adjust weapon ammo usage.
MOD_FUNCTION_DEF( ModifyAmmoUsage, int, ( PREFIX1 int defaultValue, int weapon, qboolean alt ),
		( PREFIX2 defaultValue, weapon, alt ), return )

//////////////////////////
// misc
//////////////////////////

// Support modifying integer constants anywhere in the game code.
// Intended for simple cases that don't justify a separate mod function.
MOD_FUNCTION_DEF( AdjustGeneralConstant, int, ( PREFIX1 generalConstant_t gcType, int defaultValue ),
		( PREFIX2 gcType, defaultValue ), return )

// Allow mods to handle custom console commands. Returns qtrue to suppress normal handling of command.
MOD_FUNCTION_DEF( ModConsoleCommand, qboolean, ( PREFIX1 const char *cmd ),
		( PREFIX2 cmd ), return )

// Allows mods to handle client commands. Returns qtrue to suppress normal handling of command.
// Can be called for connecting clients and during intermission.
MOD_FUNCTION_DEF( ModClientCommand, qboolean, ( PREFIX1 int clientNum, const char *cmd ),
		( PREFIX2 clientNum, cmd ), return )

// Allows mods to adjust attributes sent to clients via "scores" command.
MOD_FUNCTION_DEF( AdjustScoreboardAttributes, int, ( PREFIX1 int clientNum, scoreboardAttribute_t saType, int defaultValue ),
		( PREFIX2 clientNum, saType, defaultValue ), return )

// Check if suicide is allowed. If not, prints notification to client.
MOD_FUNCTION_DEF( CheckSuicideAllowed, qboolean, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

// Set CS_SCORES* configstrings during CalculateRanks.
MOD_FUNCTION_DEF( SetScoresConfigStrings, void, ( VOID1 ),
		( VOID2 ), )

// Wrapper to trap_Trace function allowing mod overrides.
MOD_FUNCTION_DEF( TrapTrace, void, ( PREFIX1 trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
		const vec3_t end, int passEntityNum, int contentmask, int modFlags ),
		( PREFIX2 results, start, mins, maxs, end, passEntityNum, contentmask, modFlags ), )

// Called to set certain configstrings during level spawn.
MOD_FUNCTION_DEF( SetSpawnCS, void, ( PREFIX1 int num, const char *defaultValue ),
		( PREFIX2 num, defaultValue ), )

// Called when a team scores a point in CTF.
MOD_FUNCTION_DEF( PostFlagCapture, void, ( PREFIX1 team_t capturingTeam ),
		( PREFIX2 capturingTeam ), )

//////////////////////////
// mod-specific
//////////////////////////

// Returns whether borg adaptive shields have blocked damage.
// Also sets PW_BORG_ADAPT to play effect on target if needed.
MOD_FUNCTION_DEF( CheckBorgAdapt, qboolean, ( PREFIX1 gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ),
		( PREFIX2 targ, inflictor, attacker, dir, point, damage, dflags, mod ), return )

// Checks whether specific client is the borg queen. Always returns false if not in Assimilation mode.
// Also returns false if not a valid client number, so is valid to call for any entity.
MOD_FUNCTION_DEF( IsBorgQueen, qboolean, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

//////////////////////////
// internal
// only called from mod code; uses separate modfn_lcl structure
//////////////////////////

// Support modifying integer constants in the mod code.
// Intended for simple cases that don't justify a separate mod function.
MOD_FUNCTION_LOCAL( AdjustModConstant, int, ( PREFIX1 modConstant_t mcType, int defaultValue ),
		( PREFIX2 mcType, defaultValue ), return )

// Called after main mod initialization is complete.
MOD_FUNCTION_LOCAL( PostModInit, void, ( VOID1 ),
		( VOID2 ), )

// Allows mods to add values to the mod config configstring.
// Call ModModcfgCS_Static_Update() to trigger call to load new changes.
// Requires: ModModcfgCS_Init() from comp_modcfg_cs.c
MOD_FUNCTION_LOCAL( AddModConfigInfo, void, ( PREFIX1 char *info ),
		( PREFIX2 info ), )

// Allows mods to add values to the client state info shared with the engine.
// Requires: ModGameInfo_Init() from feat_game_info.c
MOD_FUNCTION_LOCAL( AddGameInfoClient, void, ( PREFIX1 int clientNum, info_string_t *info ),
		( PREFIX2 clientNum, info ), )

// Performs processing/conversion of player model to fit mod requirements.
// Writes empty string to use random model instead.
// Requires: ModModelSelection_Init() from comp_model_selection.c
MOD_FUNCTION_LOCAL( ConvertPlayerModel, void, ( PREFIX1 int clientNum, const char *userinfo, const char *source_model,
		char *output, unsigned int outputSize ),
		( PREFIX2 clientNum, userinfo, source_model, output, outputSize ), )

// Generates a random model which meets mod requirements. Called when convert function returns empty string.
// Requires: ModModelSelection_Init() from comp_model_selection.c
MOD_FUNCTION_LOCAL( RandomPlayerModel, void, ( PREFIX1 int clientNum, const char *userinfo, char *output,
		unsigned int outputSize ),
		( PREFIX2 clientNum, userinfo, output, outputSize ), )

// Prints a message when player is blocked from joining team by join limit module.
// type will be CJA_SETTEAM or CJA_SETCLASS (not called for other types).
// Requires: ModJoinLimit_Init() from comp_join_limit.c
MOD_FUNCTION_LOCAL( JoinLimitMessage, void, ( PREFIX1 int clientNum, join_allowed_type_t type, team_t targetTeam ),
		( PREFIX2 clientNum, type, targetTeam ), )

// Allows configuring forcefield behavior parameters.
// Requires: ModForcefield_Init() from comp_forcefield.c
MOD_FUNCTION_LOCAL( ForcefieldConfig, void, ( PREFIX1 modForcefield_config_t *config ),
		( PREFIX2 config ), )

// Returns action to take due to player touching forcefield.
// Can be called with clientNum -1 and forcefield NULL, as a test for info message purposes.
// Requires: ModForcefield_Init() from comp_forcefield.c
MOD_FUNCTION_LOCAL( ForcefieldTouchResponse, modForcefield_touchResponse_t,
		( PREFIX1 forcefieldRelation_t relation, int clientNum, gentity_t *forcefield ),
		( PREFIX2 relation, clientNum, forcefield ), return )

// Determine whether portable transporter activates borg teleport mode or regular teleport.
// Requires: ModHoldableTransporter_Init() from comp_holdable_transporter.c
MOD_FUNCTION_LOCAL( BorgTeleportEnabled, qboolean, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum ), return )

// Called when borg teleport completes.
// Requires: ModHoldableTransporter_Init() from comp_holdable_transporter.c
MOD_FUNCTION_LOCAL( PostBorgTeleport, void, ( PREFIX1 int clientNum ),
		( PREFIX2 clientNum), )

// Called to determine alt fire mode for given weapon.
// Valid return values are the server alt swap codes in cg_weapons.c (u, n, s, N, S, f, F)
// Call ModModcfgCS_Static_Update when values might have changed to trigger update.
// Requires: ModAltFireConfig_Init() from comp_alt_fire_config.c
MOD_FUNCTION_LOCAL( AltFireConfig, char, ( PREFIX1 weapon_t weapon ),
		( PREFIX2 weapon ), return )

// Allows configuring the parameters for when the game exits from intermission to next map.
// Call ModIntermissionReady_Shared_UpdateConfig() to trigger call to load new changes.
// Requires: ModIntermissionReady_Init() from comp_intermission_ready.c
MOD_FUNCTION_LOCAL( IntermissionReadyConfig, void, ( PREFIX1 modIntermissionReady_config_t *config ),
		( PREFIX2 config ), )

// Retrieves sequence of messages or events to play during warmup. Returns qtrue and writes
// sequence if enabled. Returns qfalse if disabled.
// Requires: ModWarmupSequence_Init() from comp_warmup_sequence.c
MOD_FUNCTION_LOCAL( GetWarmupSequence, qboolean, ( PREFIX1 modWarmupSequence_t *sequence ),
		( PREFIX2 sequence ), return )

#undef PREFIX1
#undef PREFIX2
#undef VOID1
#undef VOID2
#undef MOD_FUNCTION_DEF
#undef MOD_FUNCTION_LOCAL
