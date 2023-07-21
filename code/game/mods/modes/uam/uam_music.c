/*
* UAM Music
* 
* Plays background track when the match is down to the final 2 players, and during
* intermission. Only intended to be loaded in Elimination mode.
*/

#define MOD_NAME ModUAMMusic

#include "mods/modes/uam/uam_local.h"

typedef enum {
	FMS_WAITING_FOR_3_PLAYERS,	// don't run finalist music unless there have been at least 3 players
	FMS_WAITING_FOR_FINALISTS,	// at least 3 players reached, now wait for it to drop to 2
	FMS_DELAY_IN_PROGRESS,		// players have dropped to 2, now begin a 2 second delay to actually play music
	FMS_COMPLETED				// music has been initiated
} finalistMusicState_t;

static struct {
	trackedCvar_t g_mod_PlayCountdownMusic;
	finalistMusicState_t finalistMusicState;
	int finalistMusicDelayEndTime;

	qboolean intermissionMusicActive;
} *MOD_STATE;

static const char *finalistMusic[] = {
	"music/briefing1.mp3",
	"music/transporterprep.mp3",
};

static const char *intermissionMusic[] = {
	"music/astrometrics.mp3",
	"music/briefing2.mp3",
	"music/itwastheborg.mp3",
};

/*
================
UAMMusic_Static_PlayIntermissionMusic

Triggers random intermission music if enabled, otherwise has no effect.
================
*/
void UAMMusic_Static_PlayIntermissionMusic( void ) {
	if ( MOD_STATE ) {
		// Note: I couldn't find a control cvar for this in the original gladiator mod,
		// so just always enabling it for now
		trap_SetConfigstring( CS_MUSIC, intermissionMusic[ irandom( 0, ARRAY_LEN( intermissionMusic ) - 1) ] );
		MOD_STATE->intermissionMusicActive = qtrue;
	}
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	if ( level.matchState == MS_ACTIVE ) {
		if ( MOD_STATE->finalistMusicState == FMS_WAITING_FOR_3_PLAYERS ) {
			if ( ModElimination_Static_CountPlayersAlive() >= 3 ) {
				MOD_STATE->finalistMusicState = FMS_WAITING_FOR_FINALISTS;
			}
		}

		else if ( MOD_STATE->finalistMusicState == FMS_WAITING_FOR_FINALISTS ) {
			if ( ModElimination_Static_CountPlayersAlive() <= 2 ) {
				MOD_STATE->finalistMusicDelayEndTime = level.time + 2000;
				MOD_STATE->finalistMusicState = FMS_DELAY_IN_PROGRESS;
			}
		}

		else if ( MOD_STATE->finalistMusicState == FMS_DELAY_IN_PROGRESS ) {
			if ( level.time >= MOD_STATE->finalistMusicDelayEndTime ) {
				if ( G_ModUtils_ReadGladiatorBoolean( MOD_STATE->g_mod_PlayCountdownMusic.string ) ) {
					// Don't start finalist music if map is already playing its own custom music.
					char buffer[256];
					buffer[0] = '\0';
					trap_GetConfigstring( CS_MUSIC, buffer, sizeof( buffer ) );
					if ( !*buffer ) {
						trap_SetConfigstring( CS_MUSIC, finalistMusic[ irandom( 0, ARRAY_LEN( finalistMusic ) - 1) ] );
					}
				}
				MOD_STATE->finalistMusicState = FMS_COMPLETED;
			}
		}
	}

	if ( level.matchState >= MS_INTERMISSION_ACTIVE && !MOD_STATE->intermissionMusicActive ) {
		UAMMusic_Static_PlayIntermissionMusic();
	}
}

/*
================
ModUAMMusic_Init
================
*/
void ModUAMMusic_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_mod_PlayCountdownMusic, "g_mod_PlayCountdownMusic", "Y", 0, qfalse );

		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );
	}
}
