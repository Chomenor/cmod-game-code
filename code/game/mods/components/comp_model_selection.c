/*
* Model Selection
* 
* This module is used to solve an issue that affected random player models selected
* in Assimilation mode and by the team groups feature. Originally when random models
* were generated, a new one would be selected on every call to ClientUserinfoChanged,
* which could lead to the model randomly changing in the middle of the game.
* 
* This module implements a wrapper for the GetPlayerModel mod function that splits
* the implementation into two functions - ConvertPlayerModel and RandomPlayerModel.
* 
* ConvertPlayerModel is called to verify a player selected model, and can override
* it if the mod wants to force a specific model. RandomPlayerModel is called only if
* ConvertPlayerModel returns an empty string, and the selected random model will remain
* the same as long as it is needed.
*/

#include "mods/g_mod_local.h"

#define PREFIX( x ) ModModelSelection_##x
#define MOD_STATE PREFIX( state )

typedef struct {
	char validModel[64];	// last valid player model in <model>/<skin> format
} modClient_t;

static struct {
	modClient_t clients[MAX_CLIENTS];

	// For mod function stacking
	ModFNType_InitClientSession Prev_InitClientSession;
	ModFNType_GenerateClientSessionInfo Prev_GenerateClientSessionInfo;
} *MOD_STATE;

ModModelSelection_shared_t *ModModelSelection_shared;

/*
===========
ModModelSelection_DefaultConvertPlayerModel
============
*/
LOGFUNCTION_SVOID( ModModelSelection_DefaultConvertPlayerModel,
		( int clientNum, const char *userinfo, const char *source_model, char *output, unsigned int outputSize ),
		( clientNum, userinfo, source_model, output, outputSize ), "G_PLAYERMODELS" ) {
	Q_strncpyz( output, source_model, outputSize );
}

/*
===========
ModModelSelection_DefaultRandomPlayerModel
============
*/
LOGFUNCTION_SVOID( ModModelSelection_DefaultRandomPlayerModel,
		( int clientNum, const char *userinfo, char *output, unsigned int outputSize ),
		( clientNum, userinfo, output, outputSize ), "G_PLAYERMODELS" ) {
}

/*
===========
(ModFN) GetPlayerModel

Retrieves player model string for client, performing any mod conversions as needed.
============
*/
LOGFUNCTION_SVOID( PREFIX(GetPlayerModel),
		( int clientNum, const char *userinfo, char *output, unsigned int outputSize ),
		( clientNum, userinfo, output, outputSize ), "G_MODFN_GETPLAYERMODEL G_PLAYERMODELS" ) {
	gclient_t *client = &level.clients[clientNum];
	modClient_t *modClient = &MOD_STATE->clients[clientNum];
	char random_model[64];
	char userinfo_model[64];
	Q_strncpyz( userinfo_model, Info_ValueForKey( userinfo, "model" ), sizeof( userinfo_model ) );

	// Try to get model using current userinfo
	*output = '\0';
	ModModelSelection_shared->ConvertPlayerModel( clientNum, userinfo, userinfo_model, output, outputSize );
	if ( *output ) {
		Q_strncpyz( modClient->validModel, userinfo_model, sizeof( modClient->validModel ) );
		return;
	}

	// Try to get model from previous valid model
	*output = '\0';
	if ( *modClient->validModel ) {
		ModModelSelection_shared->ConvertPlayerModel( clientNum, userinfo, modClient->validModel, output, outputSize );
	}
	if ( *output ) {
		return;
	}

	// Try to get random model
	*random_model = '\0';
	ModModelSelection_shared->RandomPlayerModel( clientNum, userinfo, random_model, sizeof( random_model ) );
	*output = '\0';
	if ( *random_model ) {
		ModModelSelection_shared->ConvertPlayerModel( clientNum, userinfo, random_model, output, outputSize );
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
LOGFUNCTION_SVOID( PREFIX(InitClientSession), ( int clientNum, qboolean initialConnect, const info_string_t *info ),
		( clientNum, initialConnect, info ), "G_MODFN_INITCLIENTSESSION" ) {
	modClient_t *modClient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_InitClientSession( clientNum, initialConnect, info );

	Q_strncpyz( modClient->validModel, Info_ValueForKey( info->s, "validModel" ), sizeof( modClient->validModel ) );
}

/*
================
(ModFN) GenerateClientSessionInfo
================
*/
LOGFUNCTION_SVOID( PREFIX(GenerateClientSessionInfo), ( int clientNum, info_string_t *info ),
		( clientNum, info ), "G_MODFN_GENERATECLIENTSESSIONINFO" ) {
	modClient_t *modClient = &MOD_STATE->clients[clientNum];

	MOD_STATE->Prev_GenerateClientSessionInfo( clientNum, info );

	if ( *modClient->validModel ) {
		Info_SetValueForKey_Big( info->s, "validModel", modClient->validModel );
	}
}

/*
================
ModModelSelection_Init
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

LOGFUNCTION_VOID( ModModelSelection_Init, ( void ), (), "G_MOD_INIT" ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );
		ModModelSelection_shared = G_Alloc( sizeof( *ModModelSelection_shared ) );

		ModModelSelection_shared->ConvertPlayerModel = ModModelSelection_DefaultConvertPlayerModel;
		ModModelSelection_shared->RandomPlayerModel = ModModelSelection_DefaultRandomPlayerModel;

		INIT_FN_BASE( GetPlayerModel );
		INIT_FN_STACKABLE( InitClientSession );
		INIT_FN_STACKABLE( GenerateClientSessionInfo );
	}
}
