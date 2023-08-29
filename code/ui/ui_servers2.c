// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "ui_local.h"

#define MAX_SERVERS				512
#define MAX_ADDRESSLENGTH		64
#define MAX_MAPNAMELENGTH		64

#define ART_UNKNOWNMAP			"levelshots/unknownmap"

/*
=======================================================================

SERVER INFO PARSING

=======================================================================
*/

typedef struct {
	char key[128];
	char value[128];
} statusField_t;

typedef struct {
	char score[8];
	char ping[8];
	char name[36];
} statusPlayer_t;

/*
=================
StatusParse_WriteField
=================
*/
static void StatusParse_WriteField( char *source, int sourceLen, char *buffer, int bufSize ) {
	if ( sourceLen >= bufSize )
		sourceLen = bufSize - 1;
	if ( sourceLen < 0 )
		sourceLen = 0;

	memcpy( buffer, source, sourceLen );
	buffer[sourceLen] = '\0';
}

/*
=================
StatusParse_ParseField
=================
*/
static qboolean StatusParse_ParseField( char **ptr, statusField_t *output ) {
	char *p;

	// skip leading slash
	if ( !*ptr || !**ptr )
		return qfalse;
	*ptr = strchr( *ptr, '\\' );
	if ( !*ptr || !**ptr )
		return qfalse;
	++*ptr;
	
	// empty key indicates start of player list
	if ( **ptr == '\\' ) {
		return qfalse;
	}

	// read key
	p = *ptr;
	*ptr = strchr( *ptr, '\\' );
	if ( !*ptr || !**ptr )
		return qfalse;
	if ( output )
		StatusParse_WriteField( p, *ptr - p, output->key, sizeof( output->key ) );

	// skip slash
	++*ptr;

	// read value
	p = *ptr;
	*ptr = strchr( *ptr, '\\' );
	if ( !*ptr || !**ptr )
		return qfalse;
	if ( output )
		StatusParse_WriteField( p, *ptr - p, output->value, sizeof( output->value ) );

	return qtrue;
}

/*
=================
StatusParse_ParsePlayer
=================
*/
static qboolean StatusParse_ParsePlayer( char **ptr, statusPlayer_t *output ) {
	char *p;

	// skip leading slash
	if ( !*ptr || **ptr != '\\' )
		return qfalse;
	++*ptr;

	// read score
	p = *ptr;
	*ptr = strchr( *ptr, ' ' );
	if ( !*ptr || !**ptr )
		return qfalse;
	if ( output )
		StatusParse_WriteField( p, *ptr - p, output->score, sizeof( output->score ) );

	// skip slash
	++*ptr;

	// read ping
	p = *ptr;
	*ptr = strchr( *ptr, ' ' );
	if ( !*ptr || !**ptr )
		return qfalse;
	if ( output )
		StatusParse_WriteField( p, *ptr - p, output->ping, sizeof( output->ping ) );

	// skip slash
	++*ptr;

	// read name
	p = *ptr;
	*ptr = strchr( *ptr, '\\' );
	if ( !*ptr || !**ptr )
		return qfalse;
	if ( output ) {
		// skip leading/trailing quotes from name
		char *end = *ptr;
		if ( p[0] == '\"' )
			++p;
		if ( end > p && end[-1] == '\"' )
			--end;
		StatusParse_WriteField( p, end - p, output->name, sizeof( output->name ) );
	}

	return qtrue;
}

/*
=================
StatusParse_ValueForKey

Retrieves value from server status info. Used instead of regular Info_ValueForKey
just to be on the safe side avoiding any issues due to player list at end of string.
=================
*/
static char *StatusParse_ValueForKey( char *info, const char *key ) {
	static char buffer[128];
	statusField_t field;

	while ( StatusParse_ParseField( &info, &field ) ) {
		if ( !Q_stricmp( field.key, key ) ) {
			Q_strncpyz( buffer, field.value, sizeof( buffer ) );
			return buffer;
		}
	}

	buffer[0] = '\0';
	return buffer;
}

/*
=======================================================================

SERVER INFO PAGE

=======================================================================
*/

#define	MAX_SERVERINFO_LINES		20

enum {
	ID_SERVERINFO_REFRESH = 100,
	ID_SERVERINFO_CONNECT,
	ID_SERVERINFO_BACK,
	ID_SERVERINFO_ARROWUP,
	ID_SERVERINFO_ARROWDWN,
};

typedef struct
{
	menuframework_s	menu;
	menubitmap_s	refresh;
	menubitmap_s	back;
	menubitmap_s	connect;
	int				lineCnt;
	int				lineStartCnt;
	menubitmap_s	arrowdwn;
	menubitmap_s	arrowup;

	// server query
	char			adrstr[MAX_ADDRESSLENGTH];
	qboolean		queryInProgress;
	char			info[2048];

	// map pic
	char			mapPicName[MAX_MAPNAMELENGTH];
	qhandle_t		mapPicShader;
	qboolean		mapPicUnknownRegistered;
	qhandle_t		mapPicUnknownShader;
} browserServerInfo_t;

static browserServerInfo_t s_browserserverinfo;

/*
=================
BrowserServerInfo_UpdateQuery
=================
*/
static void BrowserServerInfo_UpdateQuery( void )
{
	if ( s_browserserverinfo.queryInProgress ) {
		if ( VMExt_FN_LAN_ServerStatus_Ext( s_browserserverinfo.adrstr, s_browserserverinfo.info, sizeof( s_browserserverinfo.info ), NULL, 0 ) ) {
			s_browserserverinfo.queryInProgress = qfalse;
		}
	}
}

/*
=================
BrowserServerInfo_StartQuery
=================
*/
static void BrowserServerInfo_StartQuery( void )
{
	if ( !s_browserserverinfo.queryInProgress && *s_browserserverinfo.adrstr ) {
		s_browserserverinfo.queryInProgress = qtrue;
		s_browserserverinfo.info[0] = '\0';
		s_browserserverinfo.lineCnt = 0;
		s_browserserverinfo.lineStartCnt = 0;
		VMExt_FN_LAN_ServerStatus_Ext( NULL, NULL, 0, NULL, 0 );
		BrowserServerInfo_UpdateQuery();
	}
}

/*
=================
BrowserServerInfo_Event
=================
*/
static void BrowserServerInfo_Event( void* ptr, int event )
{
	if ( event == QM_ACTIVATED ) {
		switch (((menucommon_s*)ptr)->id)
		{
			case ID_SERVERINFO_REFRESH:
				BrowserServerInfo_StartQuery();
				break;

			case ID_SERVERINFO_CONNECT:
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", s_browserserverinfo.adrstr ) );
				break;

			case ID_SERVERINFO_BACK:
				UI_PopMenu();
				break;

			case ID_SERVERINFO_ARROWUP:
				s_browserserverinfo.lineStartCnt--;
				break;

			case ID_SERVERINFO_ARROWDWN:
				s_browserserverinfo.lineStartCnt++;
				break;
		}
	}
}

/*
=================
BrowserServerInfo_NextLinePosition

Increments currentLine, and returns the current Y coordinate to draw the current line to.
Returns 0 if current line is scrolled out of view.
=================
*/
int BrowserServerInfo_NextLinePosition( int *currentLine ) {
	int displayLine = *currentLine - s_browserserverinfo.lineStartCnt;
	int result = 0;

	if ( displayLine >= 0 && displayLine < MAX_SERVERINFO_LINES ) {
		result = 62 + displayLine * SMALLCHAR_HEIGHT;
	}

	++*currentLine;
	return result;
}

/*
=================
BrowserServerInfo_RenderInfoField
=================
*/
void BrowserServerInfo_RenderInfoField( int *currentLine, const char *key, const char *value ) {
	int yPos = BrowserServerInfo_NextLinePosition( currentLine );
	if ( yPos ) {
		char buffer[128];
		Com_sprintf( buffer, sizeof( buffer ), "%s:", key );
		UI_DrawProportionalString( 375 - 8, yPos, buffer, UI_RIGHT | UI_SMALLFONT | UI_NO_BLACK, colorTable[CT_LTBLUE2] );
		// let string use the entire right side of the screen before shrinking
		UI_DrawAutoProportionalString( 375 + 8, yPos, value, UI_LEFT | UI_NO_BLACK, colorTable[CT_LTGOLD1],
				320 * AspectCorrect_WidthScale() - ( 375 + 8 - 320 ), 4 );
	}
}

/*
=================
BrowserServerInfo_RenderPlayer
=================
*/
void BrowserServerInfo_RenderPlayer( int *currentLine, statusPlayer_t *player ) {
	int yPos = BrowserServerInfo_NextLinePosition( currentLine );
	if ( yPos ) {
		char name[36];
		qboolean isBot = !Q_stricmp( player->ping, "0" );
		const char *ping = isBot ? "BOT" : player->ping;

		// Do a bit of truncation to avoid excessively long player names overflowing
		Q_strncpyz( name, player->name, sizeof( name ) );
		UI_TruncateStringWidth( name, 150 );

		UI_DrawProportionalString( 270, yPos, name, UI_LEFT | UI_SMALLFONT | UI_NO_BLACK, colorTable[CT_LTGOLD1] );
		UI_DrawProportionalString( 430, yPos, player->score, UI_LEFT | UI_SMALLFONT, colorTable[CT_LTGOLD1] );
		UI_DrawProportionalString( 490, yPos, ping, UI_LEFT | UI_SMALLFONT, isBot ? colorTable[CT_LTGOLD1] : colorTable[CT_LTORANGE] );
	}
}

/*
=================
BrowserServerInfo_RenderLines
=================
*/
void BrowserServerInfo_RenderLines( void ) {
	int lineNum = 0;
	int yPos;
	char *infoPtr;
	statusField_t field;
	statusPlayer_t player;

	if ( s_browserserverinfo.queryInProgress ) {
		yPos = BrowserServerInfo_NextLinePosition( &lineNum );
		if ( yPos ) {
			UI_DrawProportionalString( 320, yPos, "Querying server...", UI_CENTER | UI_SMALLFONT, colorTable[CT_LTGOLD1] );
		}
	}

	else {
		const char *serverName = StatusParse_ValueForKey( s_browserserverinfo.info, "sv_hostname" );
		if ( *serverName ) {
			yPos = BrowserServerInfo_NextLinePosition( &lineNum );
			if ( yPos ) {
				UI_DrawProportionalString( 400, yPos, serverName, UI_CENTER | UI_SMALLFONT | UI_NO_BLACK, colorTable[CT_LTGOLD1] );
			}
			BrowserServerInfo_NextLinePosition( &lineNum );
		}

		yPos = BrowserServerInfo_NextLinePosition( &lineNum );
		if ( yPos ) {
			UI_DrawProportionalString( 270, yPos, "NAME", UI_LEFT | UI_SMALLFONT, colorTable[CT_LTORANGE] );
			UI_DrawProportionalString( 430, yPos, "SCORE", UI_LEFT | UI_SMALLFONT, colorTable[CT_LTORANGE] );
			UI_DrawProportionalString( 490, yPos, "PING", UI_LEFT | UI_SMALLFONT, colorTable[CT_LTORANGE] );
		}

		infoPtr = s_browserserverinfo.info;
		while ( StatusParse_ParseField( &infoPtr, NULL ) ) {
		}
		while ( StatusParse_ParsePlayer( &infoPtr, &player ) ) {
			BrowserServerInfo_RenderPlayer( &lineNum, &player );
		}
		BrowserServerInfo_NextLinePosition( &lineNum );

		BrowserServerInfo_RenderInfoField( &lineNum, "address", s_browserserverinfo.adrstr );

		infoPtr = s_browserserverinfo.info;
		while ( StatusParse_ParseField( &infoPtr, &field ) ) {
			BrowserServerInfo_RenderInfoField( &lineNum, field.key, field.value );
		}
	}

	// Update line count
	s_browserserverinfo.lineCnt = lineNum;
	if ( s_browserserverinfo.lineStartCnt > s_browserserverinfo.lineCnt - MAX_SERVERINFO_LINES )
		s_browserserverinfo.lineStartCnt = s_browserserverinfo.lineCnt - MAX_SERVERINFO_LINES;
	if ( s_browserserverinfo.lineStartCnt < 0 )
		s_browserserverinfo.lineStartCnt = 0;

	// Enable/disable the up/down arrows
	s_browserserverinfo.arrowdwn.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	s_browserserverinfo.arrowup.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	if ( s_browserserverinfo.lineStartCnt <= 0 )
		s_browserserverinfo.arrowup.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
	if ( s_browserserverinfo.lineStartCnt >= s_browserserverinfo.lineCnt - MAX_SERVERINFO_LINES )
		s_browserserverinfo.arrowdwn.generic.flags |= (QMF_HIDDEN|QMF_INACTIVE);
}

