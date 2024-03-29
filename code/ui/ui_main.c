// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

#ifndef MODULE_UI
#error Building ui module with no MODULE_UI preprocessor definition. Check the build configuration.
#endif

#include "ui_local.h"


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
#ifdef _MSC_VER
__declspec(dllexport)
#endif
intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 ) {
	switch ( command ) {
	case UI_GETAPIVERSION:
		return UI_API_VERSION;

	case UI_INIT:
		UI_Init();
		return 0;

	case UI_SHUTDOWN:
		UI_Shutdown();
		return 0;

	case UI_KEY_EVENT:
		UI_KeyEvent( arg0 );
		return 0;

	case UI_MOUSE_EVENT:
		UI_MouseEvent( arg0, arg1 );
		return 0;

	case UI_REFRESH:
		UI_Refresh( arg0 );
		return 0;

	case UI_IS_FULLSCREEN:
		return UI_IsFullscreen();

	case UI_SET_ACTIVE_MENU:
		UI_SetActiveMenu( arg0 );
		return 0;

	case UI_CONSOLE_COMMAND:
		return UI_ConsoleCommand();

	case UI_DRAW_CONNECT_SCREEN:
		UI_DrawConnectScreen( arg0 );
		return 0;
	}

	return -1;
}


/*
================
cvars
================
*/

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

vmCvar_t	ui_ffa_fraglimit;
vmCvar_t	ui_ffa_timelimit;

vmCvar_t	ui_tourney_fraglimit;
vmCvar_t	ui_tourney_timelimit;

vmCvar_t	ui_team_fraglimit;
vmCvar_t	ui_team_timelimit;
vmCvar_t	ui_team_friendly;

vmCvar_t	ui_ctf_capturelimit;
vmCvar_t	ui_ctf_timelimit;
vmCvar_t	ui_ctf_friendly;

vmCvar_t	ui_arenasFile;
vmCvar_t	ui_botsFile;
vmCvar_t	ui_botminplayers;
vmCvar_t	ui_spSkill;

vmCvar_t	ui_weaponrespawn;
vmCvar_t	ui_speed;
vmCvar_t	ui_gravity;
vmCvar_t	ui_knockback;
vmCvar_t	ui_dmgmult;
vmCvar_t	ui_adaptRespawn;
vmCvar_t	ui_holoIntro;
vmCvar_t	ui_forcerespawn;
vmCvar_t	ui_respawnGhostTime;
vmCvar_t	ui_warmup;
vmCvar_t	ui_dowarmup;
vmCvar_t	ui_team_race_blue;
vmCvar_t	ui_team_race_red;

vmCvar_t	ui_pModAssimilation;
vmCvar_t	ui_pModDisintegration;
vmCvar_t	ui_pModActionHero;
vmCvar_t	ui_pModSpecialties;
vmCvar_t	ui_pModElimination;

vmCvar_t	ui_spSelection;

vmCvar_t	ui_browserMaster;
vmCvar_t	ui_browserIpProtocol;
vmCvar_t	ui_browserGameType;
vmCvar_t	ui_browserSortKey;
vmCvar_t	ui_browserSortKey2;
vmCvar_t	ui_browserShowEmpty;
vmCvar_t	ui_browserPlayerType;

vmCvar_t	ui_drawCrosshair;
vmCvar_t	ui_drawCrosshairNames;
vmCvar_t	ui_marks;
vmCvar_t	ui_fov;

vmCvar_t	ui_server1;
vmCvar_t	ui_server2;
vmCvar_t	ui_server3;
vmCvar_t	ui_server4;
vmCvar_t	ui_server5;
vmCvar_t	ui_server6;
vmCvar_t	ui_server7;
vmCvar_t	ui_server8;
vmCvar_t	ui_server9;
vmCvar_t	ui_server10;
vmCvar_t	ui_server11;
vmCvar_t	ui_server12;
vmCvar_t	ui_server13;
vmCvar_t	ui_server14;
vmCvar_t	ui_server15;
vmCvar_t	ui_server16;

vmCvar_t	ui_cdkeychecked;
vmCvar_t	ui_cdkeychecked2;
vmCvar_t	ui_language;
vmCvar_t	ui_s_language;
vmCvar_t	ui_k_language;
vmCvar_t	ui_playerclass;
vmCvar_t	g_nojointimeout;
vmCvar_t	ui_precacheweapons;

