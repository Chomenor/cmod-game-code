// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "ui_local.h"

#define ID_NAME			10
#define ID_HANDICAP		11
#define ID_EFFECTS		12
#define ID_BACK			13

// If this gets changed, also change it in ui_playermodel
#define ID_MODEL		14
#define ID_DATA			15
#define ID_MAINMENU		16
#define ID_INGAMEMENU	17

#define MAX_NAMELENGTH	20


typedef struct {
	menuframework_s		menu;

	int					prevMenu;

	menubitmap_s		player;
	menubitmap_s		mainmenu;
	menubitmap_s		back;
	menufield_s			name;
	menulist_s			handicap;

	menubitmap_s		model;
	menubitmap_s		data;
	menubitmap_s		item_null;

	qhandle_t			corner_ul_4_4;
	qhandle_t			corner_ur_4_4;
	qhandle_t			corner_ll_4_4;
	qhandle_t			corner_ll_4_18;
	qhandle_t			corner_lr_4_18;

	menutext_s			playername;
	qhandle_t			fxBasePic;
	qhandle_t			fxPic[7];
	playerInfo_t		playerinfo;
	int					current_fx;
	char				playerModel[MAX_QPATH];
} playersettings_t;

static playersettings_t	s_playersettings;

static int gamecodetoui[] = {4,2,3,0,5,1,6};
static int uitogamecode[] = {4,6,2,3,1,5,7};

static int handicap_items[] = 
{
	MNT_HANDICAP_NONE,
	MNT_HANDICAP_95,
	MNT_HANDICAP_90,
	MNT_HANDICAP_85,
	MNT_HANDICAP_80,
	MNT_HANDICAP_75,
	MNT_HANDICAP_70,
	MNT_HANDICAP_65,
	MNT_HANDICAP_60,
	MNT_HANDICAP_55,
	MNT_HANDICAP_50,
	MNT_HANDICAP_45,
	MNT_HANDICAP_40,
	MNT_HANDICAP_35,
	MNT_HANDICAP_30,
	MNT_HANDICAP_25,
	MNT_HANDICAP_20,
	MNT_HANDICAP_15,
	MNT_HANDICAP_10,
	MNT_HANDICAP_05,
	0
};


/*
=================
PlayerSettings_DrawPlayer
=================
*/
static void PlayerSettings_DrawPlayer( void *self ) 
{
	menubitmap_s	*b;
	vec3_t			viewangles;
	char			buf[MAX_QPATH];

	trap_Cvar_VariableStringBuffer( "model", buf, sizeof( buf ) );
	if ( strcmp( buf, s_playersettings.playerModel ) != 0 ) {
		UI_PlayerInfo_SetModel( &s_playersettings.playerinfo, buf );
		strcpy( s_playersettings.playerModel, buf );

		viewangles[YAW]   = 180 - 30;
		viewangles[PITCH] = 0;
		viewangles[ROLL]  = 0;
		UI_PlayerInfo_SetInfo( &s_playersettings.playerinfo, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_COMPRESSION_RIFLE, qfalse );
	}

	b = (menubitmap_s*) self;
	UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height, &s_playersettings.playerinfo, uis.realtime/2 );
}


/*
=================
PlayerSettings_SaveChanges
=================
*/
static void PlayerSettings_SaveChanges( void ) 
{
	// name
	trap_Cvar_Set( "name", s_playersettings.name.field.buffer );

	// handicap
	trap_Cvar_SetValue( "handicap", 100 - s_playersettings.handicap.curvalue * 5 );

	// effects color
//	trap_Cvar_SetValue( "color", uitogamecode[s_playersettings.effects.curvalue] );
}


/*
=================
PlayerSettings_MenuKey
=================
*/
static sfxHandle_t PlayerSettings_MenuKey( int key ) 
{
	if( key == K_MOUSE2 || key == K_ESCAPE ) 
	{
		PlayerSettings_SaveChanges();
	}

	return Menu_DefaultKey( &s_playersettings.menu, key );
}


