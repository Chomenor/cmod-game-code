// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=============================================================================

SINGLE PLAYER POSTGAME MENU

=============================================================================
*/

#include "ui_local.h"

#define ID_AGAIN		10
#define ID_NEXT			11
#define ID_MENU			12

typedef struct {
	menuframework_s	menu;
	menubitmap_s	item_again;
	menubitmap_s	item_next;
	menubitmap_s	item_menu;

	int				ignoreKeysTime;

	int				won;
} postgameMenuInfo_t;

static postgameMenuInfo_t	postgameMenuInfo;


/*
=================
UI_SPPostgameMenu_GetCurrentMap
=================
*/
static void UI_SPPostgameMenu_GetCurrentMap( char *buffer, int bufSize )
{
	char info[MAX_INFO_STRING];
	trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) );
	Q_strncpyz( buffer, Info_ValueForKey( info, "mapname" ), bufSize );
}


/*
=================
UI_SPPostgameMenu_AgainEvent
=================
*/
static void UI_SPPostgameMenu_AgainEvent( void* ptr, int event )
{
	if (event != QM_ACTIVATED) {
		return;
	}
	UI_PopMenu();
	trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
}


/*
=================
UI_SPPostgameMenu_NextEvent
=================
*/
static void UI_SPPostgameMenu_NextEvent( void* ptr, int event ) {
	int			level;
	char		arenaInfo[MAX_INFO_STRING];
	char		map[MAX_QPATH];

	if (event != QM_ACTIVATED) {
		return;
	}
	UI_PopMenu();

	UI_SPPostgameMenu_GetCurrentMap( map, sizeof( map ) );
	level = UI_GetSPArenaNumByMap( map ) + 1;
	if ( level < 0 || level >= UI_GetNumSPArenas() ) {
		level = 0;
	}

	UI_GetSPArenaInfo( level, arenaInfo, sizeof( arenaInfo ) );
	if ( !*arenaInfo ) {
		return;
	}

	UI_SPArena_Start( arenaInfo );
}


/*
=================
UI_SPPostgameMenu_MenuEvent
=================
*/
static void UI_SPPostgameMenu_MenuEvent( void* ptr, int event )
{
	if (event != QM_ACTIVATED) {
		return;
	}
	UI_PopMenu();
	trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect; levelselect\n" );
}


/*
=================
UI_SPPostgameMenu_MenuKey
=================
*/
static sfxHandle_t UI_SPPostgameMenu_MenuKey( int key ) {
	if ( uis.realtime < postgameMenuInfo.ignoreKeysTime ) {
		return 0;
	}

	postgameMenuInfo.ignoreKeysTime	= uis.realtime + 250;

	// NO ESCAPE FOR YOU!!!
	if( key == K_ESCAPE || key == K_MOUSE2 ) {
		return 0;
	}

	return Menu_DefaultKey( &postgameMenuInfo.menu, key );
}


/*
=================
UI_SPPostgameMenu_Cache
=================
*/
void UI_SPPostgameMenu_Cache( void ) {
	qboolean	buildscript;
	buildscript = trap_Cvar_VariableValue("com_buildscript");
	if( buildscript ) {	//cache these for the pack file!
		trap_Cmd_ExecuteText( EXEC_APPEND, "music music/win\n" );
		trap_Cmd_ExecuteText( EXEC_APPEND, "music music/loss\n" );
	}
}


