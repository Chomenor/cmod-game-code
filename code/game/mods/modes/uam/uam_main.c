/*
* UAM Mode
* 
* "Unlimited Ammo Mode" is a recreation of the Gladiator Arena mod by Lt. Cmdr. Salinga.
* 
* Setting g_mod_uam to 1 enables the traditional Elimination mode from Gladiator.
* Setting g_mod_uam to 2 enables Gladiator-style weapons and effects without Elimination.
*/

#define MOD_NAME ModUAMMain

#include "mods/modes/uam/uam_local.h"

static struct {
	qboolean instagib;
} *MOD_STATE;

/*
============
(ModFN) GetDamageMult

Reduce damage amount scaled by number of players.
============
*/
static float MOD_PREFIX(GetDamageMult)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod, qboolean knockbackMode ) {
	float factor = 1.0f;

	if ( !knockbackMode && !MOD_STATE->instagib ) {
		if ( modcfg.mods_enabled.elimination ) {
			// Standard gladiator multiplier
			int numPlayers = ModElimination_Static_CountPlayersAlive();
			if ( numPlayers < 2 )
				numPlayers = 2;
			if ( numPlayers > 6 )
				numPlayers = 6;

			factor = 0.7f * numPlayers;
		} else {
			// CTF and basic FFA - higher damage factor
			int numPlayers = level.numPlayingClients;
			if ( numPlayers < 2 )
				numPlayers = 2;
			if ( numPlayers > 6 )
				numPlayers = 6;

			factor = 1.0f + 0.2f * numPlayers;
		}
	}

	return MODFN_NEXT( GetDamageMult, ( MODFN_NC, targ, inflictor, attacker, dir, point,
			damage, dflags, mod, knockbackMode ) ) / factor;
}

/*
===========
(ModFN) SpawnCenterPrintMessage

Don't display "elimination" spawn message.
============
*/
static void MOD_PREFIX(SpawnCenterPrintMessage)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
}

/*
===========
(ModFN) PostClientSpawn

Add information print on initial connect similar to original Gladiator mod.
============
*/
static void MOD_PREFIX(PostClientSpawn)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( PostClientSpawn, ( MODFN_NC, clientNum, spawnType ) );

	if ( spawnType == CST_FIRSTTIME ) {
		char message[1024];

		message[0] = '\0';
		Q_strcat( message, sizeof( message ), "\n" );
		Q_strcat( message, sizeof( message ), va( "- Forcefield-Kill is %s.\n",
				ModForcefield_Static_KillingForcefieldEnabled() ? "enabled" : "off" ) );
		Q_strcat( message, sizeof( message ), va( "- Unlagged is %s.\n",
				ModPingcomp_Static_PingCompensationEnabled() ? "enabled" : "off" ) );
		if ( ModElimMultiRound_Static_GetMultiRoundEnabled() ) {
			Q_strcat( message, sizeof( message ), va( "- Match Mode: Round %i of %i is running.\n",
					ModElimMultiRound_Static_GetCurrentRound(), ModElimMultiRound_Static_GetTotalRounds() ) );
		}
		trap_SendServerCommand( clientNum, va( "print \"%s\"", message ) );

		trap_SendServerCommand( clientNum, "print \"\n\"" );

		message[0] = '\0';
		Q_strcat( message, sizeof( message ), "Welcome to ^1Unlimited Ammo Mode Arena^7! "
				"^0(cMod " __DATE__ ")^7\nBased on Gladiator Arena by Lt. Cmdr. Salinga\n" );
		if ( *g_motd.string ) {
			Q_strcat( message, sizeof( message ), va( "- %s\n", g_motd.string ) );
		}
		trap_SendServerCommand( clientNum, va( "print \"%s\"", message ) );
	}
}

