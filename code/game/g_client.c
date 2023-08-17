// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

// g_client.c -- client functions that don't happen every frame

static vec3_t	playerMins = {-15, -15, -24};
static vec3_t	playerMaxs = {15, 15, 32};

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( vec3_t origin ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( origin, playerMins, mins );
	VectorAdd( origin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
			return qtrue;
		}
		if (hit->s.eType == ET_USEABLE && hit->s.modelindex == HI_SHIELD) {	//hit a portable force field
			return qtrue;
		}


	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
static gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
static gentity_t *SelectRandomDeathmatchSpawnPoint( qboolean useHumanSpots, qboolean useBotSpots ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL && count < MAX_SPAWN_POINTS) {
		if ( ( spot->flags & FL_NO_BOTS ) && !useHumanSpots ) {
			continue;
		}
		if ( ( spot->flags & FL_NO_HUMANS ) && !useBotSpots ) {
			continue;
		}
		if ( SpotWouldTelefrag( spot->s.origin ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean useHumanSpots, qboolean useBotSpots ) {
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( useHumanSpots, useBotSpots );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( useHumanSpots, useBotSpots );
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( useHumanSpots, useBotSpots );
		}
	}

	if (!spot) {
		// give mods a chance to find a spawn
		modfn.PatchClientSpawn( -1, &spot, origin, angles );
		if (!spot) {
			G_Error( "Couldn't find a spawn point" );
		}
		return spot;
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
static gentity_t *SelectInitialSpawnPoint( vec3_t origin, vec3_t angles, qboolean useHumanSpots, qboolean useBotSpots ) {
	gentity_t	*spot;

	spot = NULL;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( spot->spawnflags & 1 ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot->s.origin ) ) {
		return SelectSpawnPoint( vec3_origin, origin, angles, useHumanSpots, useBotSpots );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
static gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

#define BODY_QUEUE_MIN 8
#define BODY_QUEUE_MAX 32

static int			bodyFadeSound=0;
static int			bodyQueIndex;			// dead bodies
static int			bodyQueSize;
static gentity_t	*bodyQue[BODY_QUEUE_MAX];

/*
===============
InitBody
===============
*/
static void InitBody( gentity_t *ent ) {
	memset( ent, 0, sizeof( *ent ) );
	G_InitGentity( ent );
	ent->classname = "bodyque";
	ent->neverFree = qtrue;
}

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	bodyQueIndex = 0;
	bodyQueSize = 0;

	// try to allocate bodies within MAX_CLIENTS space if possible
	// original EF cgame has issues with body entities exceeding MAX_CLIENTS -
	// CG_PlayerSprites makes buggy references to cgs.clientinfo indexed by the body entity number
	for ( i = level.maxclients; i < MAX_CLIENTS && bodyQueSize < BODY_QUEUE_MAX; ++i ) {
		ent = &g_entities[i];
		if ( !ent->inuse ) {
			InitBody( ent );
			bodyQue[bodyQueSize++] = ent;
		}
	}

	// fall back to standard method if not enough slots allocated
	if ( bodyQueSize < BODY_QUEUE_MIN ) {
		G_Printf( "WARNING: Failed to allocate body que within MAX_CLIENTS entity space.\n" );

		while ( bodyQueSize < BODY_QUEUE_MIN ) {
			ent = G_Spawn();
			InitBody( ent );
			bodyQue[bodyQueSize++] = ent;
		}
	}

	if (bodyFadeSound == 0)
	{	// Initialize this sound.
		bodyFadeSound = G_SoundIndex("sound/enemies/borg/walkthroughfield.wav");
	}
}

/*
=============
BodyRezOut

After sitting around for five seconds, fade out.
=============
*/
void BodyRezOut( gentity_t *ent )
{
	if ( level.time - ent->timestamp >= 7500 ) {
		// the body ques are never actually freed, they are just unlinked
		trap_UnlinkEntity( ent );
		ent->physicsObject = qfalse;
		return;
	}

	ent->nextthink = level.time + 2500;
	ent->s.time = level.time + 2500;

	G_AddEvent(ent, EV_GENERAL_SOUND, bodyFadeSound);
}

/*
=============
(ModFN) CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.

If source player was gibbed, s.eType will be ET_INVISIBLE. The body inherits this and
will also be invisible, but can still interact with weapon fire...

If source player died with PW_DISINTEGRATE or PW_EXPLODE the body will be set to
invisible but may still be able to be gibbed...
=============
*/
gentity_t *ModFNDefault_CopyToBodyQue( int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	gentity_t		*body;
	int			contents;

	trap_UnlinkEntity (ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
	//	ent->s.eFlags &= ~EF_NODRAW;	// Just in case we died from a bottomless pit, reset EF_NODRAW
		return NULL;
	}

	// grab a body que and cycle to the next one
	body = bodyQue[ bodyQueIndex ];
	bodyQueIndex = (bodyQueIndex + 1) % bodyQueSize;

	InitBody( body );
	body->s = ent->s;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc
	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT ) {
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->r.ownerNum;

	body->nextthink = level.time + 5000;
	body->think = BodyRezOut;

	body->die = body_die;

	// if there shouldn't be a body, don't show one.
	if (ent->client &&
			((level.time - ent->client->ps.powerups[PW_DISINTEGRATE]) < 10000 ||
			(level.time - ent->client->ps.powerups[PW_EXPLODE]) < 10000))
	{
		body->s.eFlags |= EF_NODRAW;
	}

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}


	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap_LinkEntity (body);
	return body;
}

//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

/*
================
(ModFN) SpectatorClient

Returns whether specified client is currently in spectator mode.
================
*/
qboolean ModFNDefault_SpectatorClient( int clientNum ) {
	if ( G_AssertConnectedClient( clientNum ) && level.clients[clientNum].sess.sessionTeam == TEAM_SPECTATOR ) {
		return qtrue;
	}

	return qfalse;
}

/*
================
(ModFN) EffectiveScore

Returns effective score values to use for client.
================
*/
int ModFNDefault_EffectiveScore( int clientNum, effectiveScoreType_t type ) {
	if ( !G_AssertConnectedClient( clientNum ) ) {
		return 0;
	}
	
	if ( type != EST_SCOREBOARD && modfn.SpectatorClient( clientNum ) &&
			level.clients[clientNum].sess.spectatorState == SPECTATOR_FOLLOW ) {
		// If currently spectating and following somebody, PERS_SCORE will be copied from the
		// followed player. Use the copied score for scoreboard, for consistency with original
		// behavior, as it can be useful as a way to tell which player somebody is following.
		// For other purposes (such as external status queries) treat the score as 0.
		return 0;
	}

	return level.clients[clientNum].ps.persistant[PERS_SCORE];
}

/*
================
(ModFN) RealSessionTeam

Returns player-selected team, even if active team is overriden by borg assimilation.
================
*/
team_t ModFNDefault_RealSessionTeam( int clientNum ) {
	return level.clients[clientNum].sess.sessionTeam;
}

/*
================
(ModFN) RealSessionClass

Returns player-selected class, even if active class is overridden by borg assimilation.
================
*/
pclass_t ModFNDefault_RealSessionClass( int clientNum ) {
	return level.clients[clientNum].sess.sessionClass;
}

/*
================
(ModFN) ClientRespawn
================
*/
LOGFUNCTION_EVOID( ModFNDefault_ClientRespawn, ( int clientNum ), ( clientNum ), clientNum, "G_MODFN_CLIENTRESPAWN G_CLIENTSTATE" ) {
	gentity_t *ent = &g_entities[clientNum];
	modfn.CopyToBodyQue( clientNum );
	ClientSpawn(ent, CST_RESPAWN);
}

/*
================
TeamCount

Returns number of players on a team
================
*/
int TeamCount( int ignoreClientNum, team_t team, qboolean ignoreBots ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( ignoreBots && ( g_entities[i].r.svFlags & SVF_BOT ) ) {
			continue;
		}
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			count++;
		}
	}

	return count;
}


