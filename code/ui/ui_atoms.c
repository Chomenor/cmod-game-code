// Copyright (C) 1999-2000 Id Software, Inc.
//
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"
#include "stv_version.h"

uiStatic_t		uis;
qboolean		m_entersound;		// after a frame, so caching won't disrupt the sound
static void UI_LanguageFilename(char *baseName,char *baseExtension,char *finalName);
void BG_LoadItemNames(void);

// these are here so the functions in q_shared.c can link
#ifndef UI_HARD_LINKED

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	trap_Error( va("%s", text) );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	trap_Print( va("%s", text) );
}

#endif


typedef struct
{
	int				initialized;	// Has this structure been initialized
	qhandle_t		cornerUpper;
	qhandle_t		cornerUpper2;
	qhandle_t		cornerLower;
} menuframe_t;

static menuframe_t s_menuframe;

const char menuEmptyLine[] = " ";
/*
=================
UI_ClampCvar
=================
*/
float UI_ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}


/*
=================
UI_PushMenu
=================
*/
void UI_PushMenu( menuframework_s *menu )
{
	int				i;
	menucommon_s*	item;

	// avoid stacking menus invoked by hotkeys
	for (i=0 ; i<uis.menusp ; i++)
	{
		if (uis.stack[i] == menu)
		{
			uis.menusp = i;
			break;
		}
	}

	if (i == uis.menusp)
	{
		if (uis.menusp >= MAX_MENUDEPTH)
			trap_Error("UI_PushMenu: menu stack overflow");

		uis.stack[uis.menusp++] = menu;
	}

	uis.activemenu = menu;

	// default cursor position
	menu->cursor      = 0;
	menu->cursor_prev = 0;

	m_entersound = qtrue;

	trap_Key_SetCatcher( KEYCATCH_UI );

	// force first available item to have focus
	for (i=0; i<menu->nitems; i++)
	{
		item = (menucommon_s *)menu->items[i];
		if (!(item->flags & (QMF_GRAYED|QMF_MOUSEONLY|QMF_INACTIVE)))
		{
			menu->cursor_prev = -1;
			Menu_SetCursor( menu, i );
			break;
		}
	}

	uis.firstdraw = qtrue;
}

/*
=================
UI_PopMenu
=================
*/
void UI_PopMenu (void)
{
	trap_S_StartLocalSound( menu_out_sound, CHAN_LOCAL_SOUND );

	uis.menusp--;

	if (uis.menusp < 0)
		trap_Error ("UI_PopMenu: menu stack underflow");

	if (uis.menusp) {
		uis.activemenu = uis.stack[uis.menusp-1];
		uis.firstdraw = qtrue;
	}
	else {
		UI_ForceMenuOff ();
	}
}

void UI_ForceMenuOff (void)
{
	uis.menusp     = 0;
	uis.activemenu = NULL;

	trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
	trap_Key_ClearStates();
	trap_Cvar_Set( "cl_paused", "0" );
}

/*
=================
UI_LerpColor
=================
*/
static void UI_LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for (i=0; i<4; i++)
	{
		c[i] = a[i] + t*(b[i]-a[i]);
		if (c[i] < 0)
			c[i] = 0;
		else if (c[i] > 1.0)
			c[i] = 1.0;
	}
}


/*
=================
UI_DrawProportionalString2
=================
*/
#define CHARMAX 256
#define PROPB_GAP_WIDTH		4
#define PROPB_SPACE_WIDTH	12
#define PROPB_HEIGHT		36

static int	propMapBig[CHARMAX][3];
static int	propMap[CHARMAX][3];
static int	propMapTiny[CHARMAX][3];

static int const propMapB[26][3] = {
{11, 12, 33},
{49, 12, 31},
{85, 12, 31},
{120, 12, 30},
{156, 12, 21},
{183, 12, 21},
{207, 12, 32},

{13, 55, 30},
{49, 55, 13},
{66, 55, 29},
{101, 55, 31},
{135, 55, 21},
{158, 55, 40},
{204, 55, 32},

{12, 97, 31},
{48, 97, 31},
{82, 97, 30},
{118, 97, 30},
{153, 97, 30},
{185, 97, 25},
{213, 97, 30},

{11, 139, 32},
{42, 139, 51},
{93, 139, 32},
{126, 139, 31},
{158, 139, 25},
};

/*
=================
UI_DrawBannerString
=================
*/
static void UI_DrawBannerString2( int x, int y, const char* str, vec4_t color )
{
	const char* s;
	char	ch;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;

	// draw the colored text
	trap_R_SetColor( color );

//	ax = x * uis.scale + uis.bias;
	ax = x * uis.scalex;
	ay = y * uis.scaley;

	s = str;
	while ( *s )
	{
		ch = *s & 255;
		if ( ch == ' ' ) {
			ax += ((float)PROPB_SPACE_WIDTH + (float)PROPB_GAP_WIDTH)* uis.scalex;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			ch -= 'A';
			fcol = (float)propMapB[ch][0] / 256.0f;
			frow = (float)propMapB[ch][1] / 256.0f;
			fwidth = (float)propMapB[ch][2] / 256.0f;
			fheight = (float)PROPB_HEIGHT / 256.0f;
			aw = (float)propMapB[ch][2] * uis.scalex;
			ah = (float)PROPB_HEIGHT * uis.scaley;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, uis.charsetPropB );
			ax += (aw + (float)PROPB_GAP_WIDTH * uis.scalex);
		}
		s++;
	}

	trap_R_SetColor( NULL );
}

/*
=================
UI_DrawBannerString
=================
*/
void UI_DrawBannerString( int x, int y, const char* str, int style, vec4_t color ) {
	const char *	s;
	int				ch;
	int				width;
	vec4_t			drawcolor;

	// find the width of the drawn text
	s = str;
	width = 0;
	while ( *s ) {
		ch = *s;
		if ( ch == ' ' ) {
			width += PROPB_SPACE_WIDTH;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			width += propMapB[ch - 'A'][2] + PROPB_GAP_WIDTH;
		}
		s++;
	}
	width -= PROPB_GAP_WIDTH;

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			x -= width / 2;
			break;

		case UI_RIGHT:
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawBannerString2( x+2, y+2, str, drawcolor );
	}

	UI_DrawBannerString2( x, y, str, color );
}


