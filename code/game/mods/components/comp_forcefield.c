/*
* Portable Forcefield
* 
* This module implements additional forcefield functionality, configured by the
* ForcefieldConfig mod function.
*/

#define MOD_NAME ModForcefield

#include "mods/g_mod_local.h"

static struct {
	modForcefield_config_t config;

	// For quick forcefield iteration.
	// Currently only updated when forcefields are added, not removed, so each entity must
	// be verified that it is still a forcefield when accessing.
	int forcefieldIndex[MAX_GENTITIES];
	int forcefieldIndexCount;

	// Debounce certain sounds so they don't get spammed too much.
	int lastSoundTime[MAX_GENTITIES];
} *MOD_STATE;

#define IS_FORCEFIELD( ent ) ( ( ent ) && ( ent )->inuse && ( ent )->s.eType == ET_USEABLE && \
		( ent )->s.modelindex == HI_SHIELD )
#define FF_HEALTH ( MOD_STATE->config.invulnerable ? 1000 : MOD_STATE->config.health )
#define FF_TOUCH_RESPONSE( isFriend ) ( ( isFriend ) ? MOD_STATE->config.touchFriendResponse : \
		MOD_STATE->config.touchEnemyResponse )

/*
================
(ModFN Default) ForcefieldConfig
================
*/
void ModFNDefault_ForcefieldConfig( modForcefield_config_t *config ) {
	config->health = 250;
	config->duration = 25;
	config->touchFriendResponse = FFTR_PASS;
	config->touchEnemyResponse = FFTR_BLOCK;
}

/*
================
ModForcefield_UpdateConfig
================
*/
static void ModForcefield_UpdateConfig( void ) {
	memset( &MOD_STATE->config, 0, sizeof( MOD_STATE->config ) );
	modfn_lcl.ForcefieldConfig( &MOD_STATE->config );
}

