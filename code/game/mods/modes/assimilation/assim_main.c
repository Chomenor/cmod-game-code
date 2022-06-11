/*
* Assimilation Mode
* 
* This mode has two teams, a borg team and a human team. The borg team has one
* player selected as the "queen", and the rest are drones (which generally are not
* very powerful and serve to assist the queen). The borg can assimilate players
* from the humans to have them become drones for the rest of the match.
* 
* The borg team wins the game by assimilating all the human players, and the
* human team wins if they kill the borg queen.
*/

#include "mods/modes/assimilation/assim_local.h"

#define PREFIX( x ) ModAssimilation_##x
#define MOD_STATE PREFIX( state )

typedef struct {
	// Differentiates assimilated player versus chosen borg team
	qboolean assimilated;

	// For specialties, save player class prior to joining borg
	pclass_t oldClass;
} assimilation_client_t;

static struct {
	assimilation_client_t clients[MAX_CLIENTS];
	
	// Current match state
	int borgQueenClientNum;		// -1 if not selected
	int joinLimitTime;	// Time when there were enough players to start the game (0 if not started)
	int numAssimilated;

	// Which team is borg
	team_t borgTeam;	// TEAM_FREE if not selected

	// If server falls empty during intermission, don't try to keep the same borg team color next round
	qboolean staleBorgTeam;

	// For mod function stacking
	ModFNType_GenerateGlobalSessionInfo Prev_GenerateGlobalSessionInfo;
	ModFNType_CheckJoinAllowed Prev_CheckJoinAllowed;
	ModFNType_PostPlayerDie Prev_PostPlayerDie;
	ModFNType_PrePlayerLeaveTeam Prev_PrePlayerLeaveTeam;
	ModFNType_PreClientConnect Prev_PreClientConnect;
	ModFNType_InitClientSession Prev_InitClientSession;
	ModFNType_UpdateSessionClass Prev_UpdateSessionClass;
	ModFNType_SpawnConfigureClient Prev_SpawnConfigureClient;
	ModFNType_SpawnTransporterEffect Prev_SpawnTransporterEffect;
	ModFNType_RealSessionClass Prev_RealSessionClass;
	ModFNType_RealSessionTeam Prev_RealSessionTeam;
	ModFNType_CheckRespawnTime Prev_CheckRespawnTime;
	ModFNType_CheckReplaceItem Prev_CheckReplaceItem;
	ModFNType_CanItemBeDropped Prev_CanItemBeDropped;
	ModFNType_AddRegisteredItems Prev_AddRegisteredItems;
	ModFNType_CheckSuicideAllowed Prev_CheckSuicideAllowed;
	ModFNType_ModClientCommand Prev_ModClientCommand;
	ModFNType_AdjustGeneralConstant Prev_AdjustGeneralConstant;
	ModFNType_PostRunFrame Prev_PostRunFrame;
	ModFNType_MatchStateTransition Prev_MatchStateTransition;
} *MOD_STATE;

/*
================
(ModFN) IsBorgQueen
================
*/
LOGFUNCTION_SRET( qboolean, PREFIX(IsBorgQueen), ( int clientNum ),
		( clientNum ), "G_MODFN_ISBORGQUEEN" ) {
	return clientNum >= 0 && clientNum == MOD_STATE->borgQueenClientNum;
}

/*
================
(ModFN) GenerateGlobalSessionInfo

When level is exiting, save whether the borg team was red or blue, so we can try
to keep the same configuration when the map changes or restarts.
================
*/
LOGFUNCTION_SVOID( PREFIX(GenerateGlobalSessionInfo), ( info_string_t *info ), ( info ), "G_MODFN_GENERATEGLOBALSESSIONINFO" ) {
	MOD_STATE->Prev_GenerateGlobalSessionInfo( info );

	if ( !MOD_STATE->staleBorgTeam ) {
		const char *value = "";
		if ( MOD_STATE->borgTeam == TEAM_RED ) {
			value = "red";
		}
		if ( MOD_STATE->borgTeam == TEAM_BLUE ) {
			value = "blue";
		}

		Info_SetValueForKey_Big( info->s, "lastBorgTeam", value );
	}
}