cvarTable_t		cvarTable[] = {
	{ &ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE },
	{ &ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE },

	{ &ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE },

	{ &ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE },
	{ &ui_team_friendly, "ui_team_friendly",  "1", CVAR_ARCHIVE },

	{ &ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE },
	{ &ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE },
	{ &ui_ctf_friendly, "ui_ctf_friendly",  "0", CVAR_ARCHIVE },

	{ &ui_arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM },
	{ &ui_botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM },				// Used to have CVAR_ARCHIVE.
	{ &ui_botminplayers, "bot_minplayers","0", CVAR_SERVERINFO },
	{ &ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE | CVAR_LATCH },

	{ &ui_weaponrespawn, "g_weaponrespawn", "5", CVAR_ARCHIVE},
	{ &ui_speed, "g_speed", "250", CVAR_SERVERINFO | CVAR_ARCHIVE},
	{ &ui_gravity, "g_gravity", "800", CVAR_SERVERINFO | CVAR_ARCHIVE},
	{ &ui_knockback, "g_knockback", "500", CVAR_ARCHIVE},
	{ &ui_dmgmult, "g_dmgmult", "1", CVAR_ARCHIVE},
	{ &ui_adaptRespawn, "g_adaptrespawn", "1", CVAR_ARCHIVE},
	{ &ui_holoIntro, "g_holoIntro", "1", CVAR_ARCHIVE},
	{ &ui_forcerespawn, "g_forcerespawn", "0", CVAR_ARCHIVE },
	{ &ui_respawnGhostTime, "g_ghostRespawn", "5", CVAR_ARCHIVE },
	{ &ui_dowarmup, "g_dowarmup", "0", CVAR_ARCHIVE },
	{ &ui_warmup, "g_warmup", "20", CVAR_ARCHIVE },
	{ &ui_team_race_blue, "g_team_group_blue", "", CVAR_LATCH},			// Used to have CVAR_ARCHIVE
	{ &ui_team_race_red, "g_team_group_red", "", CVAR_LATCH},			// Used to have CVAR_ARCHIVE

	{ &ui_pModAssimilation, "g_pModAssimilation", "0", CVAR_SERVERINFO | CVAR_LATCH },
	{ &ui_pModDisintegration, "g_pModDisintegration", "0", CVAR_SERVERINFO | CVAR_LATCH },
	{ &ui_pModActionHero, "g_pModActionHero", "0", CVAR_SERVERINFO | CVAR_LATCH },
	{ &ui_pModSpecialties, "g_pModSpecialties", "0", CVAR_SERVERINFO | CVAR_LATCH },
	{ &ui_pModElimination, "g_pModElimination", "0", CVAR_SERVERINFO | CVAR_LATCH },

	{ &ui_spSelection, "ui_spSelection", "", CVAR_ROM },

	{ &ui_browserMaster, "ui_browserMaster", "0", CVAR_ARCHIVE },
	{ &ui_browserIpProtocol, "ui_browserIpProtocol", "0", CVAR_ARCHIVE },
	{ &ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE },
	{ &ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE },
	{ &ui_browserSortKey2, "ui_browserSortKey2", "2", CVAR_ARCHIVE },
	{ &ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE },
	{ &ui_browserPlayerType, "ui_browserPlayerType", "1", CVAR_ARCHIVE },

	{ &ui_drawCrosshair, "cg_drawCrosshair", "1", CVAR_ARCHIVE },
	{ &ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &ui_marks, "cg_marks", "1", CVAR_ARCHIVE },
	{ &ui_fov, "cg_fov", "80", CVAR_ARCHIVE },

	{ &ui_server1, "server1", "", CVAR_ARCHIVE },
	{ &ui_server2, "server2", "", CVAR_ARCHIVE },
	{ &ui_server3, "server3", "", CVAR_ARCHIVE },
	{ &ui_server4, "server4", "", CVAR_ARCHIVE },
	{ &ui_server5, "server5", "", CVAR_ARCHIVE },
	{ &ui_server6, "server6", "", CVAR_ARCHIVE },
	{ &ui_server7, "server7", "", CVAR_ARCHIVE },
	{ &ui_server8, "server8", "", CVAR_ARCHIVE },
	{ &ui_server9, "server9", "", CVAR_ARCHIVE },
	{ &ui_server10, "server10", "", CVAR_ARCHIVE },
	{ &ui_server11, "server11", "", CVAR_ARCHIVE },
	{ &ui_server12, "server12", "", CVAR_ARCHIVE },
	{ &ui_server13, "server13", "", CVAR_ARCHIVE },
	{ &ui_server14, "server14", "", CVAR_ARCHIVE },
	{ &ui_server15, "server15", "", CVAR_ARCHIVE },
	{ &ui_server16, "server16", "", CVAR_ARCHIVE },

	{ &ui_cdkeychecked, "ui_cdkeychecked", "0", CVAR_ARCHIVE | CVAR_NORESTART},
	{ &ui_cdkeychecked2, "ui_cdkeychecked2", "0", CVAR_ROM},
	{ &ui_language, "g_language", "", CVAR_ARCHIVE | CVAR_NORESTART},
	{ &ui_s_language, "s_language", "", CVAR_ARCHIVE | CVAR_NORESTART},
	{ &ui_k_language, "k_language", "", CVAR_ARCHIVE | CVAR_NORESTART},
	{ &ui_playerclass, "ui_playerclass", "", CVAR_ARCHIVE},
	{ &g_nojointimeout, "g_nojointimeout", "120", CVAR_ARCHIVE},
	{ &ui_precacheweapons, "ui_precacheweapons", "0", CVAR_ARCHIVE},
};

int		cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);


/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}
}

/*
=================
UI_NoCompat
=================
*/
qboolean UI_NoCompat(void)
{
	return ((int) trap_Cvar_VariableValue("com_novmcompat")) ? qtrue : qfalse;
}