/*
================
PickTeam

================
*/
team_t PickTeam( int clientNum ) {
	int redPlayers = 0;
	int bluePlayers = 0;

	// try to pick team with least human players, unless we are adding a bot
	if ( EF_WARN_ASSERT( clientNum >= 0 ) && !( g_entities[clientNum].r.svFlags & SVF_BOT ) ) {
		bluePlayers = TeamCount( clientNum, TEAM_BLUE, qtrue );
		redPlayers = TeamCount( clientNum, TEAM_RED, qtrue );
	}

	// still undetermined, so pick team with least players overall
	if ( bluePlayers == redPlayers ) {
		bluePlayers = TeamCount( clientNum, TEAM_BLUE, qfalse );
		redPlayers = TeamCount( clientNum, TEAM_RED, qfalse );
	}

	if ( bluePlayers > redPlayers ) {
		return TEAM_RED;
	}
	if ( redPlayers > bluePlayers ) {
		return TEAM_BLUE;
	}

	// equal team count, so join the team with the lowest score
	if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] ) {
		return TEAM_RED;
	}
	if ( level.teamScores[TEAM_BLUE] < level.teamScores[TEAM_RED] ) {
		return TEAM_BLUE;
	}
	return irandom( TEAM_RED, TEAM_BLUE );
}