/*
=================
UI_ProportionalStringWidth
=================
*/
int UI_ProportionalStringWidth( const char* str,int style ) {
	const char *	s;
	int				ch;
	int				charWidth;
	int				width;


	if (style == UI_TINYFONT)
	{
		s = str;
		width = 0;
		while ( *s ) {
			ch = *s & 255;
			charWidth = propMapTiny[ch][2];
			if ( charWidth != -1 ) {
				width += charWidth;
				width += PROP_GAP_TINY_WIDTH;
			}
			s++;
		}

		width -= PROP_GAP_TINY_WIDTH;
	}
	else if (style == UI_BIGFONT)
	{
		s = str;
		width = 0;
		while ( *s ) {
			ch = *s & 255;
			charWidth = propMapBig[ch][2];
			if ( charWidth != -1 ) {
				width += charWidth;
				width += PROP_GAP_BIG_WIDTH;
			}
			s++;
		}

		width -= PROP_GAP_BIG_WIDTH;
	}
	else
	{
		s = str;
		width = 0;
		while ( *s ) {
			ch = *s & 255;
			charWidth = propMap[ch][2];
			if ( charWidth != -1 ) {
				width += charWidth;
				width += PROP_GAP_WIDTH;
			}
			s++;
		}
		width -= PROP_GAP_WIDTH;
	}

	return width;
}

static int specialTinyPropChars[CHARMAX][2] = {
{0, 0},
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 10
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 20
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 30
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 40
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 50
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 60
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 70
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 80
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 90
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 100
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 110
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 120
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 130
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 140
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 150
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{2,-3},{0, 0},	// 160
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 170
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 180
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 190
{0,-1},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{0, 0},{2, 0},{2,-3},	// 200
{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{0,-1},{2,-3},{2,-3},	// 210
{2,-3},{3,-3},{2,-3},{2,-3},{0, 0},{0,-1},{2,-3},{2,-3},{2,-3},{2,-3},	// 220
{2,-3},{0,-1},{0,-1},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{0, 0},	// 230
{2, 0},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{0, 0},	// 240
{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{2,-3},{0, 0},{0,-1},{2,-3},{2,-3},	// 250
{2,-3},{2,-3},{2,-3},{0,-1},{2,-3}										// 255
};


static int specialPropChars[CHARMAX][2] = {
{0, 0},
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 10
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 20
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 30
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 40
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 50
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 60
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 70
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 80
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 90
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 100
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 110
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 120
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 130
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 140
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 150
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 160
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 170
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 180
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 190
{2,-2},{2,-2},{2,-2},{2,-2},{2,-2},{2,-2},{2,-2},{0, 0},{1, 1},{2,-2},	// 200
{2,-2},{2,-2},{2,-2},{2,-2},{2,-2},{2,-2},{2,-2},{0, 0},{2,-2},{2,-2},	// 210
{2,-2},{2,-2},{2,-2},{2,-2},{0, 0},{0, 0},{2,-2},{2,-2},{2,-2},{2,-2},	// 220
{2,-2},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 230
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 240
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 250
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0}										// 255
};


static int specialBigPropChars[CHARMAX][2] = {
{0, 0},
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 10
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 20
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 30
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 40
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 50
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 60
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 70
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 80
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 90
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 100
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 110
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 120
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 130
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 140
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 150
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 160
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 170
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 180
{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},	// 190
{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{0, 0},{3, 1},{3,-3},	// 200
{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{0, 0},{3,-3},{3,-3},	// 210
{3,-3},{3,-3},{3,-3},{3,-3},{0, 0},{0, 0},{3,-3},{3,-3},{3,-3},{3,-3},	// 220
{3,-3},{0, 0},{0, 0},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{0, 0},	// 230
{3, 1},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{0, 0},	// 240
{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{3,-3},{0, 0},{0, 0},{3,-3},{3,-3},	// 250
{3,-3},{3,-3},{3,-3},{0, 0},{3,-3}										// 255
};


/*
=================
UI_DrawProportionalString2
=================
*/
static void UI_DrawProportionalString2( int x, int y, const char* str, vec4_t color, int style, qhandle_t charset )
{
	const char* s;
	unsigned char ch;
	float	ax;
	float	ay,holdY;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;
	float	sizeScale;
	int		colorI;
	int		special;

	// draw the colored text
	trap_R_SetColor( color );

//	ax = x * uis.scale + uis.bias;
	ax = x * uis.scalex;
	ay = y * uis.scaley;
	holdY = ay;

	sizeScale = UI_ProportionalSizeScale( style );

	if (style & UI_TINYFONT)
	{
		s = str;
		while ( *s )
		{
			// Is this a color????
			if ( Q_IsColorString( s ) )
			{
				colorI = ColorIndex( *(s+1) );
				trap_R_SetColor( g_color_table[colorI] );
				s += 2;
				continue;
			}

			ch = *s & 255;
			if ( ch == ' ' ) {
				aw = (float)PROP_SPACE_TINY_WIDTH;
			}
			else if ( propMap[ch][2] != -1 ) {
				// Because some foreign characters were a little different
				special = specialTinyPropChars[ch][0];
				ay = holdY + (specialTinyPropChars[ch][1] * uis.scaley);

				fcol = (float ) propMapTiny[ch][0] / 256.0f;
				frow = (float)propMapTiny[ch][1] / 256.0f;
				fwidth = (float)propMapTiny[ch][2] / 256.0f;
				fheight = (float)(PROP_TINY_HEIGHT + special) / 256.0f;
				aw = (float)propMapTiny[ch][2] * uis.scalex * sizeScale;
				ah = (float)(PROP_TINY_HEIGHT+ special) * uis.scaley * sizeScale;

				trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, charset );

			}
			else
			{
				aw = 0;
			}

			ax += (aw + (float)PROP_GAP_TINY_WIDTH * uis.scalex * sizeScale);
			s++;
		}
	}
	else if (style & UI_BIGFONT)
	{
		s = str;
		while ( *s )
		{
			// Is this a color????
			if ( Q_IsColorString( s ) )
			{
				colorI = ColorIndex( *(s+1) );
				trap_R_SetColor( g_color_table[colorI] );
				s += 2;
				continue;
			}

			ch = *s & 255;
			if ( ch == ' ' ) {
				aw = (float)PROP_SPACE_BIG_WIDTH * uis.scalex;
			}
			else if ( propMap[ch][2] != -1 ) {
				// Because some foreign characters were a little different
				special = specialBigPropChars[ch][0];
				ay = holdY + (specialBigPropChars[ch][1] * uis.scaley);

				fcol = (float ) propMapBig[ch][0] / 256.0f;
				frow = (float)propMapBig[ch][1] / 256.0f;
				fwidth = (float)propMapBig[ch][2] / 256.0f;
				fheight = (float)(PROP_BIG_HEIGHT+ special) / 256.0f;
				aw = (float)propMapBig[ch][2] * uis.scalex * sizeScale;
				ah = (float)(PROP_BIG_HEIGHT+ special) * uis.scaley * sizeScale;

				trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, charset );
			}
			else
			{
				aw = 0;
			}

			ax += (aw + (float)PROP_GAP_BIG_WIDTH * uis.scalex * sizeScale);
			s++;
		}
	}
	else
	{
		s = str;
		while ( *s )
		{
			// Is this a color????
			if ( Q_IsColorString( s ) )
			{
				colorI = ColorIndex( *(s+1) );
				trap_R_SetColor( g_color_table[colorI] );
				s += 2;
				continue;
			}

			ch = *s & 255;
			if ( ch == ' ' ) {
				aw = (float)PROP_SPACE_WIDTH * uis.scalex * sizeScale;
			}
			else if ( propMap[ch][2] != -1 ) {
				// Because some foreign characters were a little different
				special = specialPropChars[ch][0];
				ay = holdY + (specialPropChars[ch][1] * uis.scaley);

				fcol = (float)propMap[ch][0] / 256.0f;
				frow = (float)propMap[ch][1] / 256.0f;
				fwidth = (float)propMap[ch][2] / 256.0f;
				fheight = (float)(PROP_HEIGHT+ special) / 256.0f;
				aw = (float)propMap[ch][2] * uis.scalex * sizeScale;
				ah = (float)(PROP_HEIGHT+ special) * uis.scaley * sizeScale;
				trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, charset );
			}
			else
			{
				aw = 0;
			}

			ax += (aw + (float)PROP_GAP_WIDTH * uis.scalex * sizeScale);
			s++;
		}
	}

	trap_R_SetColor( NULL );
}

/*
=================
UI_ProportionalSizeScale
=================
*/
float UI_ProportionalSizeScale( int style ) {
	if(  style & UI_SMALLFONT ) {
		return PROP_SMALL_SIZE_SCALE;
	}

	return 1.00;
}


/*
=================
UI_DrawProportionalString
=================
*/
void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color ) {
	vec4_t	drawcolor;
	int		width;
	float	sizeScale;
	int		charstyle=0;

	if ((style & UI_BLINK) && ((uis.realtime/BLINK_DIVISOR) & 1))
		return;

	// Get char style
	if (style & UI_TINYFONT)
	{
		charstyle = UI_TINYFONT;
	}
	else if (style & UI_SMALLFONT)
	{
		charstyle = UI_SMALLFONT;
	}
	else if (style & UI_BIGFONT)
	{
		charstyle = UI_BIGFONT;
	}
	else if (style & UI_GIANTFONT)
	{
		charstyle = UI_GIANTFONT;
	}
	else	// Just in case
	{
		charstyle = UI_SMALLFONT;
	}



	sizeScale = UI_ProportionalSizeScale( style );

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			width = UI_ProportionalStringWidth( str,charstyle) * sizeScale;
			x -= width / 2;
			break;

		case UI_RIGHT:
			width = UI_ProportionalStringWidth( str,charstyle) * sizeScale;
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x+2, y+2, str, drawcolor, sizeScale, uis.charsetProp );
	}

	if ( style & UI_INVERSE ) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, uis.charsetProp );
		return;
	}

	if ( style & UI_PULSE ) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, color, sizeScale, uis.charsetProp );

		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
		drawcolor[3] = 0.5 + 0.5 * sin( uis.realtime / PULSE_DIVISOR );
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, uis.charsetProp );
		return;
	}


	if (style & UI_TINYFONT)
	{
		UI_DrawProportionalString2( x, y, str, color, charstyle, uis.charsetPropTiny );
	}
	else if (style & UI_BIGFONT)
	{
		UI_DrawProportionalString2( x, y, str, color, charstyle, uis.charsetPropBig );
	}
	else
	{
		UI_DrawProportionalString2( x, y, str, color, charstyle, uis.charsetProp );
	}
}

