// Copyright (C) 1999-2000 Id Software, Inc.
//
//
// g_arenas.c
//

#include "g_local.h"

static gentity_t *podiumModels[3];

#define PLAYER_SCORE( clientNum ) modfn.EffectiveScore( clientNum, EST_REALSCORE )
#define SPECTATOR_CLIENT( clientNum ) ( level.clients[clientNum].sess.sessionTeam == TEAM_SPECTATOR )

/*
==================
TallyTeamPoints

Get the combined number of points awarded to individual team members in a CTF game.
==================
*/
static int TallyTeamPoints( team_t team ) {
	int scores = 0;
	int i;

	for ( i=0; i < level.maxclients; ++i ) {
		const gclient_t *client = &level.clients[i];

		if ( client->pers.connected >= CON_CONNECTING && client->sess.sessionTeam == team ) {
			scores += PLAYER_SCORE( i );
		}
	}

	return scores;
}

/*
==================
UpdateTournamentInfo

Send intermission info to clients.
==================
*/
void UpdateTournamentInfo( void ) {
	int			i = 0;
	int			playerClientNum;
	char		msg[AWARDS_MSG_LENGTH], msg2[AWARDS_MSG_LENGTH];
	int			playerRank=level.numPlayingClients-1;
	int			mvpPoints = 0, winningCaptures = 0, winningPoints = 0;
	int			loseCaptures = 0, losePoints = 0;
	char		*mvpName = "";
	int			secondPlaceTied=0;

	memset(msg, 0, AWARDS_MSG_LENGTH);
	memset(msg2, 0, AWARDS_MSG_LENGTH);

	if ( g_gametype.integer < GT_TEAM ) {
		// Was there a tie for second place on the podium?
		if ( level.numPlayingClients >= 3 && PLAYER_SCORE( level.sortedClients[1] ) == PLAYER_SCORE( level.sortedClients[2] ) ) {
			secondPlaceTied=1;
		}
	}

	else {
		// In team game, we want to represent the highest scored client from the WINNING team.
		int mvpNum = GetWinningTeamMVP();

		// If teams are tied, these values just determine which team gets the top score readout box
		team_t winningTeam = level.winningTeam;
		team_t losingTeam;

		if ( mvpNum >= 0 ) {
			gclient_t *mvpClient = &level.clients[mvpNum];
			mvpName = mvpClient->pers.netname;
			mvpPoints = PLAYER_SCORE( mvpNum );

			if ( winningTeam != TEAM_RED && winningTeam != TEAM_BLUE ) {
				winningTeam = mvpClient->sess.sessionTeam;
			}
		}

		if ( winningTeam != TEAM_RED && winningTeam != TEAM_BLUE ) {
			winningTeam = TEAM_RED;
		}

		losingTeam = winningTeam == TEAM_RED ? TEAM_BLUE : TEAM_RED;

		if ( g_gametype.integer == GT_CTF ) {
			winningPoints = TallyTeamPoints( winningTeam );
			losePoints = TallyTeamPoints( losingTeam );
		} else {
			winningPoints = level.teamScores[winningTeam];
			losePoints = level.teamScores[losingTeam];
		}

		// This should only be displayed in CTF games, but set it regardless just in case
		winningCaptures = level.teamScores[winningTeam];
		loseCaptures = level.teamScores[losingTeam];
	}

	for (playerClientNum = 0; playerClientNum < level.maxclients; playerClientNum++ )
	{
		int highestTiedRank = 0;
		gentity_t *player = &g_entities[playerClientNum];

		if ( !player->inuse || (player->r.svFlags & SVF_BOT))
		{
			continue;
		}

		// put info for the top three players into the msg
		Com_sprintf(msg, AWARDS_MSG_LENGTH, "awards %d", level.numPlayingClients);
		for( i = 0; i < level.numPlayingClients; i++ )
		{
			if (i > 2)
			{
				break;
			}
			strcpy(msg2, msg);
			Com_sprintf(msg, AWARDS_MSG_LENGTH, "%s %d", msg2, level.sortedClients[i]);
		}

		// put this guy's awards into the msg
		if ( level.clients[playerClientNum].sess.sessionTeam == TEAM_SPECTATOR )
		{
			strcpy(msg2, msg);
			Com_sprintf( msg, sizeof(msg), "%s 0", msg2);
		}
		else
		{
			modfn.CalculateAwards(playerClientNum, msg);

			// get the best rank this player's score matches
			// 0=first place, 1=second place, ...
			for ( playerRank = 0; playerRank < level.numPlayingClients - 1; playerRank++ ) {
				if ( PLAYER_SCORE( playerClientNum ) >= PLAYER_SCORE( level.sortedClients[playerRank] ) ) {
					break;
				}
			}

			// if next player down has the same score, set highestTiedRank to display "tied for rank" message
			// 0=not tied, 1=first place tie, 2=second place tie, ...
			if ( playerRank < level.numPlayingClients - 1 &&
					PLAYER_SCORE( level.sortedClients[playerRank + 1] ) == PLAYER_SCORE( playerClientNum ) ) {
				highestTiedRank = playerRank + 1;
			}
		}

		// now supply...
		//
		// 1) winning team's MVP's name
		// 2) winning team's MVP's score
		// 3) winning team's total captures
		// 4) winning team's total points
		// 5) this player's rank
		// 6) the highest rank for which this player tied
		// 7) losing team's total captures
		// 8) losing team's total points
		// 9) if second place was tied
		// 10) intermission point
		// 11) intermission angles
		//

		strcpy(msg2, msg);
		Com_sprintf(msg, AWARDS_MSG_LENGTH, "%s \"%s\" %d %d %d %d %d %d %d %d %f %f %f %f %f %f",
			msg2, mvpName, mvpPoints, winningCaptures, winningPoints, playerRank, highestTiedRank,
			loseCaptures, losePoints, secondPlaceTied, level.intermission_origin[0], level.intermission_origin[1],
			level.intermission_origin[2], level.intermission_angle[0], level.intermission_angle[1],
			level.intermission_angle[2]);

		trap_SendServerCommand(player-g_entities, msg);
	}

	if (g_gametype.integer == GT_SINGLE_PLAYER)
	{
		Com_sprintf( msg, sizeof(msg), "postgame %i", playerRank);
		trap_SendConsoleCommand( EXEC_APPEND, msg);
	}
}