/*
================
ModAssimilation_SetBorgColor
================
*/
LOGFUNCTION_SVOID( ModAssimilation_SetBorgColor, ( team_t team, const char *reason ), ( team, reason ), "G_ASSIMILATION" ) {
	MOD_STATE->borgTeam = team;
	G_DedPrintf( "assimilation: Selected %s color borg team due to %s.\n",
			MOD_STATE->borgTeam == TEAM_RED ? "red" : "blue", reason );

	if ( team == TEAM_RED ) {
		trap_SetConfigstring( CS_RED_GROUP, "Borg" );
		trap_SetConfigstring( CS_BLUE_GROUP, g_team_group_blue.string );
	} else {
		trap_SetConfigstring( CS_RED_GROUP, g_team_group_red.string );
		trap_SetConfigstring( CS_BLUE_GROUP, "Borg" );
	}
}

/*
================
ModAssimilation_DetermineBorgColor
================
*/
LOGFUNCTION_SVOID( ModAssimilation_DetermineBorgColor, ( qboolean restarting ), ( restarting ), "G_ASSIMILATION" ) {
	// Maps can force a certain borg team, along with a queen spawn point, via a
	// "team_CTF_redplayer" or "team_CTF_blueplayer" entity with spawnflags 1.
	// For example, on map "hm_borgattack" borg team is always red.
	if ( level.borgQueenStartPoint && !Q_stricmp( level.borgQueenStartPoint->classname, "team_CTF_redplayer" ) ) {
		ModAssimilation_SetBorgColor( TEAM_RED, "map spawn" );
		return;
	}
	if ( level.borgQueenStartPoint && !Q_stricmp( level.borgQueenStartPoint->classname, "team_CTF_blueplayer" ) ) {
		ModAssimilation_SetBorgColor( TEAM_BLUE, "map spawn" );
		return;
	}

	// Borg team can also be set via team group cvar. In the original implementation
	// these checks are case sensitive and may not take effect in certain situations,
	// but for the most part this matches the original EF behavior.
	if ( !Q_stricmpn( "Borg", g_team_group_red.string, 4 ) ) {
		ModAssimilation_SetBorgColor( TEAM_RED, "team group setting" );
		return;
	}
	if ( !Q_stricmpn( "Borg", g_team_group_blue.string, 4 ) ) {
		ModAssimilation_SetBorgColor( TEAM_BLUE, "team group setting" );
		return;
	}

	if ( restarting ) {
		// If coming from a map restart or map change, try to read the existing team from cvar.
		const char *team;
		info_string_t info;
		G_RetrieveGlobalSessionInfo( &info );
		team = Info_ValueForKey( info.s, "lastBorgTeam" );

		if ( !Q_stricmp( team, "red" ) ) {
			ModAssimilation_SetBorgColor( TEAM_RED, "previous session" );
			return;
		}
		if ( !Q_stricmp( team, "blue" ) ) {
			ModAssimilation_SetBorgColor( TEAM_BLUE, "previous session" );
			return;
		}
	}

	// Finally pick the team randomly. This is a bit different from the original implementation,
	// which waits until the first player joins a team, and always sets that team as human.
	// One advantage of picking the team type at this stage instead of waiting for the player to join
	// is that the UI could be updated to display the team types so the player can pick between them.
	ModAssimilation_SetBorgColor( rand() % 2 ? TEAM_RED : TEAM_BLUE, "random selection" );
}

