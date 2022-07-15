// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

GAME OPTIONS MENU

=======================================================================
*/


#include "ui_local.h"


#define NUM_CROSSHAIRS		12
#define PIC_BUTTON2			"menu/common/full_button2.tga"

#define CROSSHAIR_X			438
#define CROSSHAIR_Y			270

// should match values in cg_main.c
#define BOB_UP_DEFAULT			0.005f
#define BOB_UP_DEFAULT_STR		"0.005"
#define BOB_PITCH_DEFAULT		0.002f
#define BOB_PITCH_DEFAULT_STR	"0.002"
#define BOB_ROLL_DEFAULT		0.002f
#define BOB_ROLL_DEFAULT_STR	"0.002"

// Precache stuff for Game Options Menu
static struct
{
	menuframework_s		menu;

	qhandle_t	slant1;
	qhandle_t	slant2;
	qhandle_t	swooptop;
	qhandle_t	swoopbottom;
	qhandle_t	singraph;
	qhandle_t	graphbox;
	qhandle_t	lswoop;
	qhandle_t	lswoop2;
	qhandle_t	tallswoop;
	qhandle_t	tallswoop2;

} s_gameoptions;


extern int s_OffOnNone_Names[];

static int Preferences_OffOnCustomNone_Names[] =
{
	MNT_OFF,
	MNT_ON,
	MNT_VIDEO_CUSTOM,
	MNT_NONE
};

#define ID_TEXTLANGUAGE			126
#define ID_CROSSHAIR			127
#define ID_EJECTINGBRASS		130
#define ID_IDENTIFYTARGET		133
#define ID_FORCEMODEL			135
#define ID_DRAWTEAMOVERLAY		136
#define ID_ALLOWDOWNLOAD		137
#define ID_BACK					138
#define ID_VOICELANGUAGE		139
#define ID_DRAWTIMER			140
#define ID_DRAWFPS				141
#define ID_DRAWGUN				142
#define ID_MOTIONBOB			143



typedef struct {
	menuframework_s		menu;

	menubitmap_s		crosshair;
	menulist_s			textlanguage;
	menulist_s			voicelanguage;
	menulist_s			identifytarget;
	menulist_s			forcemodel;
	menulist_s			drawteamoverlay;
	menulist_s			drawtimer;
	menulist_s			drawfps;
	menulist_s			drawgun;
	menulist_s			motionbob;
	menulist_s			allowdownload;

	float				old_bobup;
	float				old_bobpitch;
	float				old_bobroll;

	int					currentcrosshair;
	qhandle_t			crosshairShader[NUM_CROSSHAIRS];
} preferences_t;

static preferences_t s_preferences;

int s_textlanguage_Names[] =
{
	MNT_ENGLISH,
	MNT_GERMAN,
	MNT_FRENCH,
	MNT_NONE
};

int s_voicelanguage_Names[] =
{
	MNT_ENGLISH,
	MNT_GERMAN,
	MNT_NONE
};

static int teamoverlay_names[] =
{
	MNT_TO_OFF,
	MNT_TO_UPPER_RIGHT,
	MNT_TO_LOWER_RIGHT,
	MNT_TO_LOWER_LEFT,
	0
};

