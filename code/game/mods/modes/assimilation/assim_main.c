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

#define MOD_NAME ModAssimilation

#include "mods/modes/assimilation/assim_local.h"

typedef struct {
	// Differentiates assimilated player versus chosen borg team
	qboolean assimilated;

	// For specialties, save player class prior to joining borg
	pclass_t oldClass;
} assimilation_client_t;

static struct {
	assimilation_client_t clients[MAX_CLIENTS];

	trackedCvar_t g_preferNonBotQueen;

	// Current match state
	int borgQueenClientNum;		// -1 if not selected
	int numAssimilated;

	// Which team is borg
	team_t borgTeam;	// TEAM_FREE if not selected

	// If server falls empty during intermission, don't try to keep the same borg team color next round
	qboolean staleBorgTeam;
} *MOD_STATE;

/*
================
(ModFN) IsBorgQueen
================
*/
static qboolean MOD_PREFIX(IsBorgQueen)( MODFN_CTV, int clientNum ) {
	return clientNum >= 0 && clientNum == MOD_STATE->borgQueenClientNum;
}

/*
================
(ModFN) GenerateGlobalSessionInfo

When level is exiting, save whether the borg team was red or blue, so we can try
to keep the same configuration when the map changes or restarts.
================
*/
static void MOD_PREFIX(GenerateGlobalSessionInfo)( MODFN_CTV, info_string_t *info ) {
	MODFN_NEXT( GenerateGlobalSessionInfo, ( MODFN_NC, info ) );

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
static void ModAssimilation_SetBorgColor( team_t team, const char *reason ) {
	MOD_STATE->borgTeam = team;
	G_DedPrintf( "assimilation: Selected %s color borg team due to %s.\n",
			MOD_STATE->borgTeam == TEAM_RED ? "red" : "blue", reason );

	if ( team == TEAM_RED ) {
		ModTeamGroups_Shared_ForceConfigStrings( "Borg", NULL );
	} else {
		ModTeamGroups_Shared_ForceConfigStrings( NULL, "Borg" );
	}
}

/*
================
ModAssimilation_DetermineBorgColor
================
*/
static void ModAssimilation_DetermineBorgColor( qboolean restarting ) {
	char buffer[256];

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
	trap_Cvar_VariableStringBuffer( "g_team_group_red", buffer, sizeof( buffer ) );
	if ( !Q_stricmpn( "Borg", buffer, 4 ) ) {
		ModAssimilation_SetBorgColor( TEAM_RED, "team group setting" );
		return;
	}
	trap_Cvar_VariableStringBuffer( "g_team_group_blue", buffer, sizeof( buffer ) );
	if ( !Q_stricmpn( "Borg", buffer, 4 ) ) {
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
(ModFN) JoinLimitMessage
================
*/
static void MOD_PREFIX(JoinLimitMessage)( MODFN_CTV, int clientNum, join_allowed_type_t type, team_t targetTeam ) {
	const assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];
	if ( modclient->assimilated ) {
		trap_SendServerCommand( clientNum, "cp \"You have been assimilated until next round\"" );
	} else {
		MODFN_NEXT( JoinLimitMessage, ( MODFN_NC, clientNum, type, targetTeam ) );
	}
}

/*
================
ModAssimilation_SetQueen

Called when picking a new queen.
================
*/
static void ModAssimilation_SetQueen( int clientNum ) {
	if ( clientNum == MOD_STATE->borgQueenClientNum ) {
		return;
	}

	if ( MOD_STATE->borgQueenClientNum != -1 ) {
		// Reset existing queen
		int oldQueen = MOD_STATE->borgQueenClientNum;
		MOD_STATE->borgQueenClientNum = -1;
		if ( level.clients[oldQueen].pers.connected == CON_CONNECTED ) {
			ClientSpawn( &g_entities[oldQueen], CST_RESPAWN );
		}
	}

	if ( clientNum != -1 && G_AssertConnectedClient( clientNum ) ) {
		// Set new queen
		MOD_STATE->borgQueenClientNum = clientNum;
		ClientSpawn( &g_entities[clientNum], CST_RESPAWN );
	}
}

/*
================
ModAssimilation_CheckReplaceQueen

Checks if current borg queen is valid, and picks new one if necessary.
================
*/
static void ModAssimilation_CheckReplaceQueen( void ) {
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

		// Look for human players.
		if ( MOD_STATE->g_preferNonBotQueen.integer ) {
			for ( i = 0; i < level.maxclients; i++ ) {
				gclient_t *client = &level.clients[i];
				if ( client->pers.connected == CON_CONNECTED && !modfn.SpectatorClient( i ) &&
						client->sess.sessionTeam == MOD_STATE->borgTeam && !( g_entities[i].r.svFlags & SVF_BOT ) ) {
					candidates[count++] = i;
				}
			}
		}

		// If no humans, or g_preferNonBotQueen disabled, search for any borg players.
		if ( !count ) {
			for ( i = 0; i < level.maxclients; i++ ) {
				gclient_t *client = &level.clients[i];
				if ( client->pers.connected == CON_CONNECTED && !modfn.SpectatorClient( i ) &&
						client->sess.sessionTeam == MOD_STATE->borgTeam ) {
					candidates[count++] = i;
				}
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
static void MOD_PREFIX(PostPlayerDie)( MODFN_CTV, gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
		int meansOfDeath, int *awardPoints ) {
	int clientNum = self - g_entities;
	gclient_t *client = &level.clients[clientNum];
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( PostPlayerDie, ( MODFN_NC, self, inflictor, attacker, meansOfDeath, awardPoints ) );

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
				ModJoinLimit_Static_StartMatchLock();
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
static void MOD_PREFIX(PrePlayerLeaveTeam)( MODFN_CTV, int clientNum, team_t oldTeam ) {
	MODFN_NEXT( PrePlayerLeaveTeam, ( MODFN_NC, clientNum, oldTeam ) );

	if ( clientNum == MOD_STATE->borgQueenClientNum ) {
		// If a fully qualified match was in progress when the borg queen quit,
		// award a win to the other team.
		if ( level.matchState == MS_ACTIVE && ModJoinLimit_Static_MatchLocked() ) {
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
(ModFN) PostClientConnect
================
*/
static void MOD_PREFIX(PostClientConnect)( MODFN_CTV, int clientNum, qboolean firstTime, qboolean isBot ) {
	if ( !MOD_STATE->borgTeam ) {
		// Determine borg team color.
		ModAssimilation_DetermineBorgColor( !firstTime );
	}
	MODFN_NEXT( PostClientConnect, ( MODFN_NC, clientNum, firstTime, isBot ) );
}

/*
================
(ModFN) InitClientSession
================
*/
static void MOD_PREFIX(InitClientSession)( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ) {
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( InitClientSession, ( MODFN_NC, clientNum, initialConnect, info ) );
	memset( modclient, 0, sizeof( *modclient ) );
}

/*
================
(ModFN) UpdateSessionClass

Set borg team members to borg class and vice versa.
================
*/
static void MOD_PREFIX(UpdateSessionClass)( MODFN_CTV, int clientNum ) {
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

		MODFN_NEXT( UpdateSessionClass, ( MODFN_NC, clientNum ) );
	}
}

/*
================
(ModFN) SpawnConfigureClient
================
*/
static void MOD_PREFIX(SpawnConfigureClient)( MODFN_CTV, int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( SpawnConfigureClient, ( MODFN_NC, clientNum ) );

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
}

/*
===========
(ModFN) SpawnCenterPrintMessage

Prints info messages to client during ClientSpawn.
============
*/
static void MOD_PREFIX(SpawnCenterPrintMessage)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
	gclient_t *client = &level.clients[clientNum];
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];

	if ( clientNum == MOD_STATE->borgQueenClientNum ) {
		trap_SendServerCommand( clientNum, "cp \"Assimilation:\nYou Are the Queen!\n\"" );
	}

	else if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( clientNum, "cp \"Assimilation\"" );
	}

	else if ( spawnType != CST_RESPAWN && !modclient->assimilated ) {
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
static void MOD_PREFIX(SpawnTransporterEffect)( MODFN_CTV, int clientNum, clientSpawnType_t spawnType ) {
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
		MODFN_NEXT( SpawnTransporterEffect, ( MODFN_NC, clientNum, spawnType ) );
	}
}

/*
================
(ModFN) RealSessionTeam

Returns player-selected team, even if active team is overriden by borg assimilation.
================
*/
static team_t MOD_PREFIX(RealSessionTeam)( MODFN_CTV, int clientNum ) {
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

	return MODFN_NEXT( RealSessionTeam, ( MODFN_NC, clientNum ) );
}

/*
================
(ModFN) RealSessionClass

Returns player-selected class, even if active class is overridden by borg team/assimilation.
================
*/
static pclass_t MOD_PREFIX(RealSessionClass)( MODFN_CTV, int clientNum ) {
	assimilation_client_t *modclient = &MOD_STATE->clients[clientNum];
	if ( modclient->oldClass != PC_NOCLASS ) {
		return modclient->oldClass;
	}

	return MODFN_NEXT( RealSessionClass, ( MODFN_NC, clientNum ) );
}

/*
================
(ModFN) CheckExitRules

Check for borg team win due to all players on other team being assimilated.
================
*/
static void MOD_PREFIX(CheckExitRules)( MODFN_CTV ) {
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
static void MOD_PREFIX(ClientRespawn)( MODFN_CTV, int clientNum ) {
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
static qboolean MOD_PREFIX(CheckRespawnTime)( MODFN_CTV, int clientNum, qboolean voluntary ) {
	gclient_t *client = &level.clients[clientNum];

	if ( !voluntary && level.time > client->respawnKilledTime + 3000 ) {
		return qtrue;
	}

	return MODFN_NEXT( CheckRespawnTime, ( MODFN_NC, clientNum, voluntary ) );
}

/*
================
(ModFN) CheckReplaceItem

Replace tetrion weapon with imod.
================
*/
static gitem_t *MOD_PREFIX(CheckReplaceItem)( MODFN_CTV, gitem_t *item ) {
	item = MODFN_NEXT( CheckReplaceItem, ( MODFN_NC, item ) );

	switch ( item->giTag ) {
		case WP_SCAVENGER_RIFLE:
		case WP_TETRION_DISRUPTOR:
			switch ( item->giType ) {
				case IT_WEAPON:
					return BG_FindItemForWeapon( WP_IMOD );
				case IT_AMMO:
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
static qboolean MOD_PREFIX(CanItemBeDropped)( MODFN_CTV, gitem_t *item, int clientNum ) {
	gclient_t *client = &level.clients[clientNum];

	// Allow dropping the flag, as per original implementation,
	// even though there shouldn't be flags in assimilation...
	if ( client->sess.sessionClass == PC_BORG && item->giType != IT_TEAM ) {
		return qfalse;
	}

	return MODFN_NEXT( CanItemBeDropped, ( MODFN_NC, item, clientNum ) );
}

/*
================
(ModFN) AddRegisteredItems
================
*/
static void MOD_PREFIX(AddRegisteredItems)( MODFN_CTV ) {
	MODFN_NEXT( AddRegisteredItems, ( MODFN_NC ) );

	RegisterItem( BG_FindItemForWeapon( WP_BORG_ASSIMILATOR ) );
	RegisterItem( BG_FindItemForWeapon( WP_BORG_WEAPON ) );	//queen
	RegisterItem( BG_FindItemForPowerup( PW_REGEN ) );	//queen
}

/*
=================
(ModFN) CheckSuicideAllowed
=================
*/
static qboolean MOD_PREFIX(CheckSuicideAllowed)( MODFN_CTV, int clientNum ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionClass != PC_BORG ) {
		// Disallow suicides by feds so they can't cheat their way out of dangerous situations.
		trap_SendServerCommand( clientNum, "print \"Cannot suicide in Assimilation mode.\n\"" );
		return qfalse;
	}

	return MODFN_NEXT( CheckSuicideAllowed, ( MODFN_NC, clientNum ) );
}

/*
================
(ModFN) ModClientCommand

Handle class command for borg (which can't change class directly).
================
*/
static qboolean MOD_PREFIX(ModClientCommand)( MODFN_CTV, int clientNum, const char *cmd ) {
	gclient_t *client = &level.clients[clientNum];

	if ( !Q_stricmp( cmd, "class" ) && !level.intermissiontime &&
			client->pers.connected == CON_CONNECTED && client->sess.sessionClass == PC_BORG ) {
		if ( trap_Argc() != 2 ) {
			trap_SendServerCommand( clientNum, "print \"class: Borg\n\"" );
		} else {
			trap_SendServerCommand( clientNum, "print \"Cannot manually change from class Borg\n\"" );
		}
		return qtrue;
	}

	return MODFN_NEXT( ModClientCommand, ( MODFN_NC, clientNum, cmd ) );
}

/*
==================
(ModFN) AdjustGeneralConstant
==================
*/
static int MOD_PREFIX(AdjustGeneralConstant)( MODFN_CTV, generalConstant_t gcType, int defaultValue ) {
	if ( gcType == GC_DISABLE_TEAM_FORCE_BALANCE )
		return 1;

	return MODFN_NEXT( AdjustGeneralConstant, ( MODFN_NC, gcType, defaultValue ) );
}

/*
================
(ModFN) PostRunFrame
================
*/
static void MOD_PREFIX(PostRunFrame)( MODFN_CTV ) {
	MODFN_NEXT( PostRunFrame, ( MODFN_NC ) );

	ModAssimilation_CheckReplaceQueen();

	// If server is empty reset the borg team, so the server isn't stuck on the same borg team indefinitely.
	// New borg team will be selected when first client connects via PostClientConnect.
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
static void MOD_PREFIX(MatchStateTransition)( MODFN_CTV, matchState_t oldState, matchState_t newState ) {
	MODFN_NEXT( MatchStateTransition, ( MODFN_NC, oldState, newState ) );

	if ( newState < MS_ACTIVE && !EF_WARN_ASSERT( MOD_STATE->numAssimilated == 0 ) ) {
		// Shouldn't happen - if players were assimilated we should hit CheckExitRules instead.
		MOD_STATE->numAssimilated = 0;
	}
}

/*
================
ModAssimilation_Init
================
*/
void ModAssimilation_Init( void ) {
	if ( EF_WARN_ASSERT( !MOD_STATE ) ) {
		modcfg.mods_enabled.assimilation = qtrue;
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MOD_STATE->borgQueenClientNum = -1;
		G_RegisterTrackedCvar( &MOD_STATE->g_preferNonBotQueen, "g_preferNonBotQueen", "1", 0, qfalse );

		// Support combining with other mods
		if ( G_ModUtils_GetLatchedValue( "g_pModSpecialties", "0", 0 ) ) {
			ModSpecialties_Init();
		}

		// Register mod functions
		MODFN_REGISTER( IsBorgQueen, ++modePriorityLevel );
		MODFN_REGISTER( GenerateGlobalSessionInfo, ++modePriorityLevel );
		MODFN_REGISTER( JoinLimitMessage, ++modePriorityLevel );
		MODFN_REGISTER( PostPlayerDie, ++modePriorityLevel );
		MODFN_REGISTER( PrePlayerLeaveTeam, ++modePriorityLevel );
		MODFN_REGISTER( PostClientConnect, ++modePriorityLevel );
		MODFN_REGISTER( InitClientSession, ++modePriorityLevel );
		MODFN_REGISTER( UpdateSessionClass, ++modePriorityLevel );
		MODFN_REGISTER( SpawnConfigureClient, ++modePriorityLevel );
		MODFN_REGISTER( SpawnCenterPrintMessage, ++modePriorityLevel );
		MODFN_REGISTER( SpawnTransporterEffect, ++modePriorityLevel );
		MODFN_REGISTER( RealSessionClass, ++modePriorityLevel );
		MODFN_REGISTER( RealSessionTeam, ++modePriorityLevel );
		MODFN_REGISTER( CheckExitRules, ++modePriorityLevel );
		MODFN_REGISTER( ClientRespawn, ++modePriorityLevel );
		MODFN_REGISTER( CheckRespawnTime, ++modePriorityLevel );
		MODFN_REGISTER( CheckReplaceItem, ++modePriorityLevel );
		MODFN_REGISTER( CanItemBeDropped, ++modePriorityLevel );
		MODFN_REGISTER( AddRegisteredItems, ++modePriorityLevel );
		MODFN_REGISTER( CheckSuicideAllowed, ++modePriorityLevel );
		MODFN_REGISTER( ModClientCommand, ++modePriorityLevel );
		MODFN_REGISTER( AdjustGeneralConstant, ++modePriorityLevel );
		MODFN_REGISTER( PostRunFrame, ++modePriorityLevel );
		MODFN_REGISTER( MatchStateTransition, ++modePriorityLevel );

		// Disable joining mid-match
		ModJoinLimit_Init();

		// Support ModTeamGroups_Shared_ForceConfigStrings function
		ModTeamGroups_Init();

		// Support modules
		ModAssimBorgAdapt_Init();
		ModAssimBorgTeleport_Init();
		ModAssimModels_Init();

		// Always set g_gametype 3
		trap_Cvar_Set( "g_gametype", "3" );
	}
}