/*
===========
ClientCleanName
============
*/
static void ClientCleanName( const char *in, char *out, int outSize ) {
	int		len, colorlessLen;
	char	ch;
	char	*p;
	int		spaces;

	//save room for trailing null byte
	outSize--;

	len = 0;
	colorlessLen = 0;
	p = out;
	*p = 0;
	spaces = 0;

	while( 1 ) {
		ch = *in++;
		if( !ch ) {
			break;
		}

		// don't allow leading spaces
		if( !*p && ch == ' ' ) {
			continue;
		}

		// check colors
		if( ch == Q_COLOR_ESCAPE ) {
			// solo trailing carat is not a color prefix
			if( !*in ) {
				break;
			}

			// don't allow black in a name, period
			if( ColorIndex(*in) == 0 ) {
				in++;
				continue;
			}

			// make sure room in dest for both chars
			if( len > outSize - 2 ) {
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len += 2;
			continue;
		}

		// don't allow too many consecutive spaces
		if( ch == ' ' ) {
			spaces++;
			if( spaces > 3 ) {
				continue;
			}
		}
		else {
			spaces = 0;
		}

		if( len > outSize - 1 ) {
			break;
		}

		*out++ = ch;
		colorlessLen++;
		len++;
	}
	*out = 0;

	// don't allow empty names
	if( *p == 0 || colorlessLen == 0 ) {
		Q_strncpyz( p, "RedShirt", outSize );
	}
}

/*
===========
SetSkinForModel

Converts model name to <model>/<skin> format, stripping existing skin if it already exists.
Can be called with null skin to just get the model name.
===========
*/
void SetSkinForModel( const char *model, const char *skin, char *output, unsigned int size ) {
	char temp[256];
	char *p;
	Q_strncpyz( temp, model, sizeof( temp ) );
	p = strchr( temp, '/' );
	if ( p )
		*p = '\0';

	if ( skin ) {
		if ( strlen( temp ) + strlen( skin ) + 1 >= size ) {
			Com_sprintf( output, size, "munro/%s", skin );
		} else {
			Com_sprintf( output, size, "%s/%s", temp, skin );
		}
	} else {
		Q_strncpyz( output, temp, size );
	}
}

/*
===========
(ModFN) GetPlayerModel

Retrieves player model string for client, performing any mod conversions as needed.
============
*/
void ModFNDefault_GetPlayerModel( int clientNum, const char *userinfo, char *output, unsigned int outputSize ) {
	Q_strncpyz( output, Info_ValueForKey( userinfo, "model" ), outputSize );
}

/*
===========
(ModFN) EffectiveHandicap

Returns effective handicap values to use for client.
============
*/
int ModFNDefault_EffectiveHandicap( int clientNum, effectiveHandicapType_t type ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->pers.handicap < 1 || client->pers.handicap > 100 ) {
		return 100;
	}

	return client->pers.handicap;
}

