/*
* Seeker Drone
* 
* This module implements additional seeker functionality configured by mod constants.
*/

#define MOD_NAME ModSeeker

#include "mods/g_mod_local.h"

typedef struct {
	int secondsRemaining;
} modSeeker_client_t;

static struct {
	modSeeker_client_t clients[MAX_CLIENTS];
} *MOD_STATE;

#define SECONDS_PER_SHOT modfn_lcl.AdjustModConstant( MC_SEEKER_SECONDS_PER_SHOT, 1 )
#define MOD_TYPE modfn_lcl.AdjustModConstant( MC_SEEKER_MOD_TYPE, MOD_SCAVENGER )
#define ACCELERATOR_MODE modfn_lcl.AdjustModConstant( MC_SEEKER_ACCELERATOR_MODE, 0 )

/*
================
ModSeeker_FireSeeker
================
*/
static void ModSeeker_FireSeeker( gentity_t *owner, gentity_t *target, vec3_t origin ) {
	vec3_t dir;
	VectorSubtract( target->r.currentOrigin, origin, dir );
	VectorNormalize( dir );

	G_SetQuadFactor( owner - g_entities );
	if ( MOD_TYPE == MOD_QUANTUM_ALT ) {
		FireQuantumBurstAlt( owner, origin, dir );

		// Use sound event consistent with Gladiator behavior, although it's possible
		// EV_ALT_FIRE with s.weapon parameter could be an option here as well.
		G_Sound( owner, G_SoundIndex( "sound/weapons/quantum/alt_fire.wav" ) );
	} else {
		FireScavengerBullet( owner, origin, dir );
		G_AddEvent( owner, EV_POWERUP_SEEKER_FIRE, 0 );
	}
}

/*
================
(ModFN) CheckFireSeeker
================
*/
static void MOD_PREFIX(CheckFireSeeker)( MODFN_CTV, int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];
	modSeeker_client_t *modclient = &MOD_STATE->clients[clientNum];

	// Count down number of seconds remaining
	if ( modclient->secondsRemaining > 0 ) {
		--modclient->secondsRemaining;
	}
	if ( modclient->secondsRemaining > 0 ) {
		return;
	}

	// Check if seeker disabled
	if ( SECONDS_PER_SHOT <= 0 ) {
		return;
	}

	if ( client->ps.powerups[PW_SEEKER] ) {
		vec3_t seekerPos;

		if ( SeekerAcquiresTarget( ent, seekerPos ) ) // set the client's enemy to a valid target
		{
			modclient->secondsRemaining = SECONDS_PER_SHOT;
			ModSeeker_FireSeeker( ent, ent->enemy, seekerPos );
		}
	}
}

/*
================
(ModFN) PostFireProjectile
================
*/
static void MOD_PREFIX(PostFireProjectile)( MODFN_CTV, gentity_t *projectile ) {
	MODFN_NEXT( PostFireProjectile, ( MODFN_NC, projectile ) );

	// If accelerator mode is enabled, seeker causes certain fired weapon projectiles to move extra
	// fast and have different graphics.
	if ( projectile->inuse && ACCELERATOR_MODE ) {
		int clientNum = projectile->parent - g_entities;
		if ( G_AssertConnectedClient( clientNum ) && level.clients[clientNum].ps.powerups[PW_SEEKER] ) {
			if ( projectile->methodOfDeath == MOD_QUANTUM || projectile->methodOfDeath == MOD_GRENADE ) {
				projectile->s.eType = ET_ALT_MISSILE;
				projectile->s.weapon = WP_QUANTUM_BURST;
			}

			// Note this scaling is currently done after SnapVector, which is probably not
			// technically ideal but for this case the difference should be negligible.

			if ( projectile->methodOfDeath == MOD_QUANTUM ) {
				VectorScale( projectile->s.pos.trDelta, 2.5f, projectile->s.pos.trDelta );
			}

			if ( projectile->methodOfDeath == MOD_GRENADE || projectile->methodOfDeath == MOD_GRENADE_ALT ) {
				VectorScale( projectile->s.pos.trDelta, 2.0f, projectile->s.pos.trDelta );
			}
		}
	}
}

/*
================
ModSeeker_Init

Call with registerPhoton set if there is a possibility photonAltMode will be enabled
at some point, to ensure the weapon is loaded on clients.
================
*/
void ModSeeker_Init( qboolean registerPhoton ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( CheckFireSeeker, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostFireProjectile, MODPRIORITY_GENERAL );
	}

	if ( registerPhoton ) {
		RegisterItem( BG_FindItemForWeapon( WP_QUANTUM_BURST ) );
	}
}
