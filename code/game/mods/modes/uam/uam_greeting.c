/*
* UAM Greeting
* 
* Play a random greeting sound clip at the start of the round.
*/

#define MOD_NAME ModUAMGreeting

#include "mods/modes/uam/uam_local.h"

static struct {
	trackedCvar_t g_mod_PlayWelcomeMessage;
	qboolean welcomeMessageReached;
} *MOD_STATE;

static const char *welcomeMessages[] = {
	"sound/voice/biessman/cin/08/boo.wav",
	"sound/voice/biessman/cin/21/lovinthis.wav",
	"sound/voice/biessman/forge3/comeon.wav",
	"sound/voice/biessman/forge3/kleptos.wav",
	"sound/voice/biessman/forge3/wantpain.wav",
	"sound/voice/biessman/forge3/woo.wav",
	"sound/voice/biessman/forge3/yourface.wav",
	"sound/voice/biessman/misc/letsgo.wav",
	"sound/voice/biessman/voy16/backoff.wav",
	"sound/voice/biessman/voy16/liketohear.wav",
	"sound/voice/biessman/voy5/sweet.wav",
	"sound/voice/foster/misc/victory2.wav",
	"sound/voice/hirogen/scavboss/iappreciate.wav",
	"sound/voice/klingonguard/scav2/comeout.wav",
	"sound/voice/munro/cin/09-2/letsgo.wav",
	"sound/voice/munro/cin/20/yessir.wav",
	"sound/voice/munro/cin/21/energize.wav",
	"sound/voice/munro/cin/42/understood.wav",
	"sound/voice/munro/dn8/lockandload.wav",
	"sound/voice/munro/stasis3/great.wav",
};

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	if ( !MOD_STATE->welcomeMessageReached && level.matchState == MS_ACTIVE && level.time - level.startTime >= 2000 ) {
		if ( G_ModUtils_ReadGladiatorBoolean( MOD_STATE->g_mod_PlayWelcomeMessage.string ) ) {
			G_GlobalSound( G_SoundIndex( welcomeMessages[irandom( 0, ARRAY_LEN( welcomeMessages ) - 1 )] ) );
		}

		MOD_STATE->welcomeMessageReached = qtrue;
	}
}

/*
================
ModUAMGreeting_Init
================
*/
void ModUAMGreeting_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_mod_PlayWelcomeMessage, "g_mod_PlayWelcomeMessage", "Y", 0, qfalse );

		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );
	}
}
