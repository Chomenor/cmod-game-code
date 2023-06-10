/*
* Timelimit Countdown
* 
* This module adds a voice countdown when approaching the match timelimit, similar to Gladiator mod.
*/

#define MOD_NAME ModTimeLimitCountdown

#include "mods/g_mod_local.h"

static struct {
	int nextTimelimitMessage;	// next message to play (10 = max, 0 = final, -1 = none)
} *MOD_STATE;

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	if ( level.matchState == MS_ACTIVE && g_timelimit.integer ) {
		int remaining = g_timelimit.integer*60000 - ( level.time - level.startTime );
		int secondsPosition = remaining / 1000;
		if ( secondsPosition < 0 || secondsPosition > MOD_STATE->nextTimelimitMessage ) {
			return;
		}

		if ( secondsPosition > 0 ) {
			G_GlobalSound( G_SoundIndex( va( "sound/voice/computer/voy3/%isec.wav", secondsPosition ) ) );
		} else {
			// Play final message
			G_GlobalSound( G_SoundIndex( va( "sound/voice/janeway/misc/taunt%i.wav", irandom( 1, 2 ) ) ) );
		}

		MOD_STATE->nextTimelimitMessage = secondsPosition - 1;
	}
}

/*
================
ModTimelimitCountdown_Init
================
*/
void ModTimelimitCountdown_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MOD_STATE->nextTimelimitMessage = 10;

		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
	}
}
