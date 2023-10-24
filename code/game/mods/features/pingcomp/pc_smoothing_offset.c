/*
* Player Smoothing - Offset Calculation
* 
* This module calculates the smoothing "offset" for each player. The offset is the time
* in milliseconds that the smoothed command time is behind the current server time.
* 
* For example, if the server level.time for a certain frame is 10500, and a certain client
* has an offset of 100, the smoothing system will attempt to retrieve the player position
* at commandTime 10400 to display to other clients.
* 
* The purpose of this calculation is to find the lowest offset that still allows the player
* movement to be in time for most server snapshots. There are probably better algorithms to
* accomplish this, but this one seems to work adequately and handles some lag patterns better
* than more trivial implementations.
*/

#define MOD_NAME ModPCSmoothingOffset

#include "mods/features/pingcomp/pc_local.h"

#define WORKING_CLIENT_COUNT ( level.maxclients < MAX_SMOOTHING_CLIENTS ? level.maxclients : MAX_SMOOTHING_CLIENTS )
#define CLIENT_IN_RANGE( clientNum ) ( clientNum >= 0 && clientNum < WORKING_CLIENT_COUNT )

// Use special offset calculation for bots on dedicated servers
#define SYNCHRONOUS_BOT( clientNum ) ( ( g_entities[clientNum].r.svFlags & SVF_BOT ) && MOD_STATE->dedicatedServer )

// Maximum time to shift client back from their current commandTime
#define MAX_TRUE_SHIFT 300

// Functions for conversion between offset time value and offset table indices
#define MAX_OFFSET 1000
#define MIN_OFFSET -200
#define RESOLUTION 2
#define OFFSET_TO_INDEX( offset ) ( ( ( offset ) - MIN_OFFSET ) / RESOLUTION )
#define INDEX_TO_OFFSET( index ) ( ( ( index ) * RESOLUTION ) + MIN_OFFSET )
#define OFFSET_ENTRY_COUNT OFFSET_TO_INDEX( MAX_OFFSET )

// Determines how quickly old statistics are decayed from calculation
#define DECAY_FACTOR 0.8f
#define DECAY_INTERVAL 1000

// Formula for estimating the best offset
// This calculates that it is worth delaying player movement by 10ms to gain 1 percentage point of coverage
#define OFFSET_SCORE( offset, coverage ) ( (float)( coverage ) * 1000.0f - (float)( offset ) )

// Minimum and maximum coverage values to consider when calculating the best offset
// Plays a role in limiting CPU usage during calculation
#define COVERAGE_MINIMUM 0.75f
#define COVERAGE_MAXIMUM 0.98f

// Extra delay added by default
#define BASE_OFFSET 5

typedef struct {
	qboolean initialized;
	float offsetTable[OFFSET_ENTRY_COUNT];
	float offsetTotal;
	int nextDecayTime;

	// cached offset calc
	int minimumOffsetIndex;
	float minimumOffsetTally;

	// offset stabilization
	float stabilizedOffset;
	int stabilizeLastCheckTime;
	int stabilizeHoldTime;
} SmoothingOffset_client_t;

static struct {
	trackedCvar_t g_smoothClientsOffset;

	SmoothingOffset_client_t clients[MAX_SMOOTHING_CLIENTS];
	int lastFrameTime;
	qboolean dedicatedServer;
} *MOD_STATE;

/*
==============
ModPCSmoothingOffset_ExtrapolatedTime

Returns level.time, but advanced to compensate for time elapsed between server frames.
==============
*/
static int ModPCSmoothingOffset_ExtrapolatedTime( void ) {
	int offset = trap_Milliseconds() - MOD_STATE->lastFrameTime;
	if ( offset > 0 && offset < 100 ) {
		return level.time + offset;
	}
	return level.time;
}

