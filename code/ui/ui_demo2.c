// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

DEMOS MENU

=======================================================================
*/


#include "ui_local.h"

// Number of demos in the list
#define MAX_DEMODISP 12
#define BUTTON_SCROLL_SPEED MAX_DEMODISP
#define MOUSE_SCROLL_SPEED 3
#define MAX_DEMONAMELEN 48

#define MAX_DEMOS			1024
#define NAMEBUFSIZE			( MAX_DEMOS * 32 )

#define ID_MAINMENU				10
#define ID_ENGAGE				11
#define ID_LIST					12
#define ID_UP					13
#define ID_DOWN					14

#define ID_DEMOCOMMENT1			110
#define ID_DEMOCOMMENT2			111
#define ID_DEMOCOMMENT3			112
#define ID_DEMOCOMMENT4			113
#define ID_DEMOCOMMENT5			114
#define ID_DEMOCOMMENT6			115
#define ID_DEMOCOMMENT7			116
#define ID_DEMOCOMMENT8			117
#define ID_DEMOCOMMENT9			118
#define ID_DEMOCOMMENT10		119
#define ID_DEMOCOMMENT11		120
#define ID_DEMOCOMMENT12		121

#define PIC_UNDERLINE			"menu/common/underline.tga"

#define PIC_UPARROW				"menu/common/arrow_up_16.tga"
#define PIC_DNARROW				"menu/common/arrow_dn_16.tga"

typedef struct
{
	menuframework_s	menu;

	menubitmap_s	main;
	menubitmap_s	engage;
	menulist_s		list;
	menubitmap_s	upArrow;
	menubitmap_s	downArrow;

	qhandle_t		currentGameTopLeft;		// Upper left corner of current game box
	qhandle_t		currentGameBotLeft;		// Bottom left corner of current game box
	qhandle_t		currentGameTopRight;	// Upper right corner of current game box

	qhandle_t		directoryUpperCorner1;	// Left Upper corner of directory box
	qhandle_t		directoryLowerCorner1;	// Left Lower corner of directory box
	qhandle_t		directoryUpperCorner2;	// Right Upper corner of directory box

	menubitmap_s	currentFile;

	int				numDemos;
	char			*demolist[MAX_DEMOS];

	char			nameBuf[NAMEBUFSIZE];
	char			*nameBufPos;

	char			displaylist[MAX_DEMODISP][MAX_DEMONAMELEN];

	int				currentDemoIndex;
} demos_t;

static demos_t	s_demos;


menufield_s	s_demoline1;
menufield_s	s_demoline2;
menufield_s	s_demoline3;
menufield_s	s_demoline4;
menufield_s	s_demoline5;
menufield_s	s_demoline6;
menufield_s	s_demoline7;
menufield_s	s_demoline8;
menufield_s	s_demoline9;
menufield_s	s_demoline10;
menufield_s	s_demoline11;
menufield_s	s_demoline12;

static void* g_demoline[] =
{
	&s_demoline1,
	&s_demoline2,
	&s_demoline3,
	&s_demoline4,
	&s_demoline5,
	&s_demoline6,
	&s_demoline7,
	&s_demoline8,
	&s_demoline9,
	&s_demoline10,
	&s_demoline11,
	&s_demoline12,
	NULL,
};

/*
=================
DemoMenu_PopulateList
TiM: Fills the control
list with values from the array,
based off of an offset generated
by the scroll buttons
=================
*/
void DemoMenu_PopulateList ( int startingIndex ) {
	int	i;
	char*	demoName;

	if ( s_demos.numDemos == 0 ) {
		Q_strncpyz( s_demos.displaylist[0], menu_normal_text[MNT_NO_DEMOS_FOUND], sizeof( s_demos.displaylist[0] ) );
		((menubitmap_s *)g_demoline[0])->textPtr = s_demos.displaylist[0];
		((menubitmap_s *)g_demoline[0])->generic.flags = QMF_HIGHLIGHT_IF_FOCUS;
		return;
	}

	for(i = 0; g_demoline[i]; i++)
	{
		if (i >= s_demos.numDemos)
			break;

		demoName = s_demos.demolist[startingIndex+i];

		//TiM - Error trapping. Although this should never happen
		if ( !demoName ) {
			((menubitmap_s *)g_demoline[i])->generic.flags = QMF_INACTIVE|QMF_HIDDEN;
			continue;
		}

		COM_StripExtension(demoName, s_demos.displaylist[i], sizeof(s_demos.displaylist[i]));
		Q_strupr(s_demos.displaylist[i]);

		((menubitmap_s *)g_demoline[i])->textPtr = s_demos.displaylist[i];
		((menubitmap_s *)g_demoline[i])->generic.flags = QMF_HIGHLIGHT_IF_FOCUS;
	}
}