/*
==================
SetPodiumOrigin
==================
*/
static void SetPodiumOrigin( gentity_t *pad ) {
	vec3_t vec;
	vec3_t origin;

	AngleVectors( level.intermission_angle, vec, NULL, NULL );
	VectorMA( level.intermission_origin, g_podiumDist.value, vec, origin );
	origin[2] -= g_podiumDrop.value;
	G_SetOrigin( pad, origin );
}

/*
==================
SetPodiumModelOrigin
==================
*/
static void SetPodiumModelOrigin( gentity_t *pad, gentity_t *body, vec3_t offset ) {
	vec3_t vec;
	vec3_t f, r, u;

	VectorSubtract( level.intermission_origin, pad->r.currentOrigin, vec );
	vectoangles( vec, body->s.apos.trBase );
	body->s.apos.trBase[PITCH] = 0;
	body->s.apos.trBase[ROLL] = 0;

	AngleVectors( body->s.apos.trBase, f, r, u );
	VectorMA( pad->r.currentOrigin, offset[0], f, vec );
	VectorMA( vec, offset[1], r, vec );
	VectorMA( vec, offset[2], u, vec );

	G_SetOrigin( body, vec );
}

/*
==================
(ModFN) PodiumWeapon

Determines which weapon to display for player on the winner podium.
==================
*/
weapon_t ModFNDefault_PodiumWeapon( int clientNum ) {
	return g_entities[clientNum].s.weapon;
}

/*
==================
SpawnModelOnVictoryPad
==================
*/
static gentity_t *SpawnModelOnVictoryPad( gentity_t *pad, vec3_t offset, gentity_t *ent ) {
	gentity_t	*body = G_Spawn();

	body->classname = ent->client->pers.netname;
	body->client = ent->client;
	body->s = ent->s;
	body->s.eType = ET_PLAYER;		// could be ET_INVISIBLE
	body->s.eFlags = 0;				// clear EF_TALK, etc
	body->s.powerups = 0;			// clear powerups
	body->s.loopSound = 0;			// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	body->s.event = 0;
	body->s.pos.trType = TR_STATIONARY;
	body->s.groundEntityNum = ENTITYNUM_WORLD;
	body->s.legsAnim = LEGS_IDLE;
	body->s.torsoAnim = TORSO_STAND;

	body->s.weapon = modfn.PodiumWeapon( ent - g_entities );
	// fix up some weapon holding / shooting issues
	if (body->s.weapon==WP_PHASER || body->s.weapon==WP_DREADNOUGHT || body->s.weapon == WP_NONE )
		body->s.weapon = WP_COMPRESSION_RIFLE;

	body->s.event = 0;
	body->r.svFlags = ent->r.svFlags;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);
	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_BODY;
	body->r.ownerNum = ent->r.ownerNum;
	body->takedamage = qfalse;

	SetPodiumModelOrigin( pad, body, offset );

	trap_LinkEntity (body);

	return body;
}

