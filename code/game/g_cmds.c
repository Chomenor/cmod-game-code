// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"


extern void G_CheckReplaceQueen( int clientNum );

extern int		borgQueenClientNum;
extern int		noJoinLimit;
extern int	numKilled;
extern clInitStatus_t clientInitialStatus[];
extern qboolean levelExiting;

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
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}
		Com_sprintf (entry, sizeof(entry),
			" %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime)/60000,
			scoreFlags, g_entities[level.sortedClients[i]].s.powerups,
//			GetFavoriteTargetForClient(level.sortedClients[i]),
//			GetMaxKillsForClient(level.sortedClients[i]),
			GetWorstEnemyForClient(level.sortedClients[i]),
			GetMaxDeathsForClient(level.sortedClients[i]),
			GetFavoriteWeaponForClient(level.sortedClients[i]),
			cl->ps.persistant[PERS_KILLED],
			((g_entities[cl->ps.clientNum].r.svFlags&SVF_ELIMINATED)!=0) );
		j = strlen(entry);
		if (stringlength + j > 1024)
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
void Cmd_Score_f( gentity_t *ent ) {
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
		G_SpawnItem (it_ent, it);
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
Cmd_Kill_f
=================
*/
int lastKillTime[MAX_CLIENTS];
void Cmd_Kill_f( gentity_t *ent ) {

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->ps.eFlags&EF_ELIMINATED) ) {
		return;
	}
	if ( lastKillTime[ent->client->ps.clientNum] > level.time - 30000 )
	{//can't flood-kill
		trap_SendServerCommand( ent->client->ps.clientNum, va("cp \"Cannot suicide for %d seconds", (lastKillTime[ent->client->ps.clientNum]-(level.time-30000))/1000 ) );
		return;
	}
	lastKillTime[ent->client->ps.clientNum] = level.time;
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	if ( levelExiting )
	{//no need to do this during level changes
		return;
	}
	if ( client->sess.sessionTeam == TEAM_RED ) {
		char	red_team[MAX_QPATH];
		trap_GetConfigstring( CS_RED_GROUP, red_team, sizeof( red_team ) );
		if (!red_team[0])	{
			Q_strncpyz( red_team, "red team", sizeof( red_team ) );
		}
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " joined the %s.\n\"", client->pers.netname, red_team ) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		char	blue_team[MAX_QPATH];
		trap_GetConfigstring( CS_BLUE_GROUP, blue_team, sizeof( blue_team ) );
		if (!blue_team[0]) {
			Q_strncpyz( blue_team, "blue team", sizeof( blue_team ) );
		}
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " joined the %s.\n\"", client->pers.netname, blue_team ) );
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " joined the spectators.\n\"",
		client->pers.netname));
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " joined the battle.\n\"",
		client->pers.netname));
	}
}

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
BroadCastClassChange

Let everyone know about a team change
=================
*/
void BroadcastClassChange( gclient_t *client, pclass_t oldPClass )
{
	if ( levelExiting )
	{//no need to do this during level changes
		return;
	}
	switch( client->sess.sessionClass )
	{
	case PC_INFILTRATOR:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now an Infiltrator.\n\"", client->pers.netname) );
		break;
	case PC_SNIPER:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now a Sniper.\n\"", client->pers.netname) );
		break;
	case PC_HEAVY:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now a Heavy Weapons Specialist.\n\"", client->pers.netname) );
		break;
	case PC_DEMO:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now a Demolitionist.\n\"", client->pers.netname) );
		break;
	case PC_MEDIC:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now a Medic.\n\"", client->pers.netname) );
		break;
	case PC_TECH:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is now a Technician.\n\"", client->pers.netname) );
		break;
	case PC_ACTIONHERO:
		trap_SendServerCommand( -1, va("cp \"%.15s" S_COLOR_WHITE " is the Action Hero!\n\"", client->pers.netname) );
		break;
	}
}

