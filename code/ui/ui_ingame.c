// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

INGAME MENU

=======================================================================
*/


#include "ui_local.h"

void UI_SetupWeaponsMenu( void );

int		ingameFlag = qfalse;	// true when in game menu is in use

#define INGAME_MENU_VERTICAL_SPACING	28

#define ID_TEAM					10
#define ID_ADDBOTS				11
#define ID_REMOVEBOTS			12
#define ID_SETUP				13
#define ID_SERVERINFO			14
#define ID_LEAVEARENA			15
#define ID_RESTART				16
#define ID_QUIT					17
#define ID_RESUME				18
#define ID_TEAMORDERS			19
#define ID_SCREENSHOT			20
#define ID_INGAMEMENU			125
#define ID_INGAME_QUIT_YES		131
#define ID_INGAME_QUIT_NO		132

typedef struct 
{
	menuframework_s	menu;

	menubitmap_s	team;
	menubitmap_s	setup;
	menubitmap_s	server;
	menubitmap_s	leave;
	menubitmap_s	restart;
	menubitmap_s	addbots;
	menubitmap_s	removebots;
	menubitmap_s	teamorders;
	menubitmap_s	screenshot;
	menubitmap_s	resume;
} ingamemenu_t;

static ingamemenu_t	s_ingame;

static int ingame_buttons[10][2] =
{
{152,220},
{152,220 + INGAME_MENU_VERTICAL_SPACING},
{152,220 + (INGAME_MENU_VERTICAL_SPACING *2)},
{152,220 + (INGAME_MENU_VERTICAL_SPACING *3)},
{152,220 + (INGAME_MENU_VERTICAL_SPACING *4)},

{368,220},
{368,220 + INGAME_MENU_VERTICAL_SPACING},
{368,220 + (INGAME_MENU_VERTICAL_SPACING *2)},
{368,220 + (INGAME_MENU_VERTICAL_SPACING *3)},
{368,220 + (INGAME_MENU_VERTICAL_SPACING *4)},
};


typedef struct 
{
	menuframework_s	menu;

	menubitmap_s	ingamemenu;
	menubitmap_s	no;
	menubitmap_s	yes;
} ingamequitmenu_t;

static ingamequitmenu_t	s_ingamequit;

/*
=================
InGame_RestartAction
=================
*/
static void InGame_RestartAction( qboolean result ) 
{
	if( !result ) 
	{
		return;
	}

	UI_PopMenu();
	trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
}


/*
=================
InGame_LeaveAction
=================
*/
static void InGame_LeaveAction( qboolean result ) 
{
	if( !result ) 
	{
		return;
	}

	UI_PopMenu();
	trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
}


/*
=================
InGame_Event
=================
*/
void InGame_Event( void *ptr, int notification ) 
{
	if( notification != QM_ACTIVATED ) 
	{
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) 
	{
	case ID_TEAM:
		UI_TeamMainMenu();
		break;

	case ID_SETUP:
		UI_SetupWeaponsMenu();
		break;

	case ID_SCREENSHOT:
		UI_ForceMenuOff();
		trap_Cmd_ExecuteText( EXEC_APPEND, "wait; wait; wait; wait; screenshot\n" );
		break;

	case ID_LEAVEARENA:
		UI_ConfirmMenu( menu_normal_text[MNT_LEAVE_MATCH], 0, InGame_LeaveAction );
//		trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
		break;

	case ID_RESTART:
		UI_ConfirmMenu( menu_normal_text[MNT_RESTART_MATCH], 0, InGame_RestartAction );
		break;

	case ID_QUIT:
		UI_QuitMenu();
//		UI_ConfirmMenu( "EXIT GAME?", NULL, InGame_QuitAction );
		break;

	case ID_SERVERINFO:
		UI_ServerInfoMenu();
		break;

	case ID_ADDBOTS:
		UI_AddBotsMenu();
		break;

	case ID_REMOVEBOTS:
		UI_RemoveBotsMenu();
		break;

	case ID_TEAMORDERS:
		UI_TeamOrdersMenu(0);
		break;

	case ID_RESUME:
		UI_PopMenu();
		break;
	}
}