/*
==================
CelebrateStop
==================
*/
static void CelebrateStop( gentity_t *player ) {
	int		anim;

	anim = TORSO_STAND;
	player->s.torsoAnim = ( ( player->s.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
}

/*
==================
CelebrateStart
==================
*/
#define	TIMER_GESTURE	(34*66+50)
static void CelebrateStart( gentity_t *player )
{
	player->s.torsoAnim = ( ( player->s.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | TORSO_GESTURE;
	player->nextthink = level.time + TIMER_GESTURE;
	player->think = CelebrateStop;
}

static vec3_t podiumModelOffsets[3] = { { 0, 0, 64 }, { -10, 60, 44 }, { -19, -60, 35 } };

/*
==================
PodiumPlacementThink

Update positions to accout for g_podiumDist and g_podiumDrop cvar changes.
==================
*/
static void PodiumPlacementThink( gentity_t *podium ) {
	int i;
	podium->nextthink = level.time + 100;

	SetPodiumOrigin( podium );

	for ( i = 0; i < 3; ++i ) {
		if ( podiumModels[i] ) {
			SetPodiumModelOrigin( podium, podiumModels[i], podiumModelOffsets[i] );
		}
	}
}

/*
==================
SpawnPodium
==================
*/
static gentity_t *SpawnPodium( qboolean teamPodium ) {
	gentity_t	*podium = G_Spawn();
	vec3_t		vec;

	podium->classname = "podium";
	podium->s.eType = ET_GENERAL;
	podium->s.number = podium - g_entities;
	podium->clipmask = CONTENTS_SOLID;
	podium->r.contents = CONTENTS_SOLID;
	if ( teamPodium )
		podium->s.modelindex = G_ModelIndex( TEAM_PODIUM_MODEL );
	else
		podium->s.modelindex = G_ModelIndex( SP_PODIUM_MODEL );

	SetPodiumOrigin( podium );

	VectorSubtract( level.intermission_origin, podium->r.currentOrigin, vec );
	podium->s.apos.trBase[YAW] = vectoyaw( vec );
	trap_LinkEntity (podium);

	podium->think = PodiumPlacementThink;
	podium->nextthink = level.time + 100;
	return podium;
}

/*
==================
SpawnModelsOnVictoryPads
==================
*/
void SpawnModelsOnVictoryPads( void ) {
	int i;
	static gentity_t *podium = NULL;
	static qboolean currentTeamPodium;
	qboolean useTeamPodium = g_gametype.integer >= GT_TEAM || modfn.AdjustGeneralConstant( GC_FORCE_TEAM_PODIUM, 0 );

	for ( i = 0; i < 3; ++i ) {
		if ( podiumModels[i] ) {
			G_FreeEntity( podiumModels[i] );
			podiumModels[i] = NULL;
		}
	}

	// If we don't already have a podium, or have the wrong type, spawn it now.
	if ( !podium || useTeamPodium != currentTeamPodium ) {
		if ( podium ) {
			G_FreeEntity( podium );
		}
		podium = SpawnPodium( useTeamPodium );
		currentTeamPodium = useTeamPodium;
	}

	// SPAWN PLAYER ON TOP MOST PODIUM
	if (g_gametype.integer >= GT_TEAM)
	{
		// In team game, we want to represent the highest scored client from the WINNING team.
		int mvpNum = GetWinningTeamMVP();
		if ( mvpNum >= 0 ) {
			podiumModels[0] = SpawnModelOnVictoryPad( podium, podiumModelOffsets[0], &g_entities[mvpNum] );
		}
	}
	else if ( level.numPlayingClients >= 1 )
	{
		podiumModels[0] = SpawnModelOnVictoryPad( podium, podiumModelOffsets[0], &g_entities[level.sortedClients[0]] );
	}
	if ( podiumModels[0] ) {
		podiumModels[0]->nextthink = level.time + 2000;
		podiumModels[0]->think = CelebrateStart;
	}

	// For non team game types, we want to spawn 3 characters on the victory pad
	// For team games (GT_TEAM, GT_CTF) we want to have only a single player on the pad
	if ( !useTeamPodium )
	{
		if ( level.numPlayingClients >= 2 ) {
			podiumModels[1] = SpawnModelOnVictoryPad( podium, podiumModelOffsets[1], &g_entities[level.sortedClients[1]] );
		}

		if ( level.numPlayingClients >= 3 ) {
			podiumModels[2] = SpawnModelOnVictoryPad( podium, podiumModelOffsets[2], &g_entities[level.sortedClients[2]] );
		}
	}
}

/*
===============
Svcmd_AbortPodium_f
===============
*/
void Svcmd_AbortPodium_f( void ) {
	if( g_gametype.integer != GT_SINGLE_PLAYER ) {
		return;
	}

	if( podiumModels[0] ) {
		podiumModels[0]->nextthink = level.time;
		podiumModels[0]->think = CelebrateStop;
	}
}