/*
================
ModAssimilation_MatchLocked

Match is considered locked if g_noJoinTimeout has been reached, or if one or more players
have been assimilated.
================
*/
static qboolean ModAssimilation_MatchLocked( void ) {
	if ( level.matchState >= MS_ACTIVE ) {
		if ( MOD_STATE->numAssimilated > 0 ) {
			return qtrue;
		}

		else if ( g_noJoinTimeout.integer > 0 && level.time >= MOD_STATE->joinLimitTime + ( g_noJoinTimeout.integer * 1000 ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
(ModFN) CheckJoinAllowed

Check if joining or changing team/class is disabled due to match in progress.
================
*/
LOGFUNCTION_SRET( qboolean, PREFIX(CheckJoinAllowed), ( int clientNum, join_allowed_type_t type, team_t targetTeam ),
		( clientNum, type, targetTeam ), "G_MODFN_CHECKJOINALLOWED" ) {
	gclient_t *client = &level.clients[clientNum];
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	if ( level.matchState >= MS_INTERMISSION_QUEUED ) {
		return qfalse;
	}

	if ( ModAssimilation_MatchLocked() ) {
		if ( type == CJA_SETTEAM ) {
			if ( modclient->assimilated ) {
				trap_SendServerCommand( clientNum, "cp \"You have been assimilated until next round\"" );
			} else if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
				trap_SendServerCommand( clientNum, "cp \"Wait until next round to join\"" );
			} else {
				trap_SendServerCommand( clientNum, "cp \"Wait until next round to change teams\"" );
			}
		}

		if ( type == CJA_SETCLASS ) {
			if ( modclient->assimilated ) {
				trap_SendServerCommand( clientNum, "cp \"You have been assimilated until next round\"" );
			} else if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
				trap_SendServerCommand( clientNum, "cp \"Wait until next round to join\"" );
			} else {
				trap_SendServerCommand( clientNum, "cp \"Wait until next round to change class\"" );
			}
		}

		return qfalse;
	}

	return MOD_STATE->Prev_CheckJoinAllowed( clientNum, type, targetTeam );
}

/*
================
ModAssimilation_SetQueen

Called when picking a new queen.
================
*/
LOGFUNCTION_SVOID( ModAssimilation_SetQueen, ( int clientNum ), ( clientNum ), "G_ASSIMILATION" ) {
	if ( clientNum == MOD_STATE->borgQueenClientNum ) {
		return;
	}

	if ( MOD_STATE->borgQueenClientNum != -1 ) {
		// Reset existing queen
		int oldQueen = MOD_STATE->borgQueenClientNum;
		MOD_STATE->borgQueenClientNum = -1;
		if ( level.clients[oldQueen].pers.connected == CON_CONNECTED ) {
			ClientUserinfoChanged( oldQueen );
			ClientSpawn( &g_entities[oldQueen], CST_RESPAWN );
		}
	}

	if ( clientNum != -1 && G_AssertConnectedClient( clientNum ) ) {
		// Set new queen
		MOD_STATE->borgQueenClientNum = clientNum;
		ClientUserinfoChanged( MOD_STATE->borgQueenClientNum );
		ClientSpawn( &g_entities[clientNum], CST_RESPAWN );
	}
}

/*
================
ModAssimilation_CheckReplaceQueen

Checks if current borg queen is valid, and picks new one if necessary.
================
*/
LOGFUNCTION_SVOID( ModAssimilation_CheckReplaceQueen, ( void ), (), "" ) {
	// Check if existing queen is invalid.
	// Shouldn't normally happen, as queen exits should be processed immediately.
	if ( MOD_STATE->borgQueenClientNum >= 0 ) {
		const gclient_t *client = &level.clients[MOD_STATE->borgQueenClientNum];
		if ( client->pers.connected != CON_CONNECTED || client->sess.sessionTeam == TEAM_SPECTATOR ||
				( level.matchState == MS_ACTIVE && client->ps.pm_type == PM_DEAD ) ) {
			G_DedPrintf( "assimilation: WARNING - ModAssimilation_CheckReplaceQueen - Unexpected queen exit\n" );
			MOD_STATE->borgQueenClientNum = -1;
		}
	}

	// Wait until at least 10 seconds into map to select queen.
	if ( MOD_STATE->borgQueenClientNum == -1 && level.matchState < MS_INTERMISSION_QUEUED &&
			!level.warmupTime && level.time >= 10000 ) {
		int candidates[MAX_CLIENTS];
		int count = 0;
		int i;

		for ( i = 0; i < level.maxclients; i++ ) {
			gclient_t *client = &level.clients[i];
			if ( client->pers.connected == CON_CONNECTED && client->sess.sessionTeam != TEAM_SPECTATOR &&
					client->sess.sessionTeam == MOD_STATE->borgTeam ) {
				candidates[count++] = i;
			}
		}

		if ( count > 0 ) {
			ModAssimilation_SetQueen( candidates[irandom( 0, count - 1 )] );
		}
	}
}

/*
================
(ModFN) PostPlayerDie

Check for borg queen being killed.

If non-borg are killed by assimilator weapon, set them to assimilated. Actual transition to borg team
will wait until respawn.
================
*/
LOGFUNCTION_SVOID( PREFIX(PostPlayerDie), ( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath, int *awardPoints ),
		( self, inflictor, attacker, meansOfDeath, awardPoints ), "G_MODFN_POSTPLAYERDIE" ) {
	int clientNum = self - g_entities;
	gclient_t *client = &level.clients[clientNum];
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_PostPlayerDie( self, inflictor, attacker, meansOfDeath, awardPoints );

	if ( level.matchState == MS_ACTIVE ) {
		if ( clientNum == MOD_STATE->borgQueenClientNum && level.matchState == MS_ACTIVE ) {
			// Borg team lost
			int killedByClientNum;
			if ( attacker && attacker->client ) {
				killedByClientNum = attacker - g_entities;
			} else {
				killedByClientNum = clientNum;
			}
			
			if ( level.clients[killedByClientNum].sess.sessionTeam == MOD_STATE->borgTeam ) {
				// Killed by self or another borg - give them negative score
				if ( EF_WARN_ASSERT( *awardPoints == -1 ) ) {
					*awardPoints = -500;
				}
			} else {
				// Killed by other team - reward the player who got the kill
				if ( EF_WARN_ASSERT( *awardPoints == 1 ) ) {
					*awardPoints = 500;
				}
			}

			G_LogExit( "The Borg Queen has been killed!" );
		}

		if ( meansOfDeath == MOD_ASSIMILATE ) {
			// Go to Borg team if killed by assimilation.
			if ( attacker && attacker->client && attacker->client->sess.sessionTeam != self->client->sess.sessionTeam ) {
				MOD_STATE->numAssimilated++;
				if ( EF_WARN_ASSERT( *awardPoints == 1 ) ) {
					*awardPoints = 10;	// 10 points for an assimilation
				}

				modclient->assimilated = qtrue;
			}
		}
	}
}

/*
================
(ModFN) PrePlayerLeaveTeam

Check for borg queen disconnecting or leaving the borg team.
================
*/
LOGFUNCTION_SVOID( PREFIX(PrePlayerLeaveTeam), ( int clientNum, team_t oldTeam ),
		( clientNum, oldTeam ), "G_MODFN_PREPLAYERLEAVETEAM" ) {
	MOD_STATE->Prev_PrePlayerLeaveTeam( clientNum, oldTeam );

	if ( clientNum == MOD_STATE->borgQueenClientNum ) {
		// If a fully qualified match was in progress when the borg queen quit,
		// award a win to the other team.
		if ( level.matchState == MS_ACTIVE && ModAssimilation_MatchLocked() ) {
			level.teamScores[MOD_STATE->borgTeam] -= 500;
			CalculateRanks();
			G_LogExit( "The Borg Queen has been killed!" );
		}

		// A new queen will be picked later in CheckReplaceQueen if needed.
		MOD_STATE->borgQueenClientNum = -1;
	}
}

/*
================
(ModFN) PreClientConnect
================
*/
LOGFUNCTION_SVOID( PREFIX(PreClientConnect), ( int clientNum, qboolean firstTime, qboolean isBot ),
		( clientNum, firstTime, isBot ), "G_MODFN_PRECLIENTCONNECT" ) {
	if ( !MOD_STATE->borgTeam ) {
		// Determine borg team color.
		ModAssimilation_DetermineBorgColor( !firstTime );
	}
}

/*
================
(ModFN) InitClientSession
================
*/
LOGFUNCTION_SVOID( PREFIX(InitClientSession), ( int clientNum, qboolean initialConnect, const info_string_t *info ),
		( clientNum, initialConnect, info ), "G_MODFN_INITCLIENTSESSION" ) {
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_InitClientSession( clientNum, initialConnect, info );
	memset( modclient, 0, sizeof( *modclient ) );
}

/*
================
(ModFN) UpdateSessionClass

Set borg team members to borg class and vice versa.
================
*/
LOGFUNCTION_SVOID( PREFIX(UpdateSessionClass), ( int clientNum ),
		( clientNum ), "G_MODFN_UPDATESESSIONCLASS" ) {
	gclient_t *client = &level.clients[clientNum];
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	// make sure borg team is set to borg class
	if ( client->sess.sessionTeam == MOD_STATE->borgTeam && client->sess.sessionClass != PC_BORG ) {
		modclient->oldClass = client->sess.sessionClass;
		client->sess.sessionClass = PC_BORG;
	}

	if ( client->sess.sessionTeam != MOD_STATE->borgTeam ) {
		// try to revert to pre-borg class
		if ( modclient->oldClass != PC_NOCLASS ) {
			client->sess.sessionClass = modclient->oldClass;
		}

		// make sure class is non-borg
		if ( client->sess.sessionClass == PC_BORG ) {
			client->sess.sessionClass = PC_NOCLASS;
		}

		// clear out borg-specific attributes
		modclient->assimilated = qfalse;
		modclient->oldClass = PC_NOCLASS;

		MOD_STATE->Prev_UpdateSessionClass( clientNum );
	}
}

/*
================
(ModFN) SpawnConfigureClient
================
*/
LOGFUNCTION_SVOID( PREFIX(SpawnConfigureClient), ( int clientNum ), ( clientNum ), "G_MODFN_SPAWNCONFIGURECLIENT" ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	if ( client->sess.sessionClass == PC_BORG ) {
		// Weapons
		client->ps.stats[STAT_WEAPONS] = ( 1 << WP_BORG_ASSIMILATOR );
		client->ps.ammo[WP_BORG_ASSIMILATOR] = Max_Ammo[WP_BORG_ASSIMILATOR];
		if ( clientNum != MOD_STATE->borgQueenClientNum ) {
			// projectile/shock weapon
			client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_BORG_WEAPON );
			client->ps.ammo[WP_BORG_WEAPON] = Max_Ammo[WP_BORG_WEAPON];
		}

		// Health
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];

		// Armor
		client->ps.stats[STAT_ARMOR] = 100;

		// Powerups
		if ( clientNum == MOD_STATE->borgQueenClientNum ) {
			//queen has full regeneration
			gitem_t *regen = BG_FindItemForPowerup( PW_REGEN );
			client->ps.powerups[regen->giTag] = level.time - ( level.time % 1000 );
			client->ps.powerups[regen->giTag] += 1800 * 1000;
		}

		if ( modclient->assimilated ) {
			// Add assimilated eFlag - generally unused, but just in case something in the client or engine
			// wants to reference it. Also gets copied to ent->s.eFlags via BG_PlayerStateToEntityState.
			client->ps.eFlags |= EF_ASSIMILATED;
		}
	}

	else {
		MOD_STATE->Prev_SpawnConfigureClient( clientNum );
	}
}