static int showColorChars;

/*
=================
UI_DrawString2
=================
*/
static void UI_DrawString2( int x, int y, const char* str, vec4_t color, int charw, int charh )
{
	const char* s;
	char	ch;
	int forceColor = qfalse; //APSFIXME;
	vec4_t	tempcolor;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;

	if (y < -charh)
		// offscreen
		return;

	// draw the colored text
	trap_R_SetColor( color );

//	ax = x * uis.scale + uis.bias;
	ax = x * uis.scalex;
	ay = y * uis.scaley;
	aw = charw * uis.scalex;
	ah = charh * uis.scaley;

	s = str;
	while ( *s )
	{
		if (!showColorChars)
		{
			if ( Q_IsColorString( s ) )
			{
				if ( !forceColor )
				{
					memcpy( tempcolor, g_color_table[ColorIndex(s[1])], sizeof( tempcolor ) );
					tempcolor[3] = color[3];
					trap_R_SetColor( tempcolor );
				}
				s += 2;
				continue;
			}
		}

		ch = *s & 255;

		if (ch != ' ')
		{
//			frow = (ch>>4)*0.0625;
//			fcol = (ch&15)*0.0625;
//			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol + 0.0625, frow + 0.0625, uis.charset );

			frow = (ch>>4)*0.0625;
			fcol = (ch&15)*0.0625;

			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol + 0.03125, frow + 0.0625, uis.charset );

		}

		ax += aw;
		s++;
	}

	trap_R_SetColor( NULL );
}


/*
=================
UI_DrawString
=================
*/
void UI_DrawString( int x, int y, const char* str, int style, vec4_t color )
{
	int		len;
	int		charw;
	int		charh;
	vec4_t	newcolor;
	vec4_t	lowlight;
	float	*drawcolor;
	vec4_t	dropcolor;

	if( !str ) {
		return;
	}

	if ((style & UI_BLINK) && ((uis.realtime/BLINK_DIVISOR) & 1))
		return;

	if (style & UI_TINYFONT)
	{
		charw =	TINYCHAR_WIDTH;
		charh =	TINYCHAR_HEIGHT;
	}
	else if (style & UI_BIGFONT)
	{
		charw =	BIGCHAR_WIDTH;
		charh =	BIGCHAR_HEIGHT;
	}
	else if (style & UI_GIANTFONT)
	{
		charw =	GIANTCHAR_WIDTH;
		charh =	GIANTCHAR_HEIGHT;
	}
	else
	{
		charw =	SMALLCHAR_WIDTH;
		charh =	SMALLCHAR_HEIGHT;
	}

	if (style & UI_PULSE)
	{
		lowlight[0] = 0.8*color[0];
		lowlight[1] = 0.8*color[1];
		lowlight[2] = 0.8*color[2];
		lowlight[3] = 0.8*color[3];
		UI_LerpColor(color,lowlight,newcolor,0.5+0.5*sin(uis.realtime/PULSE_DIVISOR));
		drawcolor = newcolor;
	}
	else
		drawcolor = color;

	switch (style & UI_FORMATMASK)
	{
		case UI_CENTER:
			// center justify at x
			len = strlen(str);
			x   = x - len*charw/2;
			break;

		case UI_RIGHT:
			// right justify at x
			len = strlen(str);
			x   = x - len*charw;
			break;

		default:
			// left justify at x
			break;
	}

	if (style & UI_SHOWCOLOR)
	{
		showColorChars = qtrue;
	}
	else
	{
		showColorChars = qfalse;
	}

	if ( style & UI_DROPSHADOW )
	{
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
		dropcolor[3] = drawcolor[3];
		UI_DrawString2(x+2,y+2,str,dropcolor,charw,charh);
	}

	UI_DrawString2(x,y,str,drawcolor,charw,charh);
}

