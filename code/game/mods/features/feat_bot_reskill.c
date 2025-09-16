/*
* Bot Reskill
* 
* Support commands to change the skill of bots midgame.
*/

#define MOD_NAME BotReskill

#include "mods/g_mod_local.h"

// from ai_main.h
#include "bots/botlib.h"		//bot lib interface
#include "bots/be_aas.h"
#include "bots/be_ea.h"
#include "bots/be_ai_char.h"
#include "bots/be_ai_chat.h"
#include "bots/be_ai_gen.h"
#include "bots/be_ai_goal.h"
#include "bots/be_ai_move.h"
#include "bots/be_ai_weap.h"
//
#include "bots/ai_main.h"
#include "bots/ai_dmq3.h"
#include "bots/ai_chat.h"
#include "bots/ai_cmd.h"
#include "bots/ai_dmnet.h"
//
#include "bots/chars.h"
#include "bots/inv.h"
#include "bots/syn.h"

extern bot_state_t *botstates[MAX_CLIENTS];

static struct {
	int _unused;
} *MOD_STATE;

/*
===================
ReskillBot

Set bot to specified skill.
===================
*/
static void ReskillBot( int clientNum, int skill ) {
	if ( G_IsConnectedClient( clientNum ) && ( g_entities[clientNum].r.svFlags & SVF_BOT )
			&& EF_WARN_ASSERT( botstates[clientNum] ) && botstates[clientNum]->settings.skill != skill ) {
		bot_state_t *bs = botstates[clientNum];
		char userinfo[MAX_INFO_VALUE];

		G_Printf( "Changing client %i bot skill from %i to %i\n", clientNum, bs->settings.skill, skill );
		bs->settings.skill = skill;
		trap_BotFreeCharacter( bs->character );
		trap_BotLoadCharacter( bs->settings.characterfile, skill );

		trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
		Info_SetValueForKey( userinfo, "skill", va( "%i", skill ) );
		if ( skill == 1 ) {
			Info_SetValueForKey( userinfo, "handicap", "50" );
		} else if ( skill == 2 ) {
			Info_SetValueForKey( userinfo, "handicap", "75" );
		} else {
			Info_SetValueForKey( userinfo, "handicap", "100" );
		}
		trap_SetUserinfo( clientNum, userinfo );
		ClientUserinfoChanged( clientNum );
	}
}

/*
===================
ReskillAllBots
===================
*/
static void ReskillAllBots( void ) {
	int i;
	int skill = trap_Cvar_VariableIntegerValue( "g_spSkill" );

	for ( i = 0; i < level.maxclients; ++i ) {
		ReskillBot( i, skill );
	}
}

/*
===================
(ModFN) ModConsoleCommand
===================
*/
static qboolean MOD_PREFIX(ModConsoleCommand)( MODFN_CTV, const char *cmd ) {
	if (Q_stricmp (cmd, "bot_reskill_all") == 0) {
		ReskillAllBots();
		return qtrue;
	}

	return MODFN_NEXT( ModConsoleCommand, ( MODFN_NC, cmd ) );
}

/*
================
ModBotReskill_Init
================
*/
void ModBotReskill_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( ModConsoleCommand, MODPRIORITY_GENERAL );
	}
}