/*
===========
ClientUserinfoChanged

Processes userinfo updates from the client, which includes values such as name, model, handicap, etc.
and updates configstrings to share the data with other clients.

This should be called any time characteristics such as class or team are changed that might affect
the data set by this function. This function should be safe to call at any time and should not have
any effect if nothing has changed.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent;
	char	*s;
	char	model[MAX_QPATH];
	char	oldname[MAX_STRING_CHARS];
	gclient_t	*client;
	char	*c1;
	char	userinfo[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate(userinfo) ) {
		strcpy (userinfo, "\\name\\badinfo");
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) ) {
		client->pers.localClient = qtrue;
	}

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) ) {
		client->pers.predictItemPickup = qfalse;
	} else {
		client->pers.predictItemPickup = qtrue;
	}

	// set name
	Q_strncpyz ( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey (userinfo, "name");
	ClientCleanName( s, client->pers.netname, sizeof(client->pers.netname) );

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			Q_strncpyz( client->pers.netname, "scoreboard", sizeof(client->pers.netname) );
		}
	}

	if ( client->pers.connected == CON_CONNECTED ) {
		if ( strcmp( oldname, client->pers.netname ) ) {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " renamed to %s\n\"", oldname,
				client->pers.netname) );
		}
	}

	// set handicap and max health
	client->pers.handicap = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	client->ps.stats[STAT_MAX_HEALTH] = modfn.EffectiveHandicap( clientNum, EH_MAXHEALTH );

	// set model
	*model = '\0';
	modfn.GetPlayerModel( clientNum, userinfo, model, sizeof( model ) );
	if ( *model == '\0' ) {
		Q_strncpyz( model, "munro/red", sizeof( model ) );
	}

	if ( client->sess.sessionTeam == TEAM_RED ) {
		SetSkinForModel( model, "red", model, sizeof( model ) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		SetSkinForModel( model, "blue", model, sizeof( model ) );
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// client requires team skin for spectators
		SetSkinForModel( model, "red", model, sizeof( model ) );
	}

	// colors
	c1 = Info_ValueForKey( userinfo, "color" );

	// teamInfo
	s = Info_ValueForKey( userinfo, "teamoverlay" );
	if ( ! *s || atoi( s ) != 0 ) {
		client->pers.teamInfo = qtrue;
	} else {
		client->pers.teamInfo = qfalse;
	}

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	if ( ent->r.svFlags & SVF_BOT ) {
		s = va("n\\%s\\t\\%i\\p\\%i\\model\\%s\\c1\\%s\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s",
			client->pers.netname, client->sess.sessionTeam, client->sess.sessionClass, model, c1,
			modfn.EffectiveHandicap( clientNum, EH_VISIBLE ), client->sess.wins, client->sess.losses,
			Info_ValueForKey( userinfo, "skill" ) );
	} else {
		s = va("n\\%s\\t\\%i\\p\\%i\\model\\%s\\c1\\%s\\hc\\%i\\w\\%i\\l\\%i",
			client->pers.netname, client->sess.sessionTeam, client->sess.sessionClass, model, c1,
			modfn.EffectiveHandicap( clientNum, EH_VISIBLE ), client->sess.wins, client->sess.losses );
	}

	// only bother to update if something changed, to reduce excess log messages
	{
		char buffer[1024];
		trap_GetConfigstring( CS_PLAYERS + clientNum, buffer, sizeof( buffer ) );
		buffer[sizeof( buffer ) - 1] = '\0';

		if ( strcmp( buffer, s ) ) {
			trap_SetConfigstring( CS_PLAYERS + clientNum, s );
			G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );
		}
	}
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char		*value;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	gentity_t	*ent;

	ent = &g_entities[ clientNum ];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	if ( G_FilterPacket( value ) ) {
		return "Banned.";
	}

	// check for password, but exempt bots and local player
	if (!isBot && strcmp(value, "localhost"))
	{
		value = Info_ValueForKey (userinfo, "password");
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) &&
			strcmp( g_password.string, value) != 0) {
			return "Invalid password";
		}
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	memset( client, 0, sizeof(*client) );

	client->pers.connected = CON_CONNECTING;
	client->pers.firstTime = firstTime;

	// read or initialize the session data
	if ( firstTime ) {
		G_InitSessionData( client, userinfo );
	} else {
		G_ReadSessionData( client );
	}

	if( isBot ) {
		if( !G_BotConnect( clientNum, !firstTime ) ) {
			return "BotConnectfailed";
		}
		ent->r.svFlags |= SVF_BOT;
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	ClientUserinfoChanged( clientNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime ) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname) );
	}

	if ( g_gametype.integer >= GT_TEAM &&
		client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	// if we are at the intermission, update the scoreboard
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}

	modfn.PostClientConnect( clientNum, firstTime, isBot );

	return NULL;
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum ) {
	gentity_t	*ent = &g_entities[clientNum];
	gclient_t	*client = &level.clients[clientNum];
	int			flags;
	clientSpawnType_t spawnType;

	EF_WARN_ASSERT( client->pers.connected >= CON_CONNECTING );

	if( client->botDelayBegin ) {
		G_QueueBotBegin( clientNum );
		client->botDelayBegin = qfalse;
		return;
	}

	// do some calculations to figure out what kind of spawn this is
	if ( client->pers.connected == CON_CONNECTED ) {
		spawnType = CST_TEAMCHANGE;
	} else if ( client->pers.firstTime ) {
		spawnType = CST_FIRSTTIME;
	} else if ( level.hasRestarted ) {
		spawnType = CST_MAPRESTART;
	} else {
		spawnType = CST_MAPCHANGE;
	}

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;

	// locate ent at a spawn point
	ClientSpawn( ent, spawnType );

	if ( client->sess.sessionTeam != TEAM_SPECTATOR && !g_holoIntro.integer &&
			!modfn.AdjustGeneralConstant( GC_SKIP_ENTER_GAME_PRINT, 0 ) ) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname) );
	}
	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();

	// if we are at the intermission, update the scoreboard
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}

	// kef -- should reset all of our awards-related stuff
	G_ClearClientLog(clientNum);
}

/*
===========
G_ResetClient

Resets non-persistant parts of client structure.
============
*/
void G_ResetClient( int clientNum ) {
	gclient_t *client = &level.clients[clientNum];
	clientPersistant_t pers = client->pers;
	clientSession_t sess = client->sess;
	int ping = client->ps.ping;
	int commandTime = client->ps.commandTime;
	int flags = client->ps.eFlags & EF_TELEPORT_BIT;
	int ps_persistant[MAX_PERSISTANT];
	memcpy( ps_persistant, client->ps.persistant, sizeof( ps_persistant ) );

	memset( client, 0, sizeof( *client ) );

	client->pers = pers;
	client->sess = sess;
	client->ps.ping = ping;
	client->ps.commandTime = commandTime;
	client->ps.eFlags = flags;
	memcpy( client->ps.persistant, ps_persistant, sizeof( client->ps.persistant ) );

	client->ps.clientNum = clientNum;
}