/*
===========
(ModFN) SpawnCenterPrintMessage

Prints info messages to client during ClientSpawn.
============
*/
LOGFUNCTION_SVOID( PREFIX(SpawnCenterPrintMessage), ( int clientNum, clientSpawnType_t spawnType ),
		( clientNum, spawnType ), "G_MODFN_SPAWNCENTERPRINTMESSAGE" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( clientNum == MOD_STATE->borgQueenClientNum ) {
		trap_SendServerCommand( clientNum, "cp \"Assimilation:\nYou Are the Queen!\n\"" );
	}

	else if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( clientNum, "cp \"Assimilation\"" );
	}

	else if ( spawnType != CST_RESPAWN ) {
		if ( client->sess.sessionClass == PC_BORG ) {
			trap_SendServerCommand( clientNum, "cp \"Assimilation:\nAssimilate all enemies!\n\"" );
		}

		else {
			trap_SendServerCommand( clientNum, "cp \"Assimilation:\nKill the Borg Queen!\n\"" );
		}
	}
}

/*
===========
(ModFN) SpawnTransporterEffect

Play special transporter effects for borg.
============
*/
LOGFUNCTION_SVOID( PREFIX(SpawnTransporterEffect), ( int clientNum, clientSpawnType_t spawnType ),
		( clientNum, spawnType ), "G_MODFN_SPAWNTRANSPORTEREFFECT" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( clientNum == MOD_STATE->borgQueenClientNum ) {
		// no effect for queen
		return;
	}

	if ( spawnType == CST_RESPAWN && client->sess.sessionClass == PC_BORG ) {
		// borg effect for respawning borg
		gentity_t *tent = G_TempEntity( client->ps.origin, EV_BORG_TELEPORT );
		tent->s.clientNum = clientNum;
	}

	else {
		MOD_STATE->Prev_SpawnTransporterEffect( clientNum, spawnType );
	}
}