/*
==============
ModPCSmoothingOffset_UpdateMinimumOffset

Update cached starting point for offset calculations.
==============
*/
static void ModPCSmoothingOffset_UpdateMinimumOffset( int clientNum ) {
	SmoothingOffset_client_t *modclient = &MOD_STATE->clients[clientNum];
	float *offsetTable = MOD_STATE->clients[clientNum].offsetTable;
	float target = MOD_STATE->clients[clientNum].offsetTotal * COVERAGE_MINIMUM;

	while( modclient->minimumOffsetIndex > 0 && modclient->minimumOffsetTally > target ) {
		modclient->minimumOffsetTally -= modclient->offsetTable[modclient->minimumOffsetIndex];
		--modclient->minimumOffsetIndex;
	}

	while( modclient->minimumOffsetIndex < OFFSET_ENTRY_COUNT - 1 && modclient->minimumOffsetTally < target ) {
		++modclient->minimumOffsetIndex;
		modclient->minimumOffsetTally += modclient->offsetTable[modclient->minimumOffsetIndex];
	}
}

/*
==============
ModPCSmoothingOffset_OffsetDecay
==============
*/
static void ModPCSmoothingOffset_OffsetDecay( int clientNum ) {
	SmoothingOffset_client_t *modclient = &MOD_STATE->clients[clientNum];
	int i;
	float *offsetTable = modclient->offsetTable;
	float total = 0.0f;

	for ( i = 0; i < OFFSET_ENTRY_COUNT; ++i ) {
		offsetTable[i] *= DECAY_FACTOR;
		total += offsetTable[i];
	}
	modclient->offsetTotal = total;

	modclient->minimumOffsetIndex = 0;
	modclient->minimumOffsetTally = 0.0f;
}

/*
==============
ModPCSmoothingOffset_OffsetDecayClients
==============
*/
static void ModPCSmoothingOffset_OffsetDecayClients( void ) {
	int i;
	int clientCount = WORKING_CLIENT_COUNT;

	for(i=0; i<clientCount; ++i) {
		if ( ModPingcomp_Static_SmoothingEnabledForClient( i ) && level.time >= MOD_STATE->clients[i].nextDecayTime &&
				level.clients[i].ps.commandTime + MAX_OFFSET > level.time ) {
			// Shift decay time for each client to avoid CPU impact from running all decays at once
			int offset = ( DECAY_INTERVAL * i ) / clientCount;
			ModPCSmoothingOffset_OffsetDecay( i );
			MOD_STATE->clients[i].nextDecayTime = level.time - ( level.time + offset + 1 ) % DECAY_INTERVAL + DECAY_INTERVAL;
		}
	}
}

/*
==============
ModPCSmoothingOffset_RegisterClientMove
==============
*/
static void ModPCSmoothingOffset_RegisterClientMove( int clientNum, int startTime, int endTime ) {
	SmoothingOffset_client_t *modclient = &MOD_STATE->clients[clientNum];
	if ( !modclient->initialized ) {
		memset( modclient, 0, sizeof( *modclient ) );
		modclient->initialized = qtrue;
		// Don't actually run the calculations yet, because first frame can have inconsistent times
	
	} else {
		int i;
		int time = ModPCSmoothingOffset_ExtrapolatedTime();
		float *offsetTable = modclient->offsetTable;
		int startIndex = OFFSET_TO_INDEX( time - endTime );
		int endIndex = OFFSET_TO_INDEX( time - startTime );

		if ( startIndex < 0 )
			startIndex = 0;
		if ( startIndex >= OFFSET_ENTRY_COUNT )
			startIndex = OFFSET_ENTRY_COUNT;
		if ( endIndex < startIndex )
			endIndex = startIndex;
		if ( endIndex >= OFFSET_ENTRY_COUNT )
			endIndex = OFFSET_ENTRY_COUNT;

		for ( i = startIndex; i < endIndex; ++i ) {
			offsetTable[i] += 1.0f;
			if ( i <= modclient->minimumOffsetIndex ) {
				modclient->minimumOffsetTally += 1.0f;
			}
		}

		modclient->offsetTotal += endIndex - startIndex;
	}
}

/*
==============
ModPCSmoothingOffset_GetOptimalOffset
==============
*/
static int ModPCSmoothingOffset_GetOptimalOffset( int clientNum ) {
	int i;
	SmoothingOffset_client_t *modclient = &MOD_STATE->clients[clientNum];
	float *offsetTable = modclient->offsetTable;
	float tally;
	float total = modclient->offsetTotal;
	int bestIndex = -1;
	float bestScore = 0;
	float bestCoverage = 0;

	ModPCSmoothingOffset_UpdateMinimumOffset( clientNum );
	tally = modclient->minimumOffsetTally;

	for(i=modclient->minimumOffsetIndex; i<OFFSET_ENTRY_COUNT; ++i) {
		int offset = INDEX_TO_OFFSET( i );
		float coverage = tally / total;
		float score = OFFSET_SCORE( offset, coverage );

		if ( bestIndex == -1 || score > bestScore ) {
			bestIndex = i;
			bestScore = score;
			bestCoverage = coverage;
		}

		if ( coverage > COVERAGE_MAXIMUM ) {
			break;
		}

		if ( i<OFFSET_ENTRY_COUNT - 1 ) {
			tally += offsetTable[i+1];
		}
	}

	return bestIndex == -1 ? 0 : INDEX_TO_OFFSET( bestIndex );
}

