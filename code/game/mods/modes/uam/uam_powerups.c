/*
* UAM Powerups
* 
* Configure powerups such as forcefield, detpack, seeker, and quad consistent with Gladiator mod.
*/

#define MOD_NAME ModUAMPowerups

#include "mods/modes/uam/uam_local.h"

static struct {
	// 0 = non-killing, 1 = killing with spawn delay, 2 = killing
	trackedCvar_t g_mod_useKillingForcefield;
} *MOD_STATE;

#define FORCEFIELD_KILL_ENABLED ( MOD_STATE->g_mod_useKillingForcefield.integer != 0 )
#define FORCEFIELD_SPAWN_DELAY_ENABLED ( MOD_STATE->g_mod_useKillingForcefield.integer == 1 )
#define GAMETYPE g_gametype.integer

/*
================
(ModFN) ForcefieldConfig
================
*/
static void MOD_PREFIX(ForcefieldConfig)( MODFN_CTV, modForcefield_config_t *config ) {
	MODFN_NEXT( ForcefieldConfig, ( MODFN_NC, config ) );

	config->killForcefieldFlicker = qtrue;
	config->killPlayerSizzle = qtrue;
	config->quickPass = qtrue;
	config->bounceProjectiles = qtrue;

	// Health
	config->invulnerable = FORCEFIELD_KILL_ENABLED && GAMETYPE != GT_CTF;
	config->health = GAMETYPE == GT_CTF ? 500 : 250;	// bit more health in ctf

	// Touch effects
	config->touchEnemyResponse = FORCEFIELD_KILL_ENABLED ? FFTR_KILL : FFTR_BLOCK;
	if ( GAMETYPE == GT_CTF ) {
		// Use pass through forcefields in CTF to enable more strategic uses around flags;
		// otherwise it feels like you cut off your own team too much.
		config->touchFriendResponse = FFTR_PASS;
	} else if ( GAMETYPE == GT_TEAM ) {
		config->touchFriendResponse = FFTR_BLOCK;
	} else {
		config->touchFriendResponse = FORCEFIELD_KILL_ENABLED ? FFTR_KILL : FFTR_BLOCK;
	}

	// Delay mode
	config->activateDelayMode = FORCEFIELD_SPAWN_DELAY_ENABLED;

	// Sound effects
	config->loopSound = FORCEFIELD_KILL_ENABLED ?
		"sound/ambience/borg/borgzappers.wav" : "sound/movers/doors/voyfield.wav";
	config->alternateExpireSound = FORCEFIELD_KILL_ENABLED;
	config->alternatePassSound = !FORCEFIELD_KILL_ENABLED;
	config->gladiatorAnnounce = modcfg.mods_enabled.elimination && FORCEFIELD_KILL_ENABLED;
}

/*
================
(ModFN) AdjustModConstant
================
*/
int MOD_PREFIX(AdjustModConstant)( MODFN_CTV, modConstant_t mcType, int defaultValue ) {
	switch ( mcType ) {
		case MC_DETPACK_ORIGIN_PATCH:
			return 1;
		case MC_DETPACK_PING_SOUND:
			return 1;
		case MC_DETPACK_DROP_ON_DEATH:
			return 1;
		case MC_DETPACK_GLADIATOR_ANNOUNCEMENTS:
			if ( modcfg.mods_enabled.elimination ) {
				return 1;
			}
			break;

		case MC_SEEKER_MOD_TYPE:
			return MOD_QUANTUM_ALT;
		case MC_SEEKER_SECONDS_PER_SHOT:
			return 3;

		case MC_QUAD_EFFECTS_ENABLED:
			return 1;
	}

	return MODFN_NEXT( AdjustModConstant, ( MODFN_NC, mcType, defaultValue ) );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	switch ( gcType ) {
		case GC_DECOY_SOUND_EFFECTS:
			return 1;
		case GC_DETPACK_NO_SHOCKWAVE:
			// No detpack knockback after match is over
			if ( modcfg.mods_enabled.elimination && level.matchState >= MS_INTERMISSION_QUEUED ) {
				return 1;
			}
			break;
	}

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
================
ModUAMPowerups_Init
================
*/
void ModUAMPowerups_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_mod_useKillingForcefield, "g_mod_useKillingForcefield", "2", 0, qfalse );

		// Load dependencies
		ModForcefield_Init();
		ModDetpack_Init();
		ModSeeker_Init( qtrue );
		ModQuadEffects_Init();

		MODFN_REGISTER( ForcefieldConfig, ++modePriorityLevel );
		MODFN_REGISTER( AdjustModConstant, ++modePriorityLevel );
		MODFN_REGISTER( AdjustGeneralConstant, ++modePriorityLevel );
	}
}