/*
=================
UI_DrawChar
=================
*/
void UI_DrawChar( int x, int y, int ch, int style, vec4_t color )
{
	char	buff[2];

	buff[0] = ch;
	buff[1] = '\0';

	UI_DrawString( x, y, buff, style, color );
}

qboolean UI_IsFullscreen( void ) {
	if ( uis.activemenu && ( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
		return uis.activemenu->fullscreen;
	}

	return qfalse;
}

static void NeedCDAction( qboolean result ) {
	if ( !result ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	}
}

void UI_SetActiveMenu( uiMenuCommand_t menu ) {
	// this should be the ONLY way the menu system is brought up, except for UI_ConsoleCommand below
	// enusure minumum menu data is cached
	Menu_Cache();

	switch ( menu ) {
	case UIMENU_NONE:
		UI_ForceMenuOff();
		return;
	case UIMENU_MAIN:
		UI_MainMenu();
		return;
	case UIMENU_NEED_CD:
		UI_ConfirmMenu( menu_normal_text[MNT_INSERTCD], 0, NeedCDAction );
		return;
	case UIMENU_INGAME:
		trap_Cvar_Set( "cl_paused", "1" );
		UI_InGameMenu();
		return;
	}
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key ) {
	sfxHandle_t		s;

	if (!uis.activemenu) {
		return;
	}

	if (uis.activemenu->key)
		s = uis.activemenu->key( key );
	else
		s = Menu_DefaultKey( uis.activemenu, key );

	if ((s > 0) && (s != menu_null_sound))
		trap_S_StartLocalSound( s, CHAN_LOCAL_SOUND );
}

/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent( int dx, int dy )
{
	int				i;
	menucommon_s*	m;

	if (!uis.activemenu)
		return;

	// update mouse screen position
	uis.cursorx += dx;
	if (uis.cursorx < 0)
		uis.cursorx = 0;
	else if (uis.cursorx > SCREEN_WIDTH)
		uis.cursorx = SCREEN_WIDTH;

	uis.cursory += dy;
	if (uis.cursory < 0)
		uis.cursory = 0;
	else if (uis.cursory > SCREEN_HEIGHT)
		uis.cursory = SCREEN_HEIGHT;

	// region test the active menu items
	for (i=0; i<uis.activemenu->nitems; i++)
	{
		m = (menucommon_s*)uis.activemenu->items[i];

		if (m->flags & (QMF_GRAYED|QMF_INACTIVE))
			continue;

		if ((uis.cursorx < m->left) ||
			(uis.cursorx > m->right) ||
			(uis.cursory < m->top) ||
			(uis.cursory > m->bottom))
		{
			// cursor out of item bounds
			continue;
		}

		// set focus to item at cursor
		if (uis.activemenu->cursor != i)
		{
			Menu_SetCursor( uis.activemenu, i );
			((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor_prev]))->flags &= ~QMF_HASMOUSEFOCUS;

			if ( !(((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags & QMF_SILENT ) ) {
				trap_S_StartLocalSound( menu_move_sound, CHAN_LOCAL_SOUND );
			}
		}

		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags |= QMF_HASMOUSEFOCUS;
		return;
	}

	if (uis.activemenu->nitems > 0) {
		// out of any region
		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;
	}
}

char *UI_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


char *UI_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}


/*
=================
UI_Cache
=================
*/
static void UI_Cache_f( void ) {
	MainMenu_Cache();
	InGame_Cache();
	ConfirmMenu_Cache();
	PlayerModel_Cache();
	PlayerSettings_Cache();
//	Preferences_Cache();
	ServerInfo_Cache();
	SpecifyServer_Cache();
	ArenaServers_Cache();
	StartServer_Cache();
	ServerOptions_Cache();
	DriverInfo_Cache();
//	GraphicsOptions_Cache();
//	UI_DisplayOptionsMenu_Cache();
//	UI_SoundOptionsMenu_Cache();
	UI_NetworkOptionsMenu_Cache();
	UI_SPLevelMenu_Cache();
	UI_SPSkillMenu_Cache();
	UI_SPPostgameMenu_Cache();
	TeamMain_Cache();
	UI_AddBots_Cache();
	UI_RemoveBots_Cache();
//	UI_LoadConfig_Cache();
//	UI_SaveConfigMenu_Cache();
	UI_BotSelectMenu_Cache();
	UI_CDKeyMenu_Cache();
	UI_ModsMenu_Cache();
	UI_SoundMenu_Cache();
	UI_QuitMenu_Cache();
	UI_DemosMenu_Cache();
	UI_VideoDataMenu_Cache();
	UI_GameOptionsMenu_Cache();
	UI_ControlsMouseJoyStickMenu_Cache();
	UI_ResetGameMenu_Cache();
	UI_VideoData2Menu_Cache();
	UI_VideoDriverMenu_Cache();
	UI_HolomatchInMenu_Cache();
	UI_ChooseServerTypeMenu_Cache();
}


/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand( void ) {
	char	*cmd;

	cmd = UI_Argv( 0 );

	// ensure minimum menu data is available
	Menu_Cache();

	if ( Q_stricmp (cmd, "levelselect") == 0 ) {
		UI_SPLevelMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "postgame") == 0 ) {
		UI_SPPostgameMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_cache") == 0 ) {
		UI_Cache_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_teamOrders") == 0 ) {
		UI_TeamOrdersMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_cdkey") == 0 ) {
		UI_CDKeyMenu_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "err_dialog") == 0 ) {
		UI_ConfirmMenu( UI_Argv( 1 ), 0, 0);
		return qtrue;
	}

	return qfalse;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void ) {
}



