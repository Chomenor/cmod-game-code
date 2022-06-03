// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "g_groups.h"

extern void BroadcastClassChange( gclient_t *client, pclass_t oldPClass );

// g_client.c -- client functions that don't happen every frame

void G_StoreClientInitialStatus( gentity_t *ent );

static vec3_t	playerMins = {-15, -15, -24};
static vec3_t	playerMaxs = {15, 15, 32};

int		actionHeroClientNum = -1;
int		noJoinLimit = 0;
clInitStatus_t clientInitialStatus[MAX_CLIENTS];

void G_RandomActionHero( int ignoreClientNum )
{
	int i, numConnectedClients = 0;
	int ahCandidates[MAX_CLIENTS];

	if ( g_doWarmup.integer )
	{
		if ( level.warmupTime != 0 )
		{
			if ( level.warmupTime < 0 || level.time - level.startTime <= level.warmupTime )
			{//don't choose one until warmup is done
				return;
			}
		}
	}
	else if ( level.time - level.startTime <= 3000 )
	{//don't choose one until 3 seconds into the game
		return;
	}

	if ( g_pModActionHero.integer != 0 )
	{
		for ( i = 0; i < level.maxclients; i++ )
		{
			if ( i == ignoreClientNum )
			{
				continue;
			}

			if ( level.clients[i].pers.connected != CON_DISCONNECTED )
			{
				//note: these next few checks will mean that the first player to join (usually server client if a listen server) when a new map starts is *always* the AH
				if ( &g_entities[i] != NULL && g_entities[i].client != NULL )
				{
					if ( level.clients[i].sess.sessionClass != PC_ACTIONHERO )
					{
						if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR )
						{
							ahCandidates[numConnectedClients++] = i;
						}
					}
				}
			}
		}
		if ( !numConnectedClients )
		{//WTF?!
			return;
		}
		else
		{
			actionHeroClientNum = ahCandidates[ irandom( 0, (numConnectedClients-1) ) ];
		}
	}
}

void G_CheckReplaceActionHero( int clientNum )
{
	if ( clientNum == actionHeroClientNum )
	{
		G_RandomActionHero( clientNum );
		if ( actionHeroClientNum >= 0 && actionHeroClientNum < level.maxclients )
		{
			// get and distribute relevent paramters
			ClientUserinfoChanged( actionHeroClientNum );
			ClientSpawn( &g_entities[actionHeroClientNum], CST_RESPAWN );
		}//else ERROR!!!
	}
}

