// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

// snapshot to base predictions from...
#define PREDICTION_SNAPSHOT ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ? cg.nextSnap : cg.snap )

#define MAX_CMD_BUFFER 256
#define MAX_PREDICT_CACHE 256
#define MAX_ITEM_PICKUP_LOG 16

#define CMD_BUFFER_ENTRY( cmdNum ) ( cmdBuffer.entries[( cmdNum ) % MAX_CMD_BUFFER] )
#define PREDICT_CACHE_ENTRY( frameNum ) ( predictCache.entries[( frameNum ) % MAX_PREDICT_CACHE] )

typedef struct {
	usercmd_t entries[MAX_CMD_BUFFER];
	int latest;		// cmdnum of most recent command in buffer
	int tail;		// valid cmdnum must be > this
} cmdBuffer_t;

typedef struct {
	playerState_t ps;
	int cmdNum;
	qboolean predictedTeleport;
} predictionFrame_t;

typedef struct {
	predictionFrame_t entries[MAX_PREDICT_CACHE];
	int latest;		// index of most recent frame in cache
	int tail;		// valid index must be > this
} predictCache_t;

typedef struct {
	int commandTime;	// 0 if not a valid entry
	int entityNum;
	int itemNum;
} itemPickupEntry_t;

typedef struct {
	itemPickupEntry_t itemPickups[MAX_ITEM_PICKUP_LOG];
	int itemPickupIndex;
} itemPickupLog_t;

typedef struct {
	entity_event_t event;
	int debounceTime;
	int lastCommandTime;
	int lastClientTime;
} debounceEntry_t;

static	pmove_t		cg_pmove;

static	int			cg_numSolidEntities;
static	centity_t	*cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];
static	int			cg_numTriggerEntities;
static	centity_t	*cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

// reference point for prediction error decay
static	int			lastMoveTime = -1;
static	vec3_t		lastMoveOrigin;

static	cmdBuffer_t			cmdBuffer;
static	predictCache_t		predictCache;
static	itemPickupLog_t		ipl;

// debounceTime limits the same event from being predicted more than once in a certain period of time.
// This is mostly just precautionary; under typical conditions without prediction errors there
// shouldn't be duplicate events even if debounceTime is 0.
static debounceEntry_t debounceTable[] = {
	{ EV_STEP_4, 5 },
	{ EV_FIRE_WEAPON, 50 },
	{ EV_ALT_FIRE, 50 },
	{ EV_FIRE_EMPTY_PHASER, 50 },
	{ EV_JUMP, 50 },
	{ EV_FOOTSTEP, 100 },
	{ EV_FALL_SHORT, 100 },
	{ EV_FOOTSPLASH, 100 },
	{ EV_SWIM, 100 },
	{ EV_WATER_TOUCH, 100 },
	{ EV_WATER_LEAVE, 100 },
	{ EV_WATER_UNDER, 100 },
	{ EV_WATER_CLEAR, 100 },
	{ EV_CHANGE_WEAPON, 100 },
	{ EV_USE_ITEM0, 100 },
	{ EV_NOAMMO, 100 },
	{ EV_NOAMMO_ALT, 100 },
	{ EV_TAUNT, 100 },
	{ EV_JUMP_PAD, 100 },
};

/*
====================
CG_PlayerstatePredictionActive
====================
*/
qboolean CG_PlayerstatePredictionActive( void ) {
	if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)
			|| cg_nopredict.integer || cg_synchronousClients.integer ) {
		return qfalse;
	}

	return qtrue;
}

/*
====================
CG_ExecutePredictableEvent
====================
*/
static void CG_ExecutePredictableEvent( entity_event_t event, int eventParm ) {
	centity_t *cent = &cg.predictedPlayerEntity;
	cent->currentState.event = event;
	cent->currentState.eventParm = eventParm;

	// NOTE: cent->lerpOrigin may not be the best value to use here, but it probably doesn't
	// matter much since CG_EntityEvent doesn't normally use it for predictable events anyway
	CG_EntityEvent( cent, cent->lerpOrigin );
}

/*
====================
CG_FilterItemPickup

Perform special filtering for item pickup events because they need to be deduplicated against
the specific item that is being picked up.

For client predicted pickups, we know the entityNum of the item and can use that to match
the log entry of an event has already been issued. For server events, entityNum will be -1
and we only play it if there was no predicted event with a similar type and time range.
====================
*/
static void CG_FilterItemPickup( int entityNum, int itemNum, vec3_t playerOrigin, int commandTime ) {
	int i;

	// search log for existing prediction
	for ( i = 0; i < MAX_ITEM_PICKUP_LOG; ++i ) {
		itemPickupEntry_t *entry = &ipl.itemPickups[i];

		if ( entry->commandTime <= 0 || entry->commandTime + 500 <= commandTime ) {
			// previous event too old
			continue;
		}

		if ( entry->itemNum != itemNum ) {
			// previous event was a different kind of item
			continue;
		}

		if ( entityNum >= 0 && entry->entityNum != entityNum ) {
			// previous event has different entity number
			continue;
		}

		// event already predicted
		return;
	}

	// create new log entry for predicted pickups
	if ( entityNum >= 0 ) {
		itemPickupEntry_t *entry = &ipl.itemPickups[( ipl.itemPickupIndex++ ) % MAX_ITEM_PICKUP_LOG];
		entry->commandTime = commandTime;
		entry->entityNum = entityNum;
		entry->itemNum = itemNum;
	}

	// execute the event
	CG_ExecutePredictableEvent( EV_ITEM_PICKUP, itemNum );
}