//--------------------------------------------
static char *UI_ParseFontParms(char *buffer,int	propArray[CHARMAX][3])
{
	char	*token;
	int		i,i2;

	while ( buffer )
	{
		token = COM_ParseExt( &buffer, qtrue );

		// Start with open braket
		if ( !Q_stricmp( token, "{" ) )
		{
			for (i=0;i<CHARMAX;++i)
			{
				// Brackets for the numbers
				token = COM_ParseExt( &buffer, qtrue );
				if ( !Q_stricmp( token, "{" ) )
				{
					;
				}
				else
				{
					trap_Print( va( S_COLOR_RED "UI_ParseFontParms : Invalid FONTS.DAT data, near character %d!\n",i));
					return(NULL);
				}

				for (i2=0;i2<3;++i2)
				{
					token = COM_ParseExt( &buffer, qtrue );
					propArray[i][i2] = atoi(token);
				}

				token = COM_ParseExt( &buffer, qtrue );
				if ( !Q_stricmp( token, "}" ) )
				{
					;
				}
				else
				{
					trap_Print( va( S_COLOR_RED "UI_ParseFontParms : Invalid FONTS.DAT data, near character %d!\n",i));
					return(NULL);
				}
			}
		}

		token = COM_ParseExt( &buffer, qtrue );	// Grab closing bracket
		if ( !Q_stricmp( token, "}" ) )
		{
			break;
		}
	}

	return(buffer);
}

#define FONT_BUFF_LENGTH 20000

/*
=================
UI_LoadFonts
=================
*/
void UI_LoadFonts( void )
{
	char buffer[FONT_BUFF_LENGTH];
	int len;
	fileHandle_t	f;
	char *holdBuf;

	len = trap_FS_FOpenFile( "ext_data/fonts.dat", &f, FS_READ );

	if ( !f )
	{
		trap_Print( va( S_COLOR_RED "UI_LoadFonts : FONTS.DAT file not found!\n"));
		return;
	}

	if (len > FONT_BUFF_LENGTH)
	{
		trap_Print( va( S_COLOR_RED "UI_LoadFonts : FONTS.DAT file bigger than %d!\n",FONT_BUFF_LENGTH));
		return;
	}

	// initialise the data area
	memset(buffer, 0, sizeof(buffer));

	trap_FS_Read( buffer, len, f );

	trap_FS_FCloseFile( f );

	COM_BeginParseSession();

	holdBuf = (char *) buffer;
	holdBuf = UI_ParseFontParms( holdBuf,propMapTiny);
	holdBuf = UI_ParseFontParms( holdBuf,propMap);
	holdBuf = UI_ParseFontParms( holdBuf,propMapBig);
}

/*
=================
UI_Init
=================
*/
void UI_Init( void ) {

	init_tonextint(qfalse);

	UI_RegisterCvars();

	UI_LoadMenuText();

	UI_LoadButtonText();

	UI_LoadFonts();

	BG_LoadItemNames();

	UI_InitGameinfo();

	// cache redundant calulations
	trap_GetGlconfig( &uis.glconfig );

	// for 640x480 virtualized screen
	uis.scaley = uis.glconfig.vidHeight * (1.0/480.0);
	uis.scalex = uis.glconfig.vidWidth * (1.0/640.0);
/*	uis.scale = uis.glconfig.vidHeight * (1.0/480.0);
	if ( uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640 ) {
		// wide screen
		uis.bias = 0.5 * ( uis.glconfig.vidWidth - ( uis.glconfig.vidHeight * (640.0/480.0) ) );
	}
	else {
		// no wide screen
		uis.bias = 0;
	}
*/
	// initialize the menu system
	Menu_Cache();

	uis.activemenu = NULL;
	uis.menusp     = 0;
	trap_Cvar_Create ("ui_initialsetup", "0", CVAR_ARCHIVE );
}

/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	// expect valid pointers
//	*x = *x * uis.scale + uis.bias;
	*x *= uis.scalex;
	*y *= uis.scaley;
	*w *= uis.scalex;
	*h *= uis.scaley;
}

void UI_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	hShader = trap_R_RegisterShaderNoMip( picname );
	UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}

	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uis.whiteShader );

	trap_R_SetColor( NULL );
}

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	UI_AdjustFrom640( &x, &y, &width, &height );

	trap_R_DrawStretchPic( x, y, width, 1, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x, y, 1, height, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x, y + height - 1, width, 1, 0, 0, 0, 0, uis.whiteShader );
	trap_R_DrawStretchPic( x + width - 1, y, 1, height, 0, 0, 0, 0, uis.whiteShader );

	trap_R_SetColor( NULL );
}

/*
=================
UI_Refresh
=================
*/
void UI_Refresh( int realtime )
{
	vec4_t color;

	uis.frametime = realtime - uis.realtime;
	uis.realtime  = realtime;

	if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
		return;
	}

	UI_UpdateCvars();

	if ( uis.activemenu )
	{
		if (uis.activemenu->fullscreen)
		{
			// draw the background
			trap_R_SetColor( colorTable[CT_BLACK]);
			UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.whiteShader );
		}
		else if (!uis.activemenu->nobackground)
		{
			// draw the background
			color[0] = colorTable[CT_BLACK][0];
			color[1] = colorTable[CT_BLACK][1];
			color[2] = colorTable[CT_BLACK][1];
			color[3] = .75;

			trap_R_SetColor( color);
			UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.whiteShader );
		}

		if (uis.activemenu->draw)
			uis.activemenu->draw();
		else
			Menu_Draw( uis.activemenu );

		if( uis.firstdraw ) {
			UI_MouseEvent( 0, 0 );
			uis.firstdraw = qfalse;
		}

	}

	// draw cursor
	trap_R_SetColor( NULL );
	if (uis.cursorDraw)
	{
		UI_DrawHandlePic( uis.cursorx, uis.cursory, 16, 16, uis.cursor);
	}

#ifndef NDEBUG
	if (uis.debug)
	{
		// cursor coordinates
		UI_DrawString( 0, 0, va("(%d,%d)",uis.cursorx,uis.cursory), UI_LEFT|UI_SMALLFONT, colorRed );
	}
#endif

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		trap_S_StartLocalSound( menu_in_sound, CHAN_LOCAL_SOUND );
		m_entersound = qfalse;
	}
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uis.cursorx < x ||
		uis.cursory < y ||
		uis.cursorx > x+width ||
		uis.cursory > y+height)
		return qfalse;

	return qtrue;
}