/*
================
ModForcefield_Static_KillingForcefieldEnabled

Returns whether killing forcefields are enabled, for info message purposes.
================
*/
qboolean ModForcefield_Static_KillingForcefieldEnabled( void ) {
	if ( MOD_STATE ) {
		ModForcefield_UpdateConfig();
		if ( MOD_STATE->config.touchEnemyResponse == FFTR_KILL || MOD_STATE->config.touchFriendResponse == FFTR_KILL ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
ModForcefield_DebounceSound

Returns qtrue and updates limit if sound can be played, qfalse otherwise.
================
*/
static qboolean ModForcefield_DebounceSound( int forcefieldEntitynum, int timer ) {
	int *lastTime = &MOD_STATE->lastSoundTime[forcefieldEntitynum];

	if ( *lastTime && level.time < *lastTime + timer ) {
		return qfalse;
	}

	*lastTime = level.time;
	return qtrue;
}

/*
================
ModForcefield_UpdateIndex
================
*/
static void ModForcefield_UpdateIndex( void ) {
	int i;

	MOD_STATE->forcefieldIndexCount = 0;
	for ( i = 0; i < MAX_GENTITIES; ++i ) {
		gentity_t *ent = &g_entities[i];
		if ( IS_FORCEFIELD( ent ) ) {
			MOD_STATE->forcefieldIndex[MOD_STATE->forcefieldIndexCount++] = i;
		}
	}
}

/*
================
ModForcefield_CountActive

Returns current number of forcefields in game.
================
*/
static int ModForcefield_CountActive( void ) {
	int i;
	int count = 0;

	for ( i = 0; i < MOD_STATE->forcefieldIndexCount; ++i ) {
		int entityNum = MOD_STATE->forcefieldIndex[i];
		gentity_t *ent = &g_entities[entityNum];

		if ( ent->think == G_FreeEntity ) {
			// Treat forcefields about to be freed as inactive for message purposes
			continue;
		}

		if ( IS_FORCEFIELD( ent ) ) {
			count++;
		}
	}

	return count;
}

/*
================
ModForcefield_ShieldTouch
================
*/
static void ModForcefield_ShieldTouch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	if ( other->client && G_AssertConnectedClient( other - g_entities ) ) {
		int clientNum = other - g_entities;
		forcefieldRelation_t relation = G_GetForcefieldRelation( clientNum, self );
		modForcefield_touchResponse_t response = FF_TOUCH_RESPONSE( relation != FFR_ENEMY );

		if ( response == FFTR_PASS ) {
			if ( MOD_STATE->config.quickPass ) {
				if ( ModForcefield_DebounceSound( self - g_entities, 50 ) ) {
					G_AddEvent( self, EV_GENERAL_SOUND, G_SoundIndex( "sound/movers/forceup.wav" ) );
				}
			} else {
				G_ShieldGoNotSolid( self );
			}
		}

		if ( response == FFTR_KILL ) {
			// Use the gladiator-style method to trigger forcefield kill. This does not kill through
			// invulnerability and increments PERS_HITS and PERS_SHIELDS on the forcefield owner
			// to play feedback sounds (although exactly what gets incrememented and the resulting
			// sound depends on several factors...)
			G_Damage( other, self, self->parent, NULL, NULL, 100000, 0, MOD_TRIGGER_HURT );

			//player_die( other, self, self->parent, 100000, MOD_TRIGGER_HURT );

			if ( MOD_STATE->config.killForcefieldFlicker ) {
				// Note: This spams sounds and slows down forcefield expiration if
				// forcefield is touched repeatedly by someone with invulnerability.
				self->s.eFlags |= EF_ITEMPLACEHOLDER;
				self->think = G_ShieldThink;
				self->nextthink = level.time + 1000;

				if ( ModForcefield_DebounceSound( self - g_entities, 50 ) ) {
					G_AddEvent( self, EV_GENERAL_SOUND, G_SoundIndex( "sound/ambience/spark5.wav" ) );
				}
			}
		}
	}
}

/*
================
ModForcefield_ActivateShield
================
*/
static void ModForcefield_ActivateShield( gentity_t *ent ) {
	G_ActivateShield( ent );

	// Override the ShieldTouch function
	EF_WARN_ASSERT( ent->touch == G_ShieldTouch );
	ent->touch = ModForcefield_ShieldTouch;

	if ( MOD_STATE->config.loopSound ) {
		ent->s.loopSound = G_SoundIndex( MOD_STATE->config.loopSound );
	}
}

/*
================
(ModFN) ForcefieldPlace
================
*/
static gentity_t *MOD_PREFIX(ForcefieldPlace)( MODFN_CTV, int clientNum ) {
	gentity_t *shield = MODFN_NEXT( ForcefieldPlace, ( MODFN_NC, clientNum ) );

	// Make sure config is updated (handle cvar changes etc.)
	ModForcefield_UpdateConfig();

	if ( shield ) {
		// Override the ActivateShield function
		EF_WARN_ASSERT( shield->think == G_ActivateShield );
		shield->think = ModForcefield_ActivateShield;

		ModForcefield_UpdateIndex();
	}

	return shield;
}

/*
================
(ModFN) ForcefieldSoundEvent
================
*/
static void MOD_PREFIX(ForcefieldSoundEvent)( MODFN_CTV, gentity_t *ent, forcefieldEvent_t event ) {
	if ( event == FSE_REMOVE && MOD_STATE->config.alternateExpireSound ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( "sound/movers/seedbeamoff.wav" ) );
		return;
	}

	if ( event == FSE_TEMP_DEACTIVATE && MOD_STATE->config.alternatePassSound ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( "sound/enemies/species8472/8472out.wav" ) );
		return;
	}

	if ( event == FSE_CREATE && MOD_STATE->config.activateDelayMode ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( "sound/ambience/dreadnaught/dnlcarpattern5.wav" ) );
		return;
	}

	MODFN_NEXT( ForcefieldSoundEvent, ( MODFN_NC, ent, event ) );
}

/*
================
(ModFN) ForcefieldAnnounce
================
*/
static void MOD_PREFIX(ForcefieldAnnounce)( MODFN_CTV, gentity_t *forcefield, forcefieldEvent_t event ) {
	if ( MOD_STATE->config.gladiatorAnnounce ) {
		if ( event == FSE_ACTIVATE ) {
			int count = ModForcefield_CountActive();
			if ( count <= 1 ) {
				G_GlobalSound( G_SoundIndex( "sound/voice/biessman/misc/goingup.wav" ) );
				trap_SendServerCommand( -1, "cp \"^1forcefield placed\"" );
			} else {
				G_GlobalSound( G_SoundIndex( "sound/voice/biessman/forge2/morefields.wav" ) );
				trap_SendServerCommand( -1, va( "cp \"^1forcefield no.%i placed\"", count ) );
			}
		}

		if ( event == FSE_REMOVE ) {
			int count = ModForcefield_CountActive();
			G_GlobalSound( G_SoundIndex( "sound/voice/biessman/misc/goingdown.wav" ) );
			if ( count <= 0 ) {
				trap_SendServerCommand( -1, "cp \"^2forcefield deactivated\n\"" );
			} else {
				trap_SendServerCommand( -1, va( "cp \"^2forcefield deactivated\n^2%i active left\"", count ) );
			}
		}

		// Suppress regular message
		return;
	}

	MODFN_NEXT( ForcefieldAnnounce, ( MODFN_NC, forcefield, event ) );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_FORCEFIELD_ACTIVATE_DELAY && MOD_STATE->config.activateDelayMode ) {
		return 1750;
	}
	if ( gcType == GC_FORCEFIELD_HEALTH_TOTAL ) {
		return FF_HEALTH;
	}
	if ( gcType == GC_FORCEFIELD_HEALTH_DECREMENT ) {
		if ( EF_WARN_ASSERT( MOD_STATE->config.duration > 0 ) ) {
			return FF_HEALTH / MOD_STATE->config.duration;
		}
	}
	if ( gcType == GC_FORCEFIELD_TAKE_DAMAGE ) {
		// Damage to invulnerable forcefields is already blocked via ModifyDamageFlags.
		// This is currently just used as a way to determine whether grenades/tetrion alt
		// projectiles are bounced via "other->takedamage" check in MissileImpact.
		return MOD_STATE->config.invulnerable && MOD_STATE->config.bounceProjectiles ? 0 : 1;
	}

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
==================
(ModFN) PlayerDeathEffect