/*
====================
CG_FilterPredictableEvent

This function is called when predictable playerstate events are issued by either the client
prediction system or from server snapshot transitions.

Typically most of the events hitting this function will be duplicates and need to be filtered out,
since the client prediction resimulates the same moves and reissues events every frame. Server
events are also usually redundant to client predicted ones, but they are checked here in case the
client failed to predict an event for some reason (such as a modded server issuing different events).
====================
*/
void CG_FilterPredictableEvent( entity_event_t event, int eventParm, playerState_t *ps, qboolean serverEvent ) {
	int i;
	int dtSize = sizeof( debounceTable ) / sizeof( *debounceTable );
	entity_event_t debounceEvent = event;

	if ( event == EV_NONE ) {
		return;
	}

	if ( serverEvent ) {
		// If not doing playerstate prediction, execute server events without filtering.
		if ( !CG_PlayerstatePredictionActive() ) {
			CG_ExecutePredictableEvent( event, eventParm );
			return;
		}

		// If server command time is old, and prediction is active, ignore server events for consistency
		// with the original implementation. In particular this maintains the behavior of ignoring the
		// intermission taunt event issued by the original g_arenas.c CelebrateStart function.
		if ( cg.time > cg.snap->ps.commandTime + 1000 ) {
			return;
		}
	}

	if ( event == EV_ITEM_PICKUP ) {
		// this should only be from server, as client pickup prediction is handled separately
		CG_FilterItemPickup( -1, eventParm, ps->origin, ps->commandTime );
		return;
	}

	// Check for debounceable events
	if ( event == EV_FOOTSTEP_METAL ) {
		debounceEvent = EV_FOOTSTEP;
	} else if ( event == EV_STEP_8 || event == EV_STEP_12 || event == EV_STEP_16 ) {
		debounceEvent = EV_STEP_4;
	} else if ( event == EV_FALL_MEDIUM || event == EV_FALL_FAR ) {
		debounceEvent = EV_FALL_SHORT;
	} else if ( event > EV_USE_ITEM0 && event <= EV_USE_ITEM15 ) {
		debounceEvent = EV_USE_ITEM0;
	}

	for ( i = 0; i < dtSize; ++i ) {
		if ( debounceTable[i].event == debounceEvent ) {
			if ( debounceTable[i].lastCommandTime >= 0 &&
					ps->commandTime <= debounceTable[i].lastCommandTime + debounceTable[i].debounceTime ) {
				// Drop events if event was already predicted within the debounce time range for that event.
				if ( ps->commandTime <= debounceTable[i].lastCommandTime ) {
					if ( cg_debugEvents.integer >= 3 ) {
						CG_Printf( "^1filtered predictable event:^7 event(%i) eventParm(%i) commandTime(%i) serverEvent(%s)\n",
							event, eventParm, ps->commandTime, serverEvent ? "yes" : "no" );
					}
				} else {
					if ( cg_debugEvents.integer >= 2 ) {
						CG_Printf( "^3debounce-filtered predictable event:^7 event(%i) eventParm(%i) commandTime(%i) serverEvent(%s)\n",
							event, eventParm, ps->commandTime, serverEvent ? "yes" : "no" );
					}
				}
				return;
			}

			if ( debounceTable[i].lastCommandTime >= 0 &&
					( cg.time - debounceTable[i].lastClientTime ) > ( ps->commandTime - debounceTable[i].lastCommandTime ) * 2 ) {
				// Handle special case where a predicted event is pushed to a somewhat later command time as updated snapshots
				// are received, sufficient that the event might normally be repeated, but there is a large differential
				// between the old and new events in terms of actual client time.
				if ( cg_debugEvents.integer >= 2 ) {
					CG_Printf( "^5client-time-filtered predictable event:^7 event(%i) eventParm(%i) commandTime(%i) serverEvent(%s)\n",
						event, eventParm, ps->commandTime, serverEvent ? "yes" : "no" );
				}
				return;
			}

			if ( cg_debugEvents.integer >= 2 ) {
				CG_Printf( "^2exec predictable event:^7 event(%i) eventParm(%i) commandTime(%i) serverEvent(%s)\n",
					event, eventParm, ps->commandTime, serverEvent ? "yes" : "no" );
			}

			debounceTable[i].lastCommandTime = ps->commandTime;
			debounceTable[i].lastClientTime = cg.time;

			// Make sure predicted playerstate is up to date in case any event handling code accesses it
			if ( !serverEvent ) {
				cg.predictedPlayerState = *ps;
			}

			CG_ExecutePredictableEvent( event, eventParm );
			return;
		}
	}

	// This doesn't appear to be a normal predictable event. Possibly a server-side mod sent
	// a different event through the predictable event system?
	CG_ExecutePredictableEvent( event, eventParm );
}