void INeedAHero( void )
{
	G_RandomActionHero( actionHeroClientNum );
	if ( actionHeroClientNum >= 0 && actionHeroClientNum < level.maxclients )
	{// get and distribute relevent paramters
		ClientUserinfoChanged( actionHeroClientNum );
		ClientSpawn( &g_entities[actionHeroClientNum], CST_RESPAWN );
	}//else ERROR!!!
}

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

	// find a single player start spot
	if (!spot) {
		G_Error( "Couldn't find a spawn point" );
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
LOGFUNCTION_RET( gentity_t *, ModFNDefault_CopyToBodyQue, ( int clientNum ), ( clientNum ), "G_MODFN_COPYTOBODYQUE" ) {
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

void SetScore( gentity_t *ent, int score );
void EliminationRespawn( gentity_t *ent, char *team )
{
	ent->flags &= ~FL_NOTARGET;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->client->ps.eFlags &= ~EF_NODRAW;
	ent->s.eFlags &= ~EF_ELIMINATED;
	ent->client->ps.eFlags &= ~EF_ELIMINATED;
	ent->r.svFlags &= ~SVF_ELIMINATED;
	ClientSpawn(ent, CST_RESPAWN);
	/*
	int oldScore;
	oldScore = ent->client->ps.persistant[PERS_SCORE];
	SetTeam( ent, ent->team );
	SetScore( ent, oldScore );
	*/
}

void EliminationSpectate( gentity_t *ent )
{
	modfn.CopyToBodyQue (ent - g_entities);

	ClientSpawn(ent, CST_RESPAWN);
	ent->takedamage = qfalse;
	ent->r.contents = 0;
	ent->flags |= FL_NOTARGET;
	ent->s.eFlags |= EF_NODRAW;
	ent->client->ps.eFlags |= EF_NODRAW;
	ent->client->ps.pm_type = PM_NORMAL;//PM_SPECTATOR;
	ent->s.eFlags |= EF_ELIMINATED;//FIXME:  this is not being reliably SENT!!!!!!
	ent->client->ps.eFlags |= EF_ELIMINATED;
	ent->r.svFlags |= SVF_ELIMINATED;//just in case
	VectorSet( ent->r.mins, -4, -4, -16 );
	VectorSet( ent->r.maxs, 4, 4, -8 );
	ent->client->ps.weapon = 0;
	ent->client->ps.stats[STAT_WEAPONS] = 0;
	/*
	int oldScore;
	oldScore = ent->client->ps.persistant[PERS_SCORE];
	ent->team = (char *)TeamName( ent->client->sess.sessionTeam );
	SetTeam( ent, "spectator");
	SetScore( ent, oldScore );
	//FIXME: specator mode when dead kind of freaky if trying to follow
	*/
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

	if ( g_pModElimination.integer != 0 )
	{//no players respawn when in elimination
		if ( !(level.intermissiontime && level.intermissiontime != -1) )
		{//don't do this once intermission has begun
			EliminationSpectate( ent );
		}
		return;
	}

	modfn.CopyToBodyQue( clientNum );
	ClientSpawn(ent, CST_RESPAWN);
}

/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount( int ignoreClientNum, int team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
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
team_t PickTeam( int ignoreClientNum ) {
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );

	if ( counts[TEAM_BLUE] > counts[TEAM_RED] ) {
		return TEAM_RED;
	}
	if ( counts[TEAM_RED] > counts[TEAM_BLUE] ) {
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
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
void ForceClientSkin(char *model, const char *skin ) {
	char *p;

	if ((p = strchr(model, '/')) != NULL) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}


/*
===========
ClientCheckName
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
legalSkin

Compare a list of races with an incoming race name.
Used to decide if in a CTF game where a race is specified for a given team if a skin is actually already legal.
===========
*/
qboolean legalSkin(const char *race_list, const char *race)
{
	char current_race_name[125];
	const char *s = race_list;
	const char *max_place = race_list + strlen(race_list);
	const char *marker;

	memset(current_race_name, 0, sizeof(current_race_name));
	// look through the list till it's empty
	while (s < max_place)
	{
		marker = s;
		// figure out from where we are where the next ',' or 0 is
		while (*s != ',' && *s != 0)
		{
			s++;
		}

		// copy just that name
		Q_strncpyz(current_race_name, marker, (s-marker)+1);

		// avoid the comma or increment us past the end of the string so we fail the main while loop
		s++;

		// compare and see if this race is the same as the one we want
		if (!Q_stricmp(current_race_name, race))
		{
			return qtrue;
		}
	}
	return qfalse;
}


/*
===========
randomSkin

given a race name, go find all the skins that use it, and randomly select one
===========
*/

void randomSkin(const char* race, char* model, int current_team, int clientNum)
{
	char	skinsForRace[MAX_SKINS_FOR_RACE][128];
	int		howManySkins = 0;
	int		i,x;
	int		temp;
	int		skin_count_check;
	char	skinNamesAlreadyUsed[16][128];
	int		current_skin_count = 0;
	gentity_t	*ent = NULL;
	char	userinfo[MAX_INFO_STRING];
	char	temp_model[MAX_QPATH];

	memset(skinsForRace, 0, sizeof(skinsForRace));
	memset(skinNamesAlreadyUsed, 0, sizeof(skinNamesAlreadyUsed));

	// first up, check to see if we want to select a skin from someone that's already playing on this guys team
	skin_count_check = g_random_skin_limit.integer;
	if (skin_count_check)
	{
		// sanity check the skins to compare against count
		if (skin_count_check > 16)
		{
			skin_count_check = 16;
		}

		// now construct an array of the names already used
		for (i=0; i<g_maxclients.integer; i++)
		{
			// did we find enough skins to grab a random one from yet?
			if (current_skin_count == skin_count_check)
			{
				break;
			}

			ent = g_entities + i;
			if (!ent->inuse || i == clientNum)
				continue;

			// no, so look at the next one, and see if it's in the list we are constructing
			// same team?
			if 	(ent->client && ent->client->sess.sessionTeam == current_team)
			{
				// so what's this clients model then?
				trap_GetUserinfo( i, userinfo, sizeof( userinfo ) );
				Q_strncpyz( temp_model, Info_ValueForKey (userinfo, "model"), sizeof( temp_model ) );

				// check the name
				for (x = 0; x< current_skin_count; x++)
				{
					// are we the same?
					if (!Q_stricmp(skinNamesAlreadyUsed[x], temp_model))
					{
						// yeah - ok we already got this one
						break;
					}
				}

				// ok, did we match anything?
				if (x == current_skin_count)
				{
					// no - better add this name in
					Q_strncpyz(skinNamesAlreadyUsed[current_skin_count], temp_model, sizeof(skinNamesAlreadyUsed[current_skin_count]));
					current_skin_count++;
				}
			}
		}

		// ok, array constructed. Did we get enough?
		if (current_skin_count >= skin_count_check)
		{
			// yeah, we did - so select a skin from one of these then
			temp = rand() % current_skin_count;
			Q_strncpyz( model, skinNamesAlreadyUsed[temp], MAX_QPATH );
			ForceClientSkin(model, "");
			return;
		}
	}

	// search through each and every skin we can find
	for (i=0; i<group_count && howManySkins < MAX_SKINS_FOR_RACE; i++)
	{

		// if this models race list contains the race we want, then add it to the list
		if (legalSkin(group_list[i].text, race))
		{
			Q_strncpyz( skinsForRace[howManySkins++], group_list[i].name , 128 );
		}
	}

	// set model to a random one
	if (howManySkins)
	{
		temp = rand() % howManySkins;
		Q_strncpyz( model, skinsForRace[temp], MAX_QPATH );
	}
	else
	{
		model[0] = 0;
	}

}

/*
===========
getNewSkin

Go away and actually get a random new skin based on a group name
============
*/
qboolean getNewSkin(const char *group, char *model, const char *color, const gclient_t *client, int clientNum)
{
	char	*temp_string;

	// go away and get what ever races this skin is attached to.
	// remove blue or red name
	ForceClientSkin(model, "");

	temp_string = G_searchGroupList(model);

	// are any of the races legal for this team race?
	if (legalSkin(temp_string, group))
	{
		ForceClientSkin(model, color);
		return qfalse;
	}

	//if we got this far, then we need to reset the skin to something appropriate
	randomSkin(group, model, client->sess.sessionTeam, clientNum);
	return qtrue;
}

void ClientMaxHealthForClass ( gclient_t *client, pclass_t pclass )
{
	switch( pclass )
	{
	case PC_INFILTRATOR:
		client->pers.maxHealth = 50;
		break;
	case PC_HEAVY:
		client->pers.maxHealth = 200;
		break;
	case PC_ACTIONHERO:
		client->pers.maxHealth = 150;
		break;
	case PC_MEDIC:
	case PC_SNIPER:
	case PC_DEMO:
	case PC_TECH:
		client->pers.maxHealth = 100;
		break;
	default:
		break;
	}
}

/*
===========
ClientUserinfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent;
	char	*s, *oldModel;
	char	model[MAX_QPATH];
	char	oldname[MAX_STRING_CHARS];
	gclient_t	*client;
	char	*c1;
	char	userinfo[MAX_INFO_STRING];
	qboolean	reset;
	char	*sex;

	model[0] = 0;

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

	// set max health
	client->pers.maxHealth = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 ) {
		client->pers.maxHealth = 100;
	}
	//if you have a class, ignores handicap and 100 limit, sorry
	ClientMaxHealthForClass( client, client->sess.sessionClass );
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// set model
	switch ( client->sess.sessionClass )
	{
	//FIXME: somehow map these into some text file that points to a specific model for each class?
	//OR give them a choice in the menu somehow?
	case PC_INFILTRATOR:
		//FIXME: someone else?  Male/female choice?  Random?
		oldModel = Info_ValueForKey( userinfo, "model" );
		if ( legalSkin( G_searchGroupList( oldModel ), "male"  ) )
		{
			Q_strncpyz( model, "munro", sizeof( model ) );
		}
		else if ( legalSkin( G_searchGroupList( oldModel ), "female"  ) )
		{
			Q_strncpyz( model, "alexandria", sizeof( model ) );
		}
		else
		{
			Q_strncpyz( model, "munro", sizeof( model ) );
		}
		break;
	case PC_SNIPER:
		Q_strncpyz( model, "telsia", sizeof( model ) );
		break;
	case PC_HEAVY:
		Q_strncpyz( model, "biessman", sizeof( model ) );
		break;
	case PC_DEMO:
		Q_strncpyz( model, "chang", sizeof( model ) );
		break;
	case PC_MEDIC://note: can also give health & armor & regen
		Q_strncpyz( model, "jurot", sizeof( model ) );
		break;
	case PC_TECH://note: can also give ammo & invis
		Q_strncpyz( model, "chell", sizeof( model ) );
		break;
	case PC_VIP:
		//FIXME: an admiral maybe?
		Q_strncpyz( model, "neelix", sizeof( model ) );
		break;
	case PC_BORG:
		//randomly pick one from Borg group
		if ( modfn.IsBorgQueen( clientNum ) )
		{
			Q_strncpyz( model, "borgqueen", sizeof( model ) );
		}
		else
		{//make sure it doesn't randomly pick the Borg Queen or any other inappropriate models
			oldModel = Info_ValueForKey( userinfo, "model" );
			if ( Q_strncmp( "borgqueen", oldModel, 9 ) == 0 || legalSkin(G_searchGroupList( oldModel ), "borg" ) == qfalse )
			{//not using a Borg skin
				//try to match sex
				if ( Q_strncmp( "borgqueen", oldModel, 9 ) == 0 )
				{
					sex = Info_ValueForKey( userinfo, "sex" );
				}
				else if ( legalSkin( G_searchGroupList( oldModel ), "male"  ) )
				{
					sex = "m";
				}
				else if ( legalSkin( G_searchGroupList( oldModel ), "female"  ) )
				{
					sex = "f";
				}
				else
				{
					sex = "";
				}

				if ( Q_strncmp( "janeway", oldModel, 7 ) == 0 )
				{
					Q_strncpyz( model, "borg-janeway", sizeof( model ) );
				}
				else if ( Q_strncmp( "torres", oldModel, 6 ) == 0 )
				{
					Q_strncpyz( model, "borg-torres", sizeof( model ) );
				}
				else if ( Q_strncmp( "tuvok", oldModel, 5 ) == 0 )
				{
					Q_strncpyz( model, "borg-tuvok", sizeof( model ) );
				}
				else if ( Q_strncmp( "seven", oldModel, 5 ) == 0 )
				{
					Q_strncpyz( model, "sevenofnine", sizeof( model ) );
				}

				while ( !model[0] || Q_strncmp( "borgqueen", model, 9 ) == 0 )
				{
					//if being assimilated, try to match the character/sex?
					model[0] = 0;
					getNewSkin( "Borg", model, "default", client, clientNum );
					if ( sex[0] == 'm' )
					{
						getNewSkin( "BorgMale", model, "default", client, clientNum );
					}
					else if ( sex[0] == 'f' )
					{
						getNewSkin( "BorgFemale", model, "default", client, clientNum );
					}
					else
					{
						getNewSkin( "Borg", model, "default", client, clientNum );
					}
				}
			}
			else
			{//using a borg model
				Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );
			}
		}
		break;
	case PC_ACTIONHERO:
	case PC_NOCLASS:
	default:
		Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );
		break;
	}

	// team
	if ( client->sess.sessionClass != PC_BORG )
	{//borg class doesn't need to use team color
		switch( client->sess.sessionTeam ) {
		case TEAM_RED:
			// decide if we are going to have to reset a skin cos it's not applicable to a race selected
			if (g_gametype.integer < GT_TEAM || !Q_stricmp("", g_team_group_red.string))
			{
				if ( modfn.AdjustGeneralConstant( GC_ASSIMILATION_MODELS, 0 ) && legalSkin(G_searchGroupList( model ), "borg" ) == qtrue )
				{//if you're trying to be a Borg and not a borg playerclass, then pick a different model
					getNewSkin("HazardTeam", model, "red", client, clientNum);
					ForceClientSkin(model, "red");
					// change the value in out local copy, then update it on the server
					Info_SetValueForKey(userinfo, "model", model);
					trap_SetUserinfo(clientNum, userinfo);
				}
				else
				{
					ForceClientSkin(model, "red");
				}
				break;
			}
			// at this point, we are playing CTF and there IS a race specified for this game
			else
			{
				if ( modfn.AdjustGeneralConstant( GC_ASSIMILATION_MODELS, 0 ) && Q_stricmp( "borg", g_team_group_blue.string ) == 0 )
				{//team model is set to borg, but that is now allowed, pick a different "race"
					reset = getNewSkin("HazardTeam", model, "blue", client, clientNum);
				}
				else
				{// go away and get what ever races this skin is attached to.
					reset = getNewSkin(g_team_group_red.string, model, "red", client, clientNum);
				}

				// did we get a model name back?
				if (!model[0])
				{
					// no - this almost certainly means we had a bogus race is the g_team_group_team cvar
					// so reset it to starfleet and try it again
					Com_Printf("WARNING! - Red Group %s is unknown - resetting Red Group to Allow Any Group\n", g_team_group_red.string);
					trap_Cvar_Set("g_team_group_red", "");
					G_UpdateTrackedCvar( &g_team_group_red );

					// Since we are allow any group now, just get his normal model and carry on
					Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );
					ForceClientSkin(model, "red");
					reset = qfalse;
				}

				if (reset)
				{
					trap_SendServerCommand( -1, va("print \"In-appropriate skin selected for %s on team %s\nSkin selection overridden from skin %s to skin %s\n\"",
						client->pers.netname, g_team_group_red.string, Info_ValueForKey (userinfo, "model"), model));
					ForceClientSkin(model, "red");
					// change the value in out local copy, then update it on the server
					Info_SetValueForKey(userinfo, "model", model);
					trap_SetUserinfo(clientNum, userinfo);
				}
				break;
			}
		case TEAM_BLUE:
			// decide if we are going to have to reset a skin cos it's not applicable to a race selected
			if (g_gametype.integer < GT_TEAM || !Q_stricmp("", g_team_group_blue.string))
			{
				if ( modfn.AdjustGeneralConstant( GC_ASSIMILATION_MODELS, 0 ) && legalSkin(G_searchGroupList( model ), "borg" ) == qtrue )
				{//if you're trying to be a Borg and not a borg playerclass, then pick a different model
					getNewSkin("HazardTeam", model, "blue", client, clientNum);
					ForceClientSkin(model, "blue");
					// change the value in out local copy, then update it on the server
					Info_SetValueForKey(userinfo, "model", model);
					trap_SetUserinfo(clientNum, userinfo);
				}
				else
				{
					ForceClientSkin(model, "blue");
				}
				break;
			}
			// at this point, we are playing CTF and there IS a race specified for this game
			else
			{
				if ( modfn.AdjustGeneralConstant( GC_ASSIMILATION_MODELS, 0 ) && Q_stricmp( "borg", g_team_group_blue.string ) == 0 )
				{//team model is set to borg, but that is now allowed, pick a different "race"
					reset = getNewSkin("HazardTeam", model, "blue", client, clientNum);
				}
				else
				{
					// go away and get what ever races this skin is attached to.
					reset = getNewSkin(g_team_group_blue.string, model, "blue", client, clientNum);
				}

				// did we get a model name back?
				if (!model[0])
				{
					// no - this almost certainly means we had a bogus race is the g_team_group_team cvar
					// so reset it to klingon and try it again
					Com_Printf("WARNING! - Blue Group %s is unknown - resetting Blue Group to Allow Any Group\n", g_team_group_blue.string);
					trap_Cvar_Set("g_team_group_blue", "");
					G_UpdateTrackedCvar( &g_team_group_blue );

					// Since we are allow any group now, just get his normal model and carry on
					Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );
					ForceClientSkin(model, "blue");
					reset = qfalse;
				}

				if (reset)
				{
					trap_SendServerCommand( -1, va("print \"In-appropriate skin selected for %s on team %s\nSkin selection overridden from skin %s to skin %s\n\"",
						client->pers.netname, g_team_group_blue.string, Info_ValueForKey (userinfo, "model"), model));
					ForceClientSkin(model, "blue");
					// change the value in out local copy, then update it on the server
					Info_SetValueForKey(userinfo, "model", model);
					trap_SetUserinfo(clientNum, userinfo);
				}
				break;
			}

		}
		if ( g_gametype.integer >= GT_TEAM && client->sess.sessionTeam == TEAM_SPECTATOR ) {
			// don't ever use a default skin in teamplay, it would just waste memory
			ForceClientSkin(model, "red");
		}
	}
	else
	{
		ForceClientSkin(model, "default");
		Info_SetValueForKey(userinfo, "model", model);
		trap_SetUserinfo(clientNum, userinfo);
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
			client->pers.maxHealth, client->sess.wins, client->sess.losses,
			Info_ValueForKey( userinfo, "skill" ) );
	} else {
		s = va("n\\%s\\t\\%i\\p\\%i\\model\\%s\\c1\\%s\\hc\\%i\\w\\%i\\l\\%i",
			client->pers.netname, client->sess.sessionTeam, client->sess.sessionClass, model, c1,
			client->pers.maxHealth, client->sess.wins, client->sess.losses );
	}

	trap_SetConfigstring( CS_PLAYERS+clientNum, s );

	G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

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

	modfn.PreClientConnect( clientNum, firstTime, isBot );

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	memset( client, 0, sizeof(*client) );

	client->pers.connected = CON_CONNECTING;
	client->pers.firstTime = firstTime;

	// read or initialize the session data
	if ( firstTime || level.newSession ) {
		G_InitSessionData( client, userinfo );
	} else {
		G_ReadSessionData( client );
	}

	if( isBot ) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if( !G_BotConnect( clientNum, !firstTime ) ) {
			return "BotConnectfailed";
		}
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	if ( g_pModSpecialties.integer == 0 && client->sess.sessionClass != PC_BORG )
	{
		client->sess.sessionClass = PC_NOCLASS;
	}
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

	if ( client->sess.sessionTeam != TEAM_SPECTATOR && g_holoIntro.integer==0 )
	{
		if ( g_gametype.integer != GT_TOURNAMENT ) {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname) );
		}
	}
	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();

	// if we are at the intermission, update the scoreboard
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}

	// Use intro holodeck door if desired and we did not come from a restart
	if ( g_holoIntro.integer && !(ent->r.svFlags & SVF_BOT) && (spawnType == CST_FIRSTTIME || spawnType == CST_MAPCHANGE) )
	{
		// kef -- also, don't do this if we're in intermission
		if (!level.intermissiontime)
		{
			client->ps.introTime = level.time + TIME_INTRO;
			client->ps.pm_type = PM_FREEZE;

			if (g_ghostRespawn.integer)
			{
				ent->client->ps.powerups[PW_GHOST] = level.time + (g_ghostRespawn.integer * 1000) + TIME_INTRO;
			}
		}
	}

	// kef -- should reset all of our awards-related stuff
	G_ClearClientLog(clientNum);
}

void ClientWeaponsForClass ( gclient_t *client, pclass_t pclass )
{
	switch ( pclass )
	{
	case PC_INFILTRATOR:
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
		client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;
		break;
	case PC_SNIPER:
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_IMOD );
		client->ps.ammo[WP_IMOD] = Max_Ammo[WP_IMOD];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_COMPRESSION_RIFLE );
		client->ps.ammo[WP_COMPRESSION_RIFLE] = Max_Ammo[WP_COMPRESSION_RIFLE];
		break;
	case PC_HEAVY:
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_TETRION_DISRUPTOR );
		client->ps.ammo[WP_TETRION_DISRUPTOR] = Max_Ammo[WP_TETRION_DISRUPTOR];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_QUANTUM_BURST );
		client->ps.ammo[WP_QUANTUM_BURST] = Max_Ammo[WP_QUANTUM_BURST];
		break;
	case PC_DEMO:
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_SCAVENGER_RIFLE );
		client->ps.ammo[WP_SCAVENGER_RIFLE] = Max_Ammo[WP_SCAVENGER_RIFLE];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_GRENADE_LAUNCHER );
		client->ps.ammo[WP_GRENADE_LAUNCHER] = Max_Ammo[WP_GRENADE_LAUNCHER];
		break;
	case PC_MEDIC:
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
		client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_VOYAGER_HYPO );
		client->ps.ammo[WP_VOYAGER_HYPO] = client->ps.ammo[WP_PHASER];
		break;
	case PC_TECH:
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
		client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DREADNOUGHT );
		client->ps.ammo[WP_DREADNOUGHT] = Max_Ammo[WP_DREADNOUGHT];
		break;
	case PC_VIP:
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
		client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;
		break;
	case PC_ACTIONHERO:
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
		client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_COMPRESSION_RIFLE );
		client->ps.ammo[WP_COMPRESSION_RIFLE] = Max_Ammo[WP_COMPRESSION_RIFLE];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_IMOD );
		client->ps.ammo[WP_IMOD] = Max_Ammo[WP_IMOD];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_SCAVENGER_RIFLE );
		client->ps.ammo[WP_SCAVENGER_RIFLE] = Max_Ammo[WP_SCAVENGER_RIFLE];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_STASIS );
		client->ps.ammo[WP_STASIS] = Max_Ammo[WP_STASIS];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_GRENADE_LAUNCHER );
		client->ps.ammo[WP_GRENADE_LAUNCHER] = Max_Ammo[WP_GRENADE_LAUNCHER];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_TETRION_DISRUPTOR );
		client->ps.ammo[WP_TETRION_DISRUPTOR] = Max_Ammo[WP_TETRION_DISRUPTOR];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_QUANTUM_BURST );
		client->ps.ammo[WP_QUANTUM_BURST] = Max_Ammo[WP_QUANTUM_BURST];
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_DREADNOUGHT );
		client->ps.ammo[WP_DREADNOUGHT] = Max_Ammo[WP_DREADNOUGHT];
		break;
	case PC_NOCLASS:
	default:
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_PHASER );
		client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;
		break;
	}
}

void ClientArmorForClass ( gclient_t *client, pclass_t pclass )
{
	/*
	gitem_t	*armor = BG_FindItem( "Personal Deflector Screen" );
	gitem_t	*harmor = BG_FindItem( "Isokinetic Deflector Screen" );
	assert( armor );
	assert( harmor );
	*/
	/*
	gitem_t	*bsuit = BG_FindItem( "Metaphasic Shielding" );
	client->ps.stats[STAT_HOLDABLE_ITEM] = bsuit - bg_itemlist;
	*/

	switch ( pclass )
	{
	case PC_INFILTRATOR:
		client->ps.stats[STAT_ARMOR] = 0;
		break;
	case PC_SNIPER:
		client->ps.stats[STAT_ARMOR] = 25;
		break;
	case PC_HEAVY:
		client->ps.stats[STAT_ARMOR] = 100;
		break;
	case PC_DEMO:
		client->ps.stats[STAT_ARMOR] = 50;
		break;
	case PC_MEDIC://note: can also give health & armor & regen
		client->ps.stats[STAT_ARMOR] = 75;
		break;
	case PC_TECH://note: can also give ammo & invis
		client->ps.stats[STAT_ARMOR] = 50;
		break;
	case PC_VIP:
		client->ps.stats[STAT_ARMOR] = 100;
		break;
	case PC_ACTIONHERO:
		client->ps.stats[STAT_ARMOR] = 100;
		break;
	case PC_NOCLASS:
	default:
		break;
	}
}

void ClientHoldablesForClass ( gclient_t *client, pclass_t pclass )
{
	switch ( pclass )
	{
	case PC_INFILTRATOR:
		client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItemForHoldable( HI_TRANSPORTER ) - bg_itemlist;
		break;
	case PC_SNIPER:
		client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItemForHoldable( HI_DECOY ) - bg_itemlist;
		client->ps.stats[STAT_USEABLE_PLACED] = 0;
		break;
	case PC_HEAVY:
		break;
	case PC_DEMO:
		//NOTE: instead of starting with the detpack, demo's get the detpack 10 seconds after spawning in
		client->teleportTime = level.time + 10000;
		client->ps.stats[STAT_USEABLE_PLACED] = 10;
		/*
		client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItemForHoldable( HI_DETPACK ) - bg_itemlist;
		client->ps.stats[STAT_USEABLE_PLACED] = 0;
		*/
		break;
	case PC_MEDIC:
		client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItemForHoldable( HI_MEDKIT ) - bg_itemlist;
		break;
	case PC_TECH:
		client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItemForHoldable( HI_SHIELD ) - bg_itemlist;
		break;
	case PC_VIP:
		break;
	case PC_ACTIONHERO:
		break;
	case PC_NOCLASS:
	default:
		break;
	}
}

void ClientPowerupsForClass ( gentity_t *ent, pclass_t pclass )
{
	gitem_t	*speed = BG_FindItemForPowerup( PW_HASTE );
	gitem_t	*regen = BG_FindItemForPowerup( PW_REGEN );
	gitem_t	*seeker = BG_FindItemForPowerup( PW_SEEKER );

	switch ( pclass )
	{
	case PC_INFILTRATOR:
		//INFILTRATOR gets permanent speed
		ent->client->ps.powerups[speed->giTag] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[speed->giTag] += 1800 * 1000;
		break;
	case PC_SNIPER:
		break;
	case PC_HEAVY:
		break;
	case PC_DEMO:
		break;
	case PC_MEDIC:
		ent->client->ps.powerups[regen->giTag] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[regen->giTag] += 1800 * 1000;
		break;
	case PC_TECH:
		//tech gets permanent seeker
		ent->client->ps.powerups[seeker->giTag] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[seeker->giTag] += 1800 * 1000;
		ent->count = 1;//can give away one invincibility
		//can also place ammo stations, register the model and sound for them
		G_ModelIndex( "models/mapobjects/dn/powercell.md3" );
		G_SoundIndex( "sound/player/suitenergy.wav" );
		break;
	case PC_VIP:
		ent->client->ps.powerups[seeker->giTag] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[seeker->giTag] += 1800 * 1000;
		break;
	case PC_ACTIONHERO:
		ent->client->ps.powerups[regen->giTag] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[regen->giTag] += 1800 * 1000;
		break;
	case PC_NOCLASS:
	default:
		break;
	}
}

void G_StoreClientInitialStatus( gentity_t *ent )
{
	if ( clientInitialStatus[ent->s.number].initialized )
	{//already set
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
	{//don't store their data if they're just a spectator
		return;
	}

	clientInitialStatus[ent->s.number].initialized = qtrue;
	ent->client->classChangeDebounceTime = 0;
}

/*
===========
(ModFN) UpdateSessionClass

Checks if current player class is valid, and picks a new one if necessary.
============
*/
LOGFUNCTION_VOID( ModFNDefault_UpdateSessionClass, ( int clientNum ),
		( clientNum ), "G_MODFN_UPDATESESSIONCLASS" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( g_pModSpecialties.integer != 0 )
	{
		if ( client->sess.sessionClass == PC_NOCLASS )
		{
			client->sess.sessionClass = irandom( PC_INFILTRATOR, PC_TECH );
			SetPlayerClassCvar(&g_entities[clientNum]);
		}
	}
	else if ( g_pModActionHero.integer != 0 )
	{
		if ( clientNum == actionHeroClientNum )
		{
			client->sess.sessionClass = PC_ACTIONHERO;
			BroadcastClassChange( client, PC_NOCLASS );
		}
		else if ( client->sess.sessionClass == PC_ACTIONHERO )
		{//make sure to take action hero away from previous one
			client->sess.sessionClass = PC_NOCLASS;
		}
	}
	else
	{
		client->sess.sessionClass = PC_NOCLASS;
	}
}

/*
===========
(ModFN) SpawnConfigureClient

Configures class and other client parameters during ClientSpawn.
Not called for spectators.
============
*/
LOGFUNCTION_VOID( ModFNDefault_SpawnConfigureClient, ( int clientNum ),
		( clientNum ), "G_MODFN_SPAWNCONFIGURECLIENT" ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];
	pclass_t pClass = client->sess.sessionClass;

	// health will count down towards max_health
	ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] * 1.25;

	// start with a small amount of armor as well
	client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * 0.25;

	if ( g_pModDisintegration.integer != 0 )
	{//this is instagib
		client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_COMPRESSION_RIFLE );
		client->ps.ammo[WP_COMPRESSION_RIFLE] = Max_Ammo[WP_COMPRESSION_RIFLE];
	}
	else
	{
		ClientMaxHealthForClass( client, pClass );
		if ( pClass != PC_NOCLASS )
		{//no health boost on spawn for playerclasses
			ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
		}
		ClientWeaponsForClass( client, pClass );
		ClientArmorForClass( client, pClass );
		ClientHoldablesForClass( client, pClass );
		ClientPowerupsForClass( ent, pClass );
	}
}

/*
===========
(ModFN) SpawnCenterPrintMessage

Prints info messages to clients during ClientSpawn.
============
*/
LOGFUNCTION_VOID( ModFNDefault_SpawnCenterPrintMessage, ( int clientNum, clientSpawnType_t spawnType ),
		( clientNum, spawnType ), "G_MODFN_SPAWNCENTERPRINTMESSAGE" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionTeam == TEAM_SPECTATOR )
	{//spectators just get the title of the game
		if ( g_pModElimination.integer )
		{
			trap_SendServerCommand( clientNum, "cp \"Elimination\"" );
		}
		else
		{
			switch ( g_gametype.integer )
			{
			case GT_FFA:				// free for all
				trap_SendServerCommand( clientNum, "cp \"Free For All\"" );
				break;
			case GT_TOURNAMENT:		// one on one tournament
				trap_SendServerCommand( clientNum, "cp \"Tournament\"" );
				break;
			case GT_SINGLE_PLAYER:	// single player tournament
				trap_SendServerCommand( clientNum, "cp \"SoloMatch\"" );
				break;
			case GT_TEAM:			// team deathmatch
				trap_SendServerCommand( clientNum, "cp \"Team Holomatch\"" );
				break;
			case GT_CTF:				// capture the flag
				trap_SendServerCommand( clientNum, "cp \"Capture the Flag\"" );
				break;
			}
		}
	}
	else
	{
		if ( g_pModElimination.integer )
		{
			if ( !clientInitialStatus[clientNum].initialized )
			{//first time coming in
				trap_SendServerCommand( clientNum, "cp \"Elimination\n\"" );
			}
		}
		else
		{
			if ( !clientInitialStatus[clientNum].initialized )
			{//first time coming in
				switch ( g_gametype.integer )
				{
				case GT_FFA:				// free for all
					trap_SendServerCommand( clientNum, "cp \"Free For All\"" );
					break;
				case GT_TOURNAMENT:		// one on one tournament
					trap_SendServerCommand( clientNum, "cp \"Tournament\"" );
					break;
				case GT_SINGLE_PLAYER:	// single player tournament
					trap_SendServerCommand( clientNum, "cp \"SoloMatch\"" );
					break;
				case GT_TEAM:			// team deathmatch
					trap_SendServerCommand( clientNum, "cp \"Team Holomatch\"" );
					break;
				case GT_CTF:				// capture the flag
					trap_SendServerCommand( clientNum, "cp \"Capture the Flag\"" );
					break;
				}
			}
			if ( clientNum == actionHeroClientNum )
			{
				trap_SendServerCommand( clientNum, "cp \"You are the Action Hero!\"" );
			}
			else if ( actionHeroClientNum > -1 )
			{//FIXME: this will make it so that those who spawn before the action hero won't be told who he is
				if ( !clientInitialStatus[clientNum].initialized )
				{//first time coming in
					gentity_t *aH = &g_entities[actionHeroClientNum];
					if ( aH != NULL && aH->client != NULL && aH->client->pers.netname[0] != 0 )
					{
						trap_SendServerCommand( clientNum, va("cp \"Action Hero is %s!\"", aH->client->pers.netname) );
					}
					else
					{
						trap_SendServerCommand( clientNum, "cp \"Action Hero!\"" );
					}
				}
			}
		}
	}
}