static void Preferences_SetMenuItems( void )
{
	char buffer[32];
	int *language;

	// register certain defaults in case cgame isn't loaded yet
	trap_Cvar_Register( NULL, "cg_drawGun", "1", CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "cg_bobup", BOB_UP_DEFAULT_STR, CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "cg_bobpitch", BOB_PITCH_DEFAULT_STR, CVAR_ARCHIVE );
	trap_Cvar_Register( NULL, "cg_bobroll", BOB_ROLL_DEFAULT_STR, CVAR_ARCHIVE );

	s_preferences.currentcrosshair		= (int)trap_Cvar_VariableValue( "cg_drawCrosshair" ) % NUM_CROSSHAIRS;
	s_preferences.identifytarget.curvalue	= trap_Cvar_VariableValue( "cg_drawCrosshairNames" ) != 0;
	s_preferences.forcemodel.curvalue		= trap_Cvar_VariableValue( "cg_forcemodel" ) != 0;
	s_preferences.drawteamoverlay.curvalue	= Com_Clamp( 0, 3, trap_Cvar_VariableValue( "cg_drawTeamOverlay" ) );
	s_preferences.drawtimer.curvalue		= trap_Cvar_VariableValue( "cg_drawTimer" ) != 0;
	s_preferences.drawfps.curvalue			= trap_Cvar_VariableValue( "cg_drawFPS" ) != 0;
	s_preferences.drawgun.curvalue			= trap_Cvar_VariableValue( "cg_drawGun" ) != 0;
	s_preferences.allowdownload.curvalue	= trap_Cvar_VariableValue( "cl_allowDownload" ) != 0;

	trap_Cvar_VariableStringBuffer( "g_language", buffer, 32 );
	language = s_textlanguage_Names;

	s_preferences.textlanguage.curvalue = 0;
	if (buffer[0]) {
		while (*language)	//scan for a match
		{
			if (Q_stricmp(menu_normal_text[*language],buffer)==0)
			{
				break;
			}
			language++;
			s_preferences.textlanguage.curvalue++;
		}

		if (!*language)
		{
			s_preferences.textlanguage.curvalue = 0;
		}
	}

	trap_Cvar_VariableStringBuffer( "s_language", buffer, 32 );
	language = s_voicelanguage_Names;

	s_preferences.voicelanguage.curvalue = 0;
	if (buffer[0]) {
		while (*language)	//scan for a match
		{
			if (Q_stricmp(menu_normal_text[*language],buffer)==0)
			{
				break;
			}
			language++;
			s_preferences.voicelanguage.curvalue++;
		}

		if (!*language)
		{
			s_preferences.voicelanguage.curvalue = 0;
		}
	}

	s_preferences.old_bobup = trap_Cvar_VariableValue( "cg_bobup" );
	s_preferences.old_bobpitch = trap_Cvar_VariableValue( "cg_bobpitch" );
	s_preferences.old_bobroll = trap_Cvar_VariableValue( "cg_bobroll" );
	if ( s_preferences.old_bobup == 0.0f && s_preferences.old_bobpitch == 0.0f && s_preferences.old_bobroll == 0.0f ) {
		s_preferences.motionbob.curvalue = 0;
	} else if ( s_preferences.old_bobup == BOB_UP_DEFAULT && s_preferences.old_bobpitch == BOB_PITCH_DEFAULT &&
			s_preferences.old_bobroll == BOB_ROLL_DEFAULT ) {
		s_preferences.motionbob.curvalue = 1;
	} else {
		// enable "custom" option to keep existing values
		s_preferences.motionbob.listnames = Preferences_OffOnCustomNone_Names;
		s_preferences.motionbob.curvalue = 2;
	}
}