/*
====================
CG_PatchWeaponAutoswitch

If we have a recent weapon pickup prediction, temporarily simulate weapon and ammo
so autoswitch weapon animation works correctly.
====================
*/
static void CG_PatchWeaponAutoswitch( playerState_t *ps ) {
	int i;

	for ( i = 0; i < MAX_ITEM_PICKUP_LOG; ++i ) {
		itemPickupEntry_t *entry = &ipl.itemPickups[i];

		if ( entry->commandTime > 0 && entry->commandTime + 100 > cg.snap->ps.commandTime ) {
			const gitem_t *item = &bg_itemlist[entry->itemNum];

			if ( item->giType == IT_WEAPON ) {
				ps->stats[STAT_WEAPONS] |= 1 << item->giTag;
				if ( !ps->ammo[item->giTag] ) {
					ps->ammo[item->giTag] = 1;
				}
			}
		}
	}
}

/*
====================
CG_HideGrabbedItems

Set the grid/invisible flags on items that are predicted to have been picked up.
====================
*/
static void CG_HideGrabbedItems( void ) {
	int i;

	for ( i = 0; i < MAX_ITEM_PICKUP_LOG; ++i ) {
		itemPickupEntry_t *entry = &ipl.itemPickups[i];
		const gitem_t *item = &bg_itemlist[entry->itemNum];
		centity_t *cent = &cg_entities[entry->entityNum];

		if ( entry->commandTime <= 0 ) {
			continue;
		}

		if ( item->giType == IT_TEAM ) {
			// don't do this with flags currently, as pickup prediction may not be reliable due to server
			// undercap settings etc. and then the flag would awkwardly disappear and reappear
			continue;
		}

		// pickup prediction was (roughly) more recent than server snapshot, or currently still touching item...
		if ( entry->commandTime + 100 > cg.snap->ps.commandTime || ( entry->commandTime + 600 > cg.snap->ps.commandTime &&
				entry->entityNum >= 0 && BG_PlayerTouchesItem( &cg.predictedPlayerState, &cg_entities[entry->entityNum].currentState, cg.time ) ) ) {
			// used for fade-in in case the item is restored due to misprediction
			cent->miscTime = cg.time;

			if ( item->giType == IT_WEAPON || item->giType == IT_POWERUP ) {
				// draw it "gridded out"
				cent->currentState.eFlags |= EF_ITEMPLACEHOLDER;
			} else {
				// remove it from the frame so it won't be drawn
				cent->currentState.eFlags |= EF_NODRAW;
			}
		}
	}
}

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList( void ) {
	int			i;
	centity_t	*cent;
	snapshot_t	*snap = PREDICTION_SNAPSHOT;
	entityState_t	*ent;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	for ( i = 0 ; i < snap->numEntities ; i++ ) {
		cent = &cg_entities[ snap->entities[ i ].number ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER ) {
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if ( cent->nextState.solid ) {
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}
}

/*
====================
CG_ClipMoveToEntities

====================
*/
#define SHIELD_HALFTHICKNESS		4	// should correspond with the #define in g_active.c

static void CG_ClipMoveToEntities ( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
							int skipNumber, int mask, trace_t *tr ) {
	int			i, x, zd, zu;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t 	cmodel;
	vec3_t		bmins, bmaxs;
	vec3_t		origin, angles;
	centity_t	*cent;

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];
		ent = &cent->currentState;

		if ( ent->number == skipNumber ) {
			continue;
		}

		if ( ent->solid == SOLID_BMODEL ) {
			// special value for bmodel
			cmodel = trap_CM_InlineModel( ent->modelindex );
			VectorCopy( cent->lerpAngles, angles );
			BG_EvaluateTrajectory( &cent->currentState.pos, cg.physicsTime, origin );
		}
		else if (ent->eFlags & EF_SHIELD_BOX_X)
		{	// "specially" encoded bbox for x-axis aligned shield

			x = (ent->solid & 255); // i on server side
			zd = ((ent->solid>>8) & 255);  // j on server side
			zu = ((ent->solid>>16) & 255);  // k on server side

			bmins[0] = -zd;
			bmaxs[0] = x;
			bmins[1] = -SHIELD_HALFTHICKNESS;
			bmaxs[1] = SHIELD_HALFTHICKNESS;
			bmins[2] = 0;
			bmaxs[2] = zu;

			cmodel = trap_CM_TempBoxModel( bmins, bmaxs );
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		}
		else if (ent->eFlags & EF_SHIELD_BOX_Y)
		{	// "specially" encoded bbox for y-axis aligned shield
			x = (ent->solid & 255); // i on server side
			zd = ((ent->solid>>8) & 255);  // j on server side
			zu = ((ent->solid>>16) & 255);  // k on server side

			bmins[1] = -zd;
			bmaxs[1] = x;
			bmins[0] = -SHIELD_HALFTHICKNESS;
			bmaxs[0] = SHIELD_HALFTHICKNESS;
			bmins[2] = 0;
			bmaxs[2] = zu;

			cmodel = trap_CM_TempBoxModel( bmins, bmaxs );
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		}
		else
		{
			// encoded bbox
			x = (ent->solid & 255); // i on server side
			zd = ((ent->solid>>8) & 255);  // j on server side
			zu = ((ent->solid>>16) & 255) - 32;  // k on server side

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = trap_CM_TempBoxModel( bmins, bmaxs );
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		}


		trap_CM_TransformedBoxTrace ( &trace, start, end,
			mins, maxs, cmodel,  mask, origin, angles);

		if (trace.allsolid || trace.fraction < tr->fraction) {
			trace.entityNum = ent->number;
			*tr = trace;
		} else if (trace.startsolid) {
			tr->startsolid = qtrue;
		}
		if ( tr->allsolid ) {
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void	CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
					int skipNumber, int mask ) {
	trace_t	t;

	trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t);

	*result = t;
}

/*
================
CG_PointContents
================
*/
int		CG_PointContents( const vec3_t point, int passEntityNum ) {
	int			i;
	entityState_t	*ent;
	centity_t	*cent;
	clipHandle_t cmodel;
	int			contents;

	contents = trap_CM_PointContents (point, 0);

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if ( ent->number == passEntityNum ) {
			continue;
		}

		if (ent->solid != SOLID_BMODEL) { // special value for bmodel
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		contents |= trap_CM_TransformedPointContents( point, cmodel, ent->origin, ent->angles );
	}

	return contents;
}


/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void CG_InterpolatePlayerState( qboolean grabAngles ) {
	float			f;
	int				i;
	playerState_t	*out;
	snapshot_t		*prev, *next;

	out = &cg.predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;

	*out = cg.snap->ps;

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles ) {
		usercmd_t	cmd;
		int			cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd );

		PM_UpdateViewAngles( out, &cmd );
	}

	// if the next frame is a teleport, we can't lerp to it
	if ( cg.nextFrameTeleport ) {
		return;
	}

	if ( !next || next->serverTime <= prev->serverTime ) {
		return;
	}

	f = (float)( cg.time - prev->serverTime ) / ( next->serverTime - prev->serverTime );

	i = next->ps.bobCycle;
	if ( i < prev->ps.bobCycle ) {
		i += 256;		// handle wraparound
	}
	out->bobCycle = prev->ps.bobCycle + f * ( i - prev->ps.bobCycle );

	for ( i = 0 ; i < 3 ; i++ ) {
		out->origin[i] = prev->ps.origin[i] + f * (next->ps.origin[i] - prev->ps.origin[i] );
		if ( !grabAngles ) {
			out->viewangles[i] = LerpAngle(
				prev->ps.viewangles[i], next->ps.viewangles[i], f );
		}
		out->velocity[i] = prev->ps.velocity[i] +
			f * (next->ps.velocity[i] - prev->ps.velocity[i] );
	}

}