/*
=================
DemoMenu_Graphics
=================
*/
static void DemoMenu_Graphics (void)
{
	// Draw the basic screen layout
	UI_MenuFrame(&s_demos.menu);

	UI_DrawProportionalString(  74,  66, "88134-7",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  84, "56-0990",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  188, "3456",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  206, "7618",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  395, "692",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	trap_R_SetColor( colorTable[CT_DKBLUE2]);
	UI_DrawHandlePic(30,203,  47, 130, uis.whiteShader);	// Top left column square on bottom 2 3rds
	UI_DrawHandlePic(30,336,  47, 16, uis.whiteShader);	// Middle left column square on bottom 2 3rds
	UI_DrawHandlePic(30,355,  47, 34, uis.whiteShader);	// Bottom left column square on bottom 2 3rds

	// Numbers for left hand side
	UI_DrawProportionalString(  74,  206, "52662",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  339, "662",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  358, "101235",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	// Current game box
	trap_R_SetColor( colorTable[CT_DKPURPLE2]);
	UI_DrawHandlePic(130,64,  430, 3, uis.whiteShader);	// Top of current game line
	UI_DrawHandlePic(130,85,  430, 3, uis.whiteShader);	// Bottom of current game line

	UI_DrawHandlePic(113, 64,  32,	32, s_demos.currentGameTopLeft);	// Upper left corner of current game box
	UI_DrawHandlePic(113, 97,  32,	32, s_demos.currentGameBotLeft);	// Bottom left corner of current game box
	UI_DrawHandlePic(559, 64,  32,	32, s_demos.currentGameTopRight);	// Upper right corner of current game box
	UI_DrawHandlePic(552, 97,  -32,	32, s_demos.currentGameBotLeft);	// Bottom right corner of current game box

	UI_DrawHandlePic(113,91,  18, 6, uis.whiteShader);	// Left side of current game line
	UI_DrawHandlePic(566,91,  18, 6, uis.whiteShader);	// Right side of current game line

	UI_DrawHandlePic(138,101,  142, 18, uis.whiteShader);	// Lower bar to left side of 'engage'
	UI_DrawHandlePic(416,101,  143, 18, uis.whiteShader);	// Lower bar to right side of 'engage'

	// Available Demos
	trap_R_SetColor( colorTable[CT_DKPURPLE2]);
	UI_DrawHandlePic(189, 168,  32,	 32, s_demos.directoryUpperCorner1);	// Left Upper corner of directory box
	UI_DrawHandlePic(189, 421,  32,	 32, s_demos.directoryLowerCorner1);	// Left Lower corner of directory box
	UI_DrawHandlePic(481, 168,  32,	 32, s_demos.directoryUpperCorner2);	// Right Upper corner of directory box
	UI_DrawHandlePic(470, 421, -32,	 32, s_demos.directoryLowerCorner1);	// Right Lower corner of directory box

	UI_DrawHandlePic(205, 168,  277,  18, uis.whiteShader);			// Top bar
	UI_DrawHandlePic(189, 193,  16,  224, uis.whiteShader);			// Left column
	UI_DrawHandlePic(205, 424,  277,   8, uis.whiteShader);			// Bottom bar

	//TiM - Arrow Boxes
	UI_DrawHandlePic(485, 193,  16,  16, uis.whiteShader);		// Up Arrow
	UI_DrawHandlePic(485, 212,  16,  187, uis.whiteShader);		// Right column
	UI_DrawHandlePic(485, 402,  16,  16, uis.whiteShader);		// Down Arrow

	UI_DrawProportionalString(  124,  67, "67B",UI_TINYFONT, colorTable[CT_BLACK]);

	UI_DrawProportionalString( 220, 169, menu_normal_text[MNT_CURRENTDEMOSAVAILABLE], UI_SMALLFONT, colorTable[CT_BLACK]);
}

/*
=================
Demos_MenuDraw
=================
*/
static void Demos_MenuDraw (void)
{

	// Draw graphics particular to Demo Menu
	DemoMenu_Graphics();

	Menu_Draw( &s_demos.menu );
}

/*
=================
UI_DemosMenu_Scroll

Moves view up or down. Shift can be negative for number of lines up, or positive for number of lines down.
=================
*/
static void UI_DemosMenu_Scroll( int shift ) {
	s_demos.currentDemoIndex += shift;

	if ( s_demos.currentDemoIndex > s_demos.numDemos - MAX_DEMODISP ) {
		s_demos.currentDemoIndex = s_demos.numDemos - MAX_DEMODISP;
	}
	if ( s_demos.currentDemoIndex < 0 ) {
		s_demos.currentDemoIndex = 0;
	}

	DemoMenu_PopulateList( s_demos.currentDemoIndex );
}

/*
===============
Demos_MenuEvent
===============
*/
static void Demos_MenuEvent( void *ptr, int event )
{
	int index;

	if( event != QM_ACTIVATED )
	{
		return;
	}

	switch( ((menucommon_s*)ptr)->id )
	{
		case ID_UP:
			UI_DemosMenu_Scroll( -BUTTON_SCROLL_SPEED );
			break;

		case ID_DOWN:
			UI_DemosMenu_Scroll( BUTTON_SCROLL_SPEED );
			break;

		case ID_DEMOCOMMENT1:
		case ID_DEMOCOMMENT2:
		case ID_DEMOCOMMENT3:
		case ID_DEMOCOMMENT4:
		case ID_DEMOCOMMENT5:
		case ID_DEMOCOMMENT6:
		case ID_DEMOCOMMENT7:
		case ID_DEMOCOMMENT8:
		case ID_DEMOCOMMENT9:
		case ID_DEMOCOMMENT10:
		case ID_DEMOCOMMENT11:
		case ID_DEMOCOMMENT12:
			index = ((menucommon_s*)ptr)->id - ID_DEMOCOMMENT1 + s_demos.currentDemoIndex;
			if ( s_demos.demolist[index] )
			{
				s_demos.currentFile.textPtr = s_demos.demolist[index];
				//make it so
				s_demos.engage.generic.flags	= QMF_HIGHLIGHT_IF_FOCUS;
			}
			break;


		case ID_ENGAGE:
			UI_ForceMenuOff ();
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "demo %s\n", s_demos.currentFile.textPtr) );
			break;

		case ID_MAINMENU:
			UI_PopMenu();
			break;
	}
}