/*
===========
(ModFN) UpdateSessionClass

Checks if current player class is valid, and picks a new one if necessary.
============
*/
void ModFNDefault_UpdateSessionClass( int clientNum ) {
	// With no mods enabled, always use no class.
	gclient_t *client = &level.clients[clientNum];
	client->sess.sessionClass = PC_NOCLASS;
}

/*
===========
(ModFN) SpawnConfigureClient

Configures class and other client parameters during ClientSpawn.
Not called for spectators.
============
*/
void ModFNDefault_SpawnConfigureClient( int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];

	client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
	client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;

	// health will count down towards max_health
	ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] * 1.25;

	// start with a small amount of armor as well
	client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * 0.25;
}

/*
===========
(ModFN) SpawnCenterPrintMessage

Prints info messages to clients during ClientSpawn.
============
*/
void ModFNDefault_SpawnCenterPrintMessage( int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	if ( spawnType != CST_RESPAWN || client->sess.sessionTeam == TEAM_SPECTATOR ) {
		switch ( g_gametype.integer ) {
			case GT_FFA:
				trap_SendServerCommand( clientNum, "cp \"Free For All\"" );
				break;
			case GT_SINGLE_PLAYER:
				trap_SendServerCommand( clientNum, "cp \"SoloMatch\"" );
				break;
			case GT_TEAM:
				trap_SendServerCommand( clientNum, "cp \"Team Holomatch\"" );
				break;
			case GT_CTF:
				trap_SendServerCommand( clientNum, "cp \"Capture the Flag\"" );
				break;
		}
	}
}