/*
==============
UI_DrawNumField

Take x,y positions as if 640 x 480 and scales them to the proper resolution

==============
*/
static void UI_DrawNumField (int x, int y, int width, int value,int charWidth,int charHeight)
{
	char	num[16], *ptr;
	int		l;
	int		frame;
	int		xWidth;

	if (width < 1)
		return;

	// draw number string
	if (width > 15)
		width = 15;

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;

	xWidth = (charWidth/3);

	x += (xWidth)*(width - l);

	ptr = num;
	while (*ptr && l)
	{
//if (*ptr == '-')
//	frame = STAT_MINUS;
//else
			frame = *ptr -'0';

		UI_DrawHandlePic( x,y, 16, 16, uis.smallNumbers[frame] );

		x += (xWidth);
		ptr++;
		l--;
	}

}


/*
==============
UI_PrintMenuGraphics
==============
*/
void UI_PrintMenuGraphics(menugraphics_s *menuGraphics,int maxI)
{
	int i;
	const char *text;

	// Now that all the changes are made, print up the graphics
	for (i=0;i<maxI;++i)
	{
		if (menuGraphics[i].type == MG_GRAPHIC)
		{
			trap_R_SetColor( colorTable[menuGraphics[i].color]);
			UI_DrawHandlePic( menuGraphics[i].x,
				menuGraphics[i].y,
				menuGraphics[i].width,
				menuGraphics[i].height,
				menuGraphics[i].graphic);
			trap_R_SetColor( colorTable[CT_NONE]);
		}
		else if (menuGraphics[i].type == MG_STRING)
		{
			if (menuGraphics[i].file)
			{
				text = menuGraphics[i].file;
			}
			else if (menuGraphics[i].normaltextEnum)
			{
				text= menu_normal_text[menuGraphics[i].normaltextEnum];
			}
			else
			{
				return;
			}

			UI_DrawProportionalString( menuGraphics[i].x,
				menuGraphics[i].y,
				text,
				menuGraphics[i].style,
				colorTable[menuGraphics[i].color]);
		}
		else if (menuGraphics[i].type == MG_NUMBER)
		{
			trap_R_SetColor( colorTable[menuGraphics[i].color]);
			UI_DrawNumField (menuGraphics[i].x,
				menuGraphics[i].y,
				menuGraphics[i].max,
				menuGraphics[i].target,
				menuGraphics[i].width,
				menuGraphics[i].height);
			trap_R_SetColor( colorTable[CT_NONE]);
		}
		else if (menuGraphics[i].type == MG_NONE)
		{
			;	// Don't print anything
		}
	}
}


/*
==============
UI_PrecacheMenuGraphics
==============
*/
void UI_PrecacheMenuGraphics(menugraphics_s *menuGraphics,int maxI)
{
	int i;

	for (i=0;i<maxI;++i)
	{
		if (menuGraphics[i].type == MG_GRAPHIC)
		{
			menuGraphics[i].graphic = trap_R_RegisterShaderNoMip(menuGraphics[i].file);
		}
	}

}

/*
===============
MenuFrame_Cache
===============
*/
static void MenuFrame_Cache(void)
{
	s_menuframe.cornerUpper = trap_R_RegisterShaderNoMip("menu/common/corner_ll_47_7.tga");
	s_menuframe.cornerUpper2= trap_R_RegisterShaderNoMip("menu/common/corner_ul_47_7.tga");
	s_menuframe.cornerLower = trap_R_RegisterShaderNoMip("menu/common/corner_ll_47_18.tga");
}

/*
===============
UI_FrameTop_Graphics
===============
*/
static void UI_FrameTop_Graphics(menuframework_s *menu)
{

	trap_R_SetColor( colorTable[CT_DKPURPLE2] );
	UI_DrawHandlePic(  30,  24,  47,  54, uis.whiteShader);	// Top left hand column

	trap_R_SetColor( colorTable[CT_DKPURPLE3]);
	UI_DrawHandlePic(  30,  81,  47,  34, uis.whiteShader);	// Middle left hand column
	UI_DrawHandlePic(  30,  115,  128,  64, s_menuframe.cornerUpper);	// Corner
	UI_DrawHandlePic(  111, 136,  38,  7, uis.whiteShader);	// Start of line across bottom of top third section


	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic( 152, 136, 135,   7, uis.whiteShader);	// 2nd line across bottom of top third section

	trap_R_SetColor( colorTable[CT_DKPURPLE2]);
	UI_DrawHandlePic( 290, 136,  12,   7, uis.whiteShader);	// 3rd line across bottom of top third section
	UI_DrawHandlePic( 305, 139,  60,   4, uis.whiteShader);	// 4th line across bottom of top third section

	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic( 368, 136,  111,   7, uis.whiteShader);	// 5th line across bottom of top third section

	if (menu->titleI)
	{
		UI_DrawProportionalString( menu->titleX, menu->titleY ,menu_normal_text[menu->titleI],
			UI_RIGHT|UI_BIGFONT, colorTable[CT_LTORANGE]);
	}
}

/*
===============
UI_FrameBottom_Graphics
===============
*/
static void UI_FrameBottom_Graphics(void)
{
	trap_R_SetColor( colorTable[CT_DKBROWN1]);
	UI_DrawHandlePic(  30, 147, 128,  64, s_menuframe.cornerUpper2);	// Top corner
	UI_DrawHandlePic(  50, 147,  99,   7, uis.whiteShader);
	UI_DrawHandlePic( 152, 147, 135,   7, uis.whiteShader);

	trap_R_SetColor( colorTable[CT_DKBROWN1]);
	UI_DrawHandlePic( 290, 147,  12,   7, uis.whiteShader);

	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic( 305, 147,  60,   4, uis.whiteShader);

	trap_R_SetColor( colorTable[CT_DKBROWN1]);
	UI_DrawHandlePic( 368, 147, 111,   7, uis.whiteShader);

	trap_R_SetColor( colorTable[CT_DKBROWN1]);
	UI_DrawHandlePic(  30, 173,  47,  27, uis.whiteShader);	// Top left column
	UI_DrawHandlePic(  30, 392,  47,  33, uis.whiteShader);	// Bottom left column
	UI_DrawHandlePic(  30, 425, 128,  64, s_menuframe.cornerLower);// Bottom Left Corner

	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic( 96,  438, 268,  18, uis.whiteShader);	// Bottom front Line

	trap_R_SetColor(NULL);
}

/*
=================
UI_MenuBottomLineEnd_Graphics
=================
*/
void UI_MenuBottomLineEnd_Graphics (const char *string,int color)
{
	int holdX,holdLength;

	trap_R_SetColor( colorTable[color]);
	holdX = MENU_TITLE_X - (UI_ProportionalStringWidth( string,UI_SMALLFONT));
	holdLength = (367 + 6) - holdX;
	UI_DrawHandlePic( 367,  438,  holdLength,  18, uis.whiteShader);	// Bottom end line
}