/*
==============
ModPCSmoothingOffset_UpdateStabilizedOffset
==============
*/
static void ModPCSmoothingOffset_UpdateStabilizedOffset( int clientNum ) {
	SmoothingOffset_client_t *modclient = &MOD_STATE->clients[clientNum];
	float timeDelta = level.time - modclient->stabilizeLastCheckTime;
	float targetOffset = ModPCSmoothingOffset_GetOptimalOffset( clientNum );
	int clientOffset = level.time - level.clients[clientNum].ps.commandTime;

	modclient->stabilizeLastCheckTime = level.time;
	if ( timeDelta > 100.0f ) {
		timeDelta = 100.0f;
	}

	// check for decreasing shift
	if ( targetOffset > modclient->stabilizedOffset ) {
		modclient->stabilizedOffset += timeDelta * 0.5f;
		if ( modclient->stabilizedOffset > targetOffset ) {
			modclient->stabilizedOffset = targetOffset;
		}
		modclient->stabilizeHoldTime = 500;
	}

	// check for increasing shift
	else if ( targetOffset < modclient->stabilizedOffset ) {
		modclient->stabilizeHoldTime -= timeDelta;
		if ( modclient->stabilizeHoldTime < 0 ) {
			timeDelta = -modclient->stabilizeHoldTime;
			modclient->stabilizeHoldTime = 0;

			modclient->stabilizedOffset -= timeDelta * 0.1f;
			if ( modclient->stabilizedOffset < targetOffset ) {
				modclient->stabilizedOffset = targetOffset;
			}
		}
	}

	// apply cap on shift amount
	if ( modclient->stabilizedOffset > clientOffset + MAX_TRUE_SHIFT ) {
		modclient->stabilizedOffset = clientOffset + MAX_TRUE_SHIFT;
	}
}

/*
==============
ModPCSmoothingOffset_Shared_GetOffset
==============
*/
int ModPCSmoothingOffset_Shared_GetOffset( int clientNum ) {
	EF_ERR_ASSERT( MOD_STATE );
	if ( ModPingcomp_Static_SmoothingEnabled() && CLIENT_IN_RANGE( clientNum ) ) {
		if ( SYNCHRONOUS_BOT( clientNum ) ) {
			// On dedicated server, the engine generally calls bot moves in sync with server frames.
			// If move partitioning is disabled, just set the offset to the offset of the latest move
			// (effectively no smoothing effect). If move partitioning is enabled, the move time can
			// lag the command time by up to ( fixedLength - 1 ) msec, so add this value to the offset.
			int partitioningOffset = modfn.PmoveFixedLength( qtrue ) - 1;
			if ( partitioningOffset < 0 ) {
				partitioningOffset = 0;
			}
			return ( level.time - level.clients[clientNum].pers.cmd.serverTime ) + partitioningOffset +
					MOD_STATE->g_smoothClientsOffset.integer;

		} else if (MOD_STATE->clients[clientNum].initialized ) {
			SmoothingOffset_client_t *modclient = &MOD_STATE->clients[clientNum];
			ModPCSmoothingOffset_UpdateStabilizedOffset( clientNum );
			return tonextint( modclient->stabilizedOffset ) + BASE_OFFSET + MOD_STATE->g_smoothClientsOffset.integer;
		}
	}
	return 0;
}

/* ----------------------------------------------------------------------- */

// Debug Functions

/* ----------------------------------------------------------------------- */