/*
=================
BrowserServerInfo_RenderMapPic
=================
*/
void BrowserServerInfo_RenderMapPic( void ) {
	if ( !s_browserserverinfo.queryInProgress ) {
		char *mapName = StatusParse_ValueForKey( s_browserserverinfo.info, "mapname" );
		Q_strlwr( mapName );

		if ( Q_stricmp( s_browserserverinfo.mapPicName, mapName ) ) {
			Q_strncpyz( s_browserserverinfo.mapPicName, mapName, sizeof( s_browserserverinfo.mapPicName ) );
			s_browserserverinfo.mapPicShader = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga", mapName ) );

			if ( !s_browserserverinfo.mapPicShader ) {
				if ( !s_browserserverinfo.mapPicUnknownRegistered ) {
					s_browserserverinfo.mapPicUnknownShader = trap_R_RegisterShaderNoMip( ART_UNKNOWNMAP );
					s_browserserverinfo.mapPicUnknownRegistered = qtrue;
				}

				s_browserserverinfo.mapPicShader = s_browserserverinfo.mapPicUnknownShader;
			}
		}

		if ( s_browserserverinfo.mapPicShader ) {
			UI_DrawHandlePic( 90, 75, 128, 96, s_browserserverinfo.mapPicShader );
		}

		UI_DrawAutoProportionalString( 90 + ( 128 / 2 ), 75 + 96 + 4, mapName, UI_CENTER, colorTable[CT_LTGOLD1], 140, 2 );
	}
}

/*
=================
BrowserServerInfo_MenuDraw
=================
*/
static void BrowserServerInfo_MenuDraw( void )
{
	// Draw the basic screen layout
	UI_MenuFrame2( &s_browserserverinfo.menu );

	// from ServerInfoMenu_Graphics - bottom center button
	trap_R_SetColor( colorTable[CT_LTPURPLE1] );
	UI_DrawHandlePic( 250, 400, 180, 20, uis.whiteShader );

	// from ArenaServers_Graphics
	trap_R_SetColor( colorTable[CT_LTBROWN1] );
	UI_DrawHandlePic( 30, 203, 47, 186, uis.whiteShader ); // Left side of frame

	UI_DrawProportionalString( 74, 27, "4817", UI_RIGHT | UI_TINYFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 74, 149, "133", UI_RIGHT | UI_TINYFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 74, 205, "2293", UI_RIGHT | UI_TINYFONT, colorTable[CT_BLACK] );
	UI_DrawProportionalString( 74, 395, "1159", UI_RIGHT | UI_TINYFONT, colorTable[CT_BLACK] );

	BrowserServerInfo_UpdateQuery();
	BrowserServerInfo_RenderLines();
	BrowserServerInfo_RenderMapPic();

	Menu_Draw( &s_browserserverinfo.menu );
}

/*
=================
BrowserServerInfo_MenuKey
=================
*/
static sfxHandle_t BrowserServerInfo_MenuKey( int key )
{
	if ( key == K_MWHEELUP ) {
		if ( s_browserserverinfo.lineStartCnt > 0 ) {
			s_browserserverinfo.lineStartCnt--;
		}
		return menu_null_sound;
	}

	if ( key == K_MWHEELDOWN ) {
		if ( s_browserserverinfo.lineStartCnt < s_browserserverinfo.lineCnt - MAX_SERVERINFO_LINES ) {
			s_browserserverinfo.lineStartCnt++;
		}
		return menu_null_sound;
	}

	return ( Menu_DefaultKey( &s_browserserverinfo.menu, key ) );
}

/*
=================
BrowserServerInfo_Cache
=================
*/
void BrowserServerInfo_Cache( void )
{
}

/*
=================
BrowserServerInfo_Init
=================
*/
static void BrowserServerInfo_InitMenu( const char *adrstr )
{
	int	y;

	// zero set all our globals
	memset( &s_browserserverinfo, 0 ,sizeof(browserServerInfo_t) );

	BrowserServerInfo_Cache();

	s_browserserverinfo.menu.draw						= BrowserServerInfo_MenuDraw;
	s_browserserverinfo.menu.key						= BrowserServerInfo_MenuKey;
	s_browserserverinfo.menu.wrapAround				= qtrue;
	s_browserserverinfo.menu.fullscreen				= qtrue;
	s_browserserverinfo.menu.nitems					= 0;
	s_browserserverinfo.menu.descX						= MENU_DESC_X;
	s_browserserverinfo.menu.descY						= MENU_DESC_Y;
	s_browserserverinfo.menu.listX						= 230;
	s_browserserverinfo.menu.listY						= 188;
	s_browserserverinfo.menu.titleX					= MENU_TITLE_X;
	s_browserserverinfo.menu.titleY					= MENU_TITLE_Y;
	s_browserserverinfo.menu.titleI					= MNT_SERVERINFO_TITLE;
	s_browserserverinfo.menu.footNoteEnum				= MNT_SERVERINFO;

	s_browserserverinfo.refresh.generic.type			= MTYPE_BITMAP;
	s_browserserverinfo.refresh.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_browserserverinfo.refresh.generic.x				= 90;
	s_browserserverinfo.refresh.generic.y				= 375;
	s_browserserverinfo.refresh.generic.name			= GRAPHIC_SQUARE;
	s_browserserverinfo.refresh.generic.id				= ID_SERVERINFO_REFRESH;
	s_browserserverinfo.refresh.generic.callback		= BrowserServerInfo_Event;
	s_browserserverinfo.refresh.width					= MENU_BUTTON_MED_WIDTH;
	s_browserserverinfo.refresh.height					= MENU_BUTTON_MED_HEIGHT;
	s_browserserverinfo.refresh.color					= CT_DKORANGE;
	s_browserserverinfo.refresh.color2					= CT_LTORANGE;
	s_browserserverinfo.refresh.textX					= MENU_BUTTON_TEXT_X;
	s_browserserverinfo.refresh.textY					= MENU_BUTTON_TEXT_Y;
	s_browserserverinfo.refresh.textEnum				= MBT_REFRESH;
	s_browserserverinfo.refresh.textcolor				= CT_BLACK;
	s_browserserverinfo.refresh.textcolor2				= CT_WHITE;

	y = 400;
	// Button Data
	s_browserserverinfo.back.generic.type				= MTYPE_BITMAP;
	s_browserserverinfo.back.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_browserserverinfo.back.generic.x					= 90;
	s_browserserverinfo.back.generic.y					= 400;
	s_browserserverinfo.back.generic.name				= GRAPHIC_SQUARE;
	s_browserserverinfo.back.generic.id				= ID_SERVERINFO_BACK;
	s_browserserverinfo.back.generic.callback			= BrowserServerInfo_Event;
	s_browserserverinfo.back.width						= MENU_BUTTON_MED_WIDTH;
	s_browserserverinfo.back.height					= MENU_BUTTON_MED_HEIGHT;
	s_browserserverinfo.back.color						= CT_DKPURPLE1;
	s_browserserverinfo.back.color2					= CT_LTPURPLE1;
	s_browserserverinfo.back.textX						= MENU_BUTTON_TEXT_X;
	s_browserserverinfo.back.textY						= MENU_BUTTON_TEXT_Y;
	s_browserserverinfo.back.textEnum					= MBT_BACK;
	s_browserserverinfo.back.textcolor					= CT_BLACK;
	s_browserserverinfo.back.textcolor2				= CT_WHITE;

	s_browserserverinfo.connect.generic.type			= MTYPE_BITMAP;
	s_browserserverinfo.connect.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_browserserverinfo.connect.generic.x				= 460;
	s_browserserverinfo.connect.generic.y				= y;
	s_browserserverinfo.connect.generic.name			= GRAPHIC_SQUARE;
	s_browserserverinfo.connect.generic.id				= ID_SERVERINFO_CONNECT;
	s_browserserverinfo.connect.generic.callback		= BrowserServerInfo_Event;
	s_browserserverinfo.connect.width					= MENU_BUTTON_MED_WIDTH;
	s_browserserverinfo.connect.height					= MENU_BUTTON_MED_HEIGHT;
	s_browserserverinfo.connect.color					= CT_DKPURPLE1;
	s_browserserverinfo.connect.color2					= CT_LTPURPLE1;
	s_browserserverinfo.connect.textX					= MENU_BUTTON_TEXT_X;
	s_browserserverinfo.connect.textY					= MENU_BUTTON_TEXT_Y;
	s_browserserverinfo.connect.textEnum				= MBT_ENGAGEMULTIPLAYER;
	s_browserserverinfo.connect.textcolor				= CT_BLACK;
	s_browserserverinfo.connect.textcolor2				= CT_WHITE;

	s_browserserverinfo.arrowup.generic.type			= MTYPE_BITMAP;
	s_browserserverinfo.arrowup.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_browserserverinfo.arrowup.generic.x				= 270;
	s_browserserverinfo.arrowup.generic.y				= y+2;
	s_browserserverinfo.arrowup.generic.name			= "menu/common/arrow_up_16.tga";
	s_browserserverinfo.arrowup.generic.id				= ID_SERVERINFO_ARROWUP;
	s_browserserverinfo.arrowup.generic.callback		= BrowserServerInfo_Event;
	s_browserserverinfo.arrowup.width					= 16;
	s_browserserverinfo.arrowup.height					= 16;
	s_browserserverinfo.arrowup.color					= CT_DKBLUE1;
	s_browserserverinfo.arrowup.color2					= CT_LTBLUE1;
	s_browserserverinfo.arrowup.textX					= 0;
	s_browserserverinfo.arrowup.textY					= 0;
	s_browserserverinfo.arrowup.textEnum				= MBT_NONE;
	s_browserserverinfo.arrowup.textcolor				= CT_BLACK;
	s_browserserverinfo.arrowup.textcolor2				= CT_WHITE;

	s_browserserverinfo.arrowdwn.generic.type			= MTYPE_BITMAP;
	s_browserserverinfo.arrowdwn.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_browserserverinfo.arrowdwn.generic.x				= 394;
	s_browserserverinfo.arrowdwn.generic.y				= y+2;
	s_browserserverinfo.arrowdwn.generic.name			= "menu/common/arrow_dn_16.tga";
	s_browserserverinfo.arrowdwn.generic.id			= ID_SERVERINFO_ARROWDWN;
	s_browserserverinfo.arrowdwn.generic.callback		= BrowserServerInfo_Event;
	s_browserserverinfo.arrowdwn.width					= 16;
	s_browserserverinfo.arrowdwn.height				= 16;
	s_browserserverinfo.arrowdwn.color					= CT_DKBLUE1;
	s_browserserverinfo.arrowdwn.color2				= CT_LTBLUE1;
	s_browserserverinfo.arrowdwn.textX					= 0;
	s_browserserverinfo.arrowdwn.textY					= 0;
	s_browserserverinfo.arrowdwn.textEnum				= MBT_NONE;
	s_browserserverinfo.arrowdwn.textcolor				= CT_BLACK;
	s_browserserverinfo.arrowdwn.textcolor2			= CT_WHITE;

	Menu_AddItem( &s_browserserverinfo.menu,( void * ) &s_browserserverinfo.arrowup);
	Menu_AddItem( &s_browserserverinfo.menu, ( void * ) &s_browserserverinfo.arrowdwn);
	Menu_AddItem( &s_browserserverinfo.menu, (void*) &s_browserserverinfo.refresh );
	Menu_AddItem( &s_browserserverinfo.menu, (void*) &s_browserserverinfo.back );
	Menu_AddItem( &s_browserserverinfo.menu, (void*) &s_browserserverinfo.connect );

	s_browserserverinfo.arrowup.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;

	// Query configuration
	Q_strncpyz( s_browserserverinfo.adrstr, adrstr, sizeof( s_browserserverinfo.adrstr ) );
	BrowserServerInfo_StartQuery();

	UI_PushMenu( &s_browserserverinfo.menu );
}

/*
=======================================================================

SERVER BROWSER

=======================================================================
*/

#define MAX_PINGREQUESTS		16
#define MAX_HOSTNAMELENGTH		22
#define MAX_STATUSLENGTH		64
#define MAX_LISTBOXWIDTH		65 //59
#define MAX_LISTBOXBUFFER		( MAX_LISTBOXWIDTH + 8 )	// extra space for color sequences

enum {
	ID_MASTER = 200,
	ID_IPPROTOCOL,
	ID_GAMETYPE,
	ID_SORTKEY,
	ID_SHOW_EMPTY,
	ID_LIST,
	ID_SCROLL_UP,
	ID_SCROLL_DOWN,
	ID_BACK,
	ID_REFRESH,
	ID_SPECIFY,
	ID_CREATE,
	ID_CONNECT,
	ID_REMOVE,
	ID_STOPREFRESH,
	ID_FINDSERVER,
	ID_FAVORITE,
	ID_SHOWINFO,
	ID_MAINMENU,
};

typedef struct {
	char	adrstr[MAX_ADDRESSLENGTH];
	int		start;
} pinglist_t;

#define SERVERFLAG_IPV6 1			// entry has an ipv6 address
#define SERVERFLAG_IP_PAIRED 2		// has an ipv4/ipv6 counterpart with same hostname
#define SERVERFLAG_FORCE_LIST 4		// always show regardless of ipv4/ipv6 counterpart

typedef struct servernode_s {
	char	adrstr[MAX_ADDRESSLENGTH];
	char	hostname[MAX_HOSTNAMELENGTH];
	char	mapname[MAX_MAPNAMELENGTH];
	int		numclients;
	int		numhumanclients;
	int		maxclients;
	int		pingtime;
	int		gametype;
	char	modname[12];
	int		minPing;
	int		maxPing;
	int		flags;
} servernode_t;

typedef struct {
	char			buff[MAX_LISTBOXBUFFER];
	servernode_t*	servernode;
} table_t;