Play specific effects when player is killed by a forcefield.
==================
*/
static void MOD_PREFIX(PlayerDeathEffect)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int contents, int killer, int meansOfDeath ) {
	if ( IS_FORCEFIELD( inflictor ) ) {
		// Don't treat as gibbed
		self->health = 0;

		if ( MOD_STATE->config.killPlayerSizzle ) {
			// Note: This requires welder weapon to be loaded on client for full sound effects.
			G_AddEvent( self, EV_ARCWELD_DISINT, killer );
			self->client->ps.powerups[PW_ARCWELD_DISINT] = level.time + 100000;
			VectorClear( self->client->ps.velocity );
			self->takedamage = qfalse;
			self->r.contents = 0;

			G_Sound( self, G_SoundIndex( "sound/world/electro.wav" ) );
		}

	} else {
		MODFN_NEXT( PlayerDeathEffect, ( MODFN_NC, self, inflictor, attacker, contents, killer, meansOfDeath ) );
	}
}

/*
============
(ModFN) ModifyDamageFlags

Block damage to invulnerable forcefields.
============
*/
static int MOD_PREFIX(ModifyDamageFlags)( MODFN_CTV, gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
		vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	dflags = MODFN_NEXT( ModifyDamageFlags, ( MODFN_NC, targ, inflictor, attacker, dir, point, damage, dflags, mod ) );

	if ( MOD_STATE->config.invulnerable && IS_FORCEFIELD( targ ) ) {
		dflags |= DAMAGE_TRIGGERS_ONLY;
	}

	return dflags;
}

/*
==============
(ModFN) RunPlayerMove

If quickpass is enabled, move friendly forcefields out of the way during player movement.
==============
*/
static void MOD_PREFIX(RunPlayerMove)( MODFN_CTV, int clientNum ) {
	if ( MOD_STATE->config.quickPass ) {
		int i;
		int modifiedEntities[MAX_GENTITIES];
		int oldContents[MAX_GENTITIES];
		int modifiedEntityCount = 0;

		for ( i = 0; i < MOD_STATE->forcefieldIndexCount; ++i ) {
			int entityNum = MOD_STATE->forcefieldIndex[i];
			gentity_t *ent = &g_entities[entityNum];

			if ( IS_FORCEFIELD( ent ) ) {
				forcefieldRelation_t relation = G_GetForcefieldRelation( clientNum, ent );
				modForcefield_touchResponse_t response = FF_TOUCH_RESPONSE( relation != FFR_ENEMY );
				if ( response == FFTR_PASS ) {
					// Save old contents
					oldContents[modifiedEntityCount] = ent->r.contents;
					modifiedEntities[modifiedEntityCount] = entityNum;
					modifiedEntityCount++;

					// Temporarily make forcefield pass-through except for weapon fire
					ent->r.contents &= CONTENTS_SHOTCLIP;

					// Still want to handle touch in G_TouchTriggers for sound effects
					ent->r.contents |= CONTENTS_TRIGGER;
				}
			}
		}

		MODFN_NEXT( RunPlayerMove, ( MODFN_NC, clientNum ) );

		// Restore previous contents
		for ( i = 0; i < modifiedEntityCount; ++i ) {
			g_entities[modifiedEntities[i]].r.contents = oldContents[i];
		}
	}

	else {
		MODFN_NEXT( RunPlayerMove, ( MODFN_NC, clientNum ) );
	}
}

/*
================
ModForcefield_Init
================
*/
void ModForcefield_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( ForcefieldPlace, MODPRIORITY_GENERAL );
		MODFN_REGISTER( ForcefieldSoundEvent, MODPRIORITY_GENERAL );
		MODFN_REGISTER( ForcefieldAnnounce, MODPRIORITY_GENERAL );
		MODFN_REGISTER( AdjustGeneralConstant, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PlayerDeathEffect, MODPRIORITY_GENERAL );
		MODFN_REGISTER( ModifyDamageFlags, MODPRIORITY_GENERAL );
		MODFN_REGISTER( RunPlayerMove, MODPRIORITY_GENERAL );
	}
}