/*
================
(ModFN) RealSessionTeam

Returns player-selected team, even if active team is overriden by borg assimilation.
================
*/
static team_t PREFIX(RealSessionTeam)( int clientNum ) {
	gclient_t *client = &level.clients[clientNum];
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	if ( modclient->assimilated ) {
		// client's actual team is the non-borg one
		if ( MOD_STATE->borgTeam == TEAM_RED ) {
			return TEAM_BLUE;
		}
		if ( EF_WARN_ASSERT( MOD_STATE->borgTeam == TEAM_BLUE ) ) {
			return TEAM_RED;
		}
	}

	return MOD_STATE->Prev_RealSessionTeam( clientNum );
}

/*
================
(ModFN) RealSessionClass

Returns player-selected class, even if active class is overridden by borg team/assimilation.
================
*/
static pclass_t PREFIX(RealSessionClass)( int clientNum ) {
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];
	if ( modclient->oldClass != PC_NOCLASS ) {
		return modclient->oldClass;
	}

	return MOD_STATE->Prev_RealSessionClass( clientNum );
}

/*
================
(ModFN) CheckExitRules

Check for borg team win due to all players on other team being assimilated.
================
*/
LOGFUNCTION_SVOID( PREFIX(CheckExitRules), ( void ), (), "G_MODFN_CHECKEXITRULES" ) {
	if ( MOD_STATE->numAssimilated > 0 ) {
		int i;
		gclient_t *survivor = NULL;

		for ( i = 0; i < level.maxclients; i++ ) {
			gclient_t *cl = &level.clients[i];
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR
					&& cl->sess.sessionTeam != MOD_STATE->borgTeam ) {
				survivor = cl;
				break;
			}
		}

		if ( survivor == NULL ) {
			G_LogExit( "Assimilation Complete." );
			return;
		}
	}
}

