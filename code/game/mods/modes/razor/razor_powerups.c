/*
* Razor Powerups
* 
* Configure powerups such as forcefield, detpack, seeker, and quad consistent with Pinball mod.
*/

#define MOD_NAME ModRazorPowerups

#include "mods/modes/razor/razor_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
============
(ModFN) RadiusDamage
============
*/
static qboolean MOD_PREFIX(RadiusDamage)( MODFN_CTV, vec3_t origin, gentity_t *attacker,
		float damage, float radius, gentity_t *ignore, int dflags, int mod ) {
	if ( mod == MOD_DETPACK ) {
		damage *= 2.0f;
	}
	return MODFN_NEXT( RadiusDamage, ( MODFN_NC, origin, attacker, damage, radius, ignore, dflags, mod ) );
}

/*
================
(ModFN) DetpackExplodeEffects
================
*/
static void MOD_PREFIX(DetpackExplodeEffects)( MODFN_CTV, gentity_t *detpack ) {
	G_TempEntity( detpack->s.pos.trBase, EV_DISINTEGRATION2 );
	G_Sound( detpack, G_SoundIndex( "sound/weapons/explosions/explode5.wav" ) );
	G_GlobalSound( G_SoundIndex( "sound/weapons/explosions/explode12.wav" ) );
}

/*
================
(ModFN) ForcefieldConfig
================
*/
static void MOD_PREFIX(ForcefieldConfig)( MODFN_CTV, modForcefield_config_t *config ) {
	MODFN_NEXT( ForcefieldConfig, ( MODFN_NC, config ) );

	// health
	config->invulnerable = qtrue;

	// killing
	config->killForcefieldFlicker = qtrue;
	config->killPlayerSizzle = qtrue;

	// sound effects
	config->alternateExpireSound = qtrue;
	config->loopSound = "sound/ambience/borg/borgzappers.wav";
}

/*
================
(ModFN) ForcefieldTouchResponse
================
*/
static modForcefield_touchResponse_t MOD_PREFIX(ForcefieldTouchResponse)(
		MODFN_CTV, forcefieldRelation_t relation, int clientNum, gentity_t *forcefield ) {
	if ( relation == FFR_TEAM ) {
		if ( g_friendlyFire.integer ) {
			return FFTR_KILL;
		}
		if ( clientNum >= 0 && ModRazorScoring_Static_LastPusher( clientNum ) >= 0 ) {
			return FFTR_KILL;
		}
		return FFTR_BLOCK;
	} else {
		return FFTR_KILL;
	}
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
		case MC_DETPACK_DROP_ON_DEATH:
			return 1;
		case MC_DETPACK_INVULNERABLE:
			return 1;

		case MC_SEEKER_ACCELERATOR_MODE:
			return 1;
		case MC_SEEKER_SECONDS_PER_SHOT:
			return 0;

		case MC_QUAD_EFFECTS_ENABLED:
			return 1;
		case MC_QUAD_EFFECTS_PINBALL_STYLE:
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
		case GC_SEEKER_PICKUP_SOUND:
			// In original Pinball this might have depended on client 0 location.
			return 1;
		}

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
================
(ModFN) GeneralInit
================
*/
static void MOD_PREFIX(GeneralInit)( MODFN_CTV ) {
	MODFN_NEXT( GeneralInit, ( MODFN_NC ) );

	// Disable 'target_remove_powerups' for most maps, because it is often placed ahead of
	// kill area triggers and interferes with both gold armor teleport and powerup drops.
	if ( Q_stricmp( G_ModUtils_GetMapName(), "mock-arena" ) ) {
		gentity_t *trpEntity = NULL;
		while ( ( trpEntity = G_Find( trpEntity, FOFS( classname ), "target_remove_powerups" ) ) != NULL ) {
			G_Printf( "razor: Disabling target_remove_powerups '%s'\n",
					trpEntity->targetname ? trpEntity->targetname : "<null>" );
			trpEntity->use = NULL;
		}
	}
}

/*
================
ModRazorPowerups_Init
================
*/
void ModRazorPowerups_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		// Load seeker configuration
		ModSeeker_Init( qfalse );

		ModForcefield_Init();
		ModDetpack_Init();
		ModQuadEffects_Init();

		MODFN_REGISTER( RadiusDamage, ++modePriorityLevel );
		MODFN_REGISTER( DetpackExplodeEffects, ++modePriorityLevel );
		MODFN_REGISTER( ForcefieldConfig, ++modePriorityLevel );
		MODFN_REGISTER( ForcefieldTouchResponse, ++modePriorityLevel );
		MODFN_REGISTER( AdjustModConstant, ++modePriorityLevel );
		MODFN_REGISTER( AdjustGeneralConstant, ++modePriorityLevel );
		MODFN_REGISTER( GeneralInit, ++modePriorityLevel );
	}
}
