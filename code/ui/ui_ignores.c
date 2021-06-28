// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

REMOVE BOTS MENU

=======================================================================
*/


#include "ui_local.h"

#define ID_UP				11
#define ID_DOWN				12
#define ID_PLUP				13
#define ID_PLDOWN			14
#define ID_ADD				15
#define ID_DELETE			16
#define ID_FLUSH			17
#define ID_BACK				18
#define ID_IGNORENAME			20
#define ID_PLAYERNAME			40

#define IGNORES_VIEWABLE		11
#define IGNORES_VIEWABLE_MAXLENGTH	28
#define PLAYERS_VIEWABLE		IGNORES_VIEWABLE
#define PLAYERS_VIEWABLE_MAXLENGTH	IGNORES_VIEWABLE_MAXLENGTH

#define BUTTONS_Y_POS			250

typedef struct
{
	menuframework_s	menu;

	menubitmap_s	up;
	menubitmap_s	down;

	menubitmap_s	plup;
	menubitmap_s	pldown;

	menutext_s		ignores[IGNORES_VIEWABLE];
	menutext_s		players[PLAYERS_VIEWABLE];

	menubitmap_s	add;
	menubitmap_s	delete;
	menubitmap_s	flush;
	menubitmap_s	back;

	int				numIgnores;
	int				baseIgnoreNum;
	int				selectedIgnoreNum;

	int				numPlayers;
	int				basePlayerNum;
	int				selectedPlayerNum;

	char			ignorenames[IGNORES_VIEWABLE][MAX_NAME_LENGTH];
	char			ignorenames_viewable[IGNORES_VIEWABLE][IGNORES_VIEWABLE_MAXLENGTH];
	char			playernames[PLAYERS_VIEWABLE][MAX_NAME_LENGTH];
	char			playernames_viewable[PLAYERS_VIEWABLE][PLAYERS_VIEWABLE_MAXLENGTH];
} ignoresMenuInfo_t;

static ignoresMenuInfo_t	ignoresMenuInfo;


/*
=================
UI_IgnoresMenu_SetIgnoreNames
=================
*/
static void UI_IgnoresMenu_SetIgnoreNames(void)
{
	char	ignores[MAX_IGNORE_LENGTH];
	char	curignore[MAX_NAME_LENGTH] = "";
	int index, ignindex, cignindex, entrynum = 0;
	qboolean entrystarted = qfalse;

	trap_Cvar_VariableStringBuffer(IGNORE_CVARNAME, ignores, sizeof(ignores));

	for(cignindex = ignindex = index = 0; ignores[index]; cignindex++, index++)
	{
		if(!ignores[index] || ignores[index] == IGNORE_SEP)
		{
			curignore[cignindex] = '\0';

			if(entrystarted)
			{
				entrynum++;
				entrystarted = qfalse;

				if(entrynum > ignoresMenuInfo.baseIgnoreNum && ignindex < IGNORES_VIEWABLE)
				{
					Q_strncpyz(ignoresMenuInfo.ignorenames[ignindex],
							curignore, sizeof(ignoresMenuInfo.ignorenames[0]));
					Q_strncpyz(ignoresMenuInfo.ignorenames_viewable[ignindex],
							curignore, sizeof(ignoresMenuInfo.ignorenames_viewable[0]));
					ignindex++;
				}
			}
			else
				continue;
		}
		else
		{
			if(!entrystarted)
			{
				entrystarted = qtrue;
				cignindex = 0;
			}

			curignore[cignindex] = ignores[index];
		}
	}

	ignoresMenuInfo.numIgnores = entrynum;

	for(; ignindex < IGNORES_VIEWABLE; ignindex++)
	{
		ignoresMenuInfo.ignorenames[ignindex][0] = '\0';
		ignoresMenuInfo.ignorenames_viewable[ignindex][0] = '\0';
	}
}