/*
================
(ModFN) ClientRespawn

Move players to borg team if they were assimilated.
================
*/
LOGFUNCTION_SEVOID( PREFIX(ClientRespawn), ( int clientNum ), ( clientNum ), clientNum, "G_MODFN_CLIENTRESPAWN" ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	if ( modclient->assimilated && client->sess.sessionTeam != MOD_STATE->borgTeam ) {
		// Freshly assimilated player needs to be transferred to borg team
		team_t oldTeam = client->sess.sessionTeam;
		client->sess.sessionTeam = MOD_STATE->borgTeam;
		BroadcastTeamChange( client, oldTeam );
		ClientBegin( clientNum );

		ent->s.eFlags |= EF_ASSIMILATED;
		ent->client->ps.eFlags |= EF_ASSIMILATED;
		return;
	}

	// original comment - 'assimilated players don't leave corpses'
	// Apparently the original behavior is to disable corpses for all players, not just assimilated ones...
	// CopyToBodyQueQue (ent);

	ClientSpawn( ent, CST_RESPAWN );
}

/*
==============
(ModFN) CheckRespawnTime

Force respawn after 3 seconds.
==============
*/
LOGFUNCTION_SRET( qboolean, PREFIX(CheckRespawnTime), ( int clientNum, qboolean voluntary ),
		( clientNum, voluntary ), "G_MODFN_CHECKRESPAWNTIME" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( !voluntary && level.time > client->respawnKilledTime + 3000 ) {
		return qtrue;
	}

	return MOD_STATE->Prev_CheckRespawnTime( clientNum, voluntary );
}

/*
================
(ModFN) CheckReplaceItem

Replace tetrion weapon with imod.
================
*/
LOGFUNCTION_SRET( gitem_t *, PREFIX(CheckReplaceItem), ( gentity_t *ent, gitem_t *item ),
		( ent, item ), "G_MODFN_CHECKREPLACEITEM" ) {
	item = MOD_STATE->Prev_CheckReplaceItem( ent, item );

	switch ( item->giTag ) {
		case WP_SCAVENGER_RIFLE:
		case WP_TETRION_DISRUPTOR:
			switch ( item->giType ) {
				case IT_WEAPON:
					ent->classname = "weapon_imod";
					return BG_FindItemForWeapon( WP_IMOD );
				case IT_AMMO:
					ent->classname = "ammo_imod";
					return BG_FindItemForAmmo( WP_IMOD );
			}
	}

	return item;
}