/*
===========
(ModFN) SpawnTransporterEffect

Play transporter effect when player spawns.
============
*/
LOGFUNCTION_VOID( ModFNDefault_SpawnTransporterEffect, ( int clientNum, clientSpawnType_t spawnType ),
		( clientNum, spawnType ), "G_MODFN_SPAWNTRANSPORTEREFFECT" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( spawnType != CST_MAPRESTART ) {
		gentity_t *tent = G_TempEntity( client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = clientNum;
	}
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn( gentity_t *ent, clientSpawnType_t spawnType ) {
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int		persistant[MAX_PERSISTANT];
	gentity_t	*spawnPoint;
	int		flags;
	int		savedPing;
	pclass_t	pClass = PC_NOCLASS;
	int		cCDT = 0;
	pclass_t oClass = ent->client->sess.sessionClass;

	index = ent - g_entities;
	client = ent->client;

	trap_UnlinkEntity( ent );

	modfn.PreClientSpawn( index, spawnType );

	/*
	if ( actionHeroClientNum == -1 )
	{
		G_RandomActionHero( -1 );
	}
	*/

	// find a spawn point
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
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
	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		EF_ERR_ASSERT( spawnPoint );
		modfn.PatchClientSpawn( index, &spawnPoint, spawn_origin, spawn_angles );
	}

	// toggle the teleport bit so the client knows to not lerp
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED);
	flags ^= EF_TELEPORT_BIT;

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = client->ps.persistant[i];
	}
	//okay, this is hacky, but we need to keep track of this, even if uninitialized first time you spawn, it will be stomped anyway
	if ( client->classChangeDebounceTime )
	{
		cCDT = client->classChangeDebounceTime;
	}
	memset (client, 0, sizeof(*client));
	client->classChangeDebounceTime = cCDT;
	//
	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;
	client->streakCount = 0;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
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

	client->ps.clientNum = index;

	// Determine player class
	modfn.UpdateSessionClass( index );
	if ( oClass != client->sess.sessionClass )
	{//need to send the class change
		ClientUserinfoChanged( index );
	}
	client->ps.persistant[PERS_CLASS] = client->sess.sessionClass;

	client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->ps.eFlags&EF_ELIMINATED) ) {
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = 100;

		// Don't trip the out of ammo sound in CG_CheckAmmo
		client->ps.stats[STAT_WEAPONS] = 1 << WP_PHASER;
	} else {
		// Perform class-specific configuration
		modfn.SpawnConfigureClient( index );

		G_KillBox( ent );
		trap_LinkEntity (ent);

		// force the base weapon up
		client->ps.weapon = WP_PHASER;
		client->ps.weaponstate = WEAPON_READY;

	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	// If ghosting is engaged, allow the player to be invulnerable briefly.
	if (g_ghostRespawn.integer)
	{
		ent->client->ps.powerups[PW_GHOST] = level.time + (g_ghostRespawn.integer * 1000);
	}

	client->inactivityTime = level.time + g_inactivity.integer * 1000;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
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
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities );

	// positively link the client, even if the command times are weird
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR )
	{
		if ( g_pModElimination.integer )
		{
			if ( g_gametype.integer >= GT_TEAM )
			{
				if ( TeamCount( -1, TEAM_BLUE ) > 0 && TeamCount( -1, TEAM_RED ) > 0 )
				{
					noJoinLimit = g_noJoinTimeout.integer*1000;
				}
			}
			else if ( TeamCount( -1, TEAM_FREE ) > 1 )
			{
				noJoinLimit = g_noJoinTimeout.integer*1000;
			}
		}
	}

	// send current class to client for UI team/class menu
	if ( spawnType == CST_FIRSTTIME || spawnType == CST_MAPCHANGE || spawnType == CST_MAPRESTART
			|| client->pers.uiClass != client->sess.sessionClass ) {
		SetPlayerClassCvar( ent );
		client->pers.uiClass = client->sess.sessionClass;
	}

	// print spawn messages
	modfn.SpawnCenterPrintMessage( index, spawnType );

	// play transporter effect, but not for spectators or holodeck intro viewers
	if ( !( ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->ps.eFlags&EF_ELIMINATED) )
			&& !( client->ps.introTime > level.time ) ) {
		modfn.SpawnTransporterEffect( index, spawnType );
	}

	//store intial client values
	//FIXME: when purposely change teams, this gets confused?
	G_StoreClientInitialStatus( ent );
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
		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems ( ent );
	}

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

	// if we are playing in tourney mode and losing, give a win to the other player
	if ( g_gametype.integer == GT_TOURNAMENT && !level.intermissiontime
		&& !level.warmupTime && level.sortedClients[1] == clientNum ) {
		level.clients[ level.sortedClients[0] ].sess.wins++;
		ClientUserinfoChanged( level.sortedClients[0] );
	}

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

	//also remove any initial data
	clientInitialStatus[clientNum].initialized = qfalse;

	//If the action hero leaves, have to pick a new one...
	if ( g_pModActionHero.integer != 0 )
	{
		G_CheckReplaceActionHero( clientNum );
	}
}