/*
===========
(ModFN) SpawnTransporterEffect

Play transporter effect when player spawns.
============
*/
void ModFNDefault_SpawnTransporterEffect( int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];

	if ( spawnType != CST_MAPRESTART ) {
		gentity_t *tent = G_TempEntity( client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = clientNum;
	}
}

/*
===========
(ModFN) SetClientGhosting

Activate or deactivate spawn invulnerability on client.
============
*/
void ModFNDefault_SetClientGhosting( int clientNum, qboolean active ) {
	gclient_t *client = &level.clients[clientNum];

	if ( active ) {
		int ghostLength = g_ghostRespawn.value * 1000;
		if ( ghostLength > 0 ) {
			// If holodeck door intro is playing, start countdown after it completes.
			int startTime = client->ps.introTime > level.time ? client->ps.introTime : level.time;
			client->ps.powerups[PW_GHOST] = startTime + ghostLength;
		}
	} else {
		client->ps.powerups[PW_GHOST] = 0;
	}
}

/*
============
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn( gentity_t *ent, clientSpawnType_t spawnType ) {
	int		clientNum = ent - g_entities;
	gclient_t	*client = ent->client;
	vec3_t	spawn_origin, spawn_angles;
	int		i;
	gentity_t	*spawnPoint;
	static qboolean recursive = qfalse;

	EF_WARN_ASSERT( client->pers.connected == CON_CONNECTED );

	// no recursive calls
	if ( !EF_WARN_ASSERT( !recursive ) ) {
		return;
	}
	recursive = qtrue;

	trap_UnlinkEntity( ent );

	modfn.PreClientSpawn( clientNum, spawnType );

	// during intermission just put the client in viewing spot
	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
		modfn.SpawnCenterPrintMessage( clientNum, spawnType );

		recursive = qfalse;
		return;
	}

	// find a spawn point
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		// actual spectators (not just eliminated) get a special location
		spawnPoint = SelectSpectatorSpawnPoint( spawn_origin, spawn_angles );
	} else if ( g_gametype.integer >= GT_TEAM ) {
		spawnPoint = SelectCTFSpawnPoint( ent, client->sess.sessionTeam, spawnType != CST_RESPAWN,
				spawn_origin, spawn_angles );
	} else {
		// allow selecting bot/human exclusive spots
		qboolean useBotSpots = ( ent->r.svFlags & SVF_BOT ) ? qtrue : qfalse;
		qboolean useHumanSpots = !useBotSpots;

		// the first spawn should be at a good looking spot
		if ( spawnType != CST_RESPAWN && spawnType != CST_TEAMCHANGE && client->pers.localClient ) {
			spawnPoint = SelectInitialSpawnPoint( spawn_origin, spawn_angles, useHumanSpots, useBotSpots );
		} else {
			// don't spawn near existing origin if possible
			spawnPoint = SelectSpawnPoint( client->ps.origin, spawn_origin, spawn_angles, useHumanSpots, useBotSpots );
		}
	}

	// try to avoid spawn kills
	if ( !modfn.SpectatorClient( clientNum ) ) {
		EF_ERR_ASSERT( spawnPoint );
		modfn.PatchClientSpawn( clientNum, &spawnPoint, spawn_origin, spawn_angles );
	}

	// toggle the teleport bit so the client knows to not lerp
	client->ps.eFlags ^= EF_TELEPORT_BIT;

	// clear everything but the persistant data
	G_ResetClient( clientNum );

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[clientNum];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->flags = 0;

	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );

	// Always set phaser ammo since it will count up anyway
	client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;

	// Determine player class
	modfn.UpdateSessionClass( clientNum );
	client->ps.persistant[PERS_CLASS] = client->sess.sessionClass;

	// Determine max health
	client->ps.stats[STAT_MAX_HEALTH] = modfn.EffectiveHandicap( clientNum, EH_MAXHEALTH );

	if ( modfn.SpectatorClient( clientNum ) ) {
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];

		// Don't trip the out of ammo sound in CG_CheckAmmo
		client->ps.stats[STAT_WEAPONS] = 1 << WP_PHASER;
	} else {
		// Perform class-specific configuration
		modfn.SpawnConfigureClient( clientNum );

		G_KillBox( ent );
		trap_LinkEntity (ent);
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->inactivityTime = level.time + g_inactivity.integer * 1000;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	// fire the targets of the spawn point
	G_UseTargets( spawnPoint, ent );

	// select the highest weapon number available, after any
	// spawn given items have fired
	client->ps.weapon = 1;
	for ( i = WP_NUM_WEAPONS - 1 ; i > 0 ; i-- ) {
		if ( client->ps.stats[STAT_WEAPONS] & ( 1 << i ) ) {
			client->ps.weapon = i;
			break;
		}
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities );

	// positively link the client, even if the command times are weird
	if ( !modfn.SpectatorClient( clientNum ) ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	// send current class to client for UI team/class menu
	if ( spawnType == CST_FIRSTTIME || spawnType == CST_MAPCHANGE || spawnType == CST_MAPRESTART
			|| client->pers.uiClass != client->sess.sessionClass ) {
		SetPlayerClassCvar( ent );
		client->pers.uiClass = client->sess.sessionClass;
	}

	// print spawn messages
	modfn.SpawnCenterPrintMessage( clientNum, spawnType );

	// play holodeck intro on initial connect and map change, but not on map restarts
	if ( g_holoIntro.integer && ( spawnType == CST_FIRSTTIME || spawnType == CST_MAPCHANGE ) &&
			!( ent->r.svFlags & SVF_BOT ) && !level.intermissiontime ) {
		client->ps.introTime = level.time + TIME_INTRO;
		client->ps.pm_type = PM_FREEZE;
	}

	// if ghosting is engaged, allow the player to be invulnerable briefly
	modfn.SetClientGhosting( clientNum, qtrue );

	// play transporter effect, but not for spectators or holodeck intro viewers
	if ( !( modfn.SpectatorClient( clientNum ) ) && !( client->ps.introTime > level.time ) ) {
		modfn.SpawnTransporterEffect( clientNum, spawnType );
	}

	// run any post-spawn actions
	modfn.PostClientSpawn( clientNum, spawnType );

	// check for userinfo changes
	ClientUserinfoChanged( clientNum );

	recursive = qfalse;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}

	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		modfn.PrePlayerLeaveTeam( clientNum, ent->client->sess.sessionTeam );
	}

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected >= CON_CONNECTING
				&& modfn.SpectatorClient( i )
				&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
				&& level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED
		&& !modfn.SpectatorClient( clientNum ) ) {
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems ( ent );
	}

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->ps.persistant[PERS_CLASS] = PC_NOCLASS;
	ent->client->sess.sessionTeam = TEAM_FREE;
	ent->client->sess.sessionClass = PC_NOCLASS;

	trap_SetConfigstring( CS_PLAYERS + clientNum, "");

	CalculateRanks();

	// if we are at the intermission, update the scoreboard
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}

	if ( ent->r.svFlags & SVF_BOT ) {
		BotAIShutdownClient( clientNum );
	}

	// kef -- if this guy contributed to any of our kills/deaths/weapons logs, clean 'em out
	G_ClearClientLog(clientNum);
}