/*
=================
PlayerSettings_SetMenuItems
=================
*/
static void PlayerSettings_SetMenuItems( void ) 
{
	vec3_t	viewangles;
//	int		c;
	int		h;

	// name
	Q_strncpyz( s_playersettings.name.field.buffer, UI_Cvar_VariableString("name"), sizeof(s_playersettings.name.field.buffer) );

	// effects color
//	c = trap_Cvar_VariableValue( "color" ) - 1;
//	if( c < 0 || c > 6 ) {
//		c = 6;
//	}
//	s_playersettings.effects.curvalue = gamecodetoui[c];

	// model/skin
	memset( &s_playersettings.playerinfo, 0, sizeof(playerInfo_t) );
	
	viewangles[YAW]   = 180 - 30;
	viewangles[PITCH] = 0;
	viewangles[ROLL]  = 0;

	UI_PlayerInfo_SetModel( &s_playersettings.playerinfo, UI_Cvar_VariableString( "model" ) );
	UI_PlayerInfo_SetInfo( &s_playersettings.playerinfo, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_COMPRESSION_RIFLE, qfalse );

	// handicap
	h = Com_Clamp( 5, 100, trap_Cvar_VariableValue("handicap") );
	s_playersettings.handicap.curvalue = 20 - h / 5;
}


/*
=================
PlayerSettings_MenuEvent
=================
*/
static void PlayerSettings_MenuEvent( void* ptr, int event ) 
{
	if( event != QM_ACTIVATED ) 
	{
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) 
	{

	case ID_HANDICAP:
		trap_Cvar_Set( "handicap", va( "%i", 100 - 25 * s_playersettings.handicap.curvalue ) );
		break;

	case ID_MODEL:
		UI_PopMenu();
		PlayerSettings_SaveChanges();
		UI_PlayerModelMenu(s_playersettings.prevMenu);
		break;

	case ID_BACK:
		PlayerSettings_SaveChanges();
		UI_PopMenu();
		break;

	case ID_MAINMENU:
		PlayerSettings_SaveChanges();
		UI_MainMenu();
		break;

	case ID_INGAMEMENU:
		PlayerSettings_SaveChanges();
		UI_InGameMenu();
		break;
	}
}