/*
===================
CG_TouchItem
===================
*/
static void CG_TouchItem( playerState_t *ps, centity_t *cent ) {
	gitem_t		*item;

	if ( !cg_predictItems.integer ) {
		return;
	}

	if ( !BG_PlayerTouchesItem( ps, &cent->currentState, cg.time ) ) {
		return;
	}

	if ( !BG_CanItemBeGrabbed( &cent->currentState, ps ) ) {
		return;		// can't hold it
	}

	item = &bg_itemlist[ cent->currentState.modelindex ];

	// Special case for flags.
	// We don't predict touching our own flag
	if (item->giType == IT_TEAM)
	{	// NOTE:  This code used to JUST check giTag.  The problem is that the giTag for PW_REDFLAG
		// is the same as WP_QUANTUM_BURST.  The giTag should be a SUBCHECK after giType.
		if (ps->persistant[PERS_TEAM] == TEAM_RED &&
			item->giTag == PW_REDFLAG)
			return;
		if (ps->persistant[PERS_TEAM] == TEAM_BLUE &&
			item->giTag == PW_BLUEFLAG)
			return;
	}

	if ( !( cent->currentState.eFlags & ( EF_ITEMPLACEHOLDER | EF_NODRAW ) ) ) {
		// grab it
		CG_FilterItemPickup( cent->currentState.number, cent->currentState.modelindex, ps->origin, ps->commandTime );
	}
}