/*
============
(ModFN) CanItemBeDropped

Borg don't drop items.
============
*/
LOGFUNCTION_SRET( qboolean, PREFIX(CanItemBeDropped), ( gitem_t *item, int clientNum ),
		( item, clientNum ), "G_MODFN_CANITEMBEDROPPED" ) {
	gclient_t *client = &level.clients[clientNum];

	// Allow dropping the flag, as per original implementation,
	// even though there shouldn't be flags in assimilation...
	if ( client->sess.sessionClass == PC_BORG && item->giType != IT_TEAM ) {
		return qfalse;
	}

	return MOD_STATE->Prev_CanItemBeDropped( item, clientNum );
}

/*
================
(ModFN) AddRegisteredItems
================
*/
LOGFUNCTION_SVOID( PREFIX(AddRegisteredItems), ( void ), (), "G_MODFN_ADDREGISTEREDITEMS" ) {
	MOD_STATE->Prev_AddRegisteredItems();

	RegisterItem( BG_FindItemForWeapon( WP_BORG_ASSIMILATOR ) );
	RegisterItem( BG_FindItemForWeapon( WP_BORG_WEAPON ) );	//queen
	RegisterItem( BG_FindItemForPowerup( PW_REGEN ) );	//queen
}

/*
=================
(ModFN) CheckSuicideAllowed
=================
*/
LOGFUNCTION_SRET( qboolean, PREFIX(CheckSuicideAllowed), ( int clientNum ),
		( clientNum ), "G_MODFN_CHECKSUICIDEALLOWED" ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionClass != PC_BORG ) {
		// Disallow suicides by feds so they can't cheat their way out of dangerous situations.
		trap_SendServerCommand( clientNum, "print \"Cannot suicide in Assimilation mode.\n\"" );
		return qfalse;
	}

	return MOD_STATE->Prev_CheckSuicideAllowed( clientNum );
}

/*
================
(ModFN) ModClientCommand

Handle class command for borg (which can't change class directly).
================
*/
LOGFUNCTION_SRET( qboolean, PREFIX(ModClientCommand), ( int clientNum, const char *cmd ),
		( clientNum, cmd ), "G_MODFN_MODCLIENTCOMMAND" ) {
	if ( !Q_stricmp( cmd, "class" ) && !level.intermissiontime && level.clients[clientNum].sess.sessionClass == PC_BORG ) {
		if ( trap_Argc() != 2 ) {
			trap_SendServerCommand( clientNum, "print \"class: Borg\n\"" );
		} else {
			trap_SendServerCommand( clientNum, "print \"Cannot manually change from class Borg\n\"" );
		}
		return qtrue;
	}

	return MOD_STATE->Prev_ModClientCommand( clientNum, cmd );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int PREFIX(AdjustGeneralConstant)( generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_ASSIMILATION_MODELS )
		return 1;
	if ( gcType == GC_DISABLE_TEAM_FORCE_BALANCE )
		return 1;

	return MOD_STATE->Prev_AdjustGeneralConstant( gcType, defaultValue );
}

/*
================
(ModFN) PostRunFrame
================
*/
LOGFUNCTION_SVOID( PREFIX(PostRunFrame), ( void ), (), "G_MODFN_POSTRUNFRAME" ) {
	MOD_STATE->Prev_PostRunFrame();

	ModAssimilation_CheckReplaceQueen();

	// If server is empty reset the borg team, so the server isn't stuck on the same borg team indefinitely.
	// New borg team will be selected when first client connects via PreClientConnect.
	if ( MOD_STATE->borgTeam && level.numConnectedClients == 0 ) {
		G_DedPrintf( "assimilation: Resetting borg team color due to empty server.\n" );
		MOD_STATE->borgTeam = TEAM_FREE;
	}

	// If we get to the intermission with only bots, don't transfer borg team into next round.
	// Same purpose as the check above, but for servers with bots where connected clients doesn't go to 0.
	if ( level.matchState == MS_INTERMISSION_ACTIVE && !MOD_STATE->staleBorgTeam && !G_CountHumanPlayers( -1 ) ) {
		G_DedPrintf( "assimilation: Setting stale borg team color due to bot-only intermission.\n" );
		MOD_STATE->staleBorgTeam = qtrue;
	}
}