typedef struct {
	menuframework_s		menu;

	menubitmap_s		mainmenu;
	menubitmap_s		back;
	menulist_s			master;
	menulist_s			ipprotocol;
	menulist_s			gametype;
	menulist_s			sortkey;
	menulist_s			showempty;

	menulist_s			list;
	menubitmap_s		mappic;
	menubitmap_s		arrows;
	menubitmap_s		up;
	menubitmap_s		down;
	menutext_s			status;
	char				statusBuffer[MAX_STATUSLENGTH];

	menubitmap_s		remove;
	menubitmap_s		refresh;
	menubitmap_s		stoprefresh;
	menubitmap_s		go;
	menubitmap_s		showinfo;
	menubitmap_s		favorite;

	qboolean			statusQueryEnabled;
	pinglist_t			pinglist[MAX_PINGREQUESTS];
	table_t				table[MAX_SERVERS];
	char*				items[MAX_SERVERS];
	int					numqueriedservers;
	int					currentping;
	qboolean			refreshservers;
	int					nextpingtime;
	int					refreshStartTime;
	char				favoriteaddresses[MAX_FAVORITESERVERS][MAX_ADDRESSLENGTH];
	int					numfavoriteaddresses;
} arenaservers_t;

static arenaservers_t	g_arenaservers;

// Persistant items that stay when coming back to the server menu
static servernode_t		g_serverlist[MAX_SERVERS];
static int				g_numservers;
static int				g_servertype;
static int				g_ipprotocol;
static int				g_gametype;
static int				g_sortkey;
static int				g_emptyservers;

// If true, global server 0 queries all masters
#define MASTER_MULTI_FETCH_ACTIVE VMExt_GVCommandInt( "ui_support_globalservers_multi_fetch", 0 )

// Always wait at least 1 second for Internet queries, to avoid a case on engines that
// query multiple masters where a response containing 0 (or a very low number of) servers
// could prematurely abort the refresh before other masters have a chance to respond
#define INTERNET_REFRESH_HOLD ( g_servertype >= AS_GLOBAL1 && g_servertype <= AS_GLOBAL5 && \
		uis.realtime < g_arenaservers.refreshStartTime + 1000 )

// Use cvar with different default if status query is enabled
#define SORT_KEY_CVAR ( g_arenaservers.statusQueryEnabled ? &ui_browserSortKey2 : &ui_browserSortKey )
#define SORT_KEY_CVAR_NAME ( g_arenaservers.statusQueryEnabled ? "ui_browserSortKey2" : "ui_browserSortKey" )

static int master_items[] =
{
	MNT_MASTER_ITEMS_LOCAL,
	MNT_MASTER_ITEMS_INTERNET1,
	MNT_MASTER_ITEMS_INTERNET2,
	MNT_MASTER_ITEMS_INTERNET3,
	MNT_MASTER_ITEMS_INTERNET4,
	MNT_MASTER_ITEMS_INTERNET5,
	MNT_MASTER_ITEMS_FAVORITES,
	0
};

static int master_items_multi_fetch[] =
{
	MNT_MASTER_ITEMS_LOCAL,
	MNT_BROWSER_INTERNET_ALL,
	MNT_MASTER_ITEMS_INTERNET1,
	MNT_MASTER_ITEMS_INTERNET2,
	MNT_MASTER_ITEMS_INTERNET3,
	MNT_MASTER_ITEMS_INTERNET4,
	MNT_MASTER_ITEMS_FAVORITES,
	0
};

enum {
	// Must correspond to master_items and master_items_multi_fetch
	AS_LOCAL,
	AS_GLOBAL1,
	AS_GLOBAL2,
	AS_GLOBAL3,
	AS_GLOBAL4,
	AS_GLOBAL5,
	AS_FAVORITES,
	NUM_MASTER_ITEMS
};

static int ipprotocol_items[] =
{
	MNT_BROWSER_PREFER_IPV4,
	MNT_BROWSER_PREFER_IPV6,
	0
};

enum {
	// Must correspond to ipprotocol_items
	PROTOCOL_IPV4,
	PROTOCOL_IPV6,
};

static int servertype_items[] =
{
	MNT_SERVERTYPE_ALL,
	MNT_SERVERTYPE_FREE_FOR_ALL,
	MNT_SERVERTYPE_TEAM_DEATHMATCH,
	MNT_SERVERTYPE_TOURNAMENT,
	MNT_SERVERTYPE_CAPTURETHEFLAG,
	0
};

enum {
	// Must correspond to servertype_items
	GAMES_ALL,
	GAMES_FFA,
	GAMES_TEAMPLAY,
	GAMES_TOURNEY,
	GAMES_CTF,
};

static int sortkey_items[] =
{
	MNT_SORTKEY_SERVER_NAME,
	MNT_SORTKEY_MAP_NAME,
	MNT_BROWSER_PLAYER_COUNT,
	MNT_SORTKEY_GAMETYPE,
	MNT_SORTKEY_PINGTIME,
	0
};

enum {
	// Must correspond to sortkey_items
	SORT_HOST,
	SORT_MAP,
	SORT_CLIENTS,
	SORT_GAME,
	SORT_PING,
};

static int playerstatus_items[] =
{
	MNT_BROWSER_HUMANS_ONLY,
	MNT_SERVERTYPE_ALL,
	MNT_BROWSER_BOTS_AND_HUMANS,
	0
};

enum {
	// Must correspond to playerstatus_items
	PLAYERTYPE_HUMANS_ONLY,
	PLAYERTYPE_ALL,
	PLAYERTYPE_BOTS_AND_HUMANS,
};

static int noyes_items[] =
{
	MNT_NO,
	MNT_YES,
	0
};

static char* gamenames[] = {
	"FFA",	// deathmatch
	"1v1",	// tournament
	"SP ",	// single player
	"THM",	// team deathmatch
	"CTF",	// capture the flag
	"???",	// unknown
};

/*
=================
ArenaServers_IsIpv6Address
=================
*/
static qboolean ArenaServers_IsIpv6Address( char *adrstr ) {
	const char *p = strchr( adrstr, ':' );
	if ( p && strchr( &p[1], ':' ) ) {
		return qtrue;
	}
	return qfalse;
}

/*
=================
ArenaServers_MaxPing
=================
*/
static int ArenaServers_MaxPing( void ) {
	int		maxPing;

	maxPing = (int)trap_Cvar_VariableValue( "cl_maxPing" );
	if( maxPing < 100 ) {
		maxPing = 100;
	}
	return maxPing;
}

/*
=================
ArenaServers_Compare
=================
*/
static int QDECL ArenaServers_Compare( const void *arg1, const void *arg2 ) {
	int				x;
	float			f1;
	float			f2;
	servernode_t*	t1;
	servernode_t*	t2;

	t1 = (servernode_t *)arg1;
	t2 = (servernode_t *)arg2;

	switch( g_sortkey ) {
	case SORT_HOST:
		x = Q_stricmp( t1->hostname, t2->hostname );
		if ( x ) {
			return x;
		}
		break;

	case SORT_MAP:
		x = Q_stricmp( t1->mapname, t2->mapname );
		if ( x ) {
			return x;
		}
		break;

	case SORT_CLIENTS:
		f1 = g_arenaservers.statusQueryEnabled ? t1->numhumanclients : t1->numclients;
		f2 = g_arenaservers.statusQueryEnabled ? t2->numhumanclients : t2->numclients;

		if( f1 < f2 ) {
			return 1;
		}
		if( f1 > f2 ) {
			return -1;
		}
		break;

	case SORT_GAME:
		if( t1->gametype < t2->gametype ) {
			return -1;
		}
		if( t1->gametype > t2->gametype ) {
			return 1;
		}
		break;
	}

	if( t1->pingtime < t2->pingtime ) {
		return -1;
	}
	if( t1->pingtime > t2->pingtime ) {
		return 1;
	}
	return Q_stricmp( t1->hostname, t2->hostname );
}

/*
=================
ArenaServers_Go
=================
*/
static void ArenaServers_Go( void ) {
	servernode_t*	servernode;

	servernode = g_arenaservers.table[g_arenaservers.list.curvalue].servernode;
	if( servernode ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", servernode->adrstr ) );
	}
}

/*
=================
ArenaServers_UpdatePicture
=================
*/
static void ArenaServers_UpdatePicture( void ) {
	static char		picname[64];
	servernode_t*	servernodeptr;

	if( !g_arenaservers.list.numitems ) {
		g_arenaservers.mappic.generic.name = NULL;
	}
	else {
		servernodeptr = g_arenaservers.table[g_arenaservers.list.curvalue].servernode;
		Com_sprintf( picname, sizeof(picname), "levelshots/%s.tga", servernodeptr->mapname );
		g_arenaservers.mappic.generic.name = picname;
	}

	// force shader update during draw
	g_arenaservers.mappic.shader = 0;
}

/*
=================
ArenaServers_UpdateMenu
=================
*/
static void ArenaServers_UpdateMenu( void ) {
	int				i;
	char*			buff;
	servernode_t*	servernodeptr;
	table_t*		tableptr;
	char			*pingColor;
	int				protocolMatches = 0;

	// make sure list is sorted
	qsort( g_serverlist, g_numservers, sizeof( servernode_t ), ArenaServers_Compare );

	g_arenaservers.list.numitems = 0;
	g_arenaservers.list.curvalue = 0;
	g_arenaservers.list.top      = 0;

	// build list box strings - apply culling filters
	servernodeptr = g_serverlist;
	for( i = 0; i < g_numservers; i++, servernodeptr++ ) {
		tableptr = &g_arenaservers.table[g_arenaservers.list.numitems];
		tableptr->servernode = servernodeptr;
		buff = tableptr->buff;

		// check for skipping paired ip, so dual ipv4/ipv6 servers are only displayed once
		if ( ( servernodeptr->flags & SERVERFLAG_IP_PAIRED ) && !( servernodeptr->flags & SERVERFLAG_FORCE_LIST ) ) {
			if ( ( g_arenaservers.ipprotocol.curvalue == PROTOCOL_IPV4 && ( servernodeptr->flags & SERVERFLAG_IPV6 ) ) ||
					( g_arenaservers.ipprotocol.curvalue == PROTOCOL_IPV6 && !( servernodeptr->flags & SERVERFLAG_IPV6 ) ) ) {
				continue;
			}
		}
		++protocolMatches;

		if ( g_arenaservers.statusQueryEnabled ) {
			if ( ( g_emptyservers == PLAYERTYPE_HUMANS_ONLY && !servernodeptr->numhumanclients ) ||
					( g_emptyservers == PLAYERTYPE_BOTS_AND_HUMANS && !servernodeptr->numclients ) ) {
				continue;
			}
		} else {
			if( !g_emptyservers && !servernodeptr->numclients ) {
				continue;
			}
		}

		switch( g_gametype ) {
		case GAMES_ALL:
			break;

		case GAMES_FFA:
			if( servernodeptr->gametype != GT_FFA ) {
				continue;
			}
			break;

		case GAMES_TEAMPLAY:
			if( servernodeptr->gametype != GT_TEAM ) {
				continue;
			}
			break;

		case GAMES_TOURNEY:
			if( servernodeptr->gametype != GT_TOURNAMENT ) {
				continue;
			}
			break;

		case GAMES_CTF:
			if( servernodeptr->gametype != GT_CTF ) {
				continue;
			}
			break;
		}

		if( servernodeptr->pingtime < servernodeptr->minPing ) {
			pingColor = S_COLOR_BLUE;
		}
		else if( servernodeptr->maxPing && servernodeptr->pingtime > servernodeptr->maxPing ) {
			pingColor = S_COLOR_BLUE;
		}
		else if( servernodeptr->pingtime < 200 ) {
			pingColor = S_COLOR_GREEN;
		}
		else if( servernodeptr->pingtime < 400 ) {
			pingColor = S_COLOR_YELLOW;
		}
		else {
			pingColor = S_COLOR_RED;
		}

		if ( g_arenaservers.statusQueryEnabled ) {
			Com_sprintf( buff, MAX_LISTBOXBUFFER, "%s%-22.22s %-11.11s %2d/%3d %3s %-9.9s %2d %s%3d %s",
				servernodeptr->numhumanclients ? "^5" : "",		// color servers with players blue
				servernodeptr->hostname, servernodeptr->mapname, servernodeptr->numhumanclients,
				servernodeptr->maxclients, gamenames[servernodeptr->gametype], servernodeptr->modname,
				servernodeptr->numclients - servernodeptr->numhumanclients, pingColor, servernodeptr->pingtime,
				servernodeptr->flags & SERVERFLAG_IPV6 ? "*" : "" );
		} else {
			Com_sprintf( buff, MAX_LISTBOXBUFFER, "%-23.23s %-11.11s %3d/%3d %3s %-9.9s  %s%3d %s",
				servernodeptr->hostname, servernodeptr->mapname, servernodeptr->numclients,
				servernodeptr->maxclients, gamenames[servernodeptr->gametype], servernodeptr->modname,
				pingColor, servernodeptr->pingtime, servernodeptr->flags & SERVERFLAG_IPV6 ? "*" : "" );
		}
		g_arenaservers.list.numitems++;
	}

	if ( g_arenaservers.refreshservers ) {
		// refresh in progress
		if ( g_arenaservers.numqueriedservers > 0 ) {
			Com_sprintf( g_arenaservers.statusBuffer, sizeof( g_arenaservers.statusBuffer ),
					menu_normal_text[MNT_BROWSER_SCAN_PROGRESS],
					g_arenaservers.currentping, g_arenaservers.numqueriedservers );
		} else {
			Q_strncpyz( g_arenaservers.statusBuffer, menu_normal_text[MNT_SCANNINGFORSERVERS],
					sizeof( g_arenaservers.statusBuffer ) );
		}

		// disable controls during refresh
		g_arenaservers.master.generic.flags		|= QMF_GRAYED;
		g_arenaservers.ipprotocol.generic.flags	|= QMF_GRAYED;
		g_arenaservers.gametype.generic.flags	|= QMF_GRAYED;
		g_arenaservers.sortkey.generic.flags	|= QMF_GRAYED;
		g_arenaservers.showempty.generic.flags	|= QMF_GRAYED;
		g_arenaservers.list.generic.flags		|= QMF_GRAYED;
		g_arenaservers.refresh.generic.flags	|= QMF_GRAYED;
		g_arenaservers.go.generic.flags			|= QMF_GRAYED;

		// don't display thumbnails during refresh, so any lag from image loading
		// doesn't interfere with server query
		g_arenaservers.mappic.generic.name = NULL;
		g_arenaservers.mappic.shader = 0;
	}

	else {
		// no refresh in progress
		g_arenaservers.master.generic.flags		&= ~QMF_GRAYED;
		g_arenaservers.ipprotocol.generic.flags	&= ~QMF_GRAYED;
		g_arenaservers.gametype.generic.flags	&= ~QMF_GRAYED;
		g_arenaservers.sortkey.generic.flags	&= ~QMF_GRAYED;
		g_arenaservers.showempty.generic.flags	&= ~QMF_GRAYED;
		g_arenaservers.list.generic.flags		&= ~QMF_GRAYED;
		g_arenaservers.refresh.generic.flags	&= ~QMF_GRAYED;
		g_arenaservers.go.generic.flags			&= ~QMF_GRAYED;

		if ( !g_arenaservers.list.numitems ) {
			g_arenaservers.list.generic.flags	|= QMF_GRAYED;
			g_arenaservers.go.generic.flags		|= QMF_GRAYED;
		}

		if ( g_arenaservers.numqueriedservers < 0 ) {
			Q_strncpyz( g_arenaservers.statusBuffer, menu_normal_text[MNT_NORESPONSE_MASTER],
					sizeof( g_arenaservers.statusBuffer ) );
		} else if ( g_arenaservers.numqueriedservers == 0 ) {
			Q_strncpyz( g_arenaservers.statusBuffer, menu_normal_text[MNT_NO_SERVERS],
					sizeof( g_arenaservers.statusBuffer ) );
		} else {
			Com_sprintf( g_arenaservers.statusBuffer, sizeof( g_arenaservers.statusBuffer ),
					menu_normal_text[MNT_CNT_SERVERS], g_arenaservers.list.numitems, protocolMatches );
		}

		// update picture
		ArenaServers_UpdatePicture();
	}
}