/*
=================
SetTeam
=================
*/
qboolean SetTeam( gentity_t *ent, char *s ) {
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;

	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) && !(ent->r.svFlags&SVF_BOT) ) {
		if ( g_pModElimination.integer != 0 )
		{//don't do this follow stuff, it's bad!
			return qfalse;
		}
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) && !(ent->r.svFlags&SVF_BOT) ) {
		if ( g_pModElimination.integer != 0 )
		{//don't do this follow stuff, it's bad!
			return qfalse;
		}
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}

		if ( g_teamForceBalance.integer && g_pModAssimilation.integer == 0 ) {
			int		counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount( ent->client->ps.clientNum, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( ent->client->ps.clientNum, TEAM_RED );

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				trap_SendServerCommand( ent->client->ps.clientNum, 
					"cp \"Red team has too many players.\n\"" );
				return qfalse; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				trap_SendServerCommand( ent->client->ps.clientNum, 
					"cp \"Blue team has too many players.\n\"" );
				return qfalse; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	// override decision if limiting the players
	if ( g_gametype.integer == GT_TOURNAMENT
		&& level.numNonSpectatorClients >= 2 ) {
		team = TEAM_SPECTATOR;
	} else if ( g_maxGameClients.integer > 0 && 
		level.numNonSpectatorClients >= g_maxGameClients.integer ) {
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return qfalse;
	}

	//replace them if they're the queen
	if ( borgQueenClientNum != -1 )
	{
		G_CheckReplaceQueen( clientNum );
	}
	//
	// execute the team change
	//

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;

	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
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

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );
	
	//THIS IS VERY VERY BAD, CAUSED ENDLESS WARMUP, FOUND ANOTHER WAY TO PREVENT DOORS
	/*
	if (level.time - client->pers.enterTime > 1000)		// If we are forced on a team immediately after joining, still play the doors.
	{	// We signal NOT to play the doors by setting level.restarted to true.  This is abusing the system, but it works.
		level.restarted = qtrue;
	}
	*/

	ClientBegin( clientNum, qfalse );

	return qtrue;
}