/*
=================
UI_SPPostgameMenu_Init
=================
Sets up the exact look of the menu buttons at the bottom
*/
static void UI_SPPostgameMenu_Init( void ) {

	postgameMenuInfo.menu.wrapAround	= qtrue;
	postgameMenuInfo.menu.key			= UI_SPPostgameMenu_MenuKey;
//	postgameMenuInfo.menu.draw			= UI_SPPostgameMenu_MenuDraw;
	postgameMenuInfo.ignoreKeysTime		= uis.realtime + 1500;

	UI_SPPostgameMenu_Cache();

	postgameMenuInfo.item_menu.generic.type			= MTYPE_BITMAP;
	postgameMenuInfo.item_menu.generic.name			= BUTTON_GRAPHIC_LONGRIGHT;
	postgameMenuInfo.item_menu.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	postgameMenuInfo.item_menu.generic.x			= 33;
	postgameMenuInfo.item_menu.generic.y			= 446;
	postgameMenuInfo.item_menu.generic.callback		= UI_SPPostgameMenu_MenuEvent;
	postgameMenuInfo.item_menu.generic.id			= ID_MENU;
	postgameMenuInfo.item_menu.width				= MENU_BUTTON_MED_WIDTH;
	postgameMenuInfo.item_menu.height				= MENU_BUTTON_MED_HEIGHT;
	postgameMenuInfo.item_menu.color				= CT_DKPURPLE1;
	postgameMenuInfo.item_menu.color2				= CT_LTPURPLE1;
	postgameMenuInfo.item_menu.textX				= 5;
	postgameMenuInfo.item_menu.textY				= 1;
	postgameMenuInfo.item_menu.textEnum				= MBT_RETURNMENU;
	postgameMenuInfo.item_menu.textcolor			= CT_BLACK;
	postgameMenuInfo.item_menu.textcolor2			= CT_WHITE;

	postgameMenuInfo.item_again.generic.type		= MTYPE_BITMAP;
	postgameMenuInfo.item_again.generic.name		= BUTTON_GRAPHIC_LONGRIGHT;
	postgameMenuInfo.item_again.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	postgameMenuInfo.item_again.generic.x			= 256;
	postgameMenuInfo.item_again.generic.y			= 446;
	postgameMenuInfo.item_again.generic.callback	= UI_SPPostgameMenu_AgainEvent;
	postgameMenuInfo.item_again.generic.id			= ID_AGAIN;
	postgameMenuInfo.item_again.width				= MENU_BUTTON_MED_WIDTH;
	postgameMenuInfo.item_again.height				= MENU_BUTTON_MED_HEIGHT;
	postgameMenuInfo.item_again.color				= CT_DKPURPLE1;
	postgameMenuInfo.item_again.color2				= CT_LTPURPLE1;
	postgameMenuInfo.item_again.textX				= 5;
	postgameMenuInfo.item_again.textY				= 1;
	postgameMenuInfo.item_again.textEnum			= MBT_REPLAY;
	postgameMenuInfo.item_again.textcolor			= CT_BLACK;
	postgameMenuInfo.item_again.textcolor2			= CT_WHITE;

	postgameMenuInfo.item_next.generic.type			= MTYPE_BITMAP;
	postgameMenuInfo.item_next.generic.name			= BUTTON_GRAPHIC_LONGRIGHT;
	postgameMenuInfo.item_next.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	postgameMenuInfo.item_next.generic.x			= 477;
	postgameMenuInfo.item_next.generic.y			= 446;
	postgameMenuInfo.item_next.generic.callback		= UI_SPPostgameMenu_NextEvent;
	postgameMenuInfo.item_next.generic.id			= ID_NEXT;
	postgameMenuInfo.item_next.width				= MENU_BUTTON_MED_WIDTH;
	postgameMenuInfo.item_next.height				= MENU_BUTTON_MED_HEIGHT;
	postgameMenuInfo.item_next.color				= CT_DKPURPLE1;
	postgameMenuInfo.item_next.color2				= CT_LTPURPLE1;
	postgameMenuInfo.item_next.textX				= 5;
	postgameMenuInfo.item_next.textY				= 1;
	postgameMenuInfo.item_next.textEnum				= MBT_NEXTMATCH;
	postgameMenuInfo.item_next.textcolor			= CT_BLACK;
	postgameMenuInfo.item_next.textcolor2			= CT_WHITE;


	Menu_AddItem( &postgameMenuInfo.menu, ( void * )&postgameMenuInfo.item_menu );
	Menu_AddItem( &postgameMenuInfo.menu, ( void * )&postgameMenuInfo.item_next );
	Menu_AddItem( &postgameMenuInfo.menu, ( void * )&postgameMenuInfo.item_again );
}

/*
=================
UI_SPPostgameMenu_f
=================
*/
void UI_SPPostgameMenu_f( void ) {
	int			playerGameRank;

	Mouse_Show();

	memset( &postgameMenuInfo, 0, sizeof(postgameMenuInfo) );

	postgameMenuInfo.menu.nobackground = qtrue;

	playerGameRank = atoi( UI_Argv(1));

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;

	UI_SPPostgameMenu_Init();
	UI_PushMenu( &postgameMenuInfo.menu );

	if (playerGameRank == 0)
	{
		char map[MAX_QPATH];
		postgameMenuInfo.won = 1;
		Menu_SetCursorToItem( &postgameMenuInfo.menu, &postgameMenuInfo.item_next );

		// update completion record
		UI_SPPostgameMenu_GetCurrentMap( map, sizeof( map ) );
		UI_WriteMapCompletionSkill( map, trap_Cvar_VariableValue( "g_spSkill" ) );
	}
	else {
		Menu_SetCursorToItem( &postgameMenuInfo.menu, &postgameMenuInfo.item_menu );
	}

//	trap_Cmd_ExecuteText( EXEC_APPEND, "music music/win\n" );	//?? always win?  should this be deleted and playing cg_scoreboard now?
}