/*
================
(ModFN) MatchStateTransition
================
*/
LOGFUNCTION_SVOID( PREFIX(MatchStateTransition), ( matchState_t oldState, matchState_t newState ),
		( oldState, newState ), "G_MODFN_MATCHSTATETRANSITION" ) {
	MOD_STATE->Prev_MatchStateTransition( oldState, newState );

	if ( newState < MS_ACTIVE && !EF_WARN_ASSERT( MOD_STATE->numAssimilated == 0 ) ) {
		// Shouldn't happen - if players were assimilated we should hit CheckExitRules instead.
		MOD_STATE->numAssimilated = 0;
	}

	// Update join limit timer.
	if ( newState == MS_ACTIVE ) {
		G_DedPrintf( "assimilation: Starting join limit countdown due to sufficient players.\n" );
		MOD_STATE->joinLimitTime = level.time;
	} else {
		if ( newState < MS_ACTIVE && MOD_STATE->joinLimitTime ) {
			G_DedPrintf( "assimilation: Resetting join limit time due to insufficient players.\n" );
		}
		MOD_STATE->joinLimitTime = 0;
	}
}

/*
================
ModAssimilation_Init
================
*/

#define INIT_FN_STACKABLE( name ) \
	MOD_STATE->Prev_##name = modfn.name; \
	modfn.name = PREFIX(name);

#define INIT_FN_OVERRIDE( name ) \
	modfn.name = PREFIX(name);

#define INIT_FN_BASE( name ) \
	EF_WARN_ASSERT( modfn.name == ModFNDefault_##name ); \
	modfn.name = PREFIX(name);

LOGFUNCTION_VOID( ModAssimilation_Init, ( void ), (), "G_MOD_INIT G_ASSIMILATION" ) {
	if ( EF_WARN_ASSERT( !MOD_STATE ) ) {
		modcfg.mods_enabled.assimilation = qtrue;

		// Support combining with other mods
		if ( G_ModUtils_GetLatchedValue( "g_pModSpecialties", "0", 0 ) ) {
			ModSpecialties_Init();
		}

		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MOD_STATE->borgQueenClientNum = -1;

		// Register mod functions
		INIT_FN_BASE( IsBorgQueen );
		INIT_FN_STACKABLE( GenerateGlobalSessionInfo );
		INIT_FN_STACKABLE( CheckJoinAllowed );
		INIT_FN_STACKABLE( PostPlayerDie );
		INIT_FN_STACKABLE( PrePlayerLeaveTeam );
		INIT_FN_STACKABLE( PreClientConnect );
		INIT_FN_STACKABLE( InitClientSession );
		INIT_FN_STACKABLE( UpdateSessionClass );
		INIT_FN_STACKABLE( SpawnConfigureClient );
		INIT_FN_BASE( SpawnCenterPrintMessage );
		INIT_FN_STACKABLE( SpawnTransporterEffect );
		INIT_FN_STACKABLE( RealSessionClass );
		INIT_FN_STACKABLE( RealSessionTeam );
		INIT_FN_BASE( CheckExitRules );
		INIT_FN_BASE( ClientRespawn );
		INIT_FN_STACKABLE( CheckRespawnTime );
		INIT_FN_STACKABLE( CheckReplaceItem );
		INIT_FN_STACKABLE( CanItemBeDropped );
		INIT_FN_STACKABLE( AddRegisteredItems );
		INIT_FN_STACKABLE( CheckSuicideAllowed );
		INIT_FN_STACKABLE( ModClientCommand );
		INIT_FN_STACKABLE( AdjustGeneralConstant );
		INIT_FN_STACKABLE( PostRunFrame );
		INIT_FN_STACKABLE( MatchStateTransition );

		// Support modules
		ModAssimBorgAdapt_Init();
		ModAssimBorgTeleport_Init();

		// Always set g_gametype 3
		trap_Cvar_Set( "g_gametype", "3" );
	}
}