/*
=========================
CG_GetPlayerBounds

Obtain player mins and maxs as set by PM_CheckDuck.
=========================
*/
static void CG_GetPlayerBounds( playerState_t *ps, vec3_t mins, vec3_t maxs ) {
	mins[0] = -15;
	mins[1] = -15;
	mins[2] = MINS_Z;

	maxs[0] = 15;
	maxs[1] = 15;

	if (ps->pm_type == PM_DEAD) {
		maxs[2] = -8;
	} else if ( ps->pm_flags & PMF_DUCKED ) {
		maxs[2] = 16;
	} else {
		maxs[2] = 32;
	}
}

/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and teleports
=========================
*/
static void CG_TouchTriggerPrediction( playerState_t *ps, qboolean *predictedTeleport ) {
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t cmodel;
	centity_t	*cent;
	qboolean	spectator;
	vec3_t		mins, maxs;

	*predictedTeleport = qfalse;

	// dead clients don't activate triggers
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	spectator = ( ps->pm_type == PM_SPECTATOR );

	if ( ps->pm_type != PM_NORMAL && !spectator ) {
		return;
	}

	CG_GetPlayerBounds( ps, mins, maxs );

	for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM ) {
			if ( spectator ) {
				continue;
			}

			// optimize common case
			if ( cent->currentState.pos.trType == TR_STATIONARY &&
					( ps->origin[0] - cent->currentState.pos.trBase[0] > 44 || ps->origin[0] - cent->currentState.pos.trBase[0] < -50 ) ) {
				continue;
			}

			CG_TouchItem( ps, cent );
		}

		if ( ent->solid != SOLID_BMODEL ) {
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		trap_CM_BoxTrace( &trace, ps->origin, ps->origin, mins, maxs, cmodel, -1 );

		if ( !trace.startsolid ) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			*predictedTeleport = qtrue;
		} else {
			float	s;
			vec3_t	dir;

			// we hit this push trigger
			if ( spectator ) {
				continue;
			}

			// flying characters don't hit bounce pads
			if ( ps->powerups[PW_FLIGHT] ) {
				continue;
			}

			// if we are already flying along the bounce direction, don't play sound again
			VectorNormalize2( ent->origin2, dir );
			s = DotProduct( ps->velocity, dir );
			if ( s < 500 ) {
				// don't play the event sound again if we are in a fat trigger
				BG_AddPredictableEventToPlayerstate( EV_JUMP_PAD, 0, ps );
			}
			VectorCopy( ent->origin2, ps->velocity );
		}
	}
}

/*
=================
CG_ModifyFireRate

Support using custom fire rate values specified by server.
=================
*/
static int CG_ModifyFireRate( int defaultInterval, int weapon, qboolean alt ) {
	const int *fireRates = alt ? cgs.modConfig.fireRates.alt : cgs.modConfig.fireRates.primary;
	if ( EF_WARN_ASSERT( weapon > WP_NONE && weapon < WP_NUM_WEAPONS ) && fireRates[weapon] ) {
		return fireRates[weapon];
	}

	return defaultInterval;
}

/*
=================
CG_UpdateCmdBuffer

Retrieves latest usercmds from engine and adds them to command buffer.

The purpose of the command buffer is to increase the number of stored commands from the
standard number provided by the engine (CMD_BACKUP: 64). With only 64 commands, players
with a combination of high ping (150+) and high com_maxfps (250+) can hit the command
limit and get stuck with a "connection interrupted" message.
=================
*/
static void CG_UpdateCmdBuffer( void ) {
	int latestCmd = trap_GetCurrentCmdNumber();
	int currentCmd;

	if ( latestCmd < cg.mapRestartUsercmd ) {
		// Shouldn't happen
		cg.mapRestartUsercmd = 0;
	}

	// copy commands to buffer
	for ( currentCmd = latestCmd;; --currentCmd ) {
		usercmd_t *bufferEntry = &CMD_BUFFER_ENTRY( currentCmd );

		// stop reading commands once we reach one that is already buffered
		if ( cmdBuffer.latest == currentCmd && cmdBuffer.latest > cmdBuffer.tail ) {
			break;
		}

		// stop reading if we reach a command that is out of range for engine
		if ( currentCmd <= cg.mapRestartUsercmd || currentCmd <= latestCmd - CMD_BACKUP ) {
			break;
		}

		// retrieve the command
		if ( !trap_GetUserCmd( currentCmd, bufferEntry ) ) {
			// Shouldn't happen
			cmdBuffer.tail = currentCmd;
			break;
		}
	}

	// update latest command
	cmdBuffer.latest = latestCmd;

	// update tail
	if ( cmdBuffer.tail > currentCmd )
		cmdBuffer.tail = currentCmd;
	if ( cmdBuffer.tail < cg.mapRestartUsercmd )
		cmdBuffer.tail = cg.mapRestartUsercmd;
	if ( cmdBuffer.tail < cmdBuffer.latest - MAX_CMD_BUFFER )
		cmdBuffer.tail = cmdBuffer.latest - MAX_CMD_BUFFER;
}