/*
=================
UI_IgnoresMenu_SetPlayerNames
=================
*/
static void UI_IgnoresMenu_SetPlayerNames(void)
{
	char info[MAX_INFO_STRING], *curplayer;
	int index, numPlayers, viewindex = 0;

	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));
	numPlayers = atoi(Info_ValueForKey(info, "sv_maxclients"));

	ignoresMenuInfo.numPlayers = 0;

	for(index = 0; index < numPlayers; index++)
	{
		trap_GetConfigString(CS_PLAYERS + index, info, sizeof(info));

		if(*(curplayer = Info_ValueForKey(info, "n")))
		{
			// Player is valid.
			ignoresMenuInfo.numPlayers++;
			if(ignoresMenuInfo.numPlayers > ignoresMenuInfo.basePlayerNum && viewindex < PLAYERS_VIEWABLE)
			{
				Q_strncpyz(ignoresMenuInfo.playernames[viewindex],
						curplayer, sizeof(ignoresMenuInfo.playernames[0]));
				Q_strncpyz(ignoresMenuInfo.playernames_viewable[viewindex],
						curplayer, sizeof(ignoresMenuInfo.playernames_viewable[0]));
				viewindex++;
			}
		}
	}

	for(; viewindex < PLAYERS_VIEWABLE; viewindex++)
	{
		ignoresMenuInfo.playernames[viewindex][0] = '\0';
		ignoresMenuInfo.playernames_viewable[viewindex][0] = '\0';
	}
}

/*
=================
UI_IgnoresMenu_AddEvent
=================
*/
static void UI_IgnoresMenu_AddEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
		return;

	if(!*ignoresMenuInfo.playernames[ignoresMenuInfo.selectedPlayerNum])
		return;

	trap_Cmd_ExecuteText(EXEC_NOW, va("ignore \"%s\"\n", ignoresMenuInfo.playernames[ignoresMenuInfo.selectedPlayerNum]));

	// Redraw menu with updated info.
	UI_IgnoresMenu_SetIgnoreNames();
	UI_IgnoresMenu_SetPlayerNames();
}

/*
=================
UI_IgnoresMenu_DeleteEvent
=================
*/
static void UI_IgnoresMenu_DeleteEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
		return;

	if(!*ignoresMenuInfo.ignorenames[ignoresMenuInfo.selectedIgnoreNum])
		return;

	trap_Cmd_ExecuteText(EXEC_NOW, va("unignore \"%s\"\n", ignoresMenuInfo.ignorenames[ignoresMenuInfo.selectedIgnoreNum]));

	// Redraw menu with updated info.
	UI_IgnoresMenu_SetIgnoreNames();
	UI_IgnoresMenu_SetPlayerNames();
}

/*
=================
UI_IgnoresMenu_DeleteEvent
=================
*/
static void UI_IgnoresMenu_FlushEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
		return;

	trap_Cmd_ExecuteText(EXEC_NOW, "unignoreall\n");

	// Redraw menu with updated info.
	UI_IgnoresMenu_SetIgnoreNames();
	UI_IgnoresMenu_SetPlayerNames();
}

/*
=================
UI_IgnoresMenu_IgnoreEvent
=================
*/
static void UI_IgnoresMenu_IgnoreEvent(void* ptr, int event)
{
	int newid;

	if (event != QM_ACTIVATED)
		return;

	newid = ((menucommon_s*)ptr)->id - ID_IGNORENAME;
	if(!*ignoresMenuInfo.ignorenames[newid])
		return;

	ignoresMenuInfo.ignores[ignoresMenuInfo.selectedIgnoreNum].color = colorTable[CT_DKGOLD1];
	ignoresMenuInfo.selectedIgnoreNum = newid;
	ignoresMenuInfo.ignores[ignoresMenuInfo.selectedIgnoreNum].color = colorTable[CT_YELLOW];
}

/*
=================
UI_IgnoresMenu_PlayerEvent
=================
*/
static void UI_IgnoresMenu_PlayerEvent(void* ptr, int event)
{
	int newid;

	if (event != QM_ACTIVATED)
		return;

	newid = ((menucommon_s*)ptr)->id - ID_PLAYERNAME;
	if(!*ignoresMenuInfo.playernames[newid])
		return;

	ignoresMenuInfo.players[ignoresMenuInfo.selectedPlayerNum].color = colorTable[CT_DKGOLD1];
	ignoresMenuInfo.selectedPlayerNum = newid;
	ignoresMenuInfo.players[ignoresMenuInfo.selectedPlayerNum].color = colorTable[CT_YELLOW];
}

/*
=================
UI_IgnoresMenu_BackEvent
=================
*/
static void UI_IgnoresMenu_BackEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
		return;

	UI_PopMenu();
}