/*
=================
ArenaServers_Favorites_Import

Load favorite servers list from cvars.
=================
*/
void ArenaServers_Favorites_Import( void )
{
	int				i;
	char			adrstr[MAX_ADDRESSLENGTH];

	g_arenaservers.numfavoriteaddresses = 0;

	for (i=0; i<MAX_FAVORITESERVERS; i++)
	{
		trap_Cvar_VariableStringBuffer( va("server%d",i+1), adrstr, MAX_ADDRESSLENGTH );
		if (!adrstr[0])
			continue;

		// quick sanity check to avoid slow domain name resolving
		// first character must be numeric
		if (adrstr[0] != '[' && (adrstr[0] < '0' || adrstr[0] > '9'))
			continue;

		// favorite server addresses must be maintained outside refresh list
		// this mimics local and global netadr's stored in client
		// these can be fetched to fill ping list
		Q_strncpyz( g_arenaservers.favoriteaddresses[g_arenaservers.numfavoriteaddresses], adrstr,
				sizeof( g_arenaservers.favoriteaddresses[g_arenaservers.numfavoriteaddresses] ) );
		g_arenaservers.numfavoriteaddresses++;
	}
}

/*
=================
ArenaServers_Favorites_Export

Write favorite server list to cvars.
=================
*/
static void ArenaServers_Favorites_Export( void )
{
	int	i;

	for (i=0; i<g_arenaservers.numfavoriteaddresses; i++)
		trap_Cvar_Set( va("server%d",i+1), g_arenaservers.favoriteaddresses[i] );

	for (; i<MAX_FAVORITESERVERS; i++)
		trap_Cvar_Set( va("server%d",i+1), "" );
}

/*
=================
ArenaServers_Favorites_Remove
=================
*/
static void ArenaServers_Favorites_Remove( void )
{
	int				i;
	servernode_t*	servernodeptr;
	table_t*		tableptr;

	if (!g_arenaservers.list.numitems)
		return;

	// remove selected item from display list
	// items are in scattered order due to sort and cull
	// perform delete on list box contents, resync all lists

	tableptr      = &g_arenaservers.table[g_arenaservers.list.curvalue];
	servernodeptr = tableptr->servernode;

	// find address in master list
	for (i=0; i<g_arenaservers.numfavoriteaddresses; i++)
		if (!Q_stricmp(g_arenaservers.favoriteaddresses[i],servernodeptr->adrstr))
				break;

	// delete address from master list
	if (i <= g_arenaservers.numfavoriteaddresses-1)
	{
		if (i < g_arenaservers.numfavoriteaddresses-1)
		{
			// shift items up
			memmove( &g_arenaservers.favoriteaddresses[i], &g_arenaservers.favoriteaddresses[i+1],
					(g_arenaservers.numfavoriteaddresses - i - 1)*sizeof(g_arenaservers.favoriteaddresses[i]));
		}
		g_arenaservers.numfavoriteaddresses--;
	}

	// find address in server list
	for (i=0; i<g_numservers; i++)
		if (&g_serverlist[i] == servernodeptr)
				break;

	// delete address from server list
	if (i <= g_numservers-1)
	{
		if (i < g_numservers-1)
		{
			// shift items up
			memmove( &g_serverlist[i], &g_serverlist[i+1], (g_numservers - i - 1)*sizeof(g_serverlist[i]));
		}
		g_numservers--;
	}

	g_arenaservers.numqueriedservers = g_arenaservers.numfavoriteaddresses;
	g_arenaservers.currentping       = g_arenaservers.numfavoriteaddresses;
	ArenaServers_UpdateMenu();
	ArenaServers_Favorites_Export();
}

/*
=================
ArenaServers_Favorites_Add

Add current server to favorites
=================
*/
void ArenaServers_Favorites_Add( void )
{
	if ( g_arenaservers.list.curvalue < g_arenaservers.list.numitems &&
			g_arenaservers.numfavoriteaddresses < MAX_FAVORITESERVERS ) {
		const char *adr = g_arenaservers.table[g_arenaservers.list.curvalue].servernode->adrstr;
		int i;

		for ( i = 0; i < g_arenaservers.numfavoriteaddresses; i++ ) {
			if ( !Q_stricmp( adr, g_arenaservers.favoriteaddresses[i] ) ) {
				// already in list
				return;
			}
		}

		Q_strncpyz( g_arenaservers.favoriteaddresses[g_arenaservers.numfavoriteaddresses], adr,
				sizeof( g_arenaservers.favoriteaddresses[g_arenaservers.numfavoriteaddresses] ) );
		g_arenaservers.numfavoriteaddresses++;
		ArenaServers_Favorites_Export();
	}
}

/*
=================
ArenaServers_StopRefresh
=================
*/
static void ArenaServers_StopRefresh( void )
{
	if (!g_arenaservers.refreshservers)
		// not currently refreshing
		return;

	g_arenaservers.refreshservers = qfalse;

	// final tally
	if (g_arenaservers.numqueriedservers >= 0)
	{
		g_arenaservers.currentping       = g_numservers;
		g_arenaservers.numqueriedservers = g_numservers;
	}

	ArenaServers_UpdateMenu();
}

/*
=================
ArenaServers_UpdateIpv6Flags

Should be called after each new server added to g_serverlist.
=================
*/
static void ArenaServers_UpdateIpv6Flags( void ) {
	if ( EF_WARN_ASSERT( g_numservers > 0 ) ) {
		int i;
		int existingCount = g_numservers - 1;
		servernode_t *latest = &g_serverlist[g_numservers - 1];

		// update ip protocol of the server that was just added
		if ( ArenaServers_IsIpv6Address( latest->adrstr ) ) {
			latest->flags |= SERVERFLAG_IPV6;
		}

		// look for pairings
		for ( i = 0; i < existingCount; ++i ) {
			if ( ( g_serverlist[i].flags & SERVERFLAG_IPV6 ) != ( latest->flags & SERVERFLAG_IPV6 ) &&
					!strcmp( g_serverlist[i].hostname, latest->hostname ) ) {
				// found server with same hostname but different ip protocol than the new one
				// set paired flag on both servers; ArenaServers_UpdateMenu will sort out which one(s) to display
				g_serverlist[i].flags |= SERVERFLAG_IP_PAIRED;
				latest->flags |= SERVERFLAG_IP_PAIRED;
			}
		}
	}
}

/*
=================
ArenaServers_InsertUnresponsive

Insert an unresponsive server into list. Only used for favorites category.
=================
*/
static void ArenaServers_InsertUnresponsive( char *adrstr ) {
	servernode_t *servernodeptr;

	if ( g_numservers >= MAX_SERVERS ) {
		// list full
		return;
	}

	// next slot
	servernodeptr = &g_serverlist[g_numservers++];
	memset( servernodeptr, 0, sizeof( *servernodeptr ) );

	Q_strncpyz( servernodeptr->adrstr, adrstr, MAX_ADDRESSLENGTH );
	Q_strncpyz( servernodeptr->hostname, adrstr, MAX_HOSTNAMELENGTH );

	servernodeptr->pingtime = ArenaServers_MaxPing();
	servernodeptr->gametype = 5;

	ArenaServers_UpdateIpv6Flags();
}

/*
=================
ArenaServers_InsertFromStatus

Add a server using data from getstatus-based query.
=================
*/
static void ArenaServers_InsertFromStatus( char* adrstr, char* info, int pingtime )
{
	servernode_t*	servernodeptr;
	char*			s;

	if ( g_numservers >= MAX_SERVERS ) {
		// list full
		return;
	}

	// next slot
	servernodeptr = &g_serverlist[g_numservers++];
	memset( servernodeptr, 0, sizeof( *servernodeptr ) );

	Q_strncpyz( servernodeptr->adrstr, adrstr, MAX_ADDRESSLENGTH );

	s = StatusParse_ValueForKey( info, "sv_hostname" );
	Q_StripColor( s );
	Q_strncpyz( servernodeptr->hostname, s, MAX_HOSTNAMELENGTH );
	Q_strupr( servernodeptr->hostname );

	s = StatusParse_ValueForKey( info, "mapname" );
	Q_StripColor( s );
	Q_strncpyz( servernodeptr->mapname, s, MAX_MAPNAMELENGTH );
	Q_strupr( servernodeptr->mapname );

	servernodeptr->maxclients = atoi( StatusParse_ValueForKey( info, "sv_maxclients" ) );
	servernodeptr->pingtime   = pingtime;
	servernodeptr->minPing    = atoi( StatusParse_ValueForKey( info, "sv_minPing" ) );
	servernodeptr->maxPing    = atoi( StatusParse_ValueForKey( info, "sv_maxPing" ) );

	servernodeptr->gametype = atoi( StatusParse_ValueForKey( info, "g_gametype" ) );
	if( servernodeptr->gametype < 0 ) {
		servernodeptr->gametype = 0;
	}
	else if( servernodeptr->gametype > 5 ) {
		servernodeptr->gametype = 5;
	}

	s = StatusParse_ValueForKey( info, "gamename");
	if( *s && Q_stricmp( s, "baseEF" ) ) {
		Q_StripColor( s );
		Q_strncpyz( servernodeptr->modname, s, sizeof(servernodeptr->modname) );
	} else if ( atoi ( StatusParse_ValueForKey( info, "g_pModAssimilation" ) ) ) {
		Q_strncpyz( servernodeptr->modname, "Assimilation", sizeof(servernodeptr->modname) );
	} else if ( atoi ( StatusParse_ValueForKey( info, "g_pModElimination" ) ) ) {
		Q_strncpyz( servernodeptr->modname, "Elimination", sizeof(servernodeptr->modname) );
	} else if ( atoi ( StatusParse_ValueForKey( info, "g_pModSpecialties" ) ) ) {
		Q_strncpyz( servernodeptr->modname, "Specialties", sizeof(servernodeptr->modname) );
	} else if ( atoi ( StatusParse_ValueForKey( info, "g_pModDisintegration" ) ) ) {
		Q_strncpyz( servernodeptr->modname, "Disintegration", sizeof(servernodeptr->modname) );
	} else if ( atoi ( StatusParse_ValueForKey( info, "g_pModActionHero" ) ) ) {
		Q_strncpyz( servernodeptr->modname, "ActionHero", sizeof(servernodeptr->modname) );
	} else {
		*servernodeptr->modname = '\0';
	}

	// count the number of players
	servernodeptr->numclients = 0;
	servernodeptr->numhumanclients = 0;
	{
		statusPlayer_t player;
		s = info;
		while ( StatusParse_ParseField( &s, NULL ) ) {
		}
		while ( StatusParse_ParsePlayer( &s, &player ) ) {
			++servernodeptr->numclients;
			if ( atoi( player.ping ) ) {
				++servernodeptr->numhumanclients;
			}
		}
	}

	// update ip type flags
	ArenaServers_UpdateIpv6Flags();
	if ( servernodeptr->flags & SERVERFLAG_IPV6 ) {
		if ( atoi( StatusParse_ValueForKey( info, "sv_forceListIpv6" ) ) ) {
			servernodeptr->flags |= SERVERFLAG_FORCE_LIST;
		}
	} else {
		if ( atoi( StatusParse_ValueForKey( info, "sv_forceListIpv4" ) ) ) {
			servernodeptr->flags |= SERVERFLAG_FORCE_LIST;
		}
	}
}

