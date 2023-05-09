/*
* Player Smoothing - Debug Commands
* 
* This module implements the "smoothing_debug_frames" console command, which allows
* checking smoothing frame time history for a given client on a running server.
* 
* TODO: Add statistics tracking features.
*/

#define MOD_NAME ModPCSmoothingDebug

#include "mods/features/pingcomp/pc_local.h"

#define WORKING_CLIENT_COUNT ( level.maxclients < MAX_SMOOTHING_CLIENTS ? level.maxclients : MAX_SMOOTHING_CLIENTS )
#define CLIENT_IN_RANGE( clientNum ) ( clientNum >= 0 && clientNum < WORKING_CLIENT_COUNT )

#define MAX_DEBUG_FRAMES 32

typedef struct {
	int levelTime;
	short offsetTarget;
	short offsetResult;
	short buffer;
} SmoothingDebug_frame_t;

typedef struct {
	SmoothingDebug_frame_t frames[MAX_DEBUG_FRAMES];
	unsigned int frameCounter;
} SmoothingDebug_client_t;

static struct {
	SmoothingDebug_client_t clients[MAX_SMOOTHING_CLIENTS];
} *MOD_STATE;

/*
===================
ModPCSmoothingDebug_ClientFromArgument
===================
*/
static int ModPCSmoothingDebug_ClientFromArgument( int index ) {
	int clientNum;
	char str[128];
	trap_Argv( index, str, sizeof( str ) );
	if ( !*str ) {
		G_Printf( "Invalid client number.\n" );
		return -1;
	}
	clientNum = atoi( str );
	if ( !G_IsConnectedClient( clientNum ) ) {
		G_Printf( "Client not connected.\n" );
		return -1;
	}
	return clientNum;
}

/*
===================
ModPCSmoothingDebug_PrintFrames
===================
*/
static void ModPCSmoothingDebug_PrintFrames( void ) {
	int clientNum = ModPCSmoothingDebug_ClientFromArgument( 1 );
	if ( CLIENT_IN_RANGE( clientNum ) ) {
		SmoothingDebug_client_t *modclient = &MOD_STATE->clients[clientNum];
		int i;

		G_Printf( "format: levelTime / offset(achieved) / buffer\n" );
		G_Printf( "levelTime: The server time at which the snapshot was sent.\n" );
		G_Printf( "offset: How far target/achieved smoothed time was behind levelTime.\n" );
		G_Printf( "buffer: How far target smoothed time was behind actual command time.\n" );

		for ( i = 0; i < MAX_DEBUG_FRAMES; ++i ) {
			const SmoothingDebug_frame_t *frame = &modclient->frames[( modclient->frameCounter + i ) % MAX_DEBUG_FRAMES];
			G_Printf( "%i / %i(%i) / %i\n", frame->levelTime, frame->offsetTarget, frame->offsetResult, frame->buffer );
		}
	}
}

/*
===================
ModPCSmoothingDebug_Static_LogFrame
===================
*/
void ModPCSmoothingDebug_Static_LogFrame( int clientNum, int targetTime, int resultTime ) {
	if ( MOD_STATE && CLIENT_IN_RANGE( clientNum ) ) {
		SmoothingDebug_client_t *modclient = &MOD_STATE->clients[clientNum];
		SmoothingDebug_frame_t *frame = &modclient->frames[( modclient->frameCounter++ ) % MAX_DEBUG_FRAMES];
		frame->levelTime = level.time;
		frame->offsetTarget = level.time - targetTime;
		frame->offsetResult = level.time - resultTime;
		frame->buffer = level.clients[clientNum].ps.commandTime - targetTime;
	}
}

/*
===================
(ModFN) ModConsoleCommand

Handle smoothing_debug_frames command.
===================
*/
static qboolean MOD_PREFIX(ModConsoleCommand)( MODFN_CTV, const char *cmd ) {
	if (Q_stricmp (cmd, "smoothing_debug_frames") == 0) {
		ModPCSmoothingDebug_PrintFrames();
		return qtrue;
	}

	return MODFN_NEXT( ModConsoleCommand, ( MODFN_NC, cmd ) );
}

/*
================
ModPCSmoothingDebug_Init
================
*/
void ModPCSmoothingDebug_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( ModConsoleCommand, MODPRIORITY_GENERAL );
	}
}