/*
=================
UI_IgnoresMenu_UpEvent
=================
*/
static void UI_IgnoresMenu_UpEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
		return;

	if(ignoresMenuInfo.baseIgnoreNum > 0)
	{
		ignoresMenuInfo.baseIgnoreNum--;
		UI_IgnoresMenu_SetIgnoreNames();
	}
}


/*
=================
UI_IgnoresMenu_DownEvent
=================
*/
static void UI_IgnoresMenu_DownEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
		return;

	if(ignoresMenuInfo.baseIgnoreNum + IGNORES_VIEWABLE < ignoresMenuInfo.numIgnores)
	{
		ignoresMenuInfo.baseIgnoreNum++;
		UI_IgnoresMenu_SetIgnoreNames();
	}
}

/*
=================
UI_IgnoresMenu_PlUpEvent
=================
*/
static void UI_IgnoresMenu_PlUpEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
		return;

	if(ignoresMenuInfo.basePlayerNum > 0)
	{
		ignoresMenuInfo.basePlayerNum--;
		UI_IgnoresMenu_SetPlayerNames();
	}
}


/*
=================
UI_IgnoresMenu_PlDownEvent
=================
*/
static void UI_IgnoresMenu_PlDownEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
		return;

	if(ignoresMenuInfo.basePlayerNum + IGNORES_VIEWABLE < ignoresMenuInfo.numPlayers)
	{
		ignoresMenuInfo.basePlayerNum++;
		UI_IgnoresMenu_SetPlayerNames();
	}
}

/*
=================
UI_RemoveMenu_Draw
=================
*/
static void UI_IgnoresMenu_Draw(void)
{
	UI_MenuFrame(&ignoresMenuInfo.menu);

	trap_R_SetColor( colorTable[CT_DKPURPLE2]);
	UI_DrawHandlePic(30,201,  47, 90, uis.whiteShader);	//Left side of frame
	UI_DrawHandlePic(30,293,  47, 97, uis.whiteShader);

	// Playerlist frame
	trap_R_SetColor( colorTable[CT_DKBLUE1]);
	UI_DrawHandlePic( 81, 172,  225,  18, uis.whiteShader);	// Top
	UI_DrawHandlePic(288, 189,   18, 246, uis.whiteShader);	// Side
	UI_DrawProportionalString( 90, 173, menu_normal_text[MNT_IGNORES_PLAYERLIST],UI_SMALLFONT,colorTable[CT_BLACK]); // Top

	// Ignores frame
	trap_R_SetColor( colorTable[CT_DKBLUE1]);
	UI_DrawHandlePic(404, 172,  225,  18, uis.whiteShader);
	UI_DrawHandlePic(404, 189,   18, 246, uis.whiteShader);
	UI_DrawProportionalString(425, 173, menu_normal_text[MNT_IGNORES],UI_SMALLFONT,colorTable[CT_BLACK]);


	// standard menu drawing
	Menu_Draw( &ignoresMenuInfo.menu );
}

/*
=================
UI_Ignores_Cache
=================
*/
void UI_Ignores_Cache( void )
{
	trap_R_RegisterShaderNoMip( "menu/common/arrow_up_16.tga" );
	trap_R_RegisterShaderNoMip( "menu/common/arrow_dn_16.tga" );
}