/*
=================
ArenaServers_InsertFromInfo

Add a server using data from getinfo-based query.
=================
*/
static void ArenaServers_InsertFromInfo( char* adrstr, char* info, int pingtime )
{
	servernode_t*	servernodeptr;
	char*			s;

	if ( g_numservers >= MAX_SERVERS ) {
		// list full
		return;
	}

	// next slot
	servernodeptr = &g_serverlist[g_numservers++];
	memset( servernodeptr, 0, sizeof( *servernodeptr ) );

	Q_strncpyz( servernodeptr->adrstr, adrstr, MAX_ADDRESSLENGTH );

	Q_strncpyz( servernodeptr->hostname, Info_ValueForKey( info, "hostname"), MAX_HOSTNAMELENGTH );
	Q_CleanStr( servernodeptr->hostname );
	Q_strupr( servernodeptr->hostname );

	Q_strncpyz( servernodeptr->mapname, Info_ValueForKey( info, "mapname"), MAX_MAPNAMELENGTH );
	Q_CleanStr( servernodeptr->mapname );
	Q_strupr( servernodeptr->mapname );

	servernodeptr->numclients = atoi( Info_ValueForKey( info, "clients") );
	servernodeptr->maxclients = atoi( Info_ValueForKey( info, "sv_maxclients") );
	servernodeptr->pingtime   = pingtime;
	servernodeptr->minPing    = atoi( Info_ValueForKey( info, "minPing") );
	servernodeptr->maxPing    = atoi( Info_ValueForKey( info, "maxPing") );

	servernodeptr->gametype = atoi( Info_ValueForKey( info, "gametype") );
	if( servernodeptr->gametype < 0 ) {
		servernodeptr->gametype = 0;
	}
	else if( servernodeptr->gametype > 5 ) {
		servernodeptr->gametype = 5;
	}

	s = Info_ValueForKey( info, "game");
	if( *s ) {
		Q_strncpyz( servernodeptr->modname, s, sizeof(servernodeptr->modname) );
	}
	else {
		*servernodeptr->modname = '\0';
	}

	// update ip type flags
	ArenaServers_UpdateIpv6Flags();
	if ( servernodeptr->flags & SERVERFLAG_IPV6 ) {
		if ( atoi( Info_ValueForKey( info, "forceListIpv6" ) ) ) {
			servernodeptr->flags |= SERVERFLAG_FORCE_LIST;
		}
	} else {
		if ( atoi( Info_ValueForKey( info, "forceListIpv4" ) ) ) {
			servernodeptr->flags |= SERVERFLAG_FORCE_LIST;
		}
	}
}

/*
=================
ArenaServers_StatusRefresh

Search for servers using getstatus query instead of original getinfo method, if engine
support is available.
=================
*/
static void ArenaServers_StatusRefresh( void )
{
	int		i;
	int		time;
	int		maxPing;
	char	adrstr[MAX_ADDRESSLENGTH];
	char	info[2048];

	// process ping results
	maxPing = ArenaServers_MaxPing();

	for ( i = 0; i < MAX_PINGREQUESTS; i++ ) {
		pinglist_t *listEntry = &g_arenaservers.pinglist[i];
		char extInfo[1024];

		if ( !listEntry->adrstr[0] ) {
			// nothing in progress
			continue;
		}

		time = uis.realtime - listEntry->start;
		if (time > maxPing)
		{
			// timed out
			if ( g_servertype == AS_FAVORITES ) {
				ArenaServers_InsertUnresponsive( listEntry->adrstr );
			}

			VMExt_FN_LAN_ServerStatus_Ext( listEntry->adrstr, NULL, 0, NULL, 0 );
		}

		else {
			if ( !VMExt_FN_LAN_ServerStatus_Ext( listEntry->adrstr, info, sizeof( info ), extInfo, sizeof( extInfo ) ) ) {
				// still waiting
				continue;
			}

			// insert ping results
			time = atoi( Info_ValueForKey( extInfo, "ping" ) );
			ArenaServers_InsertFromStatus( listEntry->adrstr, info, time );
		}

		// clear this query from internal list
		listEntry->adrstr[0] = '\0';
	}

	// get results of servers query
	// counts can increase as servers respond
	switch (g_servertype)
	{
		case AS_LOCAL:
			g_arenaservers.numqueriedservers = trap_LAN_GetLocalServerCount();
			break;

		case AS_GLOBAL1:
		case AS_GLOBAL2:
		case AS_GLOBAL3:
		case AS_GLOBAL4:
		case AS_GLOBAL5:
			g_arenaservers.numqueriedservers = trap_LAN_GetGlobalServerCount();
			break;

		case AS_FAVORITES:
			g_arenaservers.numqueriedservers = g_arenaservers.numfavoriteaddresses;
			break;
	}

//	if (g_arenaservers.numqueriedservers > g_arenaservers.maxservers)
//		g_arenaservers.numqueriedservers = g_arenaservers.maxservers;

	// send ping requests in reasonable bursts
	// iterate ping through all found servers
	for (i=0; i<MAX_PINGREQUESTS && g_arenaservers.currentping < g_arenaservers.numqueriedservers; i++)
	{
		if ( g_arenaservers.pinglist[i].adrstr[0] ) {
			// already querying in this slot
			continue;
		}

		// get an address to ping
		switch (g_servertype)
		{
			case AS_LOCAL:
				trap_LAN_GetLocalServerAddressString( g_arenaservers.currentping, adrstr, MAX_ADDRESSLENGTH );
				break;

			case AS_GLOBAL1:
			case AS_GLOBAL2:
			case AS_GLOBAL3:
			case AS_GLOBAL4:
			case AS_GLOBAL5:
				trap_LAN_GetGlobalServerAddressString( g_arenaservers.currentping, adrstr, MAX_ADDRESSLENGTH );
				break;

			case AS_FAVORITES:
				Q_strncpyz( adrstr, g_arenaservers.favoriteaddresses[g_arenaservers.currentping], sizeof( adrstr ) );
				break;

		}

		Q_strncpyz( g_arenaservers.pinglist[i].adrstr, adrstr, sizeof( g_arenaservers.pinglist[i].adrstr ) );
		g_arenaservers.pinglist[i].start = uis.realtime;

		// advance to next server
		g_arenaservers.currentping++;
	}

	for ( i = 0; i < MAX_PINGREQUESTS && !g_arenaservers.pinglist[i].adrstr[0]; i++ ) {
	}
	if ( !INTERNET_REFRESH_HOLD && i == MAX_PINGREQUESTS ) {
		// all pings completed
		ArenaServers_StopRefresh();
		return;
	}

	// update the user interface with ping status
	ArenaServers_UpdateMenu();
}

/*
=================
ArenaServers_DoRefresh
=================
*/
static void ArenaServers_DoRefresh( void )
{
	int		i;
	int		j;
	int		time;
	int		maxPing;
	char	adrstr[MAX_ADDRESSLENGTH];
	char	info[MAX_INFO_STRING];

	// allow max 5 seconds for first results
	if (uis.realtime < g_arenaservers.refreshStartTime + 5000)
	{
		// wait for timeout or results
		if (g_servertype == AS_LOCAL)
		{
			if (!trap_LAN_GetLocalServerCount())
			{
				// still waiting for response
				return;
			}
		}
		else if(( g_servertype >= AS_GLOBAL1) && ( g_servertype <= AS_GLOBAL5))
		{
			if (trap_LAN_GetGlobalServerCount() < 0)
			{
				// still waiting for master server response
				return;
			}
		}
	}

	if (uis.realtime < g_arenaservers.nextpingtime)
	{
		// wait for time trigger
		return;
	}

	// trigger at 10Hz intervals
	g_arenaservers.nextpingtime = uis.realtime + 50;

	if ( g_arenaservers.statusQueryEnabled ) {
		ArenaServers_StatusRefresh();
		return;
	}

	// process ping results
	maxPing = ArenaServers_MaxPing();
	for (i=0; i<MAX_PINGREQUESTS; i++)
	{
		trap_LAN_GetPing( i, adrstr, MAX_ADDRESSLENGTH, &time );
		if (!adrstr[0])
		{
			// ignore empty or pending pings
			continue;
		}

		// find ping result in our local list
		for (j=0; j<MAX_PINGREQUESTS; j++)
			if (!Q_stricmp( adrstr, g_arenaservers.pinglist[j].adrstr ))
				break;

		if (j < MAX_PINGREQUESTS)
		{
			// found it
			if (!time)
			{
				time = uis.realtime - g_arenaservers.pinglist[j].start;
				if (time < maxPing)
				{
					// still waiting
					continue;
				}
			}

			if (time > maxPing)
			{
				// timed out
				if ( g_servertype == AS_FAVORITES ) {
					ArenaServers_InsertUnresponsive( adrstr );
				}
			}
			else
			{
				// insert ping results
				trap_LAN_GetPingInfo( i, info, MAX_INFO_STRING );
				ArenaServers_InsertFromInfo( adrstr, info, time );
			}

			// clear this query from internal list
			g_arenaservers.pinglist[j].adrstr[0] = '\0';
		}

		// clear this query from external list
		trap_LAN_ClearPing( i );
	}

	// get results of servers query
	// counts can increase as servers respond
	switch (g_servertype)
	{
		case AS_LOCAL:
			g_arenaservers.numqueriedservers = trap_LAN_GetLocalServerCount();
			break;

		case AS_GLOBAL1:
		case AS_GLOBAL2:
		case AS_GLOBAL3:
		case AS_GLOBAL4:
		case AS_GLOBAL5:
			g_arenaservers.numqueriedservers = trap_LAN_GetGlobalServerCount();
			break;

		case AS_FAVORITES:
			g_arenaservers.numqueriedservers = g_arenaservers.numfavoriteaddresses;
			break;
	}

//	if (g_arenaservers.numqueriedservers > g_arenaservers.maxservers)
//		g_arenaservers.numqueriedservers = g_arenaservers.maxservers;

	// send ping requests in reasonable bursts
	// iterate ping through all found servers
	for (i=0; i<MAX_PINGREQUESTS && g_arenaservers.currentping < g_arenaservers.numqueriedservers; i++)
	{
		if (trap_LAN_GetPingQueueCount() >= MAX_PINGREQUESTS)
		{
			// ping queue is full
			break;
		}

		// find empty slot
		for (j=0; j<MAX_PINGREQUESTS; j++)
			if (!g_arenaservers.pinglist[j].adrstr[0])
				break;

		if (j >= MAX_PINGREQUESTS)
			// no empty slots available yet - wait for timeout
			break;

		// get an address to ping
		switch (g_servertype)
		{
			case AS_LOCAL:
				trap_LAN_GetLocalServerAddressString( g_arenaservers.currentping, adrstr, MAX_ADDRESSLENGTH );
				break;

			case AS_GLOBAL1:
			case AS_GLOBAL2:
			case AS_GLOBAL3:
			case AS_GLOBAL4:
			case AS_GLOBAL5:
				trap_LAN_GetGlobalServerAddressString( g_arenaservers.currentping, adrstr, MAX_ADDRESSLENGTH );
				break;

			case AS_FAVORITES:
				Q_strncpyz( adrstr, g_arenaservers.favoriteaddresses[g_arenaservers.currentping], sizeof( adrstr ) );
				break;

		}

		Q_strncpyz( g_arenaservers.pinglist[j].adrstr, adrstr, sizeof( g_arenaservers.pinglist[j].adrstr ) );
		g_arenaservers.pinglist[j].start = uis.realtime;

		trap_Cmd_ExecuteText( EXEC_NOW, va( "ping %s\n", adrstr )  );

		// advance to next server
		g_arenaservers.currentping++;
	}

	if (!INTERNET_REFRESH_HOLD && !trap_LAN_GetPingQueueCount())
	{
		// all pings completed
		ArenaServers_StopRefresh();
		return;
	}

	// update the user interface with ping status
	ArenaServers_UpdateMenu();
}

