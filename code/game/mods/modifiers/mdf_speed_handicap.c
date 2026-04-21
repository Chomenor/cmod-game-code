/*
 * Speed Handicap
 *
 * Support g_speedHandicap cvar. If enabled, handicap reduces movement speed instead
 * of health and damage.
 */

#define MOD_NAME ModSpeedHandicap

#include "mods/g_mod_local.h"

static struct {
	trackedCvar_t g_speedHandicap;
} *MOD_STATE;

/*
================
ModSpeedHandicap_Static_IsEnabled
================
*/
qboolean ModSpeedHandicap_Static_IsEnabled( void ) {
	return MOD_STATE && MOD_STATE->g_speedHandicap.integer;
}

/*
================
(ModFN) EffectiveHandicap

Disable standard health/damage reductions when g_speedHandicap is enabled.
================
*/
static int MOD_PREFIX(EffectiveHandicap)( MODFN_CTV, int clientNum, effectiveHandicapType_t type ) {
	gclient_t *client = &level.clients[clientNum];

	if ( MOD_STATE->g_speedHandicap.integer ) {
		if ( type == EH_VISIBLE ) {
			// This currently gets overridden for specialties classes/action hero, but the client
			// doesn't display handicap in this case anyway (class icon is in the way)...
			return client->pers.handicap > 0 && client->pers.handicap < 100 ? client->pers.handicap : 100;
		} else {
			return 100;
		}
	}

	return MODFN_NEXT( EffectiveHandicap, ( MODFN_NC, clientNum, type ) );
}

/*
==============
(ModFN) PmoveInit

Apply speed reduction when g_speedHandicap is enabled.
==============
*/
static void MOD_PREFIX(PmoveInit)( MODFN_CTV, int clientNum, pmove_t *pmove ) {
	gclient_t *client = &level.clients[clientNum];
	MODFN_NEXT( PmoveInit, ( MODFN_NC, clientNum, pmove ) );

	if ( MOD_STATE->g_speedHandicap.integer && client->ps.pm_type == PM_NORMAL &&
			client->pers.handicap > 0 && client->pers.handicap < 100 ) {
		client->ps.speed = client->ps.speed * client->pers.handicap / 100;
	}
}

/*
================
ModSpeedHandicap_Init
================
*/
void ModSpeedHandicap_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( EffectiveHandicap, MODPRIORITY_LOW );
		MODFN_REGISTER( PmoveInit, MODPRIORITY_MODIFICATIONS );

		G_RegisterTrackedCvar( &MOD_STATE->g_speedHandicap, "g_speedHandicap", "0", 0, qfalse );
	}
}