/*
=================
UI_DemosMenu_Key
=================
*/
static sfxHandle_t UI_DemosMenu_Key( int key )
{
	if ( key == K_MWHEELUP ) {
		UI_DemosMenu_Scroll( -MOUSE_SCROLL_SPEED );
		return menu_null_sound;
	}

	if ( key == K_MWHEELDOWN ) {
		UI_DemosMenu_Scroll( MOUSE_SCROLL_SPEED );
		return menu_null_sound;
	}

	return Menu_DefaultKey( &s_demos.menu, key );
}

/*
===============
UI_DemosMenu_Cache
===============
*/
void UI_DemosMenu_Cache( void )
{
	s_demos.currentGameTopLeft = trap_R_RegisterShaderNoMip("menu/common/corner_ul_18_24.tga");
	s_demos.currentGameBotLeft = trap_R_RegisterShaderNoMip("menu/common/corner_ll_18_18.tga");
	s_demos.currentGameTopRight = trap_R_RegisterShaderNoMip("menu/common/corner_ur_18_24.tga");

	s_demos.directoryUpperCorner1 = trap_R_RegisterShaderNoMip("menu/common/corner_ul_16_18.tga");
	s_demos.directoryLowerCorner1 = trap_R_RegisterShaderNoMip("menu/common/corner_ll_8_16.tga");
	s_demos.directoryUpperCorner2 = trap_R_RegisterShaderNoMip("menu/common/corner_ur_16_18.tga");

	trap_R_RegisterShaderNoMip(PIC_UPARROW);
	trap_R_RegisterShaderNoMip(PIC_DNARROW);

	trap_R_RegisterShaderNoMip(PIC_UNDERLINE);

}

/*
===============
Demos_SearchDemos

Searches for demo filenames matching extension and adds them to s_demos.demolist, with string
data stored in s_demos.nameBuf. Both s_demos.numDemos and s_demos.nameBufPos will be incremented
by this function.
===============
*/
static void Demos_AddDemos( const char *extension ) {
	int i;
	int count = 0;
	int bufAvailable = sizeof( s_demos.nameBuf ) - ( s_demos.nameBufPos - s_demos.nameBuf );

	if ( bufAvailable > 0 ) {
		count = trap_FS_GetFileList( "demos", extension, s_demos.nameBufPos, bufAvailable );
	}

	for ( i = 0; i < count; ++i ) {
		if ( s_demos.numDemos >= MAX_DEMOS ) {
			break;
		}

		s_demos.demolist[s_demos.numDemos] = s_demos.nameBufPos;
		s_demos.numDemos++;

		// advance name buffer pointer
		while ( *s_demos.nameBufPos ) {
			s_demos.nameBufPos++;
		}
		s_demos.nameBufPos++;
	}
}