/*
==================
(ModFN) SetSpawnCS

If map message field is empty, add UAM message in place of it.
==================
*/
static void MOD_PREFIX(SetSpawnCS)( MODFN_CTV, int num, const char *defaultValue ) {
	if ( num == CS_MESSAGE && !*defaultValue ) {
		trap_SetConfigstring( num, va( "^4Prepare to enter: ^1U.A.M. Arena%s",
				ModPingcomp_Static_PingCompensationEnabled() ? " ^3(unlagged)" : "" ) );
	} else {
		MODFN_NEXT( SetSpawnCS, ( MODFN_NC, num, defaultValue ) );
	}
}

/*
==================
(ModFN) ItemDropExpireTime

Dropped items don't expire in Elimination mode.
==================
*/
static int MOD_PREFIX(ItemDropExpireTime)( MODFN_CTV, const gitem_t *item ) {
	if ( modcfg.mods_enabled.elimination ) {
		return 0;
	} else {
		return MODFN_NEXT( ItemDropExpireTime, ( MODFN_NC, item ) );
	}
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	switch ( gcType ) {
		case GC_INTERMISSION_DELAY_TIME:
			if ( modcfg.mods_enabled.elimination ) {
				return 5000;
			}
			break;
		case GC_CHAT_HIT_WARNING:
			return 1;
		case GC_FORCE_TEAM_PODIUM:
			if ( !ModElimMultiRound_Static_GetIsFinalScores() ) {
				return 1;
			}
			break;
		case GC_TOSS_ITEMS_ON_SUICIDE:
			return 1;
	}

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
==================
(ModFN) AdjustModConstant
==================
*/
int MOD_PREFIX(AdjustModConstant)( MODFN_CTV, modConstant_t mcType, int defaultValue ) {
	switch ( mcType ) {
		case MC_BOTADD_PER_TEAM_COUNT:
			// bot_minplayers specified the total target number of players, not per-team
			return 0;
		case MC_BOTADD_PER_TEAM_ADJUST:
			// add/remove bots one by one, not both teams at once
			return 1;
		case MC_BOTADD_IGNORE_SPECTATORS:
			// in UAM FFA elimination, spectators do cause bots to be kicked by default
			if ( modcfg.mods_enabled.elimination ) {
				return 0;
			}
			break;

		case MC_GHOST_SPARKLE_ENABLED:
			// enable spawn invulnerability effect
			if ( modcfg.mods_enabled.elimination ) {
				return 1;
			}
			break;
	}

	return MODFN_NEXT( AdjustModConstant, ( MODFN_NC, mcType, defaultValue ) );
}

/*
============
(ModFN) CanItemBeDropped

In UAM both powerup and holdable drops are fully enabled in both teams and ffa.
============
*/
static qboolean MOD_PREFIX(CanItemBeDropped)( MODFN_CTV, gitem_t *item, int clientNum ) {
	if ( item->giType == IT_POWERUP || item->giType == IT_HOLDABLE ) {
		return qtrue;
	}

	return MODFN_NEXT( CanItemBeDropped, ( MODFN_NC, item, clientNum ) );
}

/*
==============
(ModFN) CheckRespawnTime

In UAM the delay to respawn eliminated players to spectator is a bit longer.
==============
*/
static qboolean MOD_PREFIX(CheckRespawnTime)( MODFN_CTV, int clientNum, qboolean voluntary ) {
	if ( modcfg.mods_enabled.elimination && level.matchState >= MS_ACTIVE ) {
		gclient_t *client = &level.clients[clientNum];

		if ( voluntary && level.time > client->respawnKilledTime + 3000 ) {
			return qtrue;
		}

		if ( !voluntary && level.time > client->respawnKilledTime + 4300 ) {
			return qtrue;
		}

		return qfalse;
	}

	return MODFN_NEXT( CheckRespawnTime, ( MODFN_NC, clientNum, voluntary ) );
}

/*
================
(ModFN) GeneralInit

Check for warning prints.
================
*/
static void MOD_PREFIX(GeneralInit)( MODFN_CTV ) {
	MODFN_NEXT( GeneralInit, ( MODFN_NC ) );

	if ( g_gametype.integer != GT_CTF && g_speed.integer < 320 ) {
		G_DedPrintf( "NOTE: Recommend setting 'g_speed' to at least 320 for UAM mode.\n" );
	}
	if ( modcfg.mods_enabled.elimination && !g_doWarmup.integer ) {
		G_DedPrintf( "NOTE: Recommend setting 'g_doWarmup' to 1 for UAM mode.\n" );
	}
	if ( g_holoIntro.integer ) {
		G_DedPrintf( "NOTE: Recommend setting 'g_holoIntro' to 0 for UAM mode.\n" );
	}
	if ( !trap_Cvar_VariableIntegerValue( "g_unlagged" ) ) {
		G_DedPrintf( "NOTE: Recommend setting 'g_unlagged' to at least 1 for UAM mode.\n" );
	}
}

/*
================
ModUAM_Init
================
*/
void ModUAM_Init( void ) {
	if ( !EF_WARN_ASSERT( !MOD_STATE ) ) {
		return;
	}

	MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );
	modcfg.mods_enabled.uam = qtrue;

	if ( trap_Cvar_VariableIntegerValue( "g_gametype" ) != 4 && trap_Cvar_VariableIntegerValue( "g_mod_uam" ) != 2 ) {
		// Load elimination configuration
		ModElimination_Init();
	}

	// Death effects and non-disappearing bodies configuration
	// Needs to be called ahead of instagib init so it doesn't override instagib effects.
	ModUAMDeathEffects_Init();

	// Weapon configuration
	if ( trap_Cvar_VariableIntegerValue( "g_mod_instagib" ) ) {
		ModUAMInstagib_Init();
		MOD_STATE->instagib = qtrue;
	} else {
		// Wait until next round to process weapon cvar changes in elimination
		ModUAMWeapons_Init( !modcfg.mods_enabled.elimination );
	}

	// Enabled items configuration
	ModGladiatorItemEnable_Init();

	// Powerup configuration
	ModUAMPowerups_Init();

	// Warmup sequence configuration
	if ( modcfg.mods_enabled.elimination ) {
		ModUAMWarmupSequence_Init();
	}

	// Extra sound effects configuration
	ModUAMGreeting_Init();
	ModUAMWeaponEffects_Init();

	// Elimination-specific configuration
	if ( modcfg.mods_enabled.elimination ) {
		ModTimelimitCountdown_Init();
		ModUAMMusic_Init();
		ModUAMWinnerTaunt_Init();
		ModEndSound_Init();
		ModGhostSparkle_Init();
	}

	ModBotAdding_Init();

	// Set most info cvars to empty so they aren't included in serverinfo
	trap_Cvar_Set( "g_mod_uam", modcfg.mods_enabled.elimination ? "1" : "2" );
	trap_Cvar_Set( "g_mod_instagib", MOD_STATE->instagib ? "1" : "0" );
	trap_Cvar_Set( "g_pModElimination", "" );
	trap_Cvar_Set( "g_pModAssimilation", "" );
	trap_Cvar_Set( "g_pModActionHero", "" );
	trap_Cvar_Set( "g_pModDisintegration", "" );
	trap_Cvar_Set( "g_pModSpecialties", "" );

	// Set mod name for server browsers
	trap_Cvar_Set( "gamename", "UAM" );

	MODFN_REGISTER( GetDamageMult, ++modePriorityLevel );
	MODFN_REGISTER( SpawnCenterPrintMessage, ++modePriorityLevel );
	MODFN_REGISTER( PostClientSpawn, ++modePriorityLevel );
	MODFN_REGISTER( SetSpawnCS, ++modePriorityLevel );
	MODFN_REGISTER( ItemDropExpireTime, ++modePriorityLevel );
	MODFN_REGISTER( AdjustGeneralConstant, ++modePriorityLevel );
	MODFN_REGISTER( AdjustModConstant, ++modePriorityLevel );
	MODFN_REGISTER( CanItemBeDropped, ++modePriorityLevel );
	MODFN_REGISTER( CheckRespawnTime, ++modePriorityLevel );
	MODFN_REGISTER( GeneralInit, ++modePriorityLevel );
}
