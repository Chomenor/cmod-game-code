// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[1400];
	int			stringlength;
	int			i, j;
	gclient_t	*cl;
	int			numSorted;
	int			scoreFlags;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	for (i=0 ; i < numSorted ; i++) {
		int scoreClientNum = level.sortedClients[i];
		int		ping;

		cl = &level.clients[scoreClientNum];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}
		Com_sprintf (entry, sizeof(entry),
			" %i %i %i %i %i %i %i %i %i %i %i", scoreClientNum,
			modfn.EffectiveScore(scoreClientNum, EST_SCOREBOARD), ping,
			modfn.AdjustScoreboardAttributes(scoreClientNum, SA_PLAYERTIME,
					( level.time - cl->pers.enterTime ) / 60000),
			scoreFlags, g_entities[scoreClientNum].s.powerups,
//			GetFavoriteTargetForClient(scoreClientNum),
//			GetMaxKillsForClient(scoreClientNum),
			GetWorstEnemyForClient(scoreClientNum),
			GetMaxDeathsForClient(scoreClientNum),
			GetFavoriteWeaponForClient(scoreClientNum),
			modfn.AdjustScoreboardAttributes(scoreClientNum, SA_NUM_DEATHS,
					cl->sess.sessionTeam == TEAM_SPECTATOR ? 0 : cl->ps.persistant[PERS_KILLED]),
			modfn.AdjustScoreboardAttributes(scoreClientNum, SA_ELIMINATED, 0) );
		j = strlen(entry);
		if (stringlength + j >= sizeof(entry))
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	trap_SendServerCommand( ent-g_entities, va("scores %i %i %i%s", i,
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
		string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
static void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}



/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You must be alive to use this command.\n\""));
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			trap_SendServerCommand( to-g_entities, va("print \"Bad client slot: %i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client %i is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to-g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (gentity_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	name = ConcatArgs( 1 );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_NUM_WEAPONS) - 1 - ( 1 << WP_NONE );
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = 999;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		ent->client->ps.stats[STAT_ARMOR] = 200;

		if (!give_all)
			return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		it_ent->item = it;
		FinishSpawningItem(it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item (it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (gentity_t *ent)
{
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent-g_entities,
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	level.intermissiontime = -1;
	// Special 'level shot' setting -- Terrible ABUSE!!!  HORRIBLE NASTY HOBBITTESSSES

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}

/*
=================
(ModFN) CheckSuicideAllowed

Check if suicide is allowed. If not, prints notification to client.
=================
*/
qboolean ModFNDefault_CheckSuicideAllowed( int clientNum ) {
	gclient_t *client = &level.clients[clientNum];

	if( client->pers.suicideTime && client->pers.suicideTime > level.time - 30000 ) {
		// can't flood-kill
		trap_SendServerCommand( clientNum, va("cp \"Cannot suicide for %d seconds",
			(client->pers.suicideTime-(level.time-30000))/1000 ) );
		return qfalse;
	}

	return qtrue;
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( !modfn.SpectatorClient( ent - g_entities ) && modfn.CheckSuicideAllowed( ent - g_entities ) ) {
		ent->client->pers.suicideTime = level.time;
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die( ent, ent, ent, 100000, MOD_SUICIDE );
	}
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	const char *cmd = modfn.AdjustGeneralConstant( GC_JOIN_MESSAGE_CONSOLE_PRINT, 0 ) ? "print" : "cp";
	if ( level.exiting )
	{//no need to do this during level changes
		return;
	}
	if ( client->sess.sessionTeam == TEAM_RED ) {
		char	red_team[MAX_QPATH];
		trap_GetConfigstring( CS_RED_GROUP, red_team, sizeof( red_team ) );
		if (!red_team[0])	{
			Q_strncpyz( red_team, "red team", sizeof( red_team ) );
		}
		trap_SendServerCommand( -1, va("%s \"%.15s" S_COLOR_WHITE " joined the %s.\n\"", cmd, client->pers.netname, red_team ) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		char	blue_team[MAX_QPATH];
		trap_GetConfigstring( CS_BLUE_GROUP, blue_team, sizeof( blue_team ) );
		if (!blue_team[0]) {
			Q_strncpyz( blue_team, "blue team", sizeof( blue_team ) );
		}
		trap_SendServerCommand( -1, va("%s \"%.15s" S_COLOR_WHITE " joined the %s.\n\"", cmd, client->pers.netname, blue_team ) );
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va("%s \"%.15s" S_COLOR_WHITE " joined the spectators.\n\"",
		cmd, client->pers.netname));
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		trap_SendServerCommand( -1, va("%s \"%.15s" S_COLOR_WHITE " joined the battle.\n\"",
		cmd, client->pers.netname));
	}
}

/*
=================
SetPlayerClassCvar

Send current player class to client for UI team/class menu.
=================
*/
void SetPlayerClassCvar(gentity_t *ent)
{
	gclient_t *client;

	client = ent->client;

	switch( client->sess.sessionClass )
	{
	case PC_INFILTRATOR:
//		trap_Cvar_Set("ui_playerclass", "INFILTRATOR");
		trap_SendServerCommand(ent-g_entities,"pc INFILTRATOR");
		break;
	case PC_SNIPER:
//		trap_Cvar_Set("ui_playerclass", "SNIPER");
		trap_SendServerCommand(ent-g_entities,"pc SNIPER");
		break;
	case PC_HEAVY:
//		trap_Cvar_Set("ui_playerclass", "HEAVY");
		trap_SendServerCommand(ent-g_entities,"pc HEAVY");
		break;
	case PC_DEMO:
//		trap_Cvar_Set("ui_playerclass", "DEMO");
		trap_SendServerCommand(ent-g_entities,"pc DEMO");
		break;
	case PC_MEDIC:
//		trap_Cvar_Set("ui_playerclass", "MEDIC");
		trap_SendServerCommand(ent-g_entities,"pc MEDIC");
		break;
	case PC_TECH:
//		trap_Cvar_Set("ui_playerclass", "TECH");
		trap_SendServerCommand(ent-g_entities,"pc TECH");
		break;
	case PC_BORG:
//		trap_Cvar_Set("ui_playerclass", "BORG");
		trap_SendServerCommand(ent-g_entities,"pc BORG");
		break;
	case PC_ACTIONHERO:
//		trap_Cvar_Set("ui_playerclass", "HERO");
		trap_SendServerCommand(ent-g_entities,"pc HERO");
		break;
	case PC_NOCLASS:
//		trap_Cvar_Set("ui_playerclass", "NOCLASS");
		trap_SendServerCommand(ent-g_entities,"pc NOCLASS");
		break;
	}
}

/*
=================
(ModFN) CheckJoinAllowed

Check if client is allowed to join game or change team/class.
If join was blocked, sends appropriate notification message to client.
=================
*/
qboolean ModFNDefault_CheckJoinAllowed( int clientNum, join_allowed_type_t type, team_t targetTeam ) {
	// Check for g_maxGameClients limits
	if ( g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer &&
			type != CJA_SETCLASS && type != CJA_FORCETEAM ) {
		if ( type != CJA_AUTOJOIN && targetTeam != level.clients[clientNum].sess.sessionTeam ) {
			trap_SendServerCommand( clientNum, "cp \"Too many players.\"" );
		}

		return qfalse;
	}

	return qtrue;
}

/*
=================
SetTeam
=================
*/
qboolean SetTeam( gentity_t *ent, char *s, qboolean force ) {
	gclient_t			*client = ent->client;
	team_t				team = TEAM_FREE;
	team_t				oldTeam = client->sess.sessionTeam;
	int					clientNum = client - level.clients;
	spectatorState_t	specState = SPECTATOR_NOT;
	int					specClient = 0;

	//
	// see what change is requested
	//
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) && !(ent->r.svFlags&SVF_BOT) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) && !(ent->r.svFlags&SVF_BOT) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}

		if ( g_teamForceBalance.integer && !( ent->r.svFlags & SVF_BOT ) && !force &&
				!modfn.AdjustGeneralConstant( GC_DISABLE_TEAM_FORCE_BALANCE, 0 ) ) {
			int		counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount( clientNum, TEAM_BLUE, qtrue );
			counts[TEAM_RED] = TeamCount( clientNum, TEAM_RED, qtrue );

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				trap_SendServerCommand( clientNum, "cp \"Red team has too many players.\n\"" );
				return qfalse; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				trap_SendServerCommand( clientNum, "cp \"Blue team has too many players.\n\"" );
				return qfalse; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	// decide if we will allow the change, and print any warning messages
	if ( team != TEAM_SPECTATOR && !modfn.CheckJoinAllowed( clientNum, force ? CJA_FORCETEAM : CJA_SETTEAM, team ) ) {
		return qfalse;
	}

	// ignore redundant change
	if ( team != TEAM_SPECTATOR && team == oldTeam ) {
		return qfalse;
	}

	//
	// execute the team change
	//

	if ( oldTeam != TEAM_SPECTATOR ) {
		modfn.PrePlayerLeaveTeam( clientNum, oldTeam );
	}

	if ( !modfn.SpectatorClient( clientNum ) ) {
		// Kill him (makes sure he loses flags, etc)
		player_die (ent, NULL, NULL, 100000, MOD_RESPAWN);
	}

	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		client->sess.spectatorTime = level.time;
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	BroadcastTeamChange( client, oldTeam );

	ClientBegin( clientNum );

	return qtrue;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	char		s[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		switch ( ent->client->sess.sessionTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent-g_entities, "print \"Blue team\n\"" );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent-g_entities, "print \"Red team\n\"" );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent-g_entities, "print \"Free team\n\"" );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent-g_entities, "print \"Spectator team\n\"" );
			break;
		}
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s, qfalse );
}