char *ClassNameForValue( pclass_t pClass ) 
{
	switch ( pClass )
	{
	case PC_NOCLASS://default
		return "noclass";
		break;
	case PC_INFILTRATOR://fast: low attack
		return "infiltrator";
		break;
	case PC_SNIPER://sneaky: snipe only
		return "sniper";
		break;
	case PC_HEAVY://slow: heavy attack
		return "heavy";
		break;
	case PC_DEMO://go boom
		return "demo";
		break;
	case PC_MEDIC://heal
		return "medic";
		break;
	case PC_TECH://operate
		return "tech";
		break;
	case PC_VIP://for escorts
		return "vip";
		break;
	case PC_ACTIONHERO://has everything
		return "hero";
		break;
	case PC_BORG://special weapons: slower: adapting shields
		return "borg";
		break;
	}
	return "noclass";
}
/*
=================
SetClass
=================
*/
qboolean SetClass( gentity_t *ent, char *s, char *teamName ) {
	int					pclass, oldPClass;
	gclient_t			*client;
	int					clientNum;

	//FIXME: check for appropriate game mod being on first

	//FIXME: can't change class while playing

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;

	if ( !Q_stricmp( s, "infiltrator" ) ) 
	{
		pclass = PC_INFILTRATOR;
	}
	else if ( !Q_stricmp( s, "sniper" ) ) 
	{
		pclass = PC_SNIPER;
	} 
	else if ( !Q_stricmp( s, "heavy" ) ) 
	{
		pclass = PC_HEAVY;
	} 
	else if ( !Q_stricmp( s, "demo" ) ) 
	{
		pclass = PC_DEMO;
	} 
	else if ( !Q_stricmp( s, "medic" ) ) 
	{
		pclass = PC_MEDIC;
	} 
	else if ( !Q_stricmp( s, "tech" ) ) 
	{
		pclass = PC_TECH;
	} 
	else if ( !Q_stricmp( s, "borg" ) ) 
	{
		pclass = PC_BORG;
	} 
	else if ( !Q_stricmp( s, "noclass" ) ) 
	{
		pclass = PC_NOCLASS;
	} 
	else 
	{
		pclass = irandom( PC_INFILTRATOR, PC_TECH );
	}

	//
	// decide if we will allow the change
	//
	oldPClass = client->sess.sessionClass;

	switch ( pclass )
	{
	case PC_INFILTRATOR:
	case PC_SNIPER:
	case PC_HEAVY:
	case PC_DEMO:
	case PC_MEDIC:
	case PC_TECH:
		if ( g_pModSpecialties.integer == 0 )
		{
			trap_SendServerCommand( ent-g_entities, "print \"Specialty mode is not enabled.\n\"" );
			return qfalse;
		}
		break;
	case PC_BORG:
		if ( g_pModAssimilation.integer == 0 )
		{
			trap_SendServerCommand( ent-g_entities, "print \"Assimilation mode is not enabled.\n\"" );
			return qfalse;
		}
		break;
	case PC_NOCLASS:
		if ( g_pModSpecialties.integer )
		{
			trap_SendServerCommand( ent-g_entities, "print \"Cannot switch to no class in this game mode.\n\"" );
			return qfalse;
		}
		break;
	}

	if ( pclass == oldPClass )
	{
		SetPlayerClassCvar(ent);
		return qfalse;
	}

	//
	// execute the class change
	//

	client->sess.sessionClass = pclass;

	SetPlayerClassCvar(ent);

	BroadcastClassChange( client, oldPClass );

	if ( teamName != NULL && SetTeam( ent, teamName ) )
	{
		return qtrue;
	}
	else
	{//not changing teams or couldn't change teams
		// get and distribute relevent paramters
		ClientUserinfoChanged( clientNum );

		//if in the game already, kill and respawn him, else just wait to join
		if ( client->sess.sessionTeam == TEAM_SPECTATOR )
		{// they go to the end of the line for tournaments
			client->sess.spectatorTime = level.time;
		}
		else
		{
			// he starts at 'base'
			client->pers.teamState.state = TEAM_BEGIN;

			// Kill him (makes sure he loses flags, etc)
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die (ent, NULL, NULL, 100000, MOD_RESPAWN);

			//THIS IS VERY VERY BAD, CAUSED ENDLESS WARMUP, FOUND ANOTHER WAY TO PREVENT DOORS
			/*
			if (level.time - client->pers.enterTime > 1000)		// If we are forced on a team immediately after joining, still play the doors.
			{	// We signal NOT to play the doors by setting level.restarted to true.  This is abusing the system, but it works.
				level.restarted = qtrue;
			}
			*/

			ClientBegin( clientNum, qfalse );
		}
	}
	return qtrue;
}
/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
	//don't be dead
	ent->client->ps.stats[STAT_HEALTH] = ent->client->ps.stats[STAT_MAX_HEALTH];
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
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

	if ( !s || s[0] != 's' )
	{//not trying to become a spectator
		if ( g_pModElimination.integer )
		{
			if ( noJoinLimit != 0 && ( level.time-level.startTime > noJoinLimit || numKilled > 0 ) )
			{
				if ( ent->client->ps.eFlags & EF_ELIMINATED )
				{
					trap_SendServerCommand( ent-g_entities, "cp \"You have been eliminated until next round\"" );
				}
				else if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
				{
					trap_SendServerCommand( ent-g_entities, "cp \"Wait until next round to join\"" );
				}
				else
				{
					trap_SendServerCommand( ent-g_entities, "cp \"Wait until next round to change teams\"" );
				}
				return;
			}
		}

		if ( g_pModAssimilation.integer )
		{
			if ( borgQueenClientNum != -1 && noJoinLimit != 0 && ( level.time-level.startTime > noJoinLimit || numKilled > 0 ) )
			{
				if ( ent->client->ps.eFlags & EF_ASSIMILATED )
				{
					trap_SendServerCommand( ent-g_entities, "cp \"You have been assimilated until next round\"" );
				}
				else if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
				{
					trap_SendServerCommand( ent-g_entities, "cp \"Wait until next round to join\"" );
				}
				else
				{
					trap_SendServerCommand( ent-g_entities, "cp \"Wait until next round to change teams\"" );
				}
				return;
			}
		}
	}

	// if they are playing a tournement game, count as a loss
	if ( g_gametype.integer == GT_TOURNAMENT && ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}

	//if this is a manual change, not an assimilation, uninitialize the clInitStatus data
	clientInitialStatus[ent->s.number].initialized = qfalse;
	SetTeam( ent, s );
}