/*
=================
UI_MenuFrame
=================
*/
void UI_MenuFrame(menuframework_s *menu)
{

	if (!s_menuframe.initialized)
	{
		MenuFrame_Cache();
	}

	if (!ingameFlag)
	{
		menu->fullscreen			= qtrue;
	}
	else	// In game menu
	{
		menu->fullscreen			= qfalse;
	}

	// Graphic frame
	UI_FrameTop_Graphics(menu);	// Top third
	UI_FrameBottom_Graphics();	// Bottom two thirds

	// Add foot note
	if (menu->footNoteEnum)
	{
		UI_DrawProportionalString(  MENU_TITLE_X, 440, menu_normal_text[menu->footNoteEnum],UI_RIGHT | UI_SMALLFONT, colorTable[CT_LTORANGE]);
		UI_MenuBottomLineEnd_Graphics (menu_normal_text[menu->footNoteEnum],CT_LTBROWN1);
	}

	// Print version
	UI_DrawProportionalString(  371, 445, Q3_VERSION,UI_TINYFONT, colorTable[CT_BLACK]);
}

/*
=================
UI_MenuFrame2
=================
*/
void UI_MenuFrame2(menuframework_s *menu)
{
	if (!s_menuframe.initialized)
	{
		MenuFrame_Cache();
	}

	if (!ingameFlag)
	{
		menu->fullscreen			= qtrue;
	}
	else	// In game menu
	{
		menu->fullscreen			= qfalse;
	}

	if (menu->titleI)
	{
		UI_DrawProportionalString( menu->titleX, menu->titleY ,menu_normal_text[menu->titleI],
			UI_RIGHT|UI_BIGFONT, colorTable[CT_LTORANGE]);
	}

	trap_R_SetColor( colorTable[CT_DKBROWN1]);
	UI_DrawHandlePic(  30, 25,  47,  119, uis.whiteShader);	// Top left column
	UI_DrawHandlePic(  30, 147,  47,  53, uis.whiteShader);	// left column

	UI_DrawHandlePic(  30, 175,  47,  25, uis.whiteShader);	// Mid left column
	UI_DrawHandlePic(  30, 392,  47,  33, uis.whiteShader);	// Bottom left column
	UI_DrawHandlePic(  30, 425, 128,  64, s_menuframe.cornerLower);// Bottom Left Corner

	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic( 96,  438, 268,  18, uis.whiteShader);	// Bottom front Line

	// Add foot note
	if (menu->footNoteEnum)
	{
		UI_DrawProportionalString(  MENU_TITLE_X, 440, menu_normal_text[menu->footNoteEnum],UI_RIGHT | UI_SMALLFONT, colorTable[CT_LTORANGE]);
		UI_MenuBottomLineEnd_Graphics (menu_normal_text[menu->footNoteEnum],CT_LTBROWN1);
	}
	trap_R_SetColor(NULL);

	// Print version
	UI_DrawProportionalString(  371, 445, Q3_VERSION,UI_TINYFONT, colorTable[CT_BLACK]);
}

#define MAXMENUTEXT 15000
char	MenuText[MAXMENUTEXT];

/*
=================
UI_ParseMenuText
=================
*/
static void UI_ParseMenuText()
{
	char	*token;
	char *buffer;
	int i;
	int len;

	COM_BeginParseSession();

	buffer = MenuText;
	i = 1;	// Zero is null string
	while ( buffer )
	{
		token = COM_ParseExt( &buffer, qtrue );

		len = strlen(token);
		if (len)
		{
			menu_normal_text[i] = (buffer - (len + 1));
			*(buffer - 1) = '\0';		//	Place an string end where is belongs.
			i++;
		}

		if (i> MNT_MAX_FROM_FILE)
		{
			Com_Printf( S_COLOR_RED "UI_ParseMenuText : too many values! Needed %d but got %d.\n",MNT_MAX_FROM_FILE,i);
			return;
		}
	}
	if (i != MNT_MAX_FROM_FILE)
	{
		Com_Printf( S_COLOR_RED "UI_ParseMenuText : not enough lines. Read %d of %d!\n",i,MNT_MAX_FROM_FILE);
		for(;i<MNT_MAX_FROM_FILE;i++) {
			menu_normal_text[i] = "?";
		}
	}
}

/*
=================
UI_LoadMenuText
=================
*/
void UI_LoadMenuText()
{
	int len;//,i;
	fileHandle_t	f;
	char	filename[MAX_QPATH];
	char	language[32];

	UI_LanguageFilename("ext_data/mp_normaltext","dat",filename);

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( !f )
	{
		Com_Error( ERR_FATAL, "UI_LoadMenuText : MP_NORMALTEXT.DAT file not found!\n");
		return;
	}

	if ( len > MAXMENUTEXT )
	{
		Com_Error( ERR_FATAL, "UI_LoadMenuText : MP_NORMALTEXT.DAT size (%d) > max (%d)!\n", len, MAXMENUTEXT );
		return;
	}

	// initialise the data area
	memset(MenuText, 0, sizeof(MenuText));

	trap_FS_Read( MenuText, len, f );

	trap_FS_FCloseFile( f );

	UI_ParseMenuText();

	// add some new definitions not in the text file
	trap_Cvar_VariableStringBuffer( "g_language", language, 32 );

	menu_normal_text[MNT_ORDER_TOEVERYONE] = "To Everyone";

	if ( !Q_stricmp( language, "deutsch" ) ) {
		menu_normal_text[MNT_ORDER_TOEVERYONE] = "An alle";

	} else if ( !Q_stricmp( language, "francais" ) ) {
		menu_normal_text[MNT_PC_NOCLASS] = "SansClasse";
		menu_normal_text[MNT_PC_INFILTRATOR] = "Infiltrateur";
		menu_normal_text[MNT_PC_SNIPER] = "TireurD'�lite";
		menu_normal_text[MNT_PC_HEAVY] = "Dur";
		menu_normal_text[MNT_PC_MEDIC] = "M�decin";
		menu_normal_text[MNT_PC_TECH] = "Technicien";
		menu_normal_text[MNT_SHORTCUT_USEINVENTORY] = "UTILISER INVENTAIRE";
		menu_normal_text[MNT_SHORTCUT_USEOBJECT] = "UTILISER OBJET";
		menu_normal_text[MNT_SHORTCUT_OBJECTIVES] = "VOIR OBJECTIFS";
		menu_normal_text[MNT_PARAMETERS] = "PARAMETRES";
		menu_normal_text[MNT_ADV_STATUS1_NOJOINTIMEOUT] = "Doit �tre entre 0 et 600";
		menu_normal_text[MNT_ADV_STATUS2_NOJOINTIMEOUT] = "Valeur standarde: 120";
		menu_normal_text[MNT_ADV_STATUS1_CLASSCHANGETIMEOUT] = "Doit �tre un nombre positif";
		menu_normal_text[MNT_ADV_STATUS2_CLASSCHANGETIMEOUT] = "Valeur standarde: 180";
		menu_normal_text[MNT_ACTIONHERO] = "ACTION HEROS";

		menu_normal_text[MNT_ORDER_TOEVERYONE] = "A tous";
	}
}

