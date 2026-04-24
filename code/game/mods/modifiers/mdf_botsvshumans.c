/*
 * Bots vs Humans mode
 * 
 * When enabled by g_botsVsHumans cvar, bots only attack human players (not other bots)
 * as long as there are any non-eliminated human players left in the game.
 */

#define MOD_NAME ModBotsVsHumans

#include "mods/g_mod_local.h"
#include "bots/be_aas.h"

static struct {
	trackedCvar_t g_botsVsHumans;
} *MOD_STATE;

// True if tgt_client is a bot that shouldn't be targetted
#define BVH_ACTIVE( tgt_client ) ( MOD_STATE->g_botsVsHumans.integer && G_IsValidClient( tgt_client ) && \
			( g_entities[tgt_client].r.svFlags & SVF_BOT ) && MOD_PREFIX(AreAnyHumansPlayingCached)() )

/*
================
AreAnyHumansPlaying
================
*/
static qboolean MOD_PREFIX(AreAnyHumansPlaying)( void ) {
	int i;
	for ( i = 0; i < level.maxclients; ++i ) {
		if ( level.clients[i].pers.connected >= CON_CONNECTING &&
				!( g_entities[i].r.svFlags & SVF_BOT ) && !modfn.SpectatorClient( i ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
================
AreAnyHumansPlayingCached
================
*/
static qboolean MOD_PREFIX(AreAnyHumansPlayingCached)( void ) {
	static int cache_time;
	static qboolean result;
	if ( cache_time != level.time ) {
		result = MOD_PREFIX(AreAnyHumansPlaying)();
		cache_time = level.time;
	}
	return result;
}

/*
================
(ModFN) BotEntityInfo

Ignore human player entities for targetting purposes.
================
*/
static qboolean MOD_PREFIX( BotEntityInfo )( MODFN_CTV, int entnum, void *info ) {
	if ( BVH_ACTIVE( entnum ) ) {
		// Return info with "valid" set to false to ignore player.
		aas_entityinfo_t *info_typed = (aas_entityinfo_t *)info;
		memset( info_typed, 0, sizeof( *info_typed ) );
		return qtrue;
	}

	return MODFN_NEXT( BotEntityInfo, ( MODFN_NC, entnum, info ) ) || MOD_STATE->g_botsVsHumans.integer;
}

/*
================
(ModFN) ForcefieldTouchResponse

Let bots pass through undamageable forcefields when bots-vs-humans is active,
to avoid making defense against bots too easy.
================
*/
static modForcefield_touchResponse_t MOD_PREFIX(ForcefieldTouchResponse)(
		MODFN_CTV, forcefieldRelation_t relation, int clientNum, gentity_t *forcefield ) {
	if ( BVH_ACTIVE( clientNum ) ) {
		// Check if invulnerable forcefields are enabled
		modForcefield_config_t config;
		memset( &config, 0, sizeof( config ) );
		modfn_lcl.ForcefieldConfig( &config );
		if ( config.invulnerable ) {
			return FFTR_QUICKPASS;
		}
	}
	return MODFN_NEXT( ForcefieldTouchResponse, ( MODFN_NC, relation, clientNum, forcefield ) );
}

/*
================
ModBotsVsHumans_Init
================
*/
void ModBotsVsHumans_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( BotEntityInfo, MODPRIORITY_MODIFICATIONS );
		MODFN_REGISTER( ForcefieldTouchResponse, MODPRIORITY_MODIFICATIONS );

		G_RegisterTrackedCvar( &MOD_STATE->g_botsVsHumans, "g_botsVsHumans", "0", 0, qfalse );

		ModForcefield_Init();
	}
}