/*
=================
CG_ComparePredictedPlayerstate

Returns qtrue if predicted playerstate is sufficiently equivalent to actual server snapshot to
keep using it as a prediction basis.
=================
*/
static qboolean CG_ComparePredictedPlayerstate( playerState_t *predictedPS, playerState_t *snapPS ) {
	playerState_t predictedPSAdjusted = *predictedPS;
	vec3_t originDelta;

	VectorSubtract( predictedPS->origin, snapPS->origin, originDelta );

	// ignore discrepancies in certain fields that are irrelevant or handled specially
	VectorCopy( snapPS->viewangles, predictedPSAdjusted.viewangles );
	VectorCopy( snapPS->origin, predictedPSAdjusted.origin );
	memcpy( predictedPSAdjusted.events, snapPS->events, sizeof( predictedPSAdjusted.events ) );
	memcpy( predictedPSAdjusted.eventParms, snapPS->eventParms, sizeof( predictedPSAdjusted.eventParms ) );
	predictedPSAdjusted.eventSequence = snapPS->eventSequence;
	predictedPSAdjusted.entityEventSequence = snapPS->entityEventSequence;
	predictedPSAdjusted.rechargeTime = snapPS->rechargeTime;

	if ( memcmp( snapPS, &predictedPSAdjusted, sizeof( playerState_t ) ) || VectorLengthSquared( originDelta ) > 0.1f * 0.1f ) {
		return qfalse;
	}

	return qtrue;
}

/*
=================
CG_GetPredictionBasis

Returns index in prediction cache of frame to be used as basis for client prediction, i.e. the frame
that matches the current server snapshot.

If no predicted frame matches the server snapshot, or caching is disabled, prediction cache will be
reset and a new basis frame will be created by copying the snapshot.
=================
*/
static int CG_GetPredictionBasis( void ) {
	int frameNum;
	snapshot_t *snap = PREDICTION_SNAPSHOT;

	if ( cg_predictCache.integer ) {
		// Scan cache backwards looking for frame matching snapshot
		for ( frameNum = predictCache.latest; frameNum > predictCache.tail; --frameNum ) {
			predictionFrame_t *cacheEntry = &PREDICT_CACHE_ENTRY( frameNum );

			if ( cacheEntry->ps.commandTime == snap->ps.commandTime ) {
				if ( CG_ComparePredictedPlayerstate( &cacheEntry->ps, &snap->ps ) ) {
					return frameNum;
				}
			}
		
			if ( cacheEntry->ps.commandTime < snap->ps.commandTime ) {
				break;
			}
		}
	}

	// Reset cache and create new frame
	if ( cg_predictCache.integer >= 2 ) {
		CG_Printf( "Resetting predict cache: snapTime(%i) commandTime(%i)\n", snap->serverTime, snap->ps.commandTime );
	}
	predictCache.tail = -1;
	predictCache.latest = 0;
	predictCache.entries[0].ps = snap->ps;
	predictCache.entries[0].predictedTeleport = qfalse;
	return 0;
}

/*
=================
CG_InitPmove

Sets up static "cg_pmove" structure ahead of prediction move.
=================
*/
static void CG_InitPmove( playerState_t *ps, usercmd_t *cmd ) {
	cg_pmove.ps = ps;
	cg_pmove.cmd = *cmd;

	cg_pmove.trace = CG_Trace;
	cg_pmove.pointcontents = CG_PointContents;
	if ( cg_pmove.ps->pm_type == PM_DEAD ) {
		cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else {
		cg_pmove.tracemask = MASK_PLAYERSOLID;
	}
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || (cg.snap->ps.eFlags&EF_ELIMINATED) ) {
		cg_pmove.tracemask &= ~CONTENTS_BODY;	// spectators can fly through bodies
	}
	cg_pmove.noFootsteps = ( cgs.dmflags & DF_NO_FOOTSTEPS ) > 0;
	cg_pmove.pModDisintegration = cgs.pModDisintegration > 0;

	cg_pmove.altFireMode = CG_AltFire_PredictionMode( (weapon_t)ps->weapon );
	cg_pmove.modifyFireRate = CG_ModifyFireRate;
	cg_pmove.noJumpKeySlowdown = cgs.modConfig.noJumpKeySlowdown;
	cg_pmove.bounceFix = cgs.modConfig.bounceFix;
	cg_pmove.snapVectorGravLimit = cgs.modConfig.snapVectorGravLimit;
	cg_pmove.noFlyingDrift = cgs.modConfig.noFlyingDrift;
	cg_pmove.infilJumpFactor = cgs.modConfig.infilJumpFactor;
	cg_pmove.infilAirAccelFactor = cgs.modConfig.infilAirAccelFactor;
}