/*
===============
Demos_MenuInit
===============
*/
static void Demos_MenuInit( void )
{
	int		i,x,y;

	memset( &s_demos, 0 ,sizeof(demos_t) );
	s_demos.menu.key = UI_DemosMenu_Key;

	UI_DemosMenu_Cache();

	s_demos.menu.fullscreen					= qtrue;
	s_demos.menu.wrapAround					= qtrue;
	s_demos.menu.draw						= Demos_MenuDraw;
	s_demos.menu.descX						= MENU_DESC_X;
	s_demos.menu.descY						= MENU_DESC_Y;
	s_demos.menu.titleX						= MENU_TITLE_X;
	s_demos.menu.titleY						= MENU_TITLE_Y;
	s_demos.menu.titleI						= MNT_DEMOS_TITLE;
	s_demos.menu.footNoteEnum				= MNT_DEMOS;

	s_demos.main.generic.type				= MTYPE_BITMAP;
	s_demos.main.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_demos.main.generic.x					= 482;
	s_demos.main.generic.y					= 136;
	s_demos.main.generic.name				= BUTTON_GRAPHIC_LONGRIGHT;
	s_demos.main.generic.id					= ID_MAINMENU;
	s_demos.main.generic.callback			= Demos_MenuEvent;
	s_demos.main.width						= MENU_BUTTON_MED_WIDTH;
	s_demos.main.height						= MENU_BUTTON_MED_HEIGHT;
	s_demos.main.color						= CT_DKPURPLE1;
	s_demos.main.color2						= CT_LTPURPLE1;
	s_demos.main.textX						= MENU_BUTTON_TEXT_X;
	s_demos.main.textY						= MENU_BUTTON_TEXT_Y;
	s_demos.main.textEnum					= MBT_MAINMENU;
	s_demos.main.textcolor					= CT_BLACK;
	s_demos.main.textcolor2					= CT_WHITE;

	s_demos.engage.generic.type				= MTYPE_BITMAP;
	s_demos.engage.generic.flags			= (QMF_INACTIVE|QMF_GRAYED);
	s_demos.engage.generic.x				= 283;
	s_demos.engage.generic.y				= 101;
	s_demos.engage.generic.name				= GRAPHIC_SQUARE;
	s_demos.engage.generic.id				= ID_ENGAGE;
	s_demos.engage.generic.callback			= Demos_MenuEvent;
	s_demos.engage.width					= MENU_BUTTON_MED_WIDTH;
	s_demos.engage.height					= MENU_BUTTON_MED_HEIGHT;
	s_demos.engage.color					= CT_DKPURPLE1;
	s_demos.engage.color2					= CT_LTPURPLE1;
	s_demos.engage.textX					= MENU_BUTTON_TEXT_X;
	s_demos.engage.textY					= MENU_BUTTON_TEXT_Y;
	s_demos.engage.textEnum					= MBT_ENGAGEDEMO;
	s_demos.engage.textcolor				= CT_BLACK;
	s_demos.engage.textcolor2				= CT_WHITE;

	s_demos.currentFile.generic.type		= MTYPE_BITMAP;
	s_demos.currentFile.generic.flags		= QMF_INACTIVE;
	s_demos.currentFile.generic.x			= 138;
	s_demos.currentFile.generic.y			= 68;
	s_demos.currentFile.generic.name		= BUTTON_GRAPHIC_LONGRIGHT;
	s_demos.currentFile.generic.id			= ID_MAINMENU;
	s_demos.currentFile.generic.callback	= Demos_MenuEvent;
	s_demos.currentFile.width				= 421;
	s_demos.currentFile.height				= MENU_BUTTON_MED_HEIGHT-1;
	s_demos.currentFile.color				= CT_BLACK;
	s_demos.currentFile.color2				= CT_BLACK;
	s_demos.currentFile.textX				= MENU_BUTTON_TEXT_X;
	s_demos.currentFile.textY				= MENU_BUTTON_TEXT_Y;
	s_demos.currentFile.textEnum			= MNT_NONE;
	s_demos.currentFile.textPtr				= NULL;
	s_demos.currentFile.textcolor			= CT_YELLOW;

	//TiM - Scroll Buttons
	s_demos.upArrow.generic.type			= MTYPE_BITMAP;
	s_demos.upArrow.generic.flags			= (QMF_INACTIVE|QMF_GRAYED); //Disabled by default
	s_demos.upArrow.generic.x				= 486;
	s_demos.upArrow.generic.y				= 195;
	s_demos.upArrow.generic.name			= PIC_UPARROW;
	s_demos.upArrow.generic.id				= ID_UP;
	s_demos.upArrow.generic.callback		= Demos_MenuEvent;
	s_demos.upArrow.width					= 14;
	s_demos.upArrow.height					= 14;
	s_demos.upArrow.color					= CT_DKGOLD1;
	s_demos.upArrow.color2					= CT_LTGOLD1;
	s_demos.upArrow.textcolor				= CT_BLACK;
	s_demos.upArrow.textcolor2				= CT_WHITE;

	s_demos.downArrow.generic.type			= MTYPE_BITMAP;
	s_demos.downArrow.generic.flags			= (QMF_INACTIVE|QMF_GRAYED);
	s_demos.downArrow.generic.x				= 486;
	s_demos.downArrow.generic.y				= 404;
	s_demos.downArrow.generic.name			= PIC_DNARROW;
	s_demos.downArrow.generic.id			= ID_DOWN;
	s_demos.downArrow.generic.callback		= Demos_MenuEvent;
	s_demos.downArrow.width					= 14;
	s_demos.downArrow.height				= 14;
	s_demos.downArrow.color					= CT_DKGOLD1;
	s_demos.downArrow.color2				= CT_LTGOLD1;
	s_demos.downArrow.textcolor				= CT_BLACK;
	s_demos.downArrow.textcolor2			= CT_WHITE;

	// Currently just list all demo types, regardless of client support
	// Most clients will provide an error message if unsupported demo is attempted to be played
	s_demos.numDemos = 0;
	s_demos.nameBufPos = s_demos.nameBuf;
	Demos_AddDemos( ".dm_26" );
	Demos_AddDemos( ".dm_25" );
	Demos_AddDemos( ".efdemo" );

	x = 225;
	y = 200;
	for(i = 0; g_demoline[i]; i++)
	{
		((menubitmap_s *)g_demoline[i])->generic.type			= MTYPE_BITMAP;
		((menubitmap_s *)g_demoline[i])->generic.flags			= QMF_INACTIVE | QMF_HIDDEN;
		((menubitmap_s *)g_demoline[i])->generic.x				= x;
		((menubitmap_s *)g_demoline[i])->generic.y				= y;
		((menubitmap_s *)g_demoline[i])->generic.name			= PIC_UNDERLINE;
		((menubitmap_s *)g_demoline[i])->generic.callback		= Demos_MenuEvent;
		((menubitmap_s *)g_demoline[i])->generic.id				= ID_DEMOCOMMENT1+i;
		((menubitmap_s *)g_demoline[i])->width					= 240;
		((menubitmap_s *)g_demoline[i])->height					= 16;
		((menubitmap_s *)g_demoline[i])->color					= CT_DKPURPLE1;
		((menubitmap_s *)g_demoline[i])->color2					= CT_LTPURPLE1;
		((menubitmap_s *)g_demoline[i])->textPtr				= NULL;
		((menubitmap_s *)g_demoline[i])->textX					= 4;
		((menubitmap_s *)g_demoline[i])->textY					= 1;
		((menubitmap_s *)g_demoline[i])->textcolor				= CT_DKGOLD1;
		((menubitmap_s *)g_demoline[i])->textcolor2				= CT_LTGOLD1;
		((menubitmap_s *)g_demoline[i])->textStyle				= UI_TINYFONT;
//		((menubitmap_s *)g_demoline[i])->textEnum				= MBT_DEMOLINE;

		Menu_AddItem( &s_demos.menu, ( void * )g_demoline[i]);
		y += 18;
	}

	//TiM - If the list is longer than we can fit, enable the scroll buttons
	if ( s_demos.numDemos > MAX_DEMODISP ) {
		s_demos.upArrow.generic.flags = QMF_HIGHLIGHT_IF_FOCUS;
		s_demos.downArrow.generic.flags = QMF_HIGHLIGHT_IF_FOCUS;
	}

	//Populate the controls with the values from the array
	DemoMenu_PopulateList( s_demos.currentDemoIndex );

	Menu_AddItem( &s_demos.menu, &s_demos.main );
	Menu_AddItem( &s_demos.menu, &s_demos.engage );
	Menu_AddItem( &s_demos.menu, &s_demos.currentFile );
	Menu_AddItem( &s_demos.menu, &s_demos.upArrow );
	Menu_AddItem( &s_demos.menu, &s_demos.downArrow );
}


/*
===============
UI_DemosMenu
===============
*/
void UI_DemosMenu( void )
{
	Demos_MenuInit();
	UI_PushMenu( &s_demos.menu );
}
