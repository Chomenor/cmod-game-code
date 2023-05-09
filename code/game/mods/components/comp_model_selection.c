/*
* Model Selection
* 
* This module is used to solve an issue that affected random player models selected
* in Assimilation mode and by the team groups feature. Originally when random models
* were generated, a new one would be selected on every call to ClientUserinfoChanged,
* which could lead to the model randomly changing in the middle of the game.
* 
* This module implements a wrapper for the GetPlayerModel mod function that splits
* the implementation into two mod functions - ConvertPlayerModel and RandomPlayerModel.
* 
* ConvertPlayerModel is called to verify a player selected model, and can override
* it if the mod wants to force a specific model. RandomPlayerModel is called only if
* ConvertPlayerModel returns an empty string, and the selected random model will remain
* the same as long as it is needed.
*/

#define MOD_NAME ModModelSelection

#include "mods/g_mod_local.h"

typedef struct {
	char validModel[64];	// last valid player model in <model>/<skin> format
} modClient_t;

static struct {
	modClient_t clients[MAX_CLIENTS];
} *MOD_STATE;

/*
===========
(ModFN) ConvertPlayerModel
============
*/
void ModFNDefault_ConvertPlayerModel( int clientNum, const char *userinfo, const char *source_model,
		char *output, unsigned int outputSize ) {
	Q_strncpyz( output, source_model, outputSize );
}

/*
===========
(ModFN) RandomPlayerModel
============
*/
void ModFNDefault_RandomPlayerModel( int clientNum, const char *userinfo, char *output, unsigned int outputSize ) {
}

/*
===========
(ModFN) GetPlayerModel

Retrieves player model string for client, performing any mod conversions as needed.
============
*/
static void MOD_PREFIX(GetPlayerModel)( MODFN_CTV, int clientNum, const char *userinfo,
		char *output, unsigned int outputSize ) {
	gclient_t *client = &level.clients[clientNum];
	modClient_t *modClient = &MOD_STATE->clients[clientNum];
	char random_model[64];
	char userinfo_model[64];
	Q_strncpyz( userinfo_model, Info_ValueForKey( userinfo, "model" ), sizeof( userinfo_model ) );

	// Try to get model using current userinfo
	*output = '\0';
	modfn_lcl.ConvertPlayerModel( clientNum, userinfo, userinfo_model, output, outputSize );
	if ( *output ) {
		Q_strncpyz( modClient->validModel, userinfo_model, sizeof( modClient->validModel ) );
		return;
	}

	// Try to get model from previous valid model
	*output = '\0';
	if ( *modClient->validModel ) {
		modfn_lcl.ConvertPlayerModel( clientNum, userinfo, modClient->validModel, output, outputSize );
	}
	if ( *output ) {
		return;
	}

	// Try to get random model
	*random_model = '\0';
	modfn_lcl.RandomPlayerModel( clientNum, userinfo, random_model, sizeof( random_model ) );
	*output = '\0';
	if ( *random_model ) {
		modfn_lcl.ConvertPlayerModel( clientNum, userinfo, random_model, output, outputSize );
	}
	if ( *output ) {
		Q_strncpyz( modClient->validModel, random_model, sizeof( modClient->validModel ) );
		return;
	}

	// If model still isn't valid, fall back to userinfo / default
	// This can be caused by invalid group cvar value or server file layout issue
	G_DedPrintf( "WARNING: Failed to get valid model for client %i\n", clientNum );
	if ( *userinfo_model ) {
		Q_strncpyz( output, userinfo_model, outputSize );
	} else {
		Q_strncpyz( output, "munro/red", outputSize );
	}
}

/*
================
(ModFN) InitClientSession
================
*/
static void MOD_PREFIX(InitClientSession)( MODFN_CTV, int clientNum, qboolean initialConnect, const info_string_t *info ) {
	modClient_t *modClient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( InitClientSession, ( MODFN_NC, clientNum, initialConnect, info ) );

	Q_strncpyz( modClient->validModel, Info_ValueForKey( info->s, "validModel" ), sizeof( modClient->validModel ) );
}

/*
================
(ModFN) GenerateClientSessionInfo
================
*/
static void MOD_PREFIX(GenerateClientSessionInfo)( MODFN_CTV, int clientNum, info_string_t *info ) {
	modClient_t *modClient = &MOD_STATE->clients[clientNum];

	MODFN_NEXT( GenerateClientSessionInfo, ( MODFN_NC, clientNum, info ) );

	if ( *modClient->validModel ) {
		Info_SetValueForKey_Big( info->s, "validModel", modClient->validModel );
	}
}

/*
================
ModModelSelection_Init
================
*/
void ModModelSelection_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( GetPlayerModel, MODPRIORITY_GENERAL );
		MODFN_REGISTER( InitClientSession, MODPRIORITY_GENERAL );
		MODFN_REGISTER( GenerateClientSessionInfo, MODPRIORITY_GENERAL );
	}
}