/*
=================
CG_HaveStepEvent
=================
*/
static qboolean CG_HaveStepEvent( playerState_t *ps ) {
	int i;

	for ( i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; ++i ) {
		if ( i >= 0 ) {
			int event = ps->events[i & ( MAX_PS_EVENTS - 1 )];
			if ( event == EV_STEP_4 || event == EV_STEP_8 || event == EV_STEP_12 || event == EV_STEP_16 )
				return qtrue;
		}
	}

	return qfalse;
}

/*
=================
CG_FinalPredict
=================
*/
static void CG_FinalPredict( playerState_t *ps, usercmd_t *cmd ) {
	playerState_t temp = *ps;
	temp.eventSequence = 0;

	CG_InitPmove( &temp, cmd );
	Pmove( &cg_pmove, 0, NULL, NULL );

	ps->origin[0] = temp.origin[0];
	ps->origin[1] = temp.origin[1];
	if ( !CG_HaveStepEvent( &temp ) ) {
		ps->origin[2] = temp.origin[2];
	}

	VectorCopy( temp.viewangles, ps->viewangles );
	ps->bobCycle = temp.bobCycle;
}

/*
=================
CG_PostPmove
=================
*/
static void CG_PostPmove( pmove_t *pmove, qboolean finalFragment, void *context) {
	predictionFrame_t *frame = (predictionFrame_t *)context;
	if ( finalFragment || cgs.modConfig.pMoveTriggerMode ) {
		int i;

		CG_TouchTriggerPrediction( &frame->ps, &frame->predictedTeleport );

		// Note that running events here means that CG_FilterPredictableEvent won't be called for cached
		// frames. This should be fine since cached frames should only contain duplicate events that would
		// filtered anyway, but it might have implications for testing/debug purposes.
		for ( i = frame->ps.eventSequence - MAX_PS_EVENTS; i < frame->ps.eventSequence; ++i ) {
			if ( i >= 0 ) {
				CG_FilterPredictableEvent( frame->ps.events[ i & (MAX_PS_EVENTS-1) ],
						frame->ps.eventParms[ i & (MAX_PS_EVENTS-1) ], &frame->ps, qfalse );
			}
		}
		frame->ps.eventSequence = 0;
	}
}