/*
=================
UI_IgnoresMenu_Init
=================
*/
static void UI_IgnoresMenu_Init( void ) {
	int		n;
	int		y;

	memset( &ignoresMenuInfo, 0 ,sizeof(ignoresMenuInfo) );
	ignoresMenuInfo.menu.draw					= UI_IgnoresMenu_Draw;
	ignoresMenuInfo.menu.fullscreen				= qtrue;
	ignoresMenuInfo.menu.wrapAround				= qtrue;
	ignoresMenuInfo.menu.descX					= MENU_DESC_X;
	ignoresMenuInfo.menu.descY					= MENU_DESC_Y;
	ignoresMenuInfo.menu.titleX					= MENU_TITLE_X;
	ignoresMenuInfo.menu.titleY					= MENU_TITLE_Y;
	ignoresMenuInfo.menu.titleI					= MNT_IGNORES_TITLE;
	ignoresMenuInfo.menu.footNoteEnum			= MNT_IGNORES;

	UI_Ignores_Cache();

	UI_IgnoresMenu_SetIgnoreNames();
	UI_IgnoresMenu_SetPlayerNames();

	ignoresMenuInfo.up.generic.type					= MTYPE_BITMAP;
	ignoresMenuInfo.up.generic.flags					= QMF_HIGHLIGHT_IF_FOCUS;
	ignoresMenuInfo.up.generic.x						= 405;
	ignoresMenuInfo.up.generic.y						= 196;
	ignoresMenuInfo.up.generic.id					= ID_UP;
	ignoresMenuInfo.up.generic.callback				= UI_IgnoresMenu_UpEvent;
	ignoresMenuInfo.up.width  						= 16;
	ignoresMenuInfo.up.height  						= 16;
	ignoresMenuInfo.up.color  						= CT_DKGOLD1;
	ignoresMenuInfo.up.color2  						= CT_LTGOLD1;
	ignoresMenuInfo.up.generic.name					= "menu/common/arrow_up_16.tga";

	ignoresMenuInfo.down.generic.type				= MTYPE_BITMAP;
	ignoresMenuInfo.down.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	ignoresMenuInfo.down.generic.x					= 405;
	ignoresMenuInfo.down.generic.y					= 407;
	ignoresMenuInfo.down.generic.id					= ID_DOWN;
	ignoresMenuInfo.down.generic.callback				= UI_IgnoresMenu_DownEvent;
	ignoresMenuInfo.down.width  						= 16;
	ignoresMenuInfo.down.height  					= 16;
	ignoresMenuInfo.down.color  						= CT_DKGOLD1;
	ignoresMenuInfo.down.color2  					= CT_LTGOLD1;
	ignoresMenuInfo.down.generic.name				= "menu/common/arrow_dn_16.tga";

	ignoresMenuInfo.plup.generic.type					= MTYPE_BITMAP;
	ignoresMenuInfo.plup.generic.flags					= QMF_HIGHLIGHT_IF_FOCUS;
	ignoresMenuInfo.plup.generic.x						= 289;
	ignoresMenuInfo.plup.generic.y						= 196;
	ignoresMenuInfo.plup.generic.id					= ID_PLUP;
	ignoresMenuInfo.plup.generic.callback				= UI_IgnoresMenu_PlUpEvent;
	ignoresMenuInfo.plup.width  						= 16;
	ignoresMenuInfo.plup.height  						= 16;
	ignoresMenuInfo.plup.color  						= CT_DKGOLD1;
	ignoresMenuInfo.plup.color2  						= CT_LTGOLD1;
	ignoresMenuInfo.plup.generic.name					= "menu/common/arrow_up_16.tga";

	ignoresMenuInfo.pldown.generic.type				= MTYPE_BITMAP;
	ignoresMenuInfo.pldown.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	ignoresMenuInfo.pldown.generic.x					= 289;
	ignoresMenuInfo.pldown.generic.y					= 407;
	ignoresMenuInfo.pldown.generic.id					= ID_PLDOWN;
	ignoresMenuInfo.pldown.generic.callback				= UI_IgnoresMenu_PlDownEvent;
	ignoresMenuInfo.pldown.width  						= 16;
	ignoresMenuInfo.pldown.height  					= 16;
	ignoresMenuInfo.pldown.color  						= CT_DKGOLD1;
	ignoresMenuInfo.pldown.color2  					= CT_LTGOLD1;
	ignoresMenuInfo.pldown.generic.name				= "menu/common/arrow_dn_16.tga";

	for( n = 0, y = 194; n < PLAYERS_VIEWABLE; n++, y += 20 )
	{
		ignoresMenuInfo.players[n].generic.type			= MTYPE_PTEXT;
		ignoresMenuInfo.players[n].generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
		ignoresMenuInfo.players[n].generic.id			= ID_PLAYERNAME + n;
		ignoresMenuInfo.players[n].generic.x			= 84;
		ignoresMenuInfo.players[n].generic.y			= y;
		ignoresMenuInfo.players[n].generic.callback		= UI_IgnoresMenu_PlayerEvent;
		ignoresMenuInfo.players[n].string				= ignoresMenuInfo.playernames_viewable[n];
		ignoresMenuInfo.players[n].color				= colorTable[CT_DKGOLD1];
		ignoresMenuInfo.players[n].color2				= colorTable[CT_LTGOLD1];
		ignoresMenuInfo.players[n].style				= UI_LEFT|UI_SMALLFONT;
	}

	for( n = 0, y = 194; n < IGNORES_VIEWABLE; n++, y += 20 )
	{
		ignoresMenuInfo.ignores[n].generic.type			= MTYPE_PTEXT;
		ignoresMenuInfo.ignores[n].generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
		ignoresMenuInfo.ignores[n].generic.id			= ID_IGNORENAME + n;
		ignoresMenuInfo.ignores[n].generic.x			= 426;
		ignoresMenuInfo.ignores[n].generic.y			= y;
		ignoresMenuInfo.ignores[n].generic.callback		= UI_IgnoresMenu_IgnoreEvent;
		ignoresMenuInfo.ignores[n].string				= ignoresMenuInfo.ignorenames_viewable[n];
		ignoresMenuInfo.ignores[n].color				= colorTable[CT_DKGOLD1];
		ignoresMenuInfo.ignores[n].color2				= colorTable[CT_LTGOLD1];
		ignoresMenuInfo.ignores[n].style				= UI_LEFT|UI_SMALLFONT;
	}

	ignoresMenuInfo.add.generic.type			= MTYPE_BITMAP;
	ignoresMenuInfo.add.generic.name			= GRAPHIC_SQUARE;
	ignoresMenuInfo.add.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	ignoresMenuInfo.add.generic.id				= ID_ADD;
	ignoresMenuInfo.add.generic.callback			= UI_IgnoresMenu_AddEvent;
	ignoresMenuInfo.add.generic.x				= 310;
	ignoresMenuInfo.add.generic.y				= BUTTONS_Y_POS;
	ignoresMenuInfo.add.width  				= 90;
	ignoresMenuInfo.add.height  				= MENU_BUTTON_MED_HEIGHT * 2;
	ignoresMenuInfo.add.color				= CT_DKPURPLE1;
	ignoresMenuInfo.add.color2				= CT_LTPURPLE1;
	ignoresMenuInfo.add.textX				= MENU_BUTTON_TEXT_X;
	ignoresMenuInfo.add.textY				= MENU_BUTTON_TEXT_Y + MENU_BUTTON_MED_HEIGHT/2;
	ignoresMenuInfo.add.textEnum				= MBT_ADD_IGNORE;
	ignoresMenuInfo.add.textcolor				= CT_BLACK;
	ignoresMenuInfo.add.textcolor2				= CT_WHITE;

	ignoresMenuInfo.delete.generic.type			= MTYPE_BITMAP;
	ignoresMenuInfo.delete.generic.name			= GRAPHIC_SQUARE;
	ignoresMenuInfo.delete.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	ignoresMenuInfo.delete.generic.id			= ID_DELETE;
	ignoresMenuInfo.delete.generic.callback			= UI_IgnoresMenu_DeleteEvent;
	ignoresMenuInfo.delete.generic.x			= 310;
	ignoresMenuInfo.delete.generic.y			= BUTTONS_Y_POS + 5 + MENU_BUTTON_MED_HEIGHT*2;
	ignoresMenuInfo.delete.width  				= 90;
	ignoresMenuInfo.delete.height  				= MENU_BUTTON_MED_HEIGHT * 2;
	ignoresMenuInfo.delete.color				= CT_DKPURPLE1;
	ignoresMenuInfo.delete.color2				= CT_LTPURPLE1;
	ignoresMenuInfo.delete.textX				= MENU_BUTTON_TEXT_X;
	ignoresMenuInfo.delete.textY				= MENU_BUTTON_TEXT_Y + MENU_BUTTON_MED_HEIGHT/2;
	ignoresMenuInfo.delete.textEnum				= MBT_REMOVE_IGNORE;
	ignoresMenuInfo.delete.textcolor			= CT_BLACK;
	ignoresMenuInfo.delete.textcolor2			= CT_WHITE;

	ignoresMenuInfo.flush.generic.type			= MTYPE_BITMAP;
	ignoresMenuInfo.flush.generic.name			= GRAPHIC_SQUARE;
	ignoresMenuInfo.flush.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	ignoresMenuInfo.flush.generic.id			= ID_FLUSH;
	ignoresMenuInfo.flush.generic.callback			= UI_IgnoresMenu_FlushEvent;
	ignoresMenuInfo.flush.generic.x				= 310;
	ignoresMenuInfo.flush.generic.y				= BUTTONS_Y_POS + 10 + MENU_BUTTON_MED_HEIGHT*4;
	ignoresMenuInfo.flush.width  				= 90;
	ignoresMenuInfo.flush.height  				= MENU_BUTTON_MED_HEIGHT * 2;
	ignoresMenuInfo.flush.color				= CT_DKPURPLE1;
	ignoresMenuInfo.flush.color2				= CT_LTPURPLE1;
	ignoresMenuInfo.flush.textX				= MENU_BUTTON_TEXT_X;
	ignoresMenuInfo.flush.textY				= MENU_BUTTON_TEXT_Y + MENU_BUTTON_MED_HEIGHT/2;
	ignoresMenuInfo.flush.textEnum				= MBT_FLUSH_IGNORE;
	ignoresMenuInfo.flush.textcolor				= CT_BLACK;
	ignoresMenuInfo.flush.textcolor2			= CT_WHITE;

	ignoresMenuInfo.back.generic.type			= MTYPE_BITMAP;
	ignoresMenuInfo.back.generic.name			= BUTTON_GRAPHIC_LONGRIGHT;
	ignoresMenuInfo.back.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	ignoresMenuInfo.back.generic.id				= ID_BACK;
	ignoresMenuInfo.back.generic.callback		= UI_IgnoresMenu_BackEvent;
	ignoresMenuInfo.back.generic.x				= 482;
	ignoresMenuInfo.back.generic.y				= 136;
	ignoresMenuInfo.back.width  					= MENU_BUTTON_MED_WIDTH;
	ignoresMenuInfo.back.height  				= MENU_BUTTON_MED_HEIGHT;
	ignoresMenuInfo.back.color					= CT_DKPURPLE1;
	ignoresMenuInfo.back.color2					= CT_LTPURPLE1;
	ignoresMenuInfo.back.textX					= MENU_BUTTON_TEXT_X;
	ignoresMenuInfo.back.textY					= MENU_BUTTON_TEXT_Y;
	ignoresMenuInfo.back.textEnum				= MBT_INGAMEMENU;
	ignoresMenuInfo.back.textcolor				= CT_BLACK;
	ignoresMenuInfo.back.textcolor2				= CT_WHITE;


	Menu_AddItem(&ignoresMenuInfo.menu, &ignoresMenuInfo.up);
	Menu_AddItem(&ignoresMenuInfo.menu, &ignoresMenuInfo.down);

	Menu_AddItem(&ignoresMenuInfo.menu, &ignoresMenuInfo.plup);
	Menu_AddItem(&ignoresMenuInfo.menu, &ignoresMenuInfo.pldown);

	for(n = 0; n < IGNORES_VIEWABLE; n++)
		Menu_AddItem(&ignoresMenuInfo.menu, &ignoresMenuInfo.ignores[n]);
	for(n = 0; n < PLAYERS_VIEWABLE; n++)
		Menu_AddItem(&ignoresMenuInfo.menu, &ignoresMenuInfo.players[n]);

	Menu_AddItem(&ignoresMenuInfo.menu, &ignoresMenuInfo.add);
	Menu_AddItem(&ignoresMenuInfo.menu, &ignoresMenuInfo.delete);
	Menu_AddItem(&ignoresMenuInfo.menu, &ignoresMenuInfo.flush);
	Menu_AddItem(&ignoresMenuInfo.menu, &ignoresMenuInfo.back);

	ignoresMenuInfo.baseIgnoreNum = 0;
	ignoresMenuInfo.selectedIgnoreNum = 0;
	ignoresMenuInfo.ignores[0].color = color_white;

	ignoresMenuInfo.basePlayerNum = 0;
	ignoresMenuInfo.selectedPlayerNum = 0;
	ignoresMenuInfo.players[0].color = color_white;
}


/*
=================
UI_IgnoresMenu
=================
*/
void UI_IgnoresMenu(void)
{
	UI_IgnoresMenu_Init();
	UI_PushMenu( &ignoresMenuInfo.menu );
}