/*
=================
ArenaServers_StartRefresh
=================
*/
static void ArenaServers_StartRefresh( void )
{
	int		i;

	for (i=0; i<MAX_PINGREQUESTS; i++)
	{
		g_arenaservers.pinglist[i].adrstr[0] = '\0';
		trap_LAN_ClearPing( i );
	}

	// abort any pending status requests
	if ( VMExt_FNAvailable_LAN_ServerStatus_Ext() ) {
		VMExt_FN_LAN_ServerStatus_Ext( NULL, NULL, 0, NULL, 0 );
	}

	g_arenaservers.refreshservers    = qtrue;
	g_arenaservers.currentping       = 0;
	g_arenaservers.nextpingtime      = 0;
	g_numservers                     = 0;
	g_arenaservers.numqueriedservers = 0;
	g_arenaservers.refreshStartTime  = uis.realtime;

	// place menu in zeroed state
	ArenaServers_UpdateMenu();

	if ( g_servertype == AS_LOCAL ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "localservers\n" );

	} else if ( g_servertype >= AS_GLOBAL1 && g_servertype <= AS_GLOBAL5 ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "globalservers %d %d empty full\n", g_servertype - AS_GLOBAL1,
				(int)trap_Cvar_VariableValue( "protocol" ) ) );
	}
}

/*
=================
ArenaServers_SetType
=================
*/
void ArenaServers_SetType( int type )
{
	qboolean multi_fetch = MASTER_MULTI_FETCH_ACTIVE ? qtrue : qfalse;
	char IP[128];

	if ( g_servertype != type ) {
		// trigger a refresh
		g_numservers = 0;
	}

	if (type == AS_GLOBAL1)
	{
		if (!multi_fetch)
		{
			trap_Cvar_VariableStringBuffer( "sv_master1", IP, 128);
		}
		if (!multi_fetch && !IP[0])
		{
			type= AS_GLOBAL2;
		}
	}
	if (type == AS_GLOBAL2)
	{
		trap_Cvar_VariableStringBuffer( multi_fetch ? "sv_master1" : "sv_master2", IP, 128);
		if (!IP[0])
		{
			type= AS_GLOBAL3;
		}
	}
	if (type == AS_GLOBAL3)
	{
		trap_Cvar_VariableStringBuffer( multi_fetch ? "sv_master2" : "sv_master3", IP, 128);
		if (!IP[0])
		{
			type= AS_GLOBAL4;
		}
	}
	if (type == AS_GLOBAL4)
	{
		trap_Cvar_VariableStringBuffer( multi_fetch ? "sv_master3" : "sv_master4", IP, 128);
		if (!IP[0])
		{
			type= AS_GLOBAL5;
		}
	}
	if (type == AS_GLOBAL5)
	{
		trap_Cvar_VariableStringBuffer( multi_fetch ? "sv_master4" : "sv_master5", IP, 128);
		if (!IP[0])
		{
			type= AS_FAVORITES;
		}
	}

	g_arenaservers.master.curvalue = type;
	g_servertype = type;

	if ( type == AS_FAVORITES ) {
		g_arenaservers.favorite.generic.flags |= QMF_GRAYED;
		g_arenaservers.remove.generic.flags &= ~(QMF_INACTIVE|QMF_HIDDEN);
	} else {
		g_arenaservers.favorite.generic.flags &= ~QMF_GRAYED;
		g_arenaservers.remove.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
	}

	if( !g_numservers ) {
		ArenaServers_StartRefresh();
	}
	else {
		// avoid slow operation, use existing results
		g_arenaservers.currentping       = g_numservers;
		g_arenaservers.numqueriedservers = g_numservers;
		ArenaServers_UpdateMenu();
		Q_strncpyz( g_arenaservers.statusBuffer, menu_normal_text[MNT_HITREFRESH],
				sizeof( g_arenaservers.statusBuffer ) );
	}
}

/*
=================
ArenaServers_Event
=================
*/
static void ArenaServers_Event( void* ptr, int event ) {
	int		id;
	menuframework_s*	m;


	id = ((menucommon_s*)ptr)->id;

	if( event != QM_ACTIVATED && id != ID_LIST )
	{
		return;
	}

	m = ((menucommon_s*)ptr)->parent;

	switch( id )
	{
	case ID_MAINMENU:
		UI_PopMenu();
		UI_MainMenu();
		break;

	case ID_MASTER:
		trap_Cvar_SetValue( "ui_browserMaster", g_arenaservers.master.curvalue );
		ArenaServers_SetType( g_arenaservers.master.curvalue );
		break;

	case ID_IPPROTOCOL:
		trap_Cvar_SetValue( "ui_browserIpProtocol", g_arenaservers.ipprotocol.curvalue );
		g_ipprotocol = g_arenaservers.ipprotocol.curvalue;
		ArenaServers_UpdateMenu();
		break;

	case ID_GAMETYPE:
		trap_Cvar_SetValue( "ui_browserGameType", g_arenaservers.gametype.curvalue );
		g_gametype = g_arenaservers.gametype.curvalue;
		ArenaServers_UpdateMenu();
		break;

	case ID_SORTKEY:
		trap_Cvar_SetValue( SORT_KEY_CVAR_NAME, g_arenaservers.sortkey.curvalue );
		g_sortkey = g_arenaservers.sortkey.curvalue;
		ArenaServers_UpdateMenu();
		break;

	case ID_SHOW_EMPTY:
		if ( g_arenaservers.statusQueryEnabled ) {
			trap_Cvar_SetValue( "ui_browserPlayerType", g_arenaservers.showempty.curvalue );
		} else {
			trap_Cvar_SetValue( "ui_browserShowEmpty", g_arenaservers.showempty.curvalue );
		}
		g_emptyservers = g_arenaservers.showempty.curvalue;
		ArenaServers_UpdateMenu();
		break;

	case ID_LIST:
		if( event == QM_GOTFOCUS ) {
			ArenaServers_UpdatePicture();
		}
		break;

	case ID_SCROLL_UP:
		ScrollList_Key( &g_arenaservers.list, K_UPARROW );
		break;

	case ID_SCROLL_DOWN:
		ScrollList_Key( &g_arenaservers.list, K_DOWNARROW );
		break;

	case ID_SHOWINFO:
		if ( g_arenaservers.list.curvalue < g_arenaservers.list.numitems ) {
			BrowserServerInfo_InitMenu( g_arenaservers.table[g_arenaservers.list.curvalue].servernode->adrstr );
		}
		break;

	case ID_FAVORITE:
		ArenaServers_Favorites_Add();
		break;

	case ID_BACK:
		ArenaServers_StopRefresh();
		UI_PopMenu();
		break;

	case ID_STOPREFRESH:
		ArenaServers_StopRefresh();
		break;

	case ID_REFRESH:
		ArenaServers_StartRefresh();
		break;

	case ID_SPECIFY:
			UI_SpecifyServerMenu();
		break;

	case ID_CREATE:
		UI_StartServerMenu( qtrue );
		break;

	case ID_FINDSERVER:
		UI_ArenaServersMenu();
		break;

	case ID_CONNECT:
		ArenaServers_Go();
		break;

	case ID_REMOVE:
		ArenaServers_Favorites_Remove();
		break;
	}
}