static void Preferences_Event( void* ptr, int notification )
{
	if( notification != QM_ACTIVATED )
	{
		return;
	}

	switch( ((menucommon_s*)ptr)->id )
	{
	case ID_CROSSHAIR:
		if ( VMExt_GVCommandInt( "crosshair_advance_current", 0 ) > 0 ) {
			// Engine should have taken care of the advancement
		} else {
			// No engine support; advance crosshair the traditional way
			s_preferences.currentcrosshair++;
			if( s_preferences.currentcrosshair == NUM_CROSSHAIRS )
			{
				s_preferences.currentcrosshair = 0;
			}
			trap_Cvar_SetValue( "cg_drawCrosshair", s_preferences.currentcrosshair );
		}
		break;

	case ID_IDENTIFYTARGET:
		trap_Cvar_SetValue( "cg_drawCrosshairNames", s_preferences.identifytarget.curvalue );
		break;

	case ID_FORCEMODEL:
		trap_Cvar_SetValue( "cg_forcemodel", s_preferences.forcemodel.curvalue );
		break;

	case ID_DRAWTEAMOVERLAY:
		trap_Cvar_SetValue( "cg_drawTeamOverlay", s_preferences.drawteamoverlay.curvalue );
		break;

	case ID_DRAWTIMER:
		trap_Cvar_SetValue( "cg_drawTimer", s_preferences.drawtimer.curvalue );
		break;

	case ID_DRAWFPS:
		trap_Cvar_SetValue( "cg_drawFPS", s_preferences.drawfps.curvalue );
		break;

	case ID_DRAWGUN:
		trap_Cvar_SetValue( "cg_drawGun", s_preferences.drawgun.curvalue );
		break;

	case ID_MOTIONBOB:
		if ( s_preferences.motionbob.curvalue == 0 ) {
			trap_Cvar_SetValue( "cg_bobup", 0.0f );
			trap_Cvar_SetValue( "cg_bobpitch", 0.0f );
			trap_Cvar_SetValue( "cg_bobroll", 0.0f );
		} else if ( s_preferences.motionbob.curvalue == 1 ) {
			trap_Cvar_SetValue( "cg_bobup", BOB_UP_DEFAULT );
			trap_Cvar_SetValue( "cg_bobpitch", BOB_PITCH_DEFAULT );
			trap_Cvar_SetValue( "cg_bobroll", BOB_ROLL_DEFAULT );
		} else {
			trap_Cvar_SetValue( "cg_bobup", s_preferences.old_bobup );
			trap_Cvar_SetValue( "cg_bobpitch", s_preferences.old_bobpitch );
			trap_Cvar_SetValue( "cg_bobroll", s_preferences.old_bobroll );
		}
		break;

	case ID_ALLOWDOWNLOAD:
		trap_Cvar_SetValue( "cl_allowDownload", s_preferences.allowdownload.curvalue );
		break;

	case ID_TEXTLANGUAGE:
		trap_Cvar_Set( "g_language", menu_normal_text[s_textlanguage_Names[s_preferences.textlanguage.curvalue]] );
		UI_LoadButtonText();	//reload the menus
		UI_LoadMenuText();
		BG_LoadItemNames();
		break;

	case ID_VOICELANGUAGE:
		trap_Cvar_Set( "s_language", menu_normal_text[s_voicelanguage_Names[s_preferences.voicelanguage.curvalue]] );
		trap_Cmd_ExecuteText( EXEC_APPEND, "snd_restart\n" );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
=================
Crosshair_Draw
=================
*/
/*
static void Crosshair_Draw( void *self ) {
	menulist_s	*s;
	float		*color;
	int			x, y;
	int			style;
	qboolean	focus;

	s = (menulist_s *)self;
	x = s->generic.x;
	y =	s->generic.y;

	style = UI_SMALLFONT;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if ( s->generic.flags & QMF_GRAYED )
		color = text_color_disabled;
	else if ( focus )
	{
		color = text_color_highlight;
		style |= UI_PULSE;
	}
	else if ( s->generic.flags & QMF_BLINK )
	{
		color = text_color_highlight;
		style |= UI_BLINK;
	}
	else
		color = text_color_normal;

	if ( focus )
	{
		// draw cursor
		UI_FillRect( s->generic.left, s->generic.top, s->generic.right-s->generic.left+1, s->generic.bottom-s->generic.top+1, listbar_color );
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color);
	}

	UI_DrawString( x - SMALLCHAR_WIDTH, y, s->generic.name, style|UI_RIGHT, color );
	if( !s->curvalue ) {
		return;
	}
	UI_DrawHandlePic( x + SMALLCHAR_WIDTH, y - 4, 24, 24, s_preferences.crosshairShader[s->curvalue] );
}
*/

/*
=================
GameOptions_MenuDraw
=================
*/
static void GameOptions_MenuDraw( void )
{
	qhandle_t hShader;

	hShader = VMExt_GVCommandInt( "crosshair_get_current_shader", -1 );
	if ( hShader < 0 ) {
		// No engine crosshair support - load crosshair the traditional way
		hShader = s_preferences.currentcrosshair ? s_preferences.crosshairShader[s_preferences.currentcrosshair] : 0;
	}

	UI_MenuFrame(&s_gameoptions.menu);

	UI_Setup_MenuButtons();

	trap_R_SetColor( colorTable[CT_DKPURPLE2]);
	UI_DrawHandlePic(30,203,  47, 186, uis.whiteShader);	// Long left hand column square
/*
	UI_DrawHandlePic( 543, 171, 128,	32,	s_gameoptions.swooptop);
	UI_DrawHandlePic( 543, 396, 128,	32,	s_gameoptions.swoopbottom);

	UI_DrawHandlePic(548, 195,  64,	8,		uis.whiteShader);	// Top of right hand column
	UI_DrawHandlePic(548, 389,  64,	7,		uis.whiteShader);	// Bottom of right hand column
	UI_DrawHandlePic( 548, 206, 64, 180,    uis.whiteShader);	//

	UI_DrawHandlePic( 104, 171, 436, 12,    uis.whiteShader);	// Top
	UI_DrawHandlePic( 104, 182, 16,  227,    uis.whiteShader);	// Side
	UI_DrawHandlePic( 104, 408, 436, 12,    uis.whiteShader);	// Bottom
*/
	trap_R_SetColor( colorTable[CT_DKBLUE1]);
	UI_DrawHandlePic( CROSSHAIR_X - 51, CROSSHAIR_Y - 25, 8,   87,	uis.whiteShader);		// Lefthand side of CROSSHAIR box
	UI_DrawHandlePic( CROSSHAIR_X + 75, CROSSHAIR_Y - 25, 8,   87,	uis.whiteShader);		// Righthand side of CROSSHAIR box
	UI_DrawHandlePic( CROSSHAIR_X - 43, CROSSHAIR_Y + 65, 116, 3,	uis.whiteShader);		// Bottom of CROSSHAIR box
	UI_DrawHandlePic( CROSSHAIR_X - 51, CROSSHAIR_Y + 62, 16,  16,s_gameoptions.lswoop);	// lower left hand swoop
	UI_DrawHandlePic( CROSSHAIR_X + 72, CROSSHAIR_Y + 62, 16,  16,s_gameoptions.lswoop2);	// lower right hand swoop

	UI_DrawHandlePic( CROSSHAIR_X - 51, CROSSHAIR_Y - 46, 16,  32,  s_gameoptions.tallswoop);		// upper left hand swoop
	UI_DrawHandlePic( CROSSHAIR_X + 69, CROSSHAIR_Y - 46, 16,  32,  s_gameoptions.tallswoop2);	// upper right hand swoop

	trap_R_SetColor( colorTable[CT_YELLOW]);

	if (hShader)
	{
		UI_DrawHandlePic(CROSSHAIR_X, CROSSHAIR_Y,  32, 32, hShader);	// Draw crosshair
	}
	else
	{
		UI_DrawProportionalString( CROSSHAIR_X + 16, CROSSHAIR_Y + 5, menu_normal_text[MNT_CROSSHAIR_NONE],UI_CENTER|UI_SMALLFONT, colorTable[CT_LTGOLD1]);	// No crosshair
	}

	// Menu frame numbers
	UI_DrawProportionalString(  74,  66, "1776",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  84, "9214",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  188, "2510-81",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  206, "644",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  395, "1001001",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	Menu_Draw( &s_gameoptions.menu );
}

/*
=================
GameOptions_MenuKey
=================
*/
static sfxHandle_t GameOptions_MenuKey( int key )
{
	return Menu_DefaultKey( &s_gameoptions.menu, key );
}
/*
===============
UI_GameOptionsMenu_Cache
===============
*/
void UI_GameOptionsMenu_Cache( void )
{
	int i;

	s_gameoptions.slant1 = trap_R_RegisterShaderNoMip("menu/lcarscontrols/slant1.tga");
	s_gameoptions.slant2 = trap_R_RegisterShaderNoMip("menu/lcarscontrols/slant2.tga");

	s_gameoptions.swooptop = trap_R_RegisterShaderNoMip("menu/lcarscontrols/bigswooptop.tga");
	s_gameoptions.swoopbottom = trap_R_RegisterShaderNoMip("menu/lcarscontrols/bigswoopbottom.tga");

	s_gameoptions.singraph = trap_R_RegisterShaderNoMip("menu/lcarscontrols/singraph.tga");
	s_gameoptions.graphbox = trap_R_RegisterShaderNoMip("menu/lcarscontrols/graphbox.tga");

	s_gameoptions.lswoop = trap_R_RegisterShaderNoMip("menu/lcarscontrols/lswoop.tga");
	s_gameoptions.lswoop2 = trap_R_RegisterShaderNoMip("menu/lcarscontrols/lswoop2.tga");
	s_gameoptions.tallswoop = trap_R_RegisterShaderNoMip("menu/lcarscontrols/tallswoop.tga");
	s_gameoptions.tallswoop2 = trap_R_RegisterShaderNoMip("menu/lcarscontrols/tallswoop2.tga");

	trap_R_RegisterShaderNoMip(PIC_BUTTON2);

	// Precache crosshairs
	for( i = 0; i < NUM_CROSSHAIRS; i++ )
	{
		s_preferences.crosshairShader[i] = trap_R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a' + i ) );
	}

}

/*
=================
GameOptions_MenuInit
=================
*/
static void GameOptions_MenuInit( void )
{
	int x,y,inc,width;

	UI_GameOptionsMenu_Cache();

	s_gameoptions.menu.nitems				= 0;
	s_gameoptions.menu.wrapAround			= qtrue;
	s_gameoptions.menu.draw					= GameOptions_MenuDraw;
	s_gameoptions.menu.key					= GameOptions_MenuKey;
	s_gameoptions.menu.fullscreen			= qtrue;
	s_gameoptions.menu.descX				= MENU_DESC_X;
	s_gameoptions.menu.descY				= MENU_DESC_Y;
	s_gameoptions.menu.titleX				= MENU_TITLE_X;
	s_gameoptions.menu.titleY				= MENU_TITLE_Y;
	s_gameoptions.menu.titleI				= MNT_CONTROLSMENU_TITLE;
	s_gameoptions.menu.footNoteEnum			= MNT_GAMEOPTION_LABEL;

	SetupMenu_TopButtons(&s_gameoptions.menu,MENU_GAME,NULL);

	inc = 24;
	x = 100;
	y = 170;
	width = 170;

	s_preferences.identifytarget.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.identifytarget.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.identifytarget.generic.x				= x;
	s_preferences.identifytarget.generic.y				= y;
	s_preferences.identifytarget.generic.callback		= Preferences_Event;
	s_preferences.identifytarget.generic.id				= ID_IDENTIFYTARGET;
	s_preferences.identifytarget.textEnum				= MBT_IDENTIFYTARGET1;
	s_preferences.identifytarget.textcolor				= CT_BLACK;
	s_preferences.identifytarget.textcolor2				= CT_WHITE;
	s_preferences.identifytarget.color					= CT_DKPURPLE1;
	s_preferences.identifytarget.color2					= CT_LTPURPLE1;
	s_preferences.identifytarget.textX					= MENU_BUTTON_TEXT_X;
	s_preferences.identifytarget.textY					= MENU_BUTTON_TEXT_Y;
	s_preferences.identifytarget.listnames				= s_OffOnNone_Names;
	s_preferences.identifytarget.width					= width;

	y += inc;
	s_preferences.forcemodel.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.forcemodel.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.forcemodel.generic.x				= x;
	s_preferences.forcemodel.generic.y				= y;
	s_preferences.forcemodel.generic.callback		= Preferences_Event;
	s_preferences.forcemodel.generic.id				= ID_FORCEMODEL;
	s_preferences.forcemodel.textEnum				= MBT_FORCEMODEL;
	s_preferences.forcemodel.textcolor				= CT_BLACK;
	s_preferences.forcemodel.textcolor2				= CT_WHITE;
	s_preferences.forcemodel.color					= CT_DKPURPLE1;
	s_preferences.forcemodel.color2					= CT_LTPURPLE1;
	s_preferences.forcemodel.textX					= MENU_BUTTON_TEXT_X;
	s_preferences.forcemodel.textY					= MENU_BUTTON_TEXT_Y;
	s_preferences.forcemodel.listnames				= s_OffOnNone_Names;
	s_preferences.forcemodel.width					= width;

	y += inc;
	s_preferences.drawteamoverlay.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.drawteamoverlay.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.drawteamoverlay.generic.x				= x;
	s_preferences.drawteamoverlay.generic.y				= y;
	s_preferences.drawteamoverlay.generic.callback		= Preferences_Event;
	s_preferences.drawteamoverlay.generic.id				= ID_DRAWTEAMOVERLAY;
	s_preferences.drawteamoverlay.textEnum				= MBT_DRAWTEAMOVERLAY;
	s_preferences.drawteamoverlay.textcolor				= CT_BLACK;
	s_preferences.drawteamoverlay.textcolor2				= CT_WHITE;
	s_preferences.drawteamoverlay.color					= CT_DKPURPLE1;
	s_preferences.drawteamoverlay.color2					= CT_LTPURPLE1;
	s_preferences.drawteamoverlay.textX					= MENU_BUTTON_TEXT_X;
	s_preferences.drawteamoverlay.textY					= MENU_BUTTON_TEXT_Y;
	s_preferences.drawteamoverlay.listnames				= teamoverlay_names;
	s_preferences.drawteamoverlay.width					= width;

	y += inc;
	s_preferences.drawtimer.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.drawtimer.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.drawtimer.generic.x				= x;
	s_preferences.drawtimer.generic.y				= y;
	s_preferences.drawtimer.generic.callback		= Preferences_Event;
	s_preferences.drawtimer.generic.id				= ID_DRAWTIMER;
	s_preferences.drawtimer.textEnum				= MBT_DRAW_TIMER;
	s_preferences.drawtimer.textcolor				= CT_BLACK;
	s_preferences.drawtimer.textcolor2				= CT_WHITE;
	s_preferences.drawtimer.color					= CT_DKPURPLE1;
	s_preferences.drawtimer.color2					= CT_LTPURPLE1;
	s_preferences.drawtimer.textX					= MENU_BUTTON_TEXT_X;
	s_preferences.drawtimer.textY					= MENU_BUTTON_TEXT_Y;
	s_preferences.drawtimer.listnames				= s_OffOnNone_Names;
	s_preferences.drawtimer.width					= width;

	y += inc;
	s_preferences.drawfps.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.drawfps.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.drawfps.generic.x				= x;
	s_preferences.drawfps.generic.y				= y;
	s_preferences.drawfps.generic.callback		= Preferences_Event;
	s_preferences.drawfps.generic.id			= ID_DRAWFPS;
	s_preferences.drawfps.textEnum				= MBT_DRAW_FPS;
	s_preferences.drawfps.textcolor				= CT_BLACK;
	s_preferences.drawfps.textcolor2			= CT_WHITE;
	s_preferences.drawfps.color					= CT_DKPURPLE1;
	s_preferences.drawfps.color2				= CT_LTPURPLE1;
	s_preferences.drawfps.textX					= MENU_BUTTON_TEXT_X;
	s_preferences.drawfps.textY					= MENU_BUTTON_TEXT_Y;
	s_preferences.drawfps.listnames				= s_OffOnNone_Names;
	s_preferences.drawfps.width					= width;

	y += inc;
	s_preferences.drawgun.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.drawgun.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.drawgun.generic.x				= x;
	s_preferences.drawgun.generic.y				= y;
	s_preferences.drawgun.generic.callback		= Preferences_Event;
	s_preferences.drawgun.generic.id			= ID_DRAWGUN;
	s_preferences.drawgun.textEnum				= MBT_DRAW_GUN;
	s_preferences.drawgun.textcolor				= CT_BLACK;
	s_preferences.drawgun.textcolor2			= CT_WHITE;
	s_preferences.drawgun.color					= CT_DKPURPLE1;
	s_preferences.drawgun.color2				= CT_LTPURPLE1;
	s_preferences.drawgun.textX					= MENU_BUTTON_TEXT_X;
	s_preferences.drawgun.textY					= MENU_BUTTON_TEXT_Y;
	s_preferences.drawgun.listnames				= s_OffOnNone_Names;
	s_preferences.drawgun.width					= width;

	y += inc;
	s_preferences.motionbob.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.motionbob.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.motionbob.generic.x			= x;
	s_preferences.motionbob.generic.y			= y;
	s_preferences.motionbob.generic.callback	= Preferences_Event;
	s_preferences.motionbob.generic.id			= ID_MOTIONBOB;
	s_preferences.motionbob.textEnum			= MBT_MOTION_BOBBING;
	s_preferences.motionbob.textcolor			= CT_BLACK;
	s_preferences.motionbob.textcolor2			= CT_WHITE;
	s_preferences.motionbob.color				= CT_DKPURPLE1;
	s_preferences.motionbob.color2				= CT_LTPURPLE1;
	s_preferences.motionbob.textX				= MENU_BUTTON_TEXT_X;
	s_preferences.motionbob.textY				= MENU_BUTTON_TEXT_Y;
	s_preferences.motionbob.listnames			= s_OffOnNone_Names;
	s_preferences.motionbob.width				= width;

	y += inc;
	s_preferences.allowdownload.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.allowdownload.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.allowdownload.generic.x				= x;
	s_preferences.allowdownload.generic.y				= y;
	s_preferences.allowdownload.generic.callback		= Preferences_Event;
	s_preferences.allowdownload.generic.id				= ID_ALLOWDOWNLOAD;
	s_preferences.allowdownload.textEnum				= MBT_ALLOWDOWNLOAD;
	s_preferences.allowdownload.textcolor				= CT_BLACK;
	s_preferences.allowdownload.textcolor2				= CT_WHITE;
	s_preferences.allowdownload.color					= CT_DKPURPLE1;
	s_preferences.allowdownload.color2					= CT_LTPURPLE1;
	s_preferences.allowdownload.textX					= MENU_BUTTON_TEXT_X;
	s_preferences.allowdownload.textY					= MENU_BUTTON_TEXT_Y;
	s_preferences.allowdownload.listnames				= s_OffOnNone_Names;
	s_preferences.allowdownload.width					= width;

	y += inc;
	s_preferences.textlanguage.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.textlanguage.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.textlanguage.generic.x			= x;
	s_preferences.textlanguage.generic.y			= y;
	s_preferences.textlanguage.generic.name			= "menu/common/bar1.tga";
	s_preferences.textlanguage.generic.callback		= Preferences_Event;
	s_preferences.textlanguage.generic.id			= ID_TEXTLANGUAGE;
	s_preferences.textlanguage.color				= CT_DKPURPLE1;
	s_preferences.textlanguage.color2				= CT_LTPURPLE1;
	s_preferences.textlanguage.textX				= MENU_BUTTON_TEXT_X;
	s_preferences.textlanguage.textY				= MENU_BUTTON_TEXT_Y;
	s_preferences.textlanguage.textEnum				= MBT_TEXTLANGUAGE;
	s_preferences.textlanguage.textcolor			= CT_BLACK;
	s_preferences.textlanguage.textcolor2			= CT_WHITE;
	s_preferences.textlanguage.listnames			= s_textlanguage_Names;
	s_preferences.textlanguage.width				= width;

	y += inc;
	s_preferences.voicelanguage.generic.type					= MTYPE_SPINCONTROL;
	s_preferences.voicelanguage.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.voicelanguage.generic.x					= x;
	s_preferences.voicelanguage.generic.y					= y;
	s_preferences.voicelanguage.generic.name					= "menu/common/bar1.tga";
	s_preferences.voicelanguage.generic.callback				= Preferences_Event;
	s_preferences.voicelanguage.generic.id					= ID_VOICELANGUAGE;
	s_preferences.voicelanguage.color						= CT_DKPURPLE1;
	s_preferences.voicelanguage.color2						= CT_LTPURPLE1;
	s_preferences.voicelanguage.textX						= MENU_BUTTON_TEXT_X;
	s_preferences.voicelanguage.textY						= MENU_BUTTON_TEXT_Y;
	s_preferences.voicelanguage.textEnum						= MBT_VOICELANGUAGE;
	s_preferences.voicelanguage.textcolor					= CT_BLACK;
	s_preferences.voicelanguage.textcolor2					= CT_WHITE;
	s_preferences.voicelanguage.listnames					= s_voicelanguage_Names;
	s_preferences.voicelanguage.width						= width;

	s_preferences.crosshair.generic.type				= MTYPE_BITMAP;
	s_preferences.crosshair.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_preferences.crosshair.generic.x					= CROSSHAIR_X - 34;
	s_preferences.crosshair.generic.y					= CROSSHAIR_Y - 46;
	s_preferences.crosshair.generic.name				= PIC_BUTTON2;
	s_preferences.crosshair.generic.id					= ID_CROSSHAIR;
	s_preferences.crosshair.generic.callback			= Preferences_Event;
	s_preferences.crosshair.width						= 100;
	s_preferences.crosshair.height						= 32;
	s_preferences.crosshair.color						= CT_DKBLUE1;
	s_preferences.crosshair.color2						= CT_LTBLUE1;
	s_preferences.crosshair.textX						= 20;
	s_preferences.crosshair.textY						= 1;
	s_preferences.crosshair.textEnum					= MBT_CROSSHAIR;
	s_preferences.crosshair.textcolor					= CT_BLACK;
	s_preferences.crosshair.textcolor2					= CT_WHITE;

	Preferences_SetMenuItems();

	Menu_AddItem( &s_gameoptions.menu, &s_preferences.identifytarget );
	Menu_AddItem( &s_gameoptions.menu, &s_preferences.forcemodel );
	Menu_AddItem( &s_gameoptions.menu, &s_preferences.drawteamoverlay );
	Menu_AddItem( &s_gameoptions.menu, &s_preferences.allowdownload );
	Menu_AddItem( &s_gameoptions.menu, &s_preferences.drawtimer );
	Menu_AddItem( &s_gameoptions.menu, &s_preferences.drawfps );
	Menu_AddItem( &s_gameoptions.menu, &s_preferences.drawgun );
	Menu_AddItem( &s_gameoptions.menu, &s_preferences.motionbob );
	Menu_AddItem( &s_gameoptions.menu, &s_preferences.textlanguage );
	Menu_AddItem( &s_gameoptions.menu, &s_preferences.voicelanguage );
	Menu_AddItem( &s_gameoptions.menu, &s_preferences.crosshair);

	s_gameoptions.menu.initialized = qtrue;		// Show we've been here
}

/*
=================
UI_GameOptionsMenu
=================
*/
void UI_GameOptionsMenu( void )
{

	GameOptions_MenuInit();

	UI_PushMenu( &s_gameoptions.menu );
}

