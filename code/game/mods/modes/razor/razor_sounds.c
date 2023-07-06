/*
* Razor Sounds
* 
* Add extra sound effects, such as a greeting clip when entering the game, and custom
* music during intermission.
*/

#define MOD_NAME ModRazorSounds

#include "mods/modes/razor/razor_local.h"

static struct {
	// Play "Welcome To Holomatch" sound clip 1 second after player joins game as non-spectator.
	// 0: already played (don't play again), -1: waiting for client to join game, >0: play at this time
	int introSoundTime[MAX_CLIENTS];
	int introSound;

	qboolean intermissionMusicActive;
} *MOD_STATE;

static const char *razorIntermissionMusic[] = {
	"music/briefing1.mp3",
	"music/itwastheborg.mp3",
	"music/transporterprep.mp3",
};

/*
================
(ModFN) PostClientSpawn
================
*/
static void MOD_PREFIX(PostClientSpawn)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	int *time = &MOD_STATE->introSoundTime[clientNum];
	gclient_t *client = &level.clients[clientNum];

	MODFN_NEXT( PostClientSpawn, ( MODFN_NC, clientNum, spawnType ) );

	if ( spawnType == CST_FIRSTTIME ) {
		// initialize when client first connects
		*time = -1;
	}

	if ( *time == -1 && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		// start timer when initialized client joins a team
		*time = level.time + 1000;
	}

	if ( *time > 0 && client->sess.sessionTeam == TEAM_SPECTATOR ) {
		// stop timer if client goes back to spectator
		*time = -1;
	}
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	int clientNum;

	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	// Check for playing initial "Welcome To Holomatch" clip
	for ( clientNum = 0; clientNum < level.maxclients; ++clientNum ) {
		if ( level.clients[clientNum].pers.connected == CON_CONNECTED ) {
			int *time = &MOD_STATE->introSoundTime[clientNum];
			if ( *time > 0 && *time <= level.time ) {
				// Original Pinball implementation was apparently buggy, and set the broadcast sound
				// event to the origin of client 0, with no broadcast flag. Therefore whether or not
				// the sound got played depended on where client 0 happened to be in the map.
				// The version used here fixes this and just plays the sound always.
				G_ClientGlobalSound( MOD_STATE->introSound, clientNum );
				*time = 0;
			}
		}
	}

	// Check for starting intermission music
	if ( level.matchState >= MS_INTERMISSION_ACTIVE && !MOD_STATE->intermissionMusicActive ) {
		trap_SetConfigstring( CS_MUSIC, razorIntermissionMusic[ irandom( 0, ARRAY_LEN( razorIntermissionMusic ) - 1) ] );
		MOD_STATE->intermissionMusicActive = qtrue;
	}
}

/*
================
ModRazorSounds_Init
================
*/
void ModRazorSounds_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MOD_STATE->introSound = G_SoundIndex( "sound/voice/computer/misc/intro_01.wav" );

		// Play extra sound when entering intermission
		ModEndSound_Init();

		MODFN_REGISTER( PostClientSpawn, ++modePriorityLevel );
		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );
	}
}