/*
==============
ModPCSmoothingOffset_GetOffsetAtCoverage
==============
*/
static int ModPCSmoothingOffset_GetOffsetAtCoverage( int clientNum, float percentile ) {
	int i;
	float *offsetTable = MOD_STATE->clients[clientNum].offsetTable;
	float target = MOD_STATE->clients[clientNum].offsetTotal * percentile;
	float tally = 0.0f;

	for ( i = 0; i < OFFSET_ENTRY_COUNT - 1; ++i ) {
		tally += offsetTable[i];
		if ( tally >= target ) {
			break;
		}
	}

	return INDEX_TO_OFFSET( i );
}

/*
==============
ModPCSmoothingOffset_OffsetDebug
==============
*/
static void ModPCSmoothingOffset_OffsetDebug( int clientNum ) {
	SmoothingOffset_client_t *modclient = &MOD_STATE->clients[clientNum];
	if ( modclient->initialized ) {
		int optimal = ModPCSmoothingOffset_GetOptimalOffset( clientNum );
		int i;

		G_Printf( "\nminimum considered offset (should equal coverage 75): %i\n",
				INDEX_TO_OFFSET( modclient->minimumOffsetIndex ) );
		G_Printf( "format: coverage / offset / score\n" );
		for ( i = 75; i <= 100; ++i ) {
			int offset = ModPCSmoothingOffset_GetOffsetAtCoverage( clientNum, 0.01f * i );
			G_Printf( "%i / %i / %i\n", i, offset, (int)( tonextint( OFFSET_SCORE( offset, i * 0.01f ) ) ) );
		}

		G_Printf( "calculated optimal offset: %i\n", optimal );
		G_Printf( "current stabilized offset: %i\n", (int)( tonextint( modclient->stabilizedOffset ) ) );
	} else {
		G_Printf( "offset calculation not initialized for client %i.\n", clientNum );
	}
}

/* ----------------------------------------------------------------------- */

// Interface Functions

/* ----------------------------------------------------------------------- */

/*
===================
(ModFN) ModConsoleCommand
===================
*/
static qboolean MOD_PREFIX(ModConsoleCommand)( MODFN_CTV, const char *cmd ) {
	if (Q_stricmp (cmd, "smoothing_debug_offset") == 0) {
		ModPCSmoothingOffset_OffsetDebug( 0 );
		return qtrue;
	}

	return MODFN_NEXT( ModConsoleCommand, ( MODFN_NC, cmd ) );
}

/*
==============
(ModFN) RunPlayerMove
==============
*/
static void MOD_PREFIX(RunPlayerMove)( MODFN_CTV, int clientNum, qboolean spectator ) {
	int oldTime = level.clients[clientNum].ps.commandTime;
	MODFN_NEXT( RunPlayerMove, ( MODFN_NC, clientNum, spectator ) );

	if ( !SYNCHRONOUS_BOT( clientNum ) && ModPingcomp_Static_SmoothingEnabledForClient( clientNum ) ) {
		ModPCSmoothingOffset_RegisterClientMove( clientNum, oldTime, level.clients[clientNum].ps.commandTime );
	}
}

/*
================
(ModFN) InitClientSession
================
*/
static void MOD_PREFIX(InitClientSession)( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ) {
	MODFN_NEXT( InitClientSession, ( MODFN_NC, clientNum, initialConnect, info ) );

	if ( CLIENT_IN_RANGE( clientNum ) ) {
		SmoothingOffset_client_t *modclient = &MOD_STATE->clients[clientNum];

		// make sure no data is left over from previous client
		modclient->initialized = qfalse;
	}
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	if ( ModPingcomp_Static_SmoothingEnabled() ) {
		MOD_STATE->lastFrameTime = trap_Milliseconds();
		ModPCSmoothingOffset_OffsetDecayClients();
	}
}

/*
================
ModPCSmoothingOffset_Init
================
*/
void ModPCSmoothingOffset_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_smoothClientsOffset, "g_smoothClientsOffset", "0", 0, qfalse );

		MOD_STATE->dedicatedServer = trap_Cvar_VariableIntegerValue( "dedicated" ) ||
				!trap_Cvar_VariableIntegerValue( "cl_running" ) ? qtrue : qfalse;

		MODFN_REGISTER( ModConsoleCommand, MODPRIORITY_GENERAL );
		MODFN_REGISTER( RunPlayerMove, MODPRIORITY_GENERAL );
		MODFN_REGISTER( InitClientSession, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostRunFrame, MODPRIORITY_GENERAL );
	}
}
