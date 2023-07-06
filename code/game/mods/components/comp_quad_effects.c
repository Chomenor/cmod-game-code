/*
* Quad Effects
* 
* Add additional effects when a quadded weapon hits something.
* 
* Enabled by MC_QUAD_EFFECTS_ENABLED mod constant.
*/

#define MOD_NAME ModQuadEffects

#include "mods/g_mod_local.h"

static struct {
	char quadProjectile[MAX_GENTITIES];
} *MOD_STATE;

#define QUAD_EFFECTS_ENABLED modfn_lcl.AdjustModConstant( MC_QUAD_EFFECTS_ENABLED, 0 )
#define PINBALL_STYLE modfn_lcl.AdjustModConstant( MC_QUAD_EFFECTS_PINBALL_STYLE, 0 )

/*
================
QuadEffects_ActiveForProjectile

Check if quad effects are enabled for the given projectile entity.
================
*/
static qboolean QuadEffects_ActiveForProjectile( gentity_t *ent ) {
	int entityNum = ent - g_entities;
	int parentNum;

	// Check table of fired quad projectiles
	if ( !EF_WARN_ASSERT( entityNum >= 0 && entityNum < MAX_GENTITIES ) ||
			!MOD_STATE->quadProjectile[entityNum] ) {
		return qfalse;
	}

	// Check for case where projectile was fired by shooter entity rather than player.
	// These currently don't call PostFireProjectile so can't rely on the table.
	parentNum = level.gentities[entityNum].parent - level.gentities;
	if ( parentNum < 0 || parentNum >= level.maxclients ) {
		return qfalse;
	}

	return qtrue;
}

/*
======================
(ModFN) AddWeaponEffect

Called for:
- instant hit weapons (ent is client)
- primary and alt grenades exploding by timer or tripmine (ent is projectile, trace is NULL)
- alt grenades exploding on impact (via G_MissileStick->grenadeSpewShrapnel) (ent is projectile, trace is NULL)
======================
*/
static void MOD_PREFIX(AddWeaponEffect)( MODFN_CTV, weaponEffect_t weType, gentity_t *ent, trace_t *trace ) {
	MODFN_NEXT( AddWeaponEffect, ( MODFN_NC, weType, ent, trace ) );

	if ( !QUAD_EFFECTS_ENABLED ) {
		return;
	}

	if ( weType == WE_RIFLE_ALT && ent->client->ps.powerups[PW_QUAD] ) {
		if ( PINBALL_STYLE ) {
			gentity_t *ev = G_TempEntity( trace->endpos, EV_GRENADE_SHRAPNEL_EXPLODE );
			ev->s.eventParm = DirToByte( trace->plane.normal );
		} else {
			G_TempEntity( trace->endpos, EV_EXPLODESHELL );
		}
	}

	if ( weType == WE_GRENADE_PRIMARY && QuadEffects_ActiveForProjectile( ent ) ) {
		vec3_t pos;
		VectorSet( pos, ent->r.currentOrigin[0], ent->r.currentOrigin[1], ent->r.currentOrigin[2] + 8 );
		G_TempEntity( pos, EV_DISINTEGRATION2 );
	}

	if ( weType == WE_GRENADE_ALT && QuadEffects_ActiveForProjectile( ent ) ) {
		if ( ent->s.pos.trType == TR_STATIONARY && ent->pos1[2] != -1.0f && ent->pos1[2] != 1.0f ) {
			// Mine stuck to non-flat surface
			G_TempEntity( ent->r.currentOrigin, EV_EXPLODESHELL );
		} else {
			G_TempEntity( ent->r.currentOrigin, EV_DISINTEGRATION2 );
		}
	}
}

/*
================
(ModFN) MissileImpact

Called for projectiles (including grenades) exploding on impact (not timer). Alt grenades (but not primary)
will get AddWeaponEffect called for them as well through MODFN_NEXT.
================
*/
static void MOD_PREFIX(MissileImpact)( MODFN_CTV, gentity_t *ent, trace_t *trace ) {
	MODFN_NEXT( MissileImpact, ( MODFN_NC, ent, trace ) );

	if ( !QUAD_EFFECTS_ENABLED ) {
		return;
	}
	if ( !ent->inuse ) {
		// Default MissileImpact function should leave entity active for events, but check this to be safe.
		return;
	}
	if ( ent->s.eType != ET_GENERAL ) {
		// Missile which hasn't exploded yet (such as a bouncing grenade)
		return;
	}

	if ( QuadEffects_ActiveForProjectile( ent ) &&
			( ent->methodOfDeath == MOD_QUANTUM || ent->methodOfDeath == MOD_QUANTUM_ALT ||
			( ent->methodOfDeath == MOD_GRENADE && PINBALL_STYLE ) ) ) {
		qboolean hit_alive_player = G_IsConnectedClient( trace->entityNum ) &&
				level.clients[trace->entityNum].ps.stats[STAT_HEALTH] > 0;

		// This check might not be 100% the same as gladiator, but it seems to match the general idea
		if ( hit_alive_player || trace->plane.normal[2] == 1.0f || trace->plane.normal[2] == -1.0f ) {
			ent->s.event = EV_DISINTEGRATION2;
			ent->s.eventParm = 0;
		} else {
			gentity_t *tent = G_TempEntity( ent->r.currentOrigin, EV_GRENADE_SHRAPNEL_EXPLODE );
			tent->s.eventParm = DirToByte( trace->plane.normal );

			ent->s.event = EV_MISSILE_HIT;
		}
	}
}

/*
======================
(ModFN) PostFireProjectile

Log when projectiles are fired with quad powerup.
======================
*/
static void MOD_PREFIX(PostFireProjectile)( MODFN_CTV, gentity_t *projectile ) {
	int entityNum = projectile - g_entities;
	MODFN_NEXT( PostFireProjectile, ( MODFN_NC, projectile ) );

	if ( EF_WARN_ASSERT( entityNum >= 0 && entityNum < MAX_GENTITIES ) ) {
		MOD_STATE->quadProjectile[entityNum] = 0;

		if ( projectile->inuse ) {
			int clientNum = projectile->parent - g_entities;
			if ( G_AssertConnectedClient( clientNum ) && level.clients[clientNum].ps.powerups[PW_QUAD] ) {
				MOD_STATE->quadProjectile[entityNum] = 1;
			}
		}
	}
}

/*
================
ModQuadEffects_Init
================
*/
void ModQuadEffects_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( AddWeaponEffect, ++modePriorityLevel );
		MODFN_REGISTER( MissileImpact, ++modePriorityLevel );
		MODFN_REGISTER( PostFireProjectile, ++modePriorityLevel );
	}
}
