/*
* Ghost Sparkle
* 
* Support adding an extra sparkle effect to the spawn invulnerability ghost effect,
* similar to Gladiator mod.
* 
* Enabled by MC_GHOST_SPARKLE_ENABLED mod constant.
* 
* NOTE: May need to be updated to work better with holodeck doors enabled.
*/

#define MOD_NAME ModGhostSparkle

#include "mods/g_mod_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
===========
(ModFN) SetClientGhosting
============
*/
static void MOD_PREFIX(SetClientGhosting)( MODFN_CTV, int clientNum, qboolean active ) {
	gclient_t *client = &level.clients[clientNum];
	qboolean wasActive = client->ps.powerups[PW_GHOST] >= level.time ? qtrue : qfalse;

	MODFN_NEXT( SetClientGhosting, ( MODFN_NC, clientNum, active ) );

	if ( modfn_lcl.AdjustModConstant( MC_GHOST_SPARKLE_ENABLED, 0 ) ) {
		// If ghosting active, set sparkle (PW_OUCH) effect to last for the same length.
		if ( client->ps.powerups[PW_GHOST] >= level.time && client->ps.powerups[PW_GHOST] > client->ps.powerups[PW_OUCH] ) {
			client->ps.powerups[PW_OUCH] = client->ps.powerups[PW_GHOST];
		}

		// If ghosting was inactivated, also inactivate sparkle effect.
		else if ( wasActive && !client->ps.powerups[PW_GHOST] ) {
			client->ps.powerups[PW_OUCH] = 0;
		}
	}
}

/*
================
ModGhostSparkle_Init
================
*/
void ModGhostSparkle_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( SetClientGhosting, MODPRIORITY_GENERAL );
	}
}