/*
=================
ArenaServers_Graphics
=================
*/
void ArenaServers_Graphics (void)
{
	UI_MenuFrame2(&g_arenaservers.menu);
	trap_R_SetColor( colorTable[CT_LTBROWN1]);
	UI_DrawHandlePic(  30, 203,  47, 186, uis.whiteShader);	// Left side of frame

	// Frame map picture
	trap_R_SetColor( colorTable[CT_DKPURPLE3]);
	UI_DrawHandlePic(  87,  53, 158,  16, uis.whiteShader);	// Top line
	UI_DrawHandlePic( 232,  69,  13, 103, uis.whiteShader);	// Side line
	UI_DrawProportionalString(  90, 53, menu_normal_text[MNT_CURRENTMAP],UI_SMALLFONT, colorTable[CT_BLACK]);

	// Frame sort options
	trap_R_SetColor( colorTable[CT_DKPURPLE3]);
	UI_DrawHandlePic( 252,  53, 359,  16, uis.whiteShader);	// Top line
	UI_DrawHandlePic( 252,  69,  13, 103, uis.whiteShader);	// Side line
	UI_DrawProportionalString(  272, 53, menu_normal_text[MNT_SORTPARMS],UI_SMALLFONT, colorTable[CT_BLACK]);

	// Frame servers
	trap_R_SetColor( colorTable[CT_DKORANGE]);
	UI_DrawHandlePic(  87, 175, 536,  13, uis.whiteShader);	// Top line
	UI_DrawHandlePic(  87, 188,  16,  11, uis.whiteShader);	// Side line
	UI_DrawHandlePic(  87, 202,  16,  17, uis.whiteShader);	// Side line
	UI_DrawHandlePic(  87, 222,  16, 110, uis.whiteShader);	// Side line
	UI_DrawHandlePic(  87, 335,  16,  17, uis.whiteShader);	// Side line
	UI_DrawHandlePic(  87, 355,  16,  11, uis.whiteShader);	// Side line
	UI_DrawHandlePic(  87, 366, 268,  18, uis.whiteShader);	// Bottom line

	UI_DrawProportionalString(  105, 177, menu_normal_text[MNT_HOSTNAME],UI_TINYFONT, colorTable[CT_BLACK]);
	if ( g_arenaservers.statusQueryEnabled ) {
		UI_DrawProportionalString(  288, 177, menu_normal_text[MNT_MAP],UI_TINYFONT, colorTable[CT_BLACK]);
		UI_DrawProportionalString(  394, 177, menu_normal_text[MNT_PLAYERS],UI_TINYFONT, colorTable[CT_BLACK]);
		UI_DrawProportionalString(  440, 177, menu_normal_text[MNT_TYPE],UI_TINYFONT, colorTable[CT_BLACK]);
		UI_DrawProportionalString(  472, 177, menu_normal_text[MNT_BROWSER_MOD],UI_TINYFONT, colorTable[CT_BLACK]);
		UI_DrawProportionalString(  552, 177, menu_normal_text[MNT_BROWSER_BOTS],UI_TINYFONT, colorTable[CT_BLACK]);
		UI_DrawProportionalString(  582, 177, menu_normal_text[MNT_PING],UI_TINYFONT, colorTable[CT_BLACK]);
	} else {
		UI_DrawProportionalString(  296, 177, menu_normal_text[MNT_MAP],UI_TINYFONT, colorTable[CT_BLACK]);
		UI_DrawProportionalString(  410, 177, menu_normal_text[MNT_PLAYERS],UI_TINYFONT, colorTable[CT_BLACK]);
		UI_DrawProportionalString(  456, 177, menu_normal_text[MNT_TYPE],UI_TINYFONT, colorTable[CT_BLACK]);
		UI_DrawProportionalString(  488, 177, menu_normal_text[MNT_BROWSER_MOD],UI_TINYFONT, colorTable[CT_BLACK]);
		UI_DrawProportionalString(  582, 177, menu_normal_text[MNT_PING],UI_TINYFONT, colorTable[CT_BLACK]);
	}
	UI_DrawProportionalString( 606, 177, "IPV6", UI_TINYFONT, colorTable[CT_BLACK] );

	UI_DrawProportionalString(  74,  27, "819",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74, 149, "42",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74, 205, "4363",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74, 395, "6700",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	// Left rounded ends for buttons
	trap_R_SetColor( colorTable[g_arenaservers.mainmenu.color]);
	UI_DrawHandlePic(g_arenaservers.mainmenu.generic.x - 14, g_arenaservers.mainmenu.generic.y,
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	trap_R_SetColor( colorTable[g_arenaservers.back.color]);
	UI_DrawHandlePic(g_arenaservers.back.generic.x - 14, g_arenaservers.back.generic.y,
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

}

/*
=================
ArenaServers_MenuDraw
=================
*/
static void ArenaServers_MenuDraw( void )
{
	ArenaServers_Graphics();

	if (g_arenaservers.refreshservers)
	{
		ArenaServers_DoRefresh();
	}

	trap_R_SetColor( colorTable[CT_WHITE]);
	Menu_Draw( &g_arenaservers.menu );
}

/*
=================
ArenaServers_MenuKey
=================
*/
static sfxHandle_t ArenaServers_MenuKey( int key ) {
	if( key == K_SPACE  && g_arenaservers.refreshservers ) {
		ArenaServers_StopRefresh();
		return menu_move_sound;
	}

	if( ( key == K_DEL || key == K_KP_DEL ) && ( g_servertype == AS_FAVORITES ) &&
		( Menu_ItemAtCursor( &g_arenaservers.menu) == &g_arenaservers.list ) ) {
		ArenaServers_Favorites_Remove();
		ArenaServers_UpdateMenu();
		return menu_move_sound;
	}

	if( key == K_MOUSE2 || key == K_ESCAPE ) {
		ArenaServers_StopRefresh();
	}


	return Menu_DefaultKey( &g_arenaservers.menu, key );
}

/*
=================
ArenaServers_MenuInit
=================
*/
static void ArenaServers_MenuInit( void )
{
	int i;
	int y;

	// zero set all our globals
	memset( &g_arenaservers, 0 ,sizeof(arenaservers_t) );

	g_arenaservers.statusQueryEnabled = VMExt_FNAvailable_LAN_ServerStatus_Ext();

	ArenaServers_Cache();

	g_arenaservers.menu.fullscreen					= qtrue;
	g_arenaservers.menu.wrapAround					= qtrue;
	g_arenaservers.menu.draw						= ArenaServers_MenuDraw;
	g_arenaservers.menu.key							= ArenaServers_MenuKey;
	g_arenaservers.menu.descX						= MENU_DESC_X;
	g_arenaservers.menu.descY						= MENU_DESC_Y;
	g_arenaservers.menu.titleX						= MENU_TITLE_X;
	g_arenaservers.menu.titleY						= MENU_TITLE_Y;
	g_arenaservers.menu.titleI						= MNT_MULTIPLAYER_TITLE;
	g_arenaservers.menu.footNoteEnum				= MNT_FINDSERVER;


	g_arenaservers.mainmenu.generic.type			= MTYPE_BITMAP;
	g_arenaservers.mainmenu.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.mainmenu.generic.x				= 110;
	g_arenaservers.mainmenu.generic.y				= 391;
	g_arenaservers.mainmenu.generic.name			= BUTTON_GRAPHIC_LONGRIGHT;
	g_arenaservers.mainmenu.generic.id				= ID_MAINMENU;
	g_arenaservers.mainmenu.generic.callback		= ArenaServers_Event;
	g_arenaservers.mainmenu.width					= MENU_BUTTON_MED_WIDTH;
	g_arenaservers.mainmenu.height					= MENU_BUTTON_MED_HEIGHT;
	g_arenaservers.mainmenu.color					= CT_DKPURPLE1;
	g_arenaservers.mainmenu.color2					= CT_LTPURPLE1;
	g_arenaservers.mainmenu.textX					= 5;
	g_arenaservers.mainmenu.textY					= 2;
	g_arenaservers.mainmenu.textEnum				= MBT_MAINMENU;
	g_arenaservers.mainmenu.textcolor				= CT_BLACK;
	g_arenaservers.mainmenu.textcolor2				= CT_WHITE;

	g_arenaservers.back.generic.type				= MTYPE_BITMAP;
	g_arenaservers.back.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.back.generic.x					= 110;
	g_arenaservers.back.generic.y					= 415;
	g_arenaservers.back.generic.name				= BUTTON_GRAPHIC_LONGRIGHT;
	g_arenaservers.back.generic.id					= ID_BACK;
	g_arenaservers.back.generic.callback			= ArenaServers_Event;
	g_arenaservers.back.width						= MENU_BUTTON_MED_WIDTH;
	g_arenaservers.back.height						= MENU_BUTTON_MED_HEIGHT;
	g_arenaservers.back.color						= CT_DKPURPLE1;
	g_arenaservers.back.color2						= CT_LTPURPLE1;
	g_arenaservers.back.textX						= 5;
	g_arenaservers.back.textY						= 2;
	g_arenaservers.back.textEnum					= MBT_BACK;
	g_arenaservers.back.textcolor					= CT_BLACK;
	g_arenaservers.back.textcolor2					= CT_WHITE;

	y = 72;
	g_arenaservers.master.generic.type				= MTYPE_SPINCONTROL;
	g_arenaservers.master.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.master.generic.callback			= ArenaServers_Event;
	g_arenaservers.master.generic.id				= ID_MASTER;
	g_arenaservers.master.generic.x					= 274;
	g_arenaservers.master.generic.y					= y;
	g_arenaservers.master.textEnum					= MBT_SERVERS;
	g_arenaservers.master.textcolor					= CT_BLACK;
	g_arenaservers.master.textcolor2				= CT_WHITE;
	g_arenaservers.master.color						= CT_DKPURPLE1;
	g_arenaservers.master.color2					= CT_LTPURPLE1;
	g_arenaservers.master.width						= 80;
	g_arenaservers.master.textX						= 5;
	g_arenaservers.master.textY						= 2;
	g_arenaservers.master.listnames					= MASTER_MULTI_FETCH_ACTIVE ? master_items_multi_fetch : master_items;

	y += 20;
	g_arenaservers.ipprotocol.generic.type			= MTYPE_SPINCONTROL;
	g_arenaservers.ipprotocol.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.ipprotocol.generic.callback		= ArenaServers_Event;
	g_arenaservers.ipprotocol.generic.id			= ID_IPPROTOCOL;
	g_arenaservers.ipprotocol.generic.x				= 274;
	g_arenaservers.ipprotocol.generic.y				= y;
	g_arenaservers.ipprotocol.textEnum				= MBT_BROWSER_IPPROTOCOL;
	g_arenaservers.ipprotocol.textcolor				= CT_BLACK;
	g_arenaservers.ipprotocol.textcolor2			= CT_WHITE;
	g_arenaservers.ipprotocol.color					= CT_DKPURPLE1;
	g_arenaservers.ipprotocol.color2				= CT_LTPURPLE1;
	g_arenaservers.ipprotocol.width					= 80;
	g_arenaservers.ipprotocol.textX					= 5;
	g_arenaservers.ipprotocol.textY					= 2;
	g_arenaservers.ipprotocol.listnames				= ipprotocol_items;

	y += 20;
	g_arenaservers.gametype.generic.type			= MTYPE_SPINCONTROL;
	g_arenaservers.gametype.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.gametype.generic.callback		= ArenaServers_Event;
	g_arenaservers.gametype.generic.id				= ID_GAMETYPE;
	g_arenaservers.gametype.generic.x				= 274;
	g_arenaservers.gametype.generic.y				= y;
	g_arenaservers.gametype.textEnum				= MBT_SERVERTYPE_GAMETYPE;
	g_arenaservers.gametype.textcolor				= CT_BLACK;
	g_arenaservers.gametype.textcolor2				= CT_WHITE;
	g_arenaservers.gametype.color					= CT_DKPURPLE1;
	g_arenaservers.gametype.color2					= CT_LTPURPLE1;
	g_arenaservers.gametype.width					= 80;
	g_arenaservers.gametype.textX					= 5;
	g_arenaservers.gametype.textY					= 2;
	g_arenaservers.gametype.listnames				= servertype_items;

	y += 20;
	g_arenaservers.sortkey.generic.type				= MTYPE_SPINCONTROL;
	g_arenaservers.sortkey.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.sortkey.generic.callback			= ArenaServers_Event;
	g_arenaservers.sortkey.generic.id				= ID_SORTKEY;
	g_arenaservers.sortkey.generic.x				= 274;
	g_arenaservers.sortkey.generic.y				= y;
	g_arenaservers.sortkey.textEnum					= MBT_SORTTYPE;
	g_arenaservers.sortkey.textcolor				= CT_BLACK;
	g_arenaservers.sortkey.textcolor2				= CT_WHITE;
	g_arenaservers.sortkey.color					= CT_DKPURPLE1;
	g_arenaservers.sortkey.color2					= CT_LTPURPLE1;
	g_arenaservers.sortkey.width					= 80;
	g_arenaservers.sortkey.textX					= 5;
	g_arenaservers.sortkey.textY					= 2;
	g_arenaservers.sortkey.listnames				= sortkey_items;

	y += 20;
	g_arenaservers.showempty.generic.type			= MTYPE_SPINCONTROL;
	g_arenaservers.showempty.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.showempty.generic.callback		= ArenaServers_Event;
	g_arenaservers.showempty.generic.id				= ID_SHOW_EMPTY;
	g_arenaservers.showempty.generic.x				= 274;
	g_arenaservers.showempty.generic.y				= y;
	g_arenaservers.showempty.textEnum				= g_arenaservers.statusQueryEnabled ? MBT_BROWSER_PLAYERTYPE : MBT_SHOWEMPTY;
	g_arenaservers.showempty.textcolor				= CT_BLACK;
	g_arenaservers.showempty.textcolor2				= CT_WHITE;
	g_arenaservers.showempty.color					= CT_DKPURPLE1;
	g_arenaservers.showempty.color2					= CT_LTPURPLE1;
	g_arenaservers.showempty.width					= 80;
	g_arenaservers.showempty.textX					= 5;
	g_arenaservers.showempty.textY					= 2;
	g_arenaservers.showempty.listnames				= g_arenaservers.statusQueryEnabled ? playerstatus_items : noyes_items;

	g_arenaservers.list.generic.type			= MTYPE_SCROLLLIST;
	g_arenaservers.list.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.list.generic.id				= ID_LIST;
	g_arenaservers.list.generic.callback		= ArenaServers_Event;
	g_arenaservers.list.generic.x				= 103;
	g_arenaservers.list.generic.y				= 188;
	g_arenaservers.list.width					= MAX_LISTBOXWIDTH;
	g_arenaservers.list.height					= 11;
	g_arenaservers.list.itemnames				= (const char **)g_arenaservers.items;
	for( i = 0; i < MAX_SERVERS; i++ )
	{
		g_arenaservers.items[i] = g_arenaservers.table[i].buff;
	}

	g_arenaservers.mappic.generic.type			= MTYPE_BITMAP;
	g_arenaservers.mappic.generic.flags			= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	g_arenaservers.mappic.generic.x				= 95;
	g_arenaservers.mappic.generic.y				= 71;
	g_arenaservers.mappic.width					= 128;
	g_arenaservers.mappic.height				= 96;
	g_arenaservers.mappic.errorpic				= ART_UNKNOWNMAP;

	g_arenaservers.up.generic.type				= MTYPE_BITMAP;
	g_arenaservers.up.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS|QMF_MOUSEONLY;
	g_arenaservers.up.generic.callback			= ArenaServers_Event;
	g_arenaservers.up.generic.id				= ID_SCROLL_UP;
	g_arenaservers.up.generic.x					= 87;
	g_arenaservers.up.generic.y					= 204;
	g_arenaservers.up.width						= 16;
	g_arenaservers.up.height					= 16;
	g_arenaservers.up.color  					= CT_DKGOLD1;
	g_arenaservers.up.color2  					= CT_LTGOLD1;
	g_arenaservers.up.generic.name				= "menu/common/arrow_up_16.tga";
	g_arenaservers.up.textEnum					= MBT_ARROW_UP;

	g_arenaservers.down.generic.type			= MTYPE_BITMAP;
	g_arenaservers.down.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS|QMF_MOUSEONLY;
	g_arenaservers.down.generic.callback		= ArenaServers_Event;
	g_arenaservers.down.generic.id				= ID_SCROLL_DOWN;
	g_arenaservers.down.generic.x				= 87;
	g_arenaservers.down.generic.y				= 337;
	g_arenaservers.down.width					= 16;
	g_arenaservers.down.height					= 16;
	g_arenaservers.down.color  					= CT_DKGOLD1;
	g_arenaservers.down.color2  				= CT_LTGOLD1;
	g_arenaservers.down.generic.name			= "menu/common/arrow_dn_16.tga";
	g_arenaservers.down.textEnum				= MBT_ARROW_DOWN;

	g_arenaservers.showinfo.generic.type			= MTYPE_BITMAP;
	g_arenaservers.showinfo.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.showinfo.generic.callback		= ArenaServers_Event;
	g_arenaservers.showinfo.generic.id				= ID_SHOWINFO;
	g_arenaservers.showinfo.generic.x				= 285;
	g_arenaservers.showinfo.generic.y				= 391;
	g_arenaservers.showinfo.width					= MENU_BUTTON_MED_WIDTH + 15;
	g_arenaservers.showinfo.height					= MENU_BUTTON_MED_HEIGHT;
	g_arenaservers.showinfo.generic.name			= BUTTON_GRAPHIC_LONGRIGHT;
	g_arenaservers.showinfo.textEnum				= MBT_INGAMESERVERDATA;
	g_arenaservers.showinfo.color					= CT_DKPURPLE1;
	g_arenaservers.showinfo.color2					= CT_LTPURPLE1;
	g_arenaservers.showinfo.textX					= 5;
	g_arenaservers.showinfo.textY					= 2;
	g_arenaservers.showinfo.textcolor				= CT_BLACK;
	g_arenaservers.showinfo.textcolor2				= CT_WHITE;

	g_arenaservers.favorite.generic.type			= MTYPE_BITMAP;
	g_arenaservers.favorite.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.favorite.generic.callback		= ArenaServers_Event;
	g_arenaservers.favorite.generic.id				= ID_FAVORITE;
	g_arenaservers.favorite.generic.x				= 285;
	g_arenaservers.favorite.generic.y				= 415;
	g_arenaservers.favorite.width					= MENU_BUTTON_MED_WIDTH + 15;
	g_arenaservers.favorite.height					= MENU_BUTTON_MED_HEIGHT;
	g_arenaservers.favorite.generic.name			= BUTTON_GRAPHIC_LONGRIGHT;
	g_arenaservers.favorite.textEnum				= MBT_ADDTOFAVS;
	g_arenaservers.favorite.color					= CT_DKPURPLE1;
	g_arenaservers.favorite.color2					= CT_LTPURPLE1;
	g_arenaservers.favorite.textX					= 5;
	g_arenaservers.favorite.textY					= 2;
	g_arenaservers.favorite.textcolor				= CT_BLACK;
	g_arenaservers.favorite.textcolor2				= CT_WHITE;


	g_arenaservers.status.generic.type		= MTYPE_TEXT;
	g_arenaservers.status.generic.flags		= QMF_INACTIVE;
	g_arenaservers.status.generic.x			= 129;
	g_arenaservers.status.generic.y			= 368;
	g_arenaservers.status.string			= g_arenaservers.statusBuffer;
	g_arenaservers.status.style				= UI_SMALLFONT;
	g_arenaservers.status.color				= menu_text_color;

	g_arenaservers.refresh.generic.type			= MTYPE_BITMAP;
	g_arenaservers.refresh.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.refresh.generic.callback		= ArenaServers_Event;
	g_arenaservers.refresh.generic.id			= ID_REFRESH;
	g_arenaservers.refresh.generic.x			= 358;
	g_arenaservers.refresh.generic.y			= 366;
	g_arenaservers.refresh.generic.name			= "menu/common/square.tga";
	g_arenaservers.refresh.generic.callback		= ArenaServers_Event;
	g_arenaservers.refresh.width				= 85;
	g_arenaservers.refresh.height				= MENU_BUTTON_MED_HEIGHT;
	g_arenaservers.refresh.color				= CT_DKPURPLE1;
	g_arenaservers.refresh.color2				= CT_LTPURPLE1;
	g_arenaservers.refresh.textX				= 5;
	g_arenaservers.refresh.textY				= 2;
	g_arenaservers.refresh.textEnum				= MBT_REFRESH;
	g_arenaservers.refresh.textcolor			= CT_BLACK;
	g_arenaservers.refresh.textcolor2			= CT_WHITE;

	g_arenaservers.stoprefresh.generic.type			= MTYPE_BITMAP;
	g_arenaservers.stoprefresh.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.stoprefresh.generic.callback		= ArenaServers_Event;
	g_arenaservers.stoprefresh.generic.id			= ID_STOPREFRESH;
	g_arenaservers.stoprefresh.generic.x			= 446;
	g_arenaservers.stoprefresh.generic.y			= 366;
	g_arenaservers.stoprefresh.generic.name			= "menu/common/square.tga";
	g_arenaservers.stoprefresh.width				= 85;
	g_arenaservers.stoprefresh.height				= MENU_BUTTON_MED_HEIGHT;
	g_arenaservers.stoprefresh.color				= CT_DKPURPLE1;
	g_arenaservers.stoprefresh.color2				= CT_LTPURPLE1;
	g_arenaservers.stoprefresh.textX				= 5;
	g_arenaservers.stoprefresh.textY				= 2;
	g_arenaservers.stoprefresh.textEnum				= MBT_STOPREFRESH;
	g_arenaservers.stoprefresh.textcolor			= CT_BLACK;
	g_arenaservers.stoprefresh.textcolor2			= CT_WHITE;

	g_arenaservers.remove.generic.type				= MTYPE_BITMAP;
	g_arenaservers.remove.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.remove.generic.callback			= ArenaServers_Event;
	g_arenaservers.remove.generic.id				= ID_REMOVE;
	g_arenaservers.remove.generic.x					= 534;
	g_arenaservers.remove.generic.y					= 366;
	g_arenaservers.remove.generic.name				= "menu/common/square.tga";
	g_arenaservers.remove.width						= 85;
	g_arenaservers.remove.height					= MENU_BUTTON_MED_HEIGHT;
	g_arenaservers.remove.color						= CT_DKPURPLE1;
	g_arenaservers.remove.color2					= CT_LTPURPLE1;
	g_arenaservers.remove.textX						= 5;
	g_arenaservers.remove.textY						= 2;
	g_arenaservers.remove.textEnum					= MBT_REMOVE;
	g_arenaservers.remove.textcolor					= CT_BLACK;
	g_arenaservers.remove.textcolor2				= CT_WHITE;

	g_arenaservers.go.generic.type			= MTYPE_BITMAP;
	g_arenaservers.go.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.go.generic.x				= 462;
	g_arenaservers.go.generic.y				= 391;
	g_arenaservers.go.generic.name			= "menu/common/square.tga";
	g_arenaservers.go.generic.id			= ID_CONNECT;
	g_arenaservers.go.generic.callback		= ArenaServers_Event;
	g_arenaservers.go.width					= 150;
	g_arenaservers.go.height				= 42;
	g_arenaservers.go.color					= CT_DKPURPLE1;
	g_arenaservers.go.color2				= CT_LTPURPLE1;
	g_arenaservers.go.textX					= 5;
	g_arenaservers.go.textY					= 2;
	g_arenaservers.go.textEnum				= MBT_ENGAGEMULTIPLAYER;
	g_arenaservers.go.textcolor				= CT_BLACK;
	g_arenaservers.go.textcolor2			= CT_WHITE;

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.mainmenu );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.back );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.master );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.ipprotocol );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.gametype );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.sortkey );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.showempty );

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.mappic );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.list );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.status );

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.up );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.down );

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.refresh );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.stoprefresh );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.remove );
	if ( g_arenaservers.statusQueryEnabled ) {
		Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.showinfo );
	}
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.favorite );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.go );

	ArenaServers_Favorites_Import();

	g_ipprotocol = Com_Clamp( 0, 1, ui_browserIpProtocol.integer );
	g_arenaservers.ipprotocol.curvalue = g_ipprotocol;

	g_servertype = Com_Clamp( 0, NUM_MASTER_ITEMS - 1, ui_browserMaster.integer );
	g_arenaservers.master.curvalue = g_servertype;

	g_gametype = Com_Clamp( 0, 4, ui_browserGameType.integer );
	g_arenaservers.gametype.curvalue = g_gametype;

	g_sortkey = Com_Clamp( 0, 4, SORT_KEY_CVAR->integer );
	g_arenaservers.sortkey.curvalue = g_sortkey;

	if ( g_arenaservers.statusQueryEnabled ) {
		g_emptyservers = Com_Clamp( 0, 2, ui_browserPlayerType.integer );
	} else {
		g_emptyservers = Com_Clamp( 0, 1, ui_browserShowEmpty.integer );
	}
	g_arenaservers.showempty.curvalue = g_emptyservers;

	// set source category and refresh if needed
	ArenaServers_SetType( g_servertype );
}

