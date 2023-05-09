/*
* Team Groups
* 
* This module implements the "g_team_group_red" and "g_team_group_blue" cvars, which can be used
* to specify a race for each team. When a race is set for a given team, player models on that
* team must belong to that race, or a random one will be selected instead.
*/

#define MOD_NAME ModTeamGroups

#include "mods/g_mod_local.h"

#define GROUP_NAME_LENGTH 256

static struct {
	// Copied from g_team_group_red and g_team_group_blue cvars
	char redGroup[GROUP_NAME_LENGTH];
	char blueGroup[GROUP_NAME_LENGTH];

	// Override configstrings (used by Assimilation to set borg team)
	qboolean csInitialized;
	char forceRedGroup[GROUP_NAME_LENGTH];
	char forceBlueGroup[GROUP_NAME_LENGTH];
} *MOD_STATE;

/*
============
ModTeamGroups_SetConfigStrings
============
*/
static void ModTeamGroups_SetConfigStrings( void ) {
	trap_SetConfigstring( CS_RED_GROUP, *MOD_STATE->forceRedGroup ? MOD_STATE->forceRedGroup : MOD_STATE->redGroup );
	trap_SetConfigstring( CS_BLUE_GROUP, *MOD_STATE->forceBlueGroup ? MOD_STATE->forceBlueGroup : MOD_STATE->blueGroup );
	MOD_STATE->csInitialized = qtrue;
}

/*
============
ModTeamGroups_Shared_ForceConfigStrings

Force a certain value for red and/or blue team group configstrings. Values can be NULL
to use default group. Used by Assimilation mode when setting borg team.
============
*/
void ModTeamGroups_Shared_ForceConfigStrings( const char *redGroup, const char *blueGroup ) {
	EF_ERR_ASSERT( MOD_STATE );
	Q_strncpyz( MOD_STATE->forceRedGroup, redGroup ? redGroup : "", sizeof( MOD_STATE->forceRedGroup ) );
	Q_strncpyz( MOD_STATE->forceBlueGroup, blueGroup ? blueGroup : "", sizeof( MOD_STATE->forceBlueGroup ) );

	// Don't set yet if initial SetConfigStrings call is still pending, to avoid unnecessary configstring broadcasts.
	if ( MOD_STATE->csInitialized ) {
		ModTeamGroups_SetConfigStrings();
	}
}

/*
============
(ModFN) ConvertPlayerModel

Check if current model matches team race. Returns empty string if source model is invalid
and a random one should be selected instead.
============
*/
static void MOD_PREFIX(ConvertPlayerModel)( MODFN_CTV, int clientNum, const char *userinfo, const char *source_model,
		char *output, unsigned int outputSize ) {
	gclient_t *client = &level.clients[clientNum];

	if ( client->sess.sessionTeam == TEAM_RED || client->sess.sessionTeam == TEAM_BLUE ) {
		const char *groupName = ( client->sess.sessionTeam == TEAM_RED ) ? MOD_STATE->redGroup : MOD_STATE->blueGroup;
		if ( *groupName ) {
			// A race is specified for this team. Make sure this model fits it.
			const char *model_races = ModModelGroups_Shared_SearchGroupList( source_model );
			if ( !ModModelGroups_Shared_ListContainsRace( model_races, groupName ) ) {
				*output = '\0';
				return;
			}
		}
	}

	MODFN_NEXT( ConvertPlayerModel, ( MODFN_NC, clientNum, userinfo, source_model, output, outputSize ) );
}

/*
============
(ModFN) RandomPlayerModel

Selects a random model that matches team race requirement. Returns empty string on error.
============
*/
static void MOD_PREFIX(RandomPlayerModel)( MODFN_CTV, int clientNum, const char *userinfo,
		char *output, unsigned int outputSize ) {
	gclient_t *client = &level.clients[clientNum];
	*output = '\0';

	if ( client->sess.sessionTeam == TEAM_RED || client->sess.sessionTeam == TEAM_BLUE ) {
		const char *groupName = ( client->sess.sessionTeam == TEAM_RED ) ? MOD_STATE->redGroup : MOD_STATE->blueGroup;
		if ( *groupName ) {
			// A race is specified for this team. Select random model matching this race.
			ModModelGroups_Shared_RandomModelForRace( groupName, output, outputSize );
			if ( *output ) {
				return;
			}
		}
	}

	MODFN_NEXT( RandomPlayerModel, ( MODFN_NC, clientNum, userinfo, output, outputSize ) );
}

/*
================
(ModFN) PostModInit

Delay full initialization until g_gametype is determined, because other mods might have modified it.
================
*/
static void MOD_PREFIX(PostModInit)( MODFN_CTV ) {
	MODFN_NEXT( PostModInit, ( MODFN_NC ) );

	if ( G_ModUtils_GetLatchedValue( "g_gametype", "0", 0 ) >= GT_TEAM ) {
		// Grab group names from cvars
		char redGroup[GROUP_NAME_LENGTH];
		char blueGroup[GROUP_NAME_LENGTH];
		trap_Cvar_Register( NULL, "g_team_group_red", "", CVAR_LATCH );
		trap_Cvar_Register( NULL, "g_team_group_blue", "", CVAR_LATCH );
		trap_Cvar_VariableStringBuffer( "g_team_group_red", redGroup, sizeof( redGroup ) );
		trap_Cvar_VariableStringBuffer( "g_team_group_blue", blueGroup, sizeof( redGroup ) );

		// See if we actually need to do the full init
		if ( *redGroup || *blueGroup ) {
			Q_strncpyz( MOD_STATE->redGroup, redGroup, sizeof( MOD_STATE->redGroup ) );
			Q_strncpyz( MOD_STATE->blueGroup, blueGroup, sizeof( MOD_STATE->blueGroup ) );
			ModModelGroups_Init();
		}
	}

	ModTeamGroups_SetConfigStrings();
}

/*
================
ModTeamGroups_Init
================
*/
void ModTeamGroups_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( ConvertPlayerModel, MODPRIORITY_GENERAL );
		MODFN_REGISTER( RandomPlayerModel, MODPRIORITY_GENERAL );
		MODFN_REGISTER( PostModInit, MODPRIORITY_GENERAL );

		ModModelSelection_Init();
	}
}