#define MAXBUTTONTEXT 15000
char	ButtonText[MAXBUTTONTEXT];

/*
=================
UI_ParseButtonText
=================
*/
static void UI_ParseButtonText()
{
	char	*token;
	char *buffer;
	int i;
	int len;

	COM_BeginParseSession();

	buffer = ButtonText;
	i = 1;	// Zero is null string
	while ( buffer )
	{
//		G_ParseString( &buffer, &token);
		token = COM_ParseExt( &buffer, qtrue );

		len = strlen(token);
		if (len)
		{

			if ((len == 1) && (token[0] == '/'))	// A NULL?
			{
				menu_button_text[i][0] = menuEmptyLine;
				menu_button_text[i][1] = menuEmptyLine;
			}
			else
			{
				menu_button_text[i][0] = (buffer - (len + 1));	// The +1 is to get rid of the " at the beginning of the sting.
			}

			*(buffer - 1) = '\0';		//	Place an string end where is belongs.

			token = COM_ParseExt( &buffer, qtrue );
			len = strlen(token);
			if (len)
			{
				menu_button_text[i][1] = (buffer - (len+1));	// The +1 is to get rid of the " at the beginning of the sting.
				*(buffer-1) = '\0';		//	Place an string end where is belongs.
			}
			++i;
		}

		if (i> MBT_MAX_FROM_FILE)
		{
			Com_Printf( S_COLOR_RED "UI_ParseButtonText : too many values!\n");
			return;
		}
	}
	if (i != MBT_MAX_FROM_FILE)
	{
		Com_Printf( S_COLOR_RED "UI_ParseButtonText : not enough lines. Read %d of %d!\n",i,MBT_MAX_FROM_FILE);
		for(;i<MBT_MAX_FROM_FILE;i++) {
			menu_button_text[i][0] = "?";
			menu_button_text[i][1] = "?";
		}
	}
}

/*
=================
UI_LoadButtonText
=================
*/
void UI_LoadButtonText()
{
	char	filename[MAX_QPATH];
	int len,i;
	fileHandle_t	f;
	char	language[32];

	UI_LanguageFilename("ext_data/mp_buttontext","dat",filename);

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( !f )
	{
		Com_Printf( S_COLOR_RED "UI_LoadButtonText : MP_BUTTONTEXT.DAT file not found!\n");
		return;
	}

	if ( len > MAXBUTTONTEXT )
	{
		Com_Printf( S_COLOR_RED "UI_LoadButtonText : MP_BUTTONTEXT.DAT too big!\n");
		return;
	}

	for (i=0;i<MBT_MAX;++i)
	{
		menu_button_text[i][0] = menuEmptyLine;
		menu_button_text[i][1] = menuEmptyLine;
	}

	// initialise the data area
	memset(ButtonText, 0, sizeof(ButtonText));

	trap_FS_Read( ButtonText, len, f );

	trap_FS_FCloseFile( f );

	UI_ParseButtonText();

	// add some new definitions not in the text file
	trap_Cvar_VariableStringBuffer( "g_language", language, 32 );

	if ( !Q_stricmp( language, "francais" ) ) {
		menu_button_text[MBT_ASSIMILATION][1] = "JOUEURS TUE JOINDRENT LES BORG";
		menu_button_text[MBT_SPECIALTIES][1] = "JOUERS DOIVENT SELECTER UNE CLASSE";
		menu_button_text[MBT_DISINTEGRATION][1] = "UN SEUL COUP REUSSI CAUSE LE MORT IMMEDIATEMENT";
		menu_button_text[MBT_ACTIONHERO][1] = "UN JOUEUR FORTUIT RECOIT TOUS LES ARMES ET DE L'ARMURE";
		menu_button_text[MBT_ELIMINATION][1] = "DERNIER PERSONNE VIVANTE GAGNE LE JEUX";
		menu_button_text[MBT_PLAYERCLASS][1] = "CLASSE DE JOUEUR";
		menu_button_text[MBT_TEAMCLASS][1] = "SELECTES L'EQUIPE ET LE CLASSE DE JOUEUR";
		menu_button_text[MBT_AUTOTEAM][1] = "JOUEURS NOUVEAUX ENTRENT L'EQUIPE PLUS NECESSITEUX";
		menu_button_text[MBT_NOJOINTIMEOUT][1] = "EMPLOYEE DANS L'ASSIMILATION ET L'ELIMINATION";
		menu_button_text[MBT_CLASSCHANGE][1] = "DELAI FORCEE AVENT QU'EN PUISSE CHANGER LA CLASSE";
	}
}

/*
=================
UI_InitSpinControl
=================
*/
void UI_InitSpinControl(menulist_s *spincontrol)
{
	spincontrol->generic.type		= MTYPE_SPINCONTROL;
	spincontrol->generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	spincontrol->textcolor			= CT_BLACK;
	spincontrol->textcolor2			= CT_WHITE;
	spincontrol->color				= CT_DKPURPLE1;
	spincontrol->color2				= CT_LTPURPLE1;
	spincontrol->textX				= MENU_BUTTON_TEXT_X;
	spincontrol->textY				= MENU_BUTTON_TEXT_Y;
}

/*
UI_LanguageFilename - create a filename with an extension based on the value in g_language
*/
static void UI_LanguageFilename(char *baseName,char *baseExtension,char *finalName)
{
	char	language[32];
	fileHandle_t	file;

	trap_Cvar_VariableStringBuffer( "g_language", language, 32 );

	// If it's English then no extension
	if (language[0]=='\0' || Q_stricmp ("ENGLISH",language)==0)
	{
		Com_sprintf(finalName,MAX_QPATH,"%s.%s",baseName,baseExtension);
	}
	else
	{
		Com_sprintf(finalName,MAX_QPATH,"%s_%s.%s",baseName,language,baseExtension);

		//Attempt to load the file
		trap_FS_FOpenFile( finalName, &file, FS_READ );

		if ( file == 0 )	//	This extension doesn't exist, go English.
		{
			Com_sprintf(finalName,MAX_QPATH,"%s.%s",baseName,baseExtension);	//the caller will give the error if this isn't there
		}
		else
		{
			trap_FS_FCloseFile( file );
		}
	}
}