/*
=================
Cmd_Team_f
=================
*/
void Cmd_Class_f( gentity_t *ent ) {
	char		s[MAX_TOKEN_CHARS];
	qboolean	check = qtrue;

	if ( trap_Argc() != 2 ) 
	{//Just asking what class they're on
		char	*className;
		switch ( ent->client->sess.sessionClass ) {
		case PC_NOCLASS://default
			className = "Noclass";
			break;
		case PC_INFILTRATOR://fast: low attack
			className = "Infiltrator";
			break;
		case PC_SNIPER://sneaky: snipe only
			className = "Sniper";
			break;
		case PC_HEAVY://slow: heavy attack
			className = "Heavy";
			break;
		case PC_DEMO://go boom
			className = "Demo";
			break;
		case PC_MEDIC://heal
			className = "Medic";
			break;
		case PC_TECH://operate
			className = "Tech";
			break;
		case PC_VIP://for escorts
			className = "VIP";
			break;
		case PC_ACTIONHERO://has everything
			return;//can't set this via a command, it is automatic
			//className = "Hero";
			break;
		case PC_BORG://special weapons: slower: adapting shields
			className = "Borg";
			check = qfalse;
			break;
		default:
			trap_SendServerCommand( ent-g_entities, "print \"Unknown current class!\n\"" );
			return;
			break;
		}
		if ( check && g_pModSpecialties.integer == 0 )
		{//FIXME: if this guys has a specialty class and we're not in specialties mode, there is a serious problem
			trap_SendServerCommand( ent-g_entities, "print \"Specialty mode is not enabled.\n\"" );
			return;
		}
		trap_SendServerCommand( ent-g_entities, va( "print \"class: %s\n\"", className ) );
		return;
	}

	if ( g_pModElimination.integer )
	{
		if ( noJoinLimit != 0 && ( level.time-level.startTime > noJoinLimit || numKilled > 0 ) )
		{
			if ( ent->client->ps.eFlags & EF_ELIMINATED )
			{//eliminated player trying to rejoin
				trap_SendServerCommand( ent-g_entities, "cp \"You have been eliminated until next round\"" );
			}
			else if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
			{
				trap_SendServerCommand( ent-g_entities, "cp \"Wait until next round to join\"" );
			}
			else
			{
				trap_SendServerCommand( ent-g_entities, "cp \"Wait until next round to change classes\"" );
			}
			return;
		}
	}

	if ( g_pModAssimilation.integer )
	{
		if ( ent->client->ps.eFlags & EF_ASSIMILATED )
		{//assimilated player trying to switch
			trap_SendServerCommand( ent-g_entities, "cp \"You have been assimilated until next round\"" );
			return;
		}
		else if ( noJoinLimit != 0 && ( level.time-level.startTime > noJoinLimit || numKilled > 0 ) )
		{//spectator coming in after start of game
			if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
			{
				trap_SendServerCommand( ent-g_entities, "cp \"Wait until next round to join\"" );
			}
			else
			{
				trap_SendServerCommand( ent-g_entities, "cp \"Wait until next round to change class\"" );
			}
			return;
		}
	}

	if ( g_pModSpecialties.integer )
	{
		if ( ent->client->classChangeDebounceTime > level.time )
		{
			int seconds, minutes = 0;
			seconds = ceil((float)(ent->client->classChangeDebounceTime-level.time)/1000.0f);
			if ( seconds >= 60 )
			{
				minutes = floor(seconds/60.0f);
				seconds -= minutes*60;
				if ( minutes > 1 )
				{
					if ( seconds )
					{
						if ( seconds > 1 )
						{
							trap_SendServerCommand( ent-g_entities, va("cp \"Cannot change classes again for %d minutes and %d seconds\"", minutes, seconds ) );
						}
						else
						{
							trap_SendServerCommand( ent-g_entities, va("cp \"Cannot change classes again for %d minutes\"", minutes ) );
						}
					}
					else
					{
						trap_SendServerCommand( ent-g_entities, va("cp \"Cannot change classes again for %d minutes\"", minutes ) );
					}
				}
				else
				{
					if ( seconds )
					{
						if ( seconds > 1 )
						{
							trap_SendServerCommand( ent-g_entities, va("cp \"Cannot change classes again for %d minute and %d seconds\"", minutes, seconds ) );
						}
						else
						{
							trap_SendServerCommand( ent-g_entities, va("cp \"Cannot change classes again for %d minute and %d second\"", minutes, seconds ) );
						}
					}
					else
					{
						trap_SendServerCommand( ent-g_entities, va("cp \"Cannot change classes again for %d minute\"", minutes ) );
					}
				}
			}
			else
			{
				if ( seconds > 1 ) 
				{
					trap_SendServerCommand( ent-g_entities, va("cp \"Cannot change classes again for %d seconds\"", seconds ) );
				}
				else
				{
					trap_SendServerCommand( ent-g_entities, va("cp \"Cannot change classes again for %d second\"", seconds ) );
				}
			}
			return;
		}
	}

	//trying to set your class
	trap_Argv( 1, s, sizeof( s ) );
	//can't manually change to some classes
	if ( Q_stricmp( "borg", s ) == 0 || Q_stricmp( "hero", s ) == 0 || Q_stricmp( "vip", s ) == 0 )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"Cannot manually change to class %s\n\"", s ) );
		return;
	}
	
	//can't change from a Borg class
	if ( ent->client->sess.sessionClass == PC_BORG )
	{
		trap_SendServerCommand( ent-g_entities, "print \"Cannot manually change from class Borg\n\"" );
		return;
	}

	//if this is a manual change, not an assimilation, uninitialize the clInitStatus data
	clientInitialStatus[ent->s.number].initialized = qfalse;
	if ( SetClass( ent, s, NULL ) )
	{
		//if still in warmup, don't debounce class changes
		if ( g_doWarmup.integer )
		{
			if ( level.warmupTime != 0 )
			{
				if ( level.warmupTime < 0 || level.time - level.startTime <= level.warmupTime )
				{
					return;
				}
			}
		}
		//if warmuptime is over, don't change classes again for a bit
		ent->client->classChangeDebounceTime = level.time + (g_classChangeDebounceTime.integer*1000);
	}
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
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	if ( g_pModElimination.integer != 0 )
	{//don't do this follow stuff, it's bad!
		VectorCopy( level.clients[i].ps.viewangles, ent->client->ps.viewangles );
		VectorCopy( level.clients[i].ps.origin, ent->client->ps.origin );
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( g_gametype.integer == GT_TOURNAMENT && ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
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

	// if they are playing a tournement game, count as a loss
	if ( g_gametype.integer == GT_TOURNAMENT && ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		if ( g_pModElimination.integer == 0 )
		{
			SetTeam( ent, "spectator" );
		}
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
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
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		if ( g_pModElimination.integer != 0 )
		{//don't do this follow stuff, it's bad!
			if ( level.clients[ clientnum ].ps.eFlags&EF_ELIMINATED)
			{//don't cycle to a dead guy
				continue;
			}
			VectorCopy( level.clients[clientnum].ps.viewangles, ent->client->ps.viewangles );
			VectorCopy( level.clients[clientnum].ps.origin, ent->client->ps.origin );
			ent->client->sess.spectatorClient = clientnum;
			return;
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

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order > sizeof(gc_orders)/sizeof(char *) ) {
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
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent ) {
	int		i;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, "print \"Voting not allowed here.\n\"" );
		return;
	}

	if ( level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, "print \"A vote is already in progress.\n\"" );
		return;
	}
	if ( ent->client->pers.voteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, "print \"You have called the maximum number of votes.\n\"" );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "map_restart" ) ) {
	} else if ( !Q_stricmp( arg1, "nextmap" ) ) {
	} else if ( !Q_stricmp( arg1, "map" ) ) {
	} else if ( !Q_stricmp( arg1, "g_gametype" ) ) {
	} else if ( !Q_stricmp( arg1, "kick" ) ) {
	} else if ( !Q_stricmp( arg1, "g_doWarmup" ) ) {
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Vote commands are: map_restart, nextmap, map <mapname>, g_gametype <n>, kick <player>, and g_dowarmup <b>.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "map" ) ) {
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char	s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
		}

	} else {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
	}

	trap_SendServerCommand( -1, va("print \"%s called a vote.\n\"", ent->client->pers.netname ) );

	// start the voting, the caller autoamtically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}
	ent->client->ps.eFlags |= EF_VOTED;

	trap_SetConfigstring( CS_VOTE_TIME, va("%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteString );	
	trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64];

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, "print \"No vote in progress.\n\"" );
		return;
	}
	if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent-g_entities, "print \"Vote already cast.\n\"" );
		return;
	}

	trap_SendServerCommand( ent-g_entities, "print \"Vote cast.\n\"" );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
	}

	// a majority will be determined in G_CheckVote, which will also account
	// for players entering or leaving
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
	if ( !ent->client ) {
		return;		// not fully in game yet
	}


	trap_Argv( 0, cmd, sizeof( cmd ) );

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
	else if (Q_stricmp (cmd, "callvote") == 0)
		Cmd_CallVote_f (ent);
	else if (Q_stricmp (cmd, "vote") == 0)
		Cmd_Vote_f (ent);
	else if (Q_stricmp (cmd, "gc") == 0)
		Cmd_GameCommand_f( ent );
	else if (Q_stricmp (cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f( ent );
	else
		trap_SendServerCommand( clientNum, va("print \"unknown cmd %s\n\"", cmd ) );
}
