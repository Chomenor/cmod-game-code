/*
* Detpack Adjustments
* 
* This module implements additional detpack functionality configured by mod constants.
*/

#define MOD_NAME ModDetpack

#include "mods/g_mod_local.h"

static struct {
	int _unused;
} *MOD_STATE;

#define WARMUP_MODE ( modfn.WarmupLength() > 0 && level.matchState < MS_ACTIVE )

#define ORIGIN_PATCH modfn_lcl.AdjustModConstant( MC_DETPACK_ORIGIN_PATCH, 0 )
#define GLADIATOR_ANNOUNCEMENTS ( modfn_lcl.AdjustModConstant( MC_DETPACK_GLADIATOR_ANNOUNCEMENTS, 0 ) && !WARMUP_MODE )
#define PING_SOUND modfn_lcl.AdjustModConstant( MC_DETPACK_PING_SOUND, 0 )
#define DROP_ON_DEATH modfn_lcl.AdjustModConstant( MC_DETPACK_DROP_ON_DEATH, 0 )
#define INVULNERABLE modfn_lcl.AdjustModConstant( MC_DETPACK_INVULNERABLE, 0 )

/*
================
(ModFN) DetpackPlace
================
*/
static gentity_t *MOD_PREFIX(DetpackPlace)( MODFN_CTV, int clientNum ) {
	gentity_t *detpack = MODFN_NEXT( DetpackPlace, ( MODFN_NC, clientNum ) );
	if ( !detpack ) {
		return NULL;
	}

	if ( ORIGIN_PATCH ) {
		// This changes the location of the main blast, shockwave, and explosion if the
		// detpack is shot to use the position of the detpack rather than of the player
		// who placed it. Note that this currently differs from Gladiator and Pinball,
		// which only use the detpack location for the main blast.
		VectorCopy( detpack->s.pos.trBase, detpack->s.origin );
	}

	if ( GLADIATOR_ANNOUNCEMENTS ) {
		G_GlobalSound( G_SoundIndex( "sound/voice/computer/voy3/breachimminent.wav" ) );
	}

	if ( PING_SOUND ) {
		detpack->s.loopSound = G_SoundIndex( "sound/interface/sensorping.wav" );
	}

	detpack->takedamage = !INVULNERABLE;

	return detpack;
}

/*
================
(ModFN) DetpackShot
================
*/
static void MOD_PREFIX(DetpackShot)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	if ( GLADIATOR_ANNOUNCEMENTS ) {
		G_GlobalSound( G_SoundIndex( "sound/voice/computer/voy3/coreoffline.wav" ) );
	}

	MODFN_NEXT( DetpackShot, ( MODFN_NC, self, inflictor, attacker, damage, meansOfDeath ) );
}

/*
==================
(ModFN) PlayerDeathDiscardDetpack
==================
*/
static void MOD_PREFIX(PlayerDeathDiscardDetpack)( MODFN_CTV, int clientNum ) {
	if ( DROP_ON_DEATH || GLADIATOR_ANNOUNCEMENTS ) {
		gentity_t *self = &g_entities[clientNum];
		gentity_t *detpack = NULL;
		const char *classname = BG_FindClassnameForHoldable( HI_DETPACK );

		// Look for detpack placed by this player
		if ( EF_WARN_ASSERT( classname ) ) {
			while ( ( detpack = G_Find( detpack, FOFS( classname ), classname ) ) != NULL ) {
				// Add takedamage check to exclude detpack that is in the process of exploding
				if ( detpack->parent == self && detpack->takedamage ) {
					break;
				}
			}
		}

		if ( detpack ) {
			if ( GLADIATOR_ANNOUNCEMENTS ) {
				G_GlobalSound( G_SoundIndex( "sound/voice/computer/voy3/coreoffline.wav" ) );
			}

			if ( DROP_ON_DEATH ) {
				// Item needs to be offset to 15 units higher than placed detpack
				vec3_t newOrigin;
				VectorCopy( detpack->s.pos.trBase, newOrigin );
				newOrigin[2] += 15.0f;

				LaunchItem( BG_FindItemForHoldable( HI_DETPACK ), newOrigin, vec3_origin );
				G_FreeEntity( detpack );

				self->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
				self->client->ps.stats[STAT_USEABLE_PLACED] = 0;
				return;
			}
		}
	}

	MODFN_NEXT( PlayerDeathDiscardDetpack, ( MODFN_NC, clientNum ) );
}

/*
================
ModDetpack_Init
================
*/
void ModDetpack_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( DetpackPlace, MODPRIORITY_GENERAL );
		MODFN_REGISTER( DetpackShot, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PlayerDeathDiscardDetpack, MODPRIORITY_GENERAL );
	}
}