/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
void CG_PredictPlayerState( void ) {
	snapshot_t	*snap = PREDICTION_SNAPSHOT;
	playerState_t	oldPlayerState;
	usercmd_t	*latestCmd;

	int cmdNum;
	int frameNum;
	predictionFrame_t *frame;

	cg.hyperspace = qfalse;	// will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// predictedPlayerState is valid even if there is some
	// other error condition
	if ( !cg.validPPS ) {
		char temp_string[200];
		cg.validPPS = qtrue;
		cg.predictedPlayerState = cg.snap->ps;
		// if we need to, we should check our model cvar and update it with the right value from the userinfo strings
		// since it may have been modified by the server
		Com_sprintf(temp_string, sizeof(temp_string), "%s/%s", cgs.clientinfo[cg.predictedPlayerState.clientNum].modelName,
			cgs.clientinfo[cg.predictedPlayerState.clientNum].skinName);
		updateSkin(cg.predictedPlayerState.clientNum, temp_string);
	}

	// demo playback just copies the moves
	if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		CG_InterpolatePlayerState( qfalse );
		return;
	}

	// non-predicting local movement will grab the latest angles
	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_InterpolatePlayerState( qtrue );
		return;
	}

	CG_UpdateCmdBuffer();

	if ( cmdBuffer.tail >= cmdBuffer.latest ) {
		if ( cg_showmiss.integer ) {
			CG_Printf ("skipping prediction - no valid commands\n");
		}
		return;
	}

	latestCmd = &CMD_BUFFER_ENTRY( cmdBuffer.latest );

	// don't do predictions if we are lagged out and snapshot is way behind command time
	if ( snap->ps.commandTime + 1000 < latestCmd->serverTime ) {
		if ( cg_showmiss.integer ) {
			CG_Printf ("skipping prediction - snapshot too old\n");
		}
		return;
	}

	// also don't predict if we don't have commands immediately following snapshot
	// NOTE: currently skipping this check when command tail interects with a map restart
	// not certain about this, but it appears to match the behavior that the original
	// implementation exhibits due to nulled usercmds...
	if ( CMD_BUFFER_ENTRY( cmdBuffer.tail + 1 ).serverTime > cg.snap->ps.commandTime
			&& cmdBuffer.tail > cg.mapRestartUsercmd ) {
		if ( cg_showmiss.integer ) {
			CG_Printf ("skipping prediction - snapshot older than oldest cached commands\n");
		}
		return;
	}

	cg.physicsTime = snap->serverTime;

	frameNum = CG_GetPredictionBasis();
	frame = &PREDICT_CACHE_ENTRY( frameNum );

	// save the state from last client frame so we can detect transitions
	oldPlayerState = cg.predictedPlayerState;

	// iterate usercmds generating and/or processing pmove frames as needed
	for ( cmdNum = cmdBuffer.tail + 1; cmdNum <= cmdBuffer.latest; ++cmdNum ) {
		usercmd_t *cmd = &CMD_BUFFER_ENTRY( cmdNum );

		// check for error decay from current frame
		if ( frame->ps.commandTime == lastMoveTime && !cg.thisFrameTeleport ) {
			vec3_t	adjusted;
			vec3_t	delta;
			float	len;

			// make sure the error decay is only run once per frame, even if pmove sets the same
			// commandTime for multiple usercmds due to fixed frame lengths
			lastMoveTime = -1;

			CG_AdjustPositionForMover( frame->ps.origin,
				frame->ps.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted );

			if ( cg_showmiss.integer ) {
				if (!VectorCompare( lastMoveOrigin, adjusted )) {
					CG_Printf("prediction error\n");
				}
			}
			VectorSubtract( lastMoveOrigin, adjusted, delta );
			len = VectorLength( delta );
			if ( len > 0.1 ) {
				if ( cg_showmiss.integer ) {
					CG_Printf("Prediction miss: %f\n", len);
				}
				if ( cg_errorDecay.integer ) {
					int		t;
					float	f;

					t = cg.time - cg.predictedErrorTime;
					f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
					if ( f < 0 ) {
						f = 0;
					}
					if ( f > 0 && cg_showmiss.integer ) {
						CG_Printf("Double prediction decay: %f\n", f);
					}
					VectorScale( cg.predictedError, f, cg.predictedError );
				} else {
					VectorClear( cg.predictedError );
				}
				VectorAdd( delta, cg.predictedError, cg.predictedError );
				cg.predictedErrorTime = cg.oldTime;
			}
		}

		// check if current usercmd has time for pmove frame
		if ( PM_IsMoveNeeded( frame->ps.commandTime, cmd->serverTime, cgs.modConfig.pMoveFixed ) ) {
			predictionFrame_t *oldFrame = frame;
			frameNum += 1;
			frame = &PREDICT_CACHE_ENTRY( frameNum );

			// if we don't have a valid cached frame...
			if ( frameNum > predictCache.latest || frame->cmdNum != cmdNum ) {
				frame->ps = oldFrame->ps;
				memset( frame->ps.events, 0, sizeof( frame->ps.events ) );
				frame->ps.eventSequence = 0;
				frame->predictedTeleport = qfalse;
				frame->cmdNum = cmdNum;

				// fudge some values due to rechargeTime not being transferred over network
				frame->ps.rechargeTime = 1000;
				if ( frame->ps.ammo[WP_PHASER] < 19 ) {
					frame->ps.ammo[WP_PHASER] = 19;
				}

				// perform the move
				CG_InitPmove( &frame->ps, cmd );
				Pmove( &cg_pmove, cgs.modConfig.pMoveFixed, CG_PostPmove, frame );
				CG_PatchWeaponAutoswitch( &frame->ps );

				predictCache.latest = frameNum;
			}

			// check for predicted teleport...
			if ( frame->predictedTeleport ) {
				cg.hyperspace = qtrue;
			}
		}
	}

	// update tail range
	if ( predictCache.tail < predictCache.latest - MAX_PREDICT_CACHE )
		predictCache.tail = predictCache.latest - MAX_PREDICT_CACHE;

	// save last frame from prediction for rest of cgame to access
	cg.predictedPlayerState = frame->ps;

	if ( cg.thisFrameTeleport ) {
		// reset error decay on teleport
		VectorClear( cg.predictedError );
		if ( cg_showmiss.integer ) {
			CG_Printf( "PredictionTeleport\n" );
		}
		cg.thisFrameTeleport = qfalse;
	}

	// set error decay markers for next frame
	lastMoveTime = cg.predictedPlayerState.commandTime;
	CG_AdjustPositionForMover( cg.predictedPlayerState.origin,
		cg.predictedPlayerState.groundEntityNum,
		cg.physicsTime, cg.time, lastMoveOrigin );

	if ( cg_showmiss.integer > 1 ) {
		CG_Printf( "[%i : %i] ", latestCmd->serverTime, cg.time );
	}

	// simulate any movement fragment left over from main move due to fixed frame length
	if ( latestCmd->serverTime > cg.predictedPlayerState.commandTime ) {
		CG_FinalPredict( &cg.predictedPlayerState, latestCmd );
	}

	// adjust for the movement of the groundentity
	CG_AdjustPositionForMover( cg.predictedPlayerState.origin,
		cg.predictedPlayerState.groundEntityNum,
		cg.physicsTime, cg.time, cg.predictedPlayerState.origin );

	// set grid/invisibility on predicted picked up items
	CG_HideGrabbedItems();

	// fire transition triggered things
	CG_TransitionPlayerState( &cg.predictedPlayerState, &oldPlayerState );
}