/*
=================
ArenaServers_Cache
=================
*/
void ArenaServers_Cache( void )
{
	trap_R_RegisterShaderNoMip( ART_UNKNOWNMAP );
}

/*
=================
UI_ArenaServersMenu
=================
*/
void UI_ArenaServersMenu( void )
{
	ArenaServers_MenuInit();
	UI_PushMenu( &g_arenaservers.menu );
}

/*
=======================================================================

CHOOSE SERVER TYPE

=======================================================================
*/

static menubitmap_s			s_find_server;
static menubitmap_s			s_specific_server;
static menubitmap_s			s_create_server;

typedef struct
{
	menuframework_s		menu;
	menubitmap_s		main;

} chooseservertype_t;
static chooseservertype_t		g_chooseservertype;

/*
=================
ChooseServerType_MenuKey
=================
*/
static sfxHandle_t ChooseServerType_MenuKey( int key )
{
	return Menu_DefaultKey( &g_chooseservertype.menu, key );
}

/*
=================
ChooseServerType_Graphics
=================
*/
void ChooseServerType_Graphics (void)
{
	UI_MenuFrame(&g_chooseservertype.menu);

	trap_R_SetColor( colorTable[CT_LTPURPLE2]);
	UI_DrawHandlePic(  30, 203,  47, 186, uis.whiteShader);	// Frame Left side

	UI_DrawProportionalString(  74,  27, "66809",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  84, "809",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74, 188, "991",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74, 206, "6-513",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74, 395, "7814",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	// Left rounded ends for buttons
	trap_R_SetColor( colorTable[s_find_server.color]);
	UI_DrawHandlePic(s_find_server.generic.x - 14, s_find_server.generic.y,
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	trap_R_SetColor( colorTable[s_create_server.color]);
	UI_DrawHandlePic(s_create_server.generic.x - 14, s_create_server.generic.y,
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

	trap_R_SetColor( colorTable[s_specific_server.color]);
	UI_DrawHandlePic(s_specific_server.generic.x - 14, s_specific_server.generic.y,
		MENU_BUTTON_MED_HEIGHT, MENU_BUTTON_MED_HEIGHT, uis.graphicButtonLeftEnd);

}

/*
=================
ChooseServerType_MenuDraw
=================
*/
static void ChooseServerType_MenuDraw( void )
{
	ChooseServerType_Graphics();

	Menu_Draw( &g_chooseservertype.menu );
}

/*
=================
UI_ChooseServerTypeMenu_Cache
=================
*/
void UI_ChooseServerTypeMenu_Cache( void )
{
}

/*
=================
ChooseServerType_MenuInit
=================
*/
static void ChooseServerType_MenuInit( void )
{
	// zero set all our globals
	memset( &g_chooseservertype, 0 ,sizeof(chooseservertype_t) );

	g_chooseservertype.menu.fullscreen					= qtrue;
	g_chooseservertype.menu.wrapAround					= qtrue;
	g_chooseservertype.menu.draw						= ChooseServerType_MenuDraw;
	g_chooseservertype.menu.key							= ChooseServerType_MenuKey;
	g_chooseservertype.menu.descX						= MENU_DESC_X;
	g_chooseservertype.menu.descY						= MENU_DESC_Y;
	g_chooseservertype.menu.titleX						= MENU_TITLE_X;
	g_chooseservertype.menu.titleY						= MENU_TITLE_Y;
	g_chooseservertype.menu.titleI						= MNT_MULTIPLAYER_TITLE;
	g_chooseservertype.menu.footNoteEnum				= MNT_FINDSERVER;

	g_chooseservertype.main.generic.type			= MTYPE_BITMAP;
	g_chooseservertype.main.generic.name			= BUTTON_GRAPHIC_LONGRIGHT;
	g_chooseservertype.main.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_chooseservertype.main.generic.id				= ID_BACK;
	g_chooseservertype.main.generic.callback		= ArenaServers_Event;
	g_chooseservertype.main.generic.x				= 482;
	g_chooseservertype.main.generic.y				= 136;
	g_chooseservertype.main.width  					= MENU_BUTTON_MED_WIDTH;
	g_chooseservertype.main.height  				= MENU_BUTTON_MED_HEIGHT;
	g_chooseservertype.main.color					= CT_DKPURPLE1;
	g_chooseservertype.main.color2					= CT_LTPURPLE1;
	g_chooseservertype.main.textX					= MENU_BUTTON_TEXT_X;
	g_chooseservertype.main.textY					= MENU_BUTTON_TEXT_Y;
	g_chooseservertype.main.textEnum				= MBT_MAINMENU;
	g_chooseservertype.main.textcolor				= CT_BLACK;
	g_chooseservertype.main.textcolor2				= CT_WHITE;

	s_find_server.generic.type					= MTYPE_BITMAP;
	s_find_server.generic.flags					= QMF_HIGHLIGHT_IF_FOCUS;
	s_find_server.generic.x						= 282;
	s_find_server.generic.y						= 229;
	s_find_server.generic.name					= BUTTON_GRAPHIC_LONGRIGHT;
	s_find_server.generic.id					= ID_FINDSERVER;
	s_find_server.generic.callback				= ArenaServers_Event;
	s_find_server.width							= MENU_BUTTON_MED_WIDTH;
	s_find_server.height						= MENU_BUTTON_MED_HEIGHT;
	s_find_server.color							= CT_DKPURPLE1;
	s_find_server.color2						= CT_LTPURPLE1;
	s_find_server.textX							= 5;
	s_find_server.textY							= 2;
	s_find_server.textEnum						= MBT_SEARCHSERVER;
	s_find_server.textcolor						= CT_BLACK;
	s_find_server.textcolor2					= CT_WHITE;

	s_create_server.generic.type				= MTYPE_BITMAP;
	s_create_server.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_create_server.generic.x					= 282;
	s_create_server.generic.y					= 279;
	s_create_server.generic.name				= BUTTON_GRAPHIC_LONGRIGHT;
	s_create_server.generic.id					= ID_CREATE;
	s_create_server.generic.callback			= ArenaServers_Event;
	s_create_server.width						= MENU_BUTTON_MED_WIDTH;
	s_create_server.height						= MENU_BUTTON_MED_HEIGHT;
	s_create_server.color						= CT_DKPURPLE1;
	s_create_server.color2						= CT_LTPURPLE1;
	s_create_server.textX						= 5;
	s_create_server.textY						= 2;
	s_create_server.textEnum					= MBT_CREATE;
	s_create_server.textcolor					= CT_BLACK;
	s_create_server.textcolor2					= CT_WHITE;

	s_specific_server.generic.type				= MTYPE_BITMAP;
	s_specific_server.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_specific_server.generic.x					= 282;
	s_specific_server.generic.y					= 329;
	s_specific_server.generic.name				= BUTTON_GRAPHIC_LONGRIGHT;
	s_specific_server.generic.id				= ID_SPECIFY;
	s_specific_server.generic.callback			= ArenaServers_Event;
	s_specific_server.width						= MENU_BUTTON_MED_WIDTH;
	s_specific_server.height					= MENU_BUTTON_MED_HEIGHT;
	s_specific_server.color						= CT_DKPURPLE1;
	s_specific_server.color2					= CT_LTPURPLE1;
	s_specific_server.textX						= 5;
	s_specific_server.textY						= 2;
	s_specific_server.textEnum					= MBT_SPECIFY;
	s_specific_server.textcolor					= CT_BLACK;
	s_specific_server.textcolor2				= CT_WHITE;

	Menu_AddItem(&g_chooseservertype.menu, ( void * )&g_chooseservertype.main);
	Menu_AddItem(&g_chooseservertype.menu, ( void * )&s_find_server);
	Menu_AddItem(&g_chooseservertype.menu, ( void * )&s_create_server);
	Menu_AddItem(&g_chooseservertype.menu, ( void * )&s_specific_server);

	UI_ChooseServerTypeMenu_Cache();
}

/*
=================
UI_ChooseServerTypeMenu
=================
*/
void UI_ChooseServerTypeMenu( void )
{
	ChooseServerType_MenuInit();
	UI_PushMenu( &g_chooseservertype.menu );

}