/*
=================
Cmd_Class_f
=================
*/
static void Cmd_Class_f( gentity_t *ent ) {
	trap_SendServerCommand( ent - g_entities, "print \"Specialty mode is not enabled.\n\"" );
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	gclient_t *client = ent->client;
	vec3_t old_origin;
	vec3_t old_viewAngles;
	int old_ping = client->ps.ping;
	int old_rank = client->ps.persistant[PERS_RANK];

	client->sess.spectatorState = SPECTATOR_FREE;
	VectorCopy( client->ps.origin, old_origin );
	VectorCopy( client->ps.viewangles, old_viewAngles );
	
	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;
	client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = 100;
	client->ps.clientNum = ent - g_entities;
	client->ps.ping = old_ping;
	if ( g_gametype.integer >= GT_TEAM ) {
		client->ps.persistant[PERS_RANK] = old_rank;
	}

	// Make sure we have a valid commandTime
	client->ps.commandTime = client->pers.cmd.serverTime;
	if ( client->ps.commandTime < level.time - 1000 ) {
		client->ps.commandTime = level.time - 1000;
	} else if ( client->ps.commandTime > level.time + 200 ) {
		client->ps.commandTime = level.time + 200;
	}

	// Don't trip the out of ammo sound in CG_CheckAmmo
	client->ps.stats[STAT_WEAPONS] = 1 << WP_PHASER;
	client->ps.ammo[WP_PHASER] = PHASER_AMMO_MAX;

	// Keep the origin and view angle from the player that was being followed
	VectorCopy( old_origin, client->ps.origin );
	PM_UpdateViewAngles( &client->ps, &client->pers.cmd );
	SetClientViewAngle( ent, old_viewAngles );
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if ( ent->r.svFlags&SVF_BOT )
	{//bots can't follow!
		return;
	}

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( modfn.SpectatorClient( i ) ) {
		return;
	}

	// first set them to spectator
	if ( !modfn.SpectatorClient( ent - g_entities ) ) {
		SetTeam( ent, "spectator", qfalse );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
================
(ModFN) EnableCycleFollow

Returns true if follow spectators will cycle to this client by default.
================
*/
qboolean ModFNDefault_EnableCycleFollow( int clientNum ) {
	return qtrue;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int		clientnum;
	int		original;

	if ( ent->r.svFlags&SVF_BOT )
	{//bots can't follow!
		return;
	}

	// first set them to spectator
	if ( !modfn.SpectatorClient( ent - g_entities ) ) {
		SetTeam( ent, "spectator", qfalse );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	if ( clientnum < 0 ) {
		clientnum = 0;
	}
	original = clientnum;
	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator, including myself
		if ( modfn.SpectatorClient( clientnum ) ) {
			continue;
		}

		// check if mods allow following this player
		if ( !modfn.EnableCycleFollow( clientnum ) ) {
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*
==================
G_Say
==================
*/
#define	MAX_SAY_TEXT	150

#define SAY_ALL		0
#define SAY_TEAM	1
#define SAY_TELL	2
#define SAY_INVAL	3

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ) {
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( mode == SAY_TEAM  && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if ( g_gametype.integer == GT_TOURNAMENT
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		return;
	}

	trap_SendServerCommand( other-g_entities, va("%s \"%s%c%c%s\"",
		mode == SAY_TEAM ? "tchat" : "chat",
		name, Q_COLOR_ESCAPE, color, message));
}

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), "%s%c%c: ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), "(%s%c%c) (%s): ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf (name, sizeof(name), "(%s%c%c): ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && g_gametype.integer >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), "[%s%c%c] (%s): ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf (name, sizeof(name), "[%s%c%c]: ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		break;
	case SAY_INVAL:
		G_LogPrintf( "Invalid During Intermission: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), "[Invalid During Intermission%c%c]: ", Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		target = ent;
		break;
	}

	Q_strncpyz( text, chatText, sizeof(text) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text );
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Say( ent, NULL, mode, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	G_Say( ent, ent, SAY_TELL, p );
}


static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( !G_IsConnectedClient( player ) ) {
		return;
	}
	if ( order < 0 || order >= sizeof(gc_orders)/sizeof(char *) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles, TP_NORMAL );
}


/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char	cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if ( ent->client->pers.connected == CON_DISCONNECTED ) {
		return;		// not fully in game yet
	}


	trap_Argv( 0, cmd, sizeof( cmd ) );

	// Check if any mods have special handling of this command
	if ( modfn.ModClientCommand( clientNum, cmd ) ) {
		return;
	}

	// Team command can be called for connecting client when starting game from UI,
	// but there shouldn't be a need for any other commands
	if ( ent->client->pers.connected == CON_CONNECTING && Q_stricmp( cmd, "team" ) ) {
		return;
	}

	if (Q_stricmp (cmd, "say") == 0) {
		Cmd_Say_f (ent, SAY_ALL, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0) {
		Cmd_Say_f (ent, SAY_TEAM, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "tell") == 0) {
		Cmd_Tell_f ( ent );
		return;
	}
	if (Q_stricmp (cmd, "score") == 0) {
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "ready") == 0) {
		Cmd_Ready_f(ent);
		return;
	}


	// ignore all other commands when at intermission
	if (level.intermissiontime) {
		Cmd_Say_f (ent, SAY_INVAL, qtrue);
		return;
	}

	if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "levelshot") == 0)
		Cmd_LevelShot_f (ent);
	else if (Q_stricmp (cmd, "follow") == 0)
		Cmd_Follow_f (ent);
	else if (Q_stricmp (cmd, "follownext") == 0)
		Cmd_FollowCycle_f (ent, 1);
	else if (Q_stricmp (cmd, "followprev") == 0)
		Cmd_FollowCycle_f (ent, -1);
	else if (Q_stricmp (cmd, "team") == 0)
		Cmd_Team_f (ent);
	else if (Q_stricmp (cmd, "class") == 0)
		Cmd_Class_f (ent);
	else if (Q_stricmp (cmd, "where") == 0)
		Cmd_Where_f (ent);
	else if (Q_stricmp (cmd, "gc") == 0)
		Cmd_GameCommand_f( ent );
	else if (Q_stricmp (cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f( ent );
	else
		trap_SendServerCommand( clientNum, va("print \"unknown cmd %s\n\"", cmd ) );
}