/*
=================
PlayerSettingsMenu_Graphics
=================
*/
void PlayerSettingsMenu_Graphics (void)
{
	// Draw the basic screen layout
	UI_MenuFrame2(&s_playersettings.menu);

	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic(30,203, 47, 186, uis.whiteShader);	// Middle left line

	// Frame around model pictures
	trap_R_SetColor( colorTable[CT_LTORANGE]);
	UI_DrawHandlePic(  114,  62,   8,  -32, s_playersettings.corner_ll_4_18);	// UL Corner
	UI_DrawHandlePic(  114, 341,   8,  32, s_playersettings.corner_ll_4_18);	// LL Corner
	UI_DrawHandlePic(  411,  62,   8,  -32, s_playersettings.corner_lr_4_18);	// UR Corner
	UI_DrawHandlePic(  411, 341,   8,  32, s_playersettings.corner_lr_4_18);	// LR Corner
	UI_DrawHandlePic(  114,  93,   4, 258, uis.whiteShader);	// Left side
	UI_DrawHandlePic(  414,  93,   4, 258, uis.whiteShader);	// Right side
	UI_DrawHandlePic(  120,  74, 293,  18, uis.whiteShader);	// Top
	UI_DrawHandlePic(  120, 343, 293,  18, uis.whiteShader);	// Bottom

	UI_DrawProportionalString( 126,  76, menu_normal_text[MNT_CHANGEPLAYER],UI_SMALLFONT,colorTable[CT_BLACK]);	// Top

	trap_R_SetColor( colorTable[CT_DKGREY2]);
	UI_DrawHandlePic(  439, 79, 151,   295, uis.whiteShader);	// Background

	// Frame around player model
	trap_R_SetColor( colorTable[CT_LTORANGE]);
	UI_DrawHandlePic( 435,  50,   8,  -32, s_playersettings.corner_ll_4_18);	// UL Corner
	UI_DrawHandlePic( 435, 369,   8,   8, s_playersettings.corner_ll_4_4);	// LL Corner
	UI_DrawHandlePic( 440,  62, 150,  18, uis.whiteShader);	// Top
	UI_DrawHandlePic( 435,  79,   4, 295, uis.whiteShader);	// Left side
	UI_DrawHandlePic( 440, 371, 150,   4, uis.whiteShader);	// Bottom

	// Left rounded ends for buttons
	trap_R_SetColor( colorTable[s_playersettings.mainmenu.color]);
	UI_DrawHandlePic(s_playersettings.mainmenu.generic.x - 14, s_playersettings.mainmenu.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	trap_R_SetColor( colorTable[s_playersettings.back.color]);
	UI_DrawHandlePic(s_playersettings.back.generic.x - 14, s_playersettings.back.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	trap_R_SetColor( colorTable[s_playersettings.data.color]);
	UI_DrawHandlePic(s_playersettings.data.generic.x - 14, s_playersettings.data.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	trap_R_SetColor( colorTable[s_playersettings.model.color]);
	UI_DrawHandlePic(s_playersettings.model.generic.x - 14, s_playersettings.model.generic.y, 
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);


	UI_DrawProportionalString(  74,   28, "881",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  150, "2445",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  206, "600",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  395, "3-44",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

}

/*
=================
PlayerSettings_MenuDraw
=================
*/
static void PlayerSettings_MenuDraw (void)
{
	PlayerSettingsMenu_Graphics();
	Menu_Draw( &s_playersettings.menu );
}


/*
=================
PlayerSettings_MenuInit
=================
*/
static void PlayerSettings_MenuInit(int menuFrom) 
{
	int		y;
	static char	playername[32];

	memset(&s_playersettings,0,sizeof(playersettings_t));

	s_playersettings.prevMenu = menuFrom;

	PlayerSettings_Cache();

	s_playersettings.menu.key					= PlayerSettings_MenuKey;
	s_playersettings.menu.wrapAround					= qtrue;
	s_playersettings.menu.fullscreen					= qtrue;
    s_playersettings.menu.draw							= PlayerSettings_MenuDraw;
	s_playersettings.menu.descX							= MENU_DESC_X;
	s_playersettings.menu.descY							= MENU_DESC_Y;
	s_playersettings.menu.titleX						= MENU_TITLE_X;
	s_playersettings.menu.titleY						= MENU_TITLE_Y;
	s_playersettings.menu.titleI						= MNT_CHANGEPLAYER_TITLE;
	s_playersettings.menu.footNoteEnum					= MNT_CHANGEPLAYER;


	s_playersettings.mainmenu.generic.type				= MTYPE_BITMAP;      
	s_playersettings.mainmenu.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_playersettings.mainmenu.generic.x					= 110;
	s_playersettings.mainmenu.generic.y					= 391;
	s_playersettings.mainmenu.generic.name				= BUTTON_GRAPHIC_LONGRIGHT;
	s_playersettings.mainmenu.generic.callback			= PlayerSettings_MenuEvent;
	s_playersettings.mainmenu.width						= MENU_BUTTON_MED_WIDTH;
	s_playersettings.mainmenu.height					= MENU_BUTTON_MED_HEIGHT;
	s_playersettings.mainmenu.color						= CT_DKPURPLE1;
	s_playersettings.mainmenu.color2					= CT_LTPURPLE1;

	if (!ingameFlag)
	{
		s_playersettings.mainmenu.textEnum					= MBT_MAINMENU;
		s_playersettings.mainmenu.generic.id				= ID_MAINMENU;
	}
	else
	{
		s_playersettings.mainmenu.textEnum					= MBT_INGAMEMENU;
		s_playersettings.mainmenu.generic.id				= ID_INGAMEMENU;
	}

	s_playersettings.mainmenu.textX						= MENU_BUTTON_TEXT_X;
	s_playersettings.mainmenu.textY						= MENU_BUTTON_TEXT_Y;
	s_playersettings.mainmenu.textcolor					= CT_BLACK;
	s_playersettings.mainmenu.textcolor2				= CT_WHITE;

	s_playersettings.back.generic.type				= MTYPE_BITMAP;      
	s_playersettings.back.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_playersettings.back.generic.x					= 110;
	s_playersettings.back.generic.y					= 415;
	s_playersettings.back.generic.name				= BUTTON_GRAPHIC_LONGRIGHT;
	s_playersettings.back.generic.id					= ID_BACK;
	s_playersettings.back.generic.callback			= PlayerSettings_MenuEvent;
	s_playersettings.back.width						= MENU_BUTTON_MED_WIDTH;
	s_playersettings.back.height						= MENU_BUTTON_MED_HEIGHT;
	s_playersettings.back.color						= CT_DKPURPLE1;
	s_playersettings.back.color2						= CT_LTPURPLE1;
	s_playersettings.back.textX						= MENU_BUTTON_TEXT_X;
	s_playersettings.back.textY						= MENU_BUTTON_TEXT_Y;
	s_playersettings.back.textEnum					= MBT_BACK;
	s_playersettings.back.textcolor					= CT_BLACK;
	s_playersettings.back.textcolor2					= CT_WHITE;

	y = 144;
	s_playersettings.name.generic.type					= MTYPE_FIELD;
	s_playersettings.name.field.widthInChars			= MAX_NAMELENGTH;
	s_playersettings.name.field.maxchars				= MAX_NAMELENGTH;
	s_playersettings.name.generic.x						= 239;
	s_playersettings.name.generic.y						= 186;
	s_playersettings.name.field.style					= UI_SMALLFONT;
	s_playersettings.name.field.titleEnum				= MBT_PLAYER_NAME;
	s_playersettings.name.field.titlecolor				= CT_LTGOLD1;
	s_playersettings.name.field.textcolor				= CT_DKGOLD1;
	s_playersettings.name.field.textcolor2				= CT_LTGOLD1;

	y += 3 * PROP_HEIGHT;
	s_playersettings.handicap.generic.type				= MTYPE_SPINCONTROL;
	s_playersettings.handicap.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_playersettings.handicap.generic.id				= ID_HANDICAP;
	s_playersettings.handicap.generic.callback			= PlayerSettings_MenuEvent;
	s_playersettings.handicap.generic.x					= 130;
	s_playersettings.handicap.generic.y					= 236;
	s_playersettings.handicap.numitems					= 20;
	s_playersettings.handicap.textEnum					= MBT_PLAYER_HANDICAP;
	s_playersettings.handicap.textcolor					= CT_BLACK;
	s_playersettings.handicap.textcolor2				= CT_WHITE;
	s_playersettings.handicap.color						= CT_DKPURPLE1;
	s_playersettings.handicap.color2					= CT_LTPURPLE1;
	s_playersettings.handicap.width						= 80;
	s_playersettings.handicap.textX						= 5;
	s_playersettings.handicap.textY						= 2;
	s_playersettings.handicap.listnames					= handicap_items;

	s_playersettings.data.generic.type					= MTYPE_BITMAP;
	s_playersettings.data.generic.name					= BUTTON_GRAPHIC_LONGRIGHT;
	s_playersettings.data.generic.flags					= QMF_GRAYED;
	s_playersettings.data.generic.id					= ID_DATA;
	s_playersettings.data.generic.callback				= PlayerSettings_MenuEvent;
	s_playersettings.data.generic.x						= 482;
	s_playersettings.data.generic.y						= 391;
	s_playersettings.data.width							= 128;
	s_playersettings.data.height						= 64;
	s_playersettings.data.width							= MENU_BUTTON_MED_WIDTH;
	s_playersettings.data.height						= MENU_BUTTON_MED_HEIGHT;
	s_playersettings.data.color							= CT_DKPURPLE1;
	s_playersettings.data.color2						= CT_LTPURPLE1;
	s_playersettings.data.textX							= 5;
	s_playersettings.data.textY							= 2;
	s_playersettings.data.textEnum						= MBT_PLAYERDATA;
	s_playersettings.data.textcolor						= CT_BLACK;
	s_playersettings.data.textcolor2					= CT_WHITE;

	s_playersettings.model.generic.type					= MTYPE_BITMAP;
	s_playersettings.model.generic.name					= BUTTON_GRAPHIC_LONGRIGHT;
	s_playersettings.model.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_playersettings.model.generic.id					= ID_MODEL;
	s_playersettings.model.generic.callback				= PlayerSettings_MenuEvent;
	s_playersettings.model.generic.x					= 482;
	s_playersettings.model.generic.y					= 415;
	s_playersettings.model.width						= MENU_BUTTON_MED_WIDTH;
	s_playersettings.model.height						= MENU_BUTTON_MED_HEIGHT;
	s_playersettings.model.color						= CT_DKPURPLE1;
	s_playersettings.model.color2						= CT_LTPURPLE1;
	s_playersettings.model.textX						= 5;
	s_playersettings.model.textY						= 2;
	s_playersettings.model.textEnum						= MBT_CHANGEMODEL;
	s_playersettings.model.textcolor					= CT_BLACK;
	s_playersettings.model.textcolor2					= CT_WHITE;

	s_playersettings.player.generic.type				= MTYPE_BITMAP;
	s_playersettings.player.generic.flags				= QMF_INACTIVE;
	s_playersettings.player.generic.ownerdraw			= PlayerSettings_DrawPlayer;
	s_playersettings.player.generic.x					= 400;
	s_playersettings.player.generic.y					= 20;
	s_playersettings.player.width						= 32*7.3;
	s_playersettings.player.height						= 56*7.3;

	s_playersettings.playername.generic.type			= MTYPE_PTEXT;
	s_playersettings.playername.generic.flags			= QMF_INACTIVE;
	s_playersettings.playername.generic.x				= 444;
	s_playersettings.playername.generic.y				= 63;
	s_playersettings.playername.string					= s_playersettings.name.field.buffer;
	s_playersettings.playername.style					= UI_SMALLFONT;
	s_playersettings.playername.color					= colorTable[CT_BLACK];

	s_playersettings.item_null.generic.type		= MTYPE_BITMAP;
	s_playersettings.item_null.generic.flags	= QMF_LEFT_JUSTIFY|QMF_MOUSEONLY|QMF_SILENT;
	s_playersettings.item_null.generic.x		= 0;
	s_playersettings.item_null.generic.y		= 0;
	s_playersettings.item_null.width			= 640;
	s_playersettings.item_null.height			= 480;

//	if (s_playersettings.prevMenu == PS_MENU_CONTROLS)
//	{
//		SetupMenu_TopButtons(&s_playersettings.menu,MENU_PLAYER);
//	}

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.mainmenu);
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.back);
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.playername );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.name );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.handicap );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.model );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.data );

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.player );

//	Menu_AddItem( &s_playersettings.menu, &s_playersettings.item_null );

	PlayerSettings_SetMenuItems();
}


/*
=================
PlayerSettings_Cache
=================
*/
void PlayerSettings_Cache( void ) 
{
	s_playersettings.corner_ul_4_4  = trap_R_RegisterShaderNoMip("menu/common/corner_ul_4_4");
	s_playersettings.corner_ur_4_4  = trap_R_RegisterShaderNoMip("menu/common/corner_ur_4_4");
	s_playersettings.corner_ll_4_4  = trap_R_RegisterShaderNoMip("menu/common/corner_ll_4_4");
	s_playersettings.corner_ll_4_18 = trap_R_RegisterShaderNoMip("menu/common/corner_ll_4_18");
	s_playersettings.corner_lr_4_18 = trap_R_RegisterShaderNoMip("menu/common/corner_lr_4_18");
}


/*
=================
UI_PlayerSettingsMenu
=================
*/
void UI_PlayerSettingsMenu(int menuFrom) 
{
	PlayerSettings_MenuInit(menuFrom);
	UI_PushMenu( &s_playersettings.menu );
}

