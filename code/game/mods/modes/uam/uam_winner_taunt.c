/*
* UAM Winner Taunt
* 
* In FFA Elimination, the winning player automatically triggers a taunt event at the end of the round.
*/

#define MOD_NAME ModUAMWinnerTaunt

#include "mods/modes/uam/uam_local.h"

static struct {
	qboolean tauntReached;
} *MOD_STATE;

#define TAUNT_DELAY_TIME 2000

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	// Check if time reached...
	if ( level.matchState == MS_INTERMISSION_QUEUED && !MOD_STATE->tauntReached &&
			level.time >= level.intermissionQueued + TAUNT_DELAY_TIME ) {
		// Check if we have a valid winner...
		if ( g_gametype.integer < GT_TEAM && level.numConnectedClients > 0 &&
				!modfn.SpectatorClient( level.sortedClients[0] ) ) {
			gclient_t *client = &level.clients[level.sortedClients[0]];
			gentity_t *ent = &g_entities[level.sortedClients[0]];

			// Check if gesture isn't already in progress...
			if ( !client->ps.torsoTimer ) {
				G_AddEvent( ent, EV_TAUNT, 0 );
				client->ps.torsoTimer = 34*66+50;	// Don't allow another gesture for TIMER_GESTURE ms
			}
		}

		MOD_STATE->tauntReached = qtrue;
	}
}

/*
================
ModUAMWinnerTaunt_Init
================
*/
void ModUAMWinnerTaunt_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );
	}
}