/*
=================
UI_InGameMenu_Draw
=================
*/
static void UI_InGameMenu_Draw( void ) 
{

	UI_MenuFrame(&s_ingame.menu);

	// Rounded button that takes place of INGAME MENU button
	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic( 482, 136,  MENU_BUTTON_MED_WIDTH - 22, MENU_BUTTON_MED_HEIGHT, uis.whiteShader);
	UI_DrawHandlePic( 460 + MENU_BUTTON_MED_WIDTH - 6, 136,  -19,  MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);	//right

	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic(30,203,  47, 186, uis.whiteShader);	// Long left column square on bottom 3rd

	// Left rounded ends for buttons
	trap_R_SetColor( colorTable[CT_DKPURPLE1]);

	UI_DrawHandlePic(s_ingame.team.generic.x - 14, s_ingame.team.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	UI_DrawHandlePic(s_ingame.addbots.generic.x - 14, s_ingame.addbots.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	UI_DrawHandlePic(s_ingame.removebots.generic.x - 14, s_ingame.removebots.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	UI_DrawHandlePic(s_ingame.teamorders.generic.x - 14, s_ingame.teamorders.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	UI_DrawHandlePic(s_ingame.setup.generic.x - 14, s_ingame.setup.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	UI_DrawHandlePic(s_ingame.server.generic.x - 14, s_ingame.server.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	UI_DrawHandlePic(s_ingame.restart.generic.x - 14, s_ingame.restart.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT + 6, uis.graphicButtonLeftEnd);

	UI_DrawHandlePic(s_ingame.resume.generic.x - 14, s_ingame.resume.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT + 6, uis.graphicButtonLeftEnd);

	UI_DrawHandlePic(s_ingame.leave.generic.x - 14, s_ingame.leave.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT + 6, uis.graphicButtonLeftEnd);

	UI_DrawProportionalString(  74,  66, "15567",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  84, "2439",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  188, "3814",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  206, "4800",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  395, "5671-1",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	UI_DrawProportionalString( 584, 142, "1219",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	// standard menu drawing
	Menu_Draw( &s_ingame.menu );

}

/*
=================
InGame_MenuInit
=================
*/
void InGame_MenuInit( void ) 
{
	int		y,x;
	uiClientState_t	cs;
	char	info[MAX_INFO_STRING];
	int		team;

	memset( &s_ingame, 0 ,sizeof(ingamemenu_t) );

	InGame_Cache();

	s_ingame.menu.wrapAround			= qtrue;
	s_ingame.menu.fullscreen			= qtrue;
	s_ingame.menu.descX					= MENU_DESC_X;
	s_ingame.menu.descY					= MENU_DESC_Y;
	s_ingame.menu.draw					= UI_InGameMenu_Draw;
	s_ingame.menu.titleX				= MENU_TITLE_X;
	s_ingame.menu.titleY				= MENU_TITLE_Y;
	s_ingame.menu.titleI				= MNT_INGAMEMAIN_TITLE;
	s_ingame.menu.footNoteEnum			= MNT_INGAME_MENU;

	x = 305;
	y = 196;
	s_ingame.team.generic.type			= MTYPE_BITMAP;
	s_ingame.team.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingame.team.generic.x				= x;
	s_ingame.team.generic.y				= y;
	s_ingame.team.generic.id			= ID_TEAM;
	s_ingame.team.generic.name			= BUTTON_GRAPHIC_LONGRIGHT;
	s_ingame.team.generic.callback		= InGame_Event; 
	s_ingame.team.width					= MENU_BUTTON_MED_WIDTH;
	s_ingame.team.height				= MENU_BUTTON_MED_HEIGHT;
	s_ingame.team.color					= CT_DKPURPLE1;
	s_ingame.team.color2				= CT_LTPURPLE1;
	s_ingame.team.textX					= MENU_BUTTON_TEXT_X;
	s_ingame.team.textY					= MENU_BUTTON_TEXT_Y;
	s_ingame.team.textEnum				= MBT_TEAMCLASS;
	s_ingame.team.textcolor				= CT_BLACK;
	s_ingame.team.textcolor2			= CT_WHITE;


	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.addbots.generic.type		= MTYPE_BITMAP;
	s_ingame.addbots.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingame.addbots.generic.x			= x;
	s_ingame.addbots.generic.y			= y;
	s_ingame.addbots.generic.id			= ID_ADDBOTS;
	s_ingame.addbots.generic.name		= BUTTON_GRAPHIC_LONGRIGHT;
	s_ingame.addbots.generic.callback	= InGame_Event; 
	s_ingame.addbots.width				= MENU_BUTTON_MED_WIDTH;
	s_ingame.addbots.height				= MENU_BUTTON_MED_HEIGHT;
	s_ingame.addbots.color				= CT_DKPURPLE1;
	s_ingame.addbots.color2				= CT_LTPURPLE1;
	s_ingame.addbots.textX				= MENU_BUTTON_TEXT_X;
	s_ingame.addbots.textY				= MENU_BUTTON_TEXT_Y;
	s_ingame.addbots.textEnum			= MBT_INGAMEADDSIMULANTS;
	s_ingame.addbots.textcolor			= CT_BLACK;
	s_ingame.addbots.textcolor2			= CT_WHITE;
	if( !trap_Cvar_VariableValue( "sv_running" ) || !trap_Cvar_VariableValue( "bot_enable" ) || (trap_Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER)) 
	{
		s_ingame.addbots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.removebots.generic.type		= MTYPE_BITMAP;
	s_ingame.removebots.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingame.removebots.generic.x			= x;
	s_ingame.removebots.generic.y			= y;
	s_ingame.removebots.generic.id			= ID_REMOVEBOTS;
	s_ingame.removebots.generic.name		= BUTTON_GRAPHIC_LONGRIGHT;
	s_ingame.removebots.generic.callback	= InGame_Event; 
	s_ingame.removebots.width				= MENU_BUTTON_MED_WIDTH;
	s_ingame.removebots.height				= MENU_BUTTON_MED_HEIGHT;
	s_ingame.removebots.color				= CT_DKPURPLE1;
	s_ingame.removebots.color2				= CT_LTPURPLE1;
	s_ingame.removebots.textX				= MENU_BUTTON_TEXT_X;
	s_ingame.removebots.textY				= MENU_BUTTON_TEXT_Y;
	s_ingame.removebots.textEnum			= MBT_INGAMEREMOVESIMULANTS;
	s_ingame.removebots.textcolor			= CT_BLACK;
	s_ingame.removebots.textcolor2			= CT_WHITE;
	if( !trap_Cvar_VariableValue( "sv_running" ) || !trap_Cvar_VariableValue( "bot_enable" ) || (trap_Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER)) {
		s_ingame.removebots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.teamorders.generic.type		= MTYPE_BITMAP;
	s_ingame.teamorders.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingame.teamorders.generic.x			= x;
	s_ingame.teamorders.generic.y			= y;
	s_ingame.teamorders.generic.id			= ID_TEAMORDERS;
	s_ingame.teamorders.generic.name		= BUTTON_GRAPHIC_LONGRIGHT;
	s_ingame.teamorders.generic.callback	= InGame_Event; 
	s_ingame.teamorders.width				= MENU_BUTTON_MED_WIDTH;
	s_ingame.teamorders.height				= MENU_BUTTON_MED_HEIGHT;
	s_ingame.teamorders.color				= CT_DKPURPLE1;
	s_ingame.teamorders.color2				= CT_LTPURPLE1;
	s_ingame.teamorders.textX				= MENU_BUTTON_TEXT_X;
	s_ingame.teamorders.textY				= MENU_BUTTON_TEXT_Y;
	s_ingame.teamorders.textEnum			= MBT_INGAMETEAMORDERS;
	s_ingame.teamorders.textcolor			= CT_BLACK;
	s_ingame.teamorders.textcolor2			= CT_WHITE;

	// make sure it's a team game
	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	if( (atoi( Info_ValueForKey( info, "g_gametype" ) )) < GT_TEAM)
	{
		s_ingame.teamorders.generic.flags |= QMF_GRAYED;
	}
	else 
	{
		trap_GetClientState( &cs );
		trap_GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
		team = atoi( Info_ValueForKey( info, "t" ) );
		if( team == TEAM_SPECTATOR ) 
		{
			s_ingame.teamorders.generic.flags |= QMF_GRAYED;
		}
	}
	x = 152;
	y = 68;
	s_ingame.setup.generic.type			= MTYPE_BITMAP;
	s_ingame.setup.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingame.setup.generic.x			= x;
	s_ingame.setup.generic.y			= y;
	s_ingame.setup.generic.id			= ID_SETUP;
	s_ingame.setup.generic.name			= BUTTON_GRAPHIC_LONGRIGHT;
	s_ingame.setup.generic.callback		= InGame_Event; 
	s_ingame.setup.width				= MENU_BUTTON_MED_WIDTH;
	s_ingame.setup.height				= MENU_BUTTON_MED_HEIGHT;
	s_ingame.setup.color				= CT_DKPURPLE1;
	s_ingame.setup.color2				= CT_LTPURPLE1;
	s_ingame.setup.textX				= MENU_BUTTON_TEXT_X;
	s_ingame.setup.textY				= MENU_BUTTON_TEXT_Y;
	s_ingame.setup.textEnum				= MBT_INGAMESETUP;
	s_ingame.setup.textcolor			= CT_BLACK;
	s_ingame.setup.textcolor2			= CT_WHITE;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.server.generic.type		= MTYPE_BITMAP;
	s_ingame.server.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingame.server.generic.x			= x;
	s_ingame.server.generic.y			= y;
	s_ingame.server.generic.id			= ID_SERVERINFO;
	s_ingame.server.generic.name		= BUTTON_GRAPHIC_LONGRIGHT;
	s_ingame.server.generic.callback	= InGame_Event; 
	s_ingame.server.width				= MENU_BUTTON_MED_WIDTH;
	s_ingame.server.height				= MENU_BUTTON_MED_HEIGHT;
	s_ingame.server.color				= CT_DKPURPLE1;
	s_ingame.server.color2				= CT_LTPURPLE1;
	s_ingame.server.textX				= MENU_BUTTON_TEXT_X;
	s_ingame.server.textY				= MENU_BUTTON_TEXT_Y;
	s_ingame.server.textEnum			= MBT_INGAMESERVERDATA;
	s_ingame.server.textcolor			= CT_BLACK;
	s_ingame.server.textcolor2			= CT_WHITE;

	y = 365;

	s_ingame.leave.generic.type				= MTYPE_BITMAP;
	s_ingame.leave.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingame.leave.generic.x				= 126;
	s_ingame.leave.generic.y				= y;
	s_ingame.leave.generic.id				= ID_LEAVEARENA;
	s_ingame.leave.generic.name				= BUTTON_GRAPHIC_LONGRIGHT;
	s_ingame.leave.generic.callback			= InGame_Event; 
	s_ingame.leave.width					= MENU_BUTTON_MED_WIDTH;
	s_ingame.leave.height					= MENU_BUTTON_MED_HEIGHT + 6;
	s_ingame.leave.color					= CT_DKPURPLE1;
	s_ingame.leave.color2					= CT_LTPURPLE1;
	s_ingame.leave.textX					= MENU_BUTTON_TEXT_X;
	s_ingame.leave.textY					= MENU_BUTTON_TEXT_Y;
	s_ingame.leave.textEnum					= MBT_INGAMELEAVE;
	s_ingame.leave.textcolor				= CT_BLACK;
	s_ingame.leave.textcolor2				= CT_WHITE;

	s_ingame.restart.generic.type		= MTYPE_BITMAP;
	s_ingame.restart.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingame.restart.generic.x			= 305;
	s_ingame.restart.generic.y			= y;
	s_ingame.restart.generic.id			= ID_RESTART;
	s_ingame.restart.generic.name		= BUTTON_GRAPHIC_LONGRIGHT;
	s_ingame.restart.generic.callback	= InGame_Event; 
	s_ingame.restart.width				= MENU_BUTTON_MED_WIDTH;
	s_ingame.restart.height				= MENU_BUTTON_MED_HEIGHT + 6;
	s_ingame.restart.color				= CT_DKPURPLE1;
	s_ingame.restart.color2				= CT_LTPURPLE1;
	s_ingame.restart.textX				= MENU_BUTTON_TEXT_X;
	s_ingame.restart.textY				= MENU_BUTTON_TEXT_Y;
	s_ingame.restart.textEnum			= MBT_INGAMERESTART;
	s_ingame.restart.textcolor			= CT_BLACK;
	s_ingame.restart.textcolor2			= CT_WHITE;
	if( !trap_Cvar_VariableValue( "sv_running" ) ) 
	{
		s_ingame.restart.generic.flags |= QMF_GRAYED;
	}

	s_ingame.resume.generic.type			= MTYPE_BITMAP;
	s_ingame.resume.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingame.resume.generic.x				= 480;
	s_ingame.resume.generic.y				= y;
	s_ingame.resume.generic.id				= ID_RESUME;
	s_ingame.resume.generic.name			= BUTTON_GRAPHIC_LONGRIGHT;
	s_ingame.resume.generic.callback		= InGame_Event; 
	s_ingame.resume.width					= MENU_BUTTON_MED_WIDTH;
	s_ingame.resume.height					= MENU_BUTTON_MED_HEIGHT + 6;
	s_ingame.resume.color					= CT_DKPURPLE1;
	s_ingame.resume.color2					= CT_LTPURPLE1;
	s_ingame.resume.textX					= MENU_BUTTON_TEXT_X;
	s_ingame.resume.textY					= MENU_BUTTON_TEXT_Y;
	s_ingame.resume.textEnum				= MBT_INGAMERESUME;
	s_ingame.resume.textcolor				= CT_BLACK;
	s_ingame.resume.textcolor2				= CT_WHITE;

	s_ingame.screenshot.generic.type				= MTYPE_BITMAP;      
	s_ingame.screenshot.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingame.screenshot.generic.x					= 477;
	s_ingame.screenshot.generic.y					= 82;
	s_ingame.screenshot.generic.name				= GRAPHIC_SQUARE;
	s_ingame.screenshot.generic.id					= ID_SCREENSHOT;
	s_ingame.screenshot.generic.callback			= InGame_Event; 
	s_ingame.screenshot.width						= MENU_BUTTON_MED_WIDTH;
	s_ingame.screenshot.height						= 36;
	s_ingame.screenshot.color						= CT_DKPURPLE1;
	s_ingame.screenshot.color2						= CT_LTPURPLE1;
	s_ingame.screenshot.textX						= MENU_BUTTON_TEXT_X;
	s_ingame.screenshot.textY						= MENU_BUTTON_TEXT_Y;
	s_ingame.screenshot.textEnum					= MBT_SCREENSHOT;
	s_ingame.screenshot.textcolor					= CT_BLACK;
	s_ingame.screenshot.textcolor2					= CT_WHITE;


	Menu_AddItem( &s_ingame.menu, &s_ingame.team );
	Menu_AddItem( &s_ingame.menu, &s_ingame.addbots );
	Menu_AddItem( &s_ingame.menu, &s_ingame.removebots );
	Menu_AddItem( &s_ingame.menu, &s_ingame.teamorders );
	Menu_AddItem( &s_ingame.menu, &s_ingame.setup );
	Menu_AddItem( &s_ingame.menu, &s_ingame.server );
	Menu_AddItem( &s_ingame.menu, &s_ingame.leave );
	Menu_AddItem( &s_ingame.menu, &s_ingame.restart );
	Menu_AddItem( &s_ingame.menu, &s_ingame.resume );
//	Menu_AddItem( &s_ingame.menu, &s_ingame.quit );
	Menu_AddItem( &s_ingame.menu, &s_ingame.screenshot );
}

/*
=================
InGameQuit_Cache
=================
*/
void InGame_Cache( void ) 
{

}

/*
=================
UI_InGameMenu
=================
*/
void UI_InGameMenu( void ) 
{
	// force as top level menu
	uis.menusp = 0;  

	// set menu cursor to a nice location
	uis.cursorx = 319;
	uis.cursory = 80;

	ingameFlag = qtrue;	// true when ingame menu is in use
	Mouse_Show();

	InGame_MenuInit();
	UI_PushMenu( &s_ingame.menu );
}



/*
=================
InGameQuitMenu_Event
=================
*/
void InGameQuitMenu_Event( void *ptr, int notification ) 
{
	if( notification != QM_ACTIVATED ) 
	{
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) 
	{
	case ID_INGAME_QUIT_NO:
	case ID_INGAMEMENU:
		UI_PopMenu();
		return;
	case ID_INGAME_QUIT_YES:
		UI_PopMenu();
		UI_QuitMenu();
		return;
	}
}

/*
=================
UI_InGameQuitMenu_Draw
=================
*/
static void UI_InGameQuitMenu_Draw( void ) 
{
	UI_MenuFrame(&s_ingamequit.menu);

	// Rounded button that takes place of INGAME MENU button
	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic( 482, 136,  MENU_BUTTON_MED_WIDTH - 22, MENU_BUTTON_MED_HEIGHT, uis.whiteShader);
	UI_DrawHandlePic( 460 + MENU_BUTTON_MED_WIDTH -6 , 136,  -19,  MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);	//right

	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic(30,203,  47, 186, uis.whiteShader);	// Long left column square on bottom 3rd

	// standard menu drawing
	Menu_Draw( &s_ingamequit.menu );

}

/*
=================
InGameQuitMenu_Init
=================
*/
void InGameQuitMenu_Init( void ) 
{

	memset( &s_ingame, 0 ,sizeof(ingamemenu_t) );

	InGame_Cache();

	s_ingamequit.menu.wrapAround				= qtrue;
	s_ingamequit.menu.fullscreen				= qtrue;
	s_ingamequit.menu.descX						= MENU_DESC_X;
	s_ingamequit.menu.descY						= MENU_DESC_Y;
	s_ingamequit.menu.draw						= UI_InGameQuitMenu_Draw;
	s_ingamequit.menu.titleX					= MENU_TITLE_X;
	s_ingamequit.menu.titleY					= MENU_TITLE_Y;
	s_ingamequit.menu.titleI					= MNT_INGAMEMAIN_TITLE;
	s_ingamequit.menu.footNoteEnum				= MNT_INGAME_MENU;

	s_ingamequit.ingamemenu.generic.type		= MTYPE_BITMAP;
	s_ingamequit.ingamemenu.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingamequit.ingamemenu.generic.x			= 482;
	s_ingamequit.ingamemenu.generic.y			= 136;
	s_ingamequit.ingamemenu.generic.id			= ID_INGAMEMENU;
	s_ingamequit.ingamemenu.generic.name		= GRAPHIC_SQUARE;
	s_ingamequit.ingamemenu.generic.callback	= InGameQuitMenu_Event; 
	s_ingamequit.ingamemenu.width				= MENU_BUTTON_MED_WIDTH;
	s_ingamequit.ingamemenu.height				= MENU_BUTTON_MED_HEIGHT;
	s_ingamequit.ingamemenu.color				= CT_DKPURPLE1;
	s_ingamequit.ingamemenu.color2				= CT_LTPURPLE1;
	s_ingamequit.ingamemenu.textX				= MENU_BUTTON_TEXT_X;
	s_ingamequit.ingamemenu.textY				= MENU_BUTTON_TEXT_Y;
	s_ingamequit.ingamemenu.textEnum			= MBT_INGAMEMENU;
	s_ingamequit.ingamemenu.textcolor			= CT_BLACK;
	s_ingamequit.ingamemenu.textcolor2			= CT_WHITE;

	s_ingamequit.no.generic.type				= MTYPE_BITMAP;
	s_ingamequit.no.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingamequit.no.generic.x					= 100;
	s_ingamequit.no.generic.y					= 180;
	s_ingamequit.no.generic.id					= ID_INGAME_QUIT_NO;
	s_ingamequit.no.generic.name				= GRAPHIC_SQUARE;
	s_ingamequit.no.generic.callback			= InGameQuitMenu_Event; 
	s_ingamequit.no.width						= MENU_BUTTON_MED_WIDTH;
	s_ingamequit.no.height						= MENU_BUTTON_MED_HEIGHT;
	s_ingamequit.no.color						= CT_DKPURPLE1;
	s_ingamequit.no.color2						= CT_LTPURPLE1;
	s_ingamequit.no.textX						= MENU_BUTTON_TEXT_X;
	s_ingamequit.no.textY						= MENU_BUTTON_TEXT_Y;
	s_ingamequit.no.textEnum					= MBT_QUIT_NO;
	s_ingamequit.no.textcolor					= CT_BLACK;
	s_ingamequit.no.textcolor2					= CT_WHITE;	

	s_ingamequit.yes.generic.type				= MTYPE_BITMAP;
	s_ingamequit.yes.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_ingamequit.yes.generic.x					= 100;
	s_ingamequit.yes.generic.y					= 180;
	s_ingamequit.yes.generic.id					= ID_INGAME_QUIT_YES;
	s_ingamequit.yes.generic.name				= GRAPHIC_SQUARE;
	s_ingamequit.yes.generic.callback			= InGameQuitMenu_Event; 
	s_ingamequit.yes.width						= MENU_BUTTON_MED_WIDTH;
	s_ingamequit.yes.height						= MENU_BUTTON_MED_HEIGHT;
	s_ingamequit.yes.color						= CT_DKPURPLE1;
	s_ingamequit.yes.color2						= CT_LTPURPLE1;
	s_ingamequit.yes.textX						= MENU_BUTTON_TEXT_X;
	s_ingamequit.yes.textY						= MENU_BUTTON_TEXT_Y;
	s_ingamequit.yes.textEnum					= MBT_QUIT_YES;
	s_ingamequit.yes.textcolor					= CT_BLACK;
	s_ingamequit.yes.textcolor2					= CT_WHITE;

	Menu_AddItem( &s_ingamequit.menu, &s_ingamequit.ingamemenu );
	Menu_AddItem( &s_ingamequit.menu, &s_ingamequit.no );
	Menu_AddItem( &s_ingamequit.menu, &s_ingamequit.yes );
}

/*
=================
UI_InGameQuitMenu
=================
*/
void UI_InGameQuitMenu( void ) 
{
	InGameQuitMenu_Init();

	UI_PushMenu( &s_ingamequit.menu );
}
