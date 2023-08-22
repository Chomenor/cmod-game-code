// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"
#include "cg_text.h"
#include "cg_screenfx.h"

// set in CG_ParseTeamInfo
int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;
int drawTeamOverlayModificationCount = -1;

extern void InitPostGameMenuStruct();

static void CG_InterfaceStartup();

char *ingame_text[IGT_MAX];		//	Holds pointers to ingame text

interfacegraphics_s interface_graphics[IG_MAX] =
{
//	type		timer	x		y		width	height	file/text						graphic,	min		max		color			style			ptr
	SG_VAR,		0.0,	0,		0,		0,		0,		NULL,							0,		0,		0,		CT_NONE,		0,					// IG_GROW

	SG_VAR,		0.0,	0,		0,		0,		0,		NULL,							0,		0,		0,		CT_NONE,		0,				 	// IG_HEALTH_START
	SG_GRAPHIC,	0.0,	5,		429,	32,		64,		"gfx/interface/healthcap1",		0,		0,		0,		CT_DKBROWN1,	0,				 	// IG_HEALTH_BEGINCAP
	SG_GRAPHIC,	0.0,	64,		429,	6,		25,		"gfx/interface/ammobar",		0,		0,		0,		CT_DKBROWN1,	0,				 	// IG_HEALTH_BOX1
	SG_GRAPHIC,	0.0,	72,		429,	0,		25,		"gfx/interface/ammobar",		0,		0,		0,		CT_LTBROWN1,	0,				 	// IG_HEALTH_SLIDERFULL
	SG_GRAPHIC,	0.0,	0,		429,	0,		25,		"gfx/interface/ammobar",		0,		0,		0,		CT_DKBROWN1,	0,				 	// IG_HEALTH_SLIDEREMPTY
	SG_GRAPHIC,	0.0,	72,		429,	16,		32,		"gfx/interface/healthcap2",		0,		0,		147,	CT_DKBROWN1,	0,				 	// IG_HEALTH_ENDCAP
	SG_NUMBER,	0.0,	23,		425,	16,		32,		NULL,							0,		0,		0,		CT_LTBROWN1,	NUM_FONT_BIG,	 	// IG_HEALTH_COUNT
	SG_VAR,		0.0,	0,		0,		0,		0,		NULL,							0,		0,		0,		CT_NONE,		0,				 	// IG_HEALTH_END

	SG_VAR,		0.0,	0,		0,		0,		0,		NULL,							0,		0,		0,		CT_NONE,		0,				 	// IG_ARMOR_START
	SG_GRAPHIC,	0.0,	20,		458,	32,		16,		"gfx/interface/armorcap1",		0,		0,		0,		CT_DKPURPLE1,	0,			 	// IG_ARMOR_BEGINCAP
	SG_GRAPHIC,	0.0,	64,		458,	6,		12,		"gfx/interface/ammobar",		0,		0,		0,		CT_DKPURPLE1,	0,			 	// IG_ARMOR_BOX1
	SG_GRAPHIC,	0.0,	72,		458,	0,		12,		"gfx/interface/ammobar",		0,		0,		0,		CT_LTPURPLE1,	0,			 	// IG_ARMOR_SLIDERFULL
	SG_GRAPHIC,	0.0,	0,		458,	0,		12,		"gfx/interface/ammobar",		0,		0,		0,		CT_DKPURPLE1,	0,			 	// IG_ARMOR_SLIDEREMPTY
	SG_GRAPHIC,	0.0,	72,		458,	16,		16,		"gfx/interface/armorcap2",		0,		0,		147,	CT_DKPURPLE1,	0,			 	// IG_ARMOR_ENDCAP
	SG_NUMBER,	0.0,	44,		458,	16,		16,		NULL,							0,		0,		0,		CT_LTPURPLE1,	NUM_FONT_SMALL, 	// IG_ARMOR_COUNT
	SG_VAR,		0.0,	0,		0,		0,		0,		NULL,							0,		0,		0,		CT_NONE,		0,			 	// IG_ARMOR_END

	SG_VAR,		0.0,	0,		0,		0,		0,		NULL,							0,		0,		0,		CT_NONE,		0,			 	// IG_AMMO_START
	SG_GRAPHIC,	0.0,	613,	429,	32,		64,		"gfx/interface/ammouppercap1",	0,		0,		0,		CT_LTPURPLE2,	0,			 	// IG_AMMO_UPPER_BEGINCAP
	SG_GRAPHIC,	0.0,	607,	429,	16,		32,		"gfx/interface/ammouppercap2",	0,		0,		572,	CT_LTPURPLE2,	0,			 	// IG_AMMO_UPPER_ENDCAP
	SG_GRAPHIC,	0.0,	613,	458,	16,		16,		"gfx/interface/ammolowercap1",	0,		0,		0,		CT_LTPURPLE2,	0,			 	// IG_AMMO_LOWER_BEGINCAP
	SG_GRAPHIC,	0.0,	578,	458,	0,		12,		"gfx/interface/ammobar",		0,		0,		0,		CT_LTPURPLE1,	0,			 // IG_AMMO_SLIDERFULL
	SG_GRAPHIC,	0.0,	0,		458,	0,		12,		"gfx/interface/ammobar",		0,		0,		0,		CT_DKPURPLE1,	0,			 	// IG_AMMO_SLIDEREMPTY
	SG_GRAPHIC,	0.0,	607,	458,	16,		16,		"gfx/interface/ammolowercap2",	0,		0,		572,	CT_LTPURPLE2,	0,			 	// IG_AMMO_LOWER_ENDCAP
	SG_NUMBER,	0.0,	573,	425,	16,		32,		NULL,							0,		0,		0,		CT_LTPURPLE1,	NUM_FONT_BIG, 	// IG_AMMO_COUNT
	SG_VAR,		0.0,	0,		0,		0,		0,		NULL,							0,		0,		0,		CT_NONE,		0,			 	// IG_AMMO_END

};

#define LOWEROVERLAY_Y (SCREEN_HEIGHT - ICON_SIZE - 15)

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
/*
static void CG_DrawField (int x, int y, int width, int value)
{
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 )
	{
		return;
	}

	// draw number string
	if ( width > 5 )
	{
		width = 5;
	}

	switch ( width )
	{
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
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}
*/

/*
================
CG_Draw3DModel

================
*/
static void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, qhandle_t shader, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.customShader = shader;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if ( cg_draw3dIcons.integer && (ci->headOffset[0] != 404) ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->headOffset, origin );

		CG_Draw3DModel( x, y, w, h, ci->headModel, ci->headSkin, 0, origin, headAngles );
	} else if ( cg_drawIcons.integer ) {
		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	if ( cgs.clientinfo[clientNum].eliminated ) {//if eliminated, draw the cross-out
		CG_DrawPic( x, y, w, h, cgs.media.eliminatedShader );
	} else if ( ci->deferred ) {// if they are deferred, draw a cross out
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;

	if ( cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );;

		CG_Draw3DModel( x, y, w, h,
			team == TEAM_RED ? cgs.media.redFlagModel : cgs.media.blueFlagModel, 0,
			team == TEAM_RED ? cgs.media.redFlagShader[3] : cgs.media.blueFlagShader[3], origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item = BG_FindItemForPowerup( team == TEAM_RED ? PW_REDFLAG : PW_BLUEFLAG );

		if (item)
		{
			CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawStatusBarHead

================
*/
/*
static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, 480 - size, size, size,
				cg.snap->ps.clientNum, angles );
}
*/
/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team, qboolean scoreboard )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( !scoreboard && cg.snap->ps.persistant[PERS_CLASS] == PC_BORG )
	{
		hcolor[0] = 0;
		hcolor[1] = 1;
		hcolor[2] = 0;
	}
	else if ( team == TEAM_RED )
	{
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	}
	else if ( team == TEAM_BLUE )
	{
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	}
	else
	{
		return; // no team
	}

	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}

/*
================
CG_DrawAmmo

================
*/
static void CG_DrawAmmo(centity_t	*cent)
{
	float		value;
	float		xLength;
	playerState_t	*ps;
	int			max,brightColor_i,darkColor_i,numColor_i;

	ps = &cg.snap->ps;

	value = ps->ammo[cent->currentState.weapon];

	if (value < 0)
	{
		return;
	}

	interface_graphics[IG_AMMO_COUNT].max = value;

	brightColor_i = CT_LTGOLD1;
	darkColor_i = CT_DKGOLD1;
	numColor_i = CT_LTGOLD1;

	// Calc bar length
	max = ps->ammo[cent->currentState.weapon];

	if (max > 0)
	{
		xLength = 33 * (value/max);
	}
	else
	{
		max = 0;
		xLength = 0;
	}
	// Armor empty section
	interface_graphics[IG_AMMO_SLIDEREMPTY].x = 578 + xLength;
	interface_graphics[IG_AMMO_SLIDEREMPTY].width = 33 - xLength;

	// Armor full section
	interface_graphics[IG_AMMO_SLIDERFULL].width = xLength;

	interface_graphics[IG_AMMO_UPPER_BEGINCAP].color = darkColor_i;
	interface_graphics[IG_AMMO_UPPER_ENDCAP].color = darkColor_i;
	interface_graphics[IG_AMMO_LOWER_BEGINCAP].color = darkColor_i;
	interface_graphics[IG_AMMO_LOWER_ENDCAP].color = darkColor_i;
	interface_graphics[IG_AMMO_SLIDERFULL].color = brightColor_i;
	interface_graphics[IG_AMMO_SLIDEREMPTY].color = darkColor_i;
	interface_graphics[IG_AMMO_COUNT].color = numColor_i;

	// Print it
	CG_PrintInterfaceGraphics(IG_AMMO_START + 1,IG_AMMO_END);
}

/*
================
CG_DrawArmor

================
*/
static void CG_DrawArmor(centity_t	*cent)
{
	int			max;
	float		value,xLength;
	playerState_t	*ps;
	int			lengthMax;

	ps = &cg.snap->ps;

	value = ps->stats[STAT_ARMOR];

	interface_graphics[IG_ARMOR_COUNT].max = value;

	if (interface_graphics[IG_ARMOR_COUNT].max <= ps->stats[STAT_MAX_HEALTH])
	{
		interface_graphics[IG_ARMOR_COUNT].color = CT_LTPURPLE1;		//
		interface_graphics[IG_ARMOR_SLIDERFULL].color = CT_LTPURPLE1;	//
		interface_graphics[IG_ARMOR_COUNT].style &= ~UI_PULSE;			// Numbers
	}
	else
	{
		interface_graphics[IG_ARMOR_COUNT].color = CT_LTGREY;			// Numbers
		interface_graphics[IG_ARMOR_SLIDERFULL].color = CT_LTGREY;		//
		interface_graphics[IG_ARMOR_COUNT].style |= UI_PULSE;			// Numbers
	}


/*
	if (cg.oldarmor < value)
	{
		cg.oldArmorTime = cg.time + 100;
	}

	cg.oldarmor = value;

	if (cg.oldArmorTime < cg.time)
	{ */
//		interface_graphics[IG_ARMOR_COUNT].color = CT_LTPURPLE1;	// Numbers
/*	}
	else
	{
		interface_graphics[IG_ARMOR_COUNT].color = CT_YELLOW;	// Numbers
	}
*/

	max = ps->stats[STAT_MAX_HEALTH];
	lengthMax = 73;
	if (max > 0)
	{
		if (value > max)
		{
			xLength = lengthMax;
		}
		else
		{
			xLength = lengthMax * (value/max);
		}

	}
	else
	{
		max = 0;
		xLength = 0;
	}

	// Armor empty section
	interface_graphics[IG_ARMOR_SLIDEREMPTY].x = 72 + xLength;
	interface_graphics[IG_ARMOR_SLIDEREMPTY].width = lengthMax - xLength;

	// Armor full section
	interface_graphics[IG_ARMOR_SLIDERFULL].width = xLength;

	CG_PrintInterfaceGraphics(IG_ARMOR_START + 1,IG_ARMOR_END);

}

/*
================
CG_DrawHealth

================
*/
static void CG_DrawHealth(centity_t	*cent)
{
	int			max;
	float		value,xLength;
	playerState_t	*ps;
	int			lengthMax;

	ps = &cg.snap->ps;

	value = ps->stats[STAT_HEALTH];


	// Changing colors on numbers
//	if (cg.oldhealth < value)
//	{
//		cg.oldHealthTime = cg.time + 100;
//	}
//	cg.oldhealth = value;

	// Is health changing?
//	if (cg.oldHealthTime < cg.time)
//	{
//		interface_graphics[IG_HEALTH_COUNT].color = CT_LTBROWN1;	// Numbers
//	}
//	else
//	{
//	}
	interface_graphics[IG_HEALTH_COUNT].max = value;

	if (interface_graphics[IG_HEALTH_COUNT].max <= ps->stats[STAT_MAX_HEALTH])
	{
		interface_graphics[IG_HEALTH_COUNT].color = CT_LTBROWN1;	//
		interface_graphics[IG_HEALTH_SLIDERFULL].color = CT_LTBROWN1;	//
		interface_graphics[IG_HEALTH_SLIDEREMPTY].color = CT_DKBROWN1;	//
		interface_graphics[IG_HEALTH_COUNT].style &= ~UI_PULSE;			// Numbers
	}
	else
	{
		interface_graphics[IG_HEALTH_COUNT].color = CT_LTGREY;			// Numbers
		interface_graphics[IG_HEALTH_SLIDERFULL].color = CT_LTGREY;		//
		interface_graphics[IG_HEALTH_COUNT].style |= UI_PULSE;			// Numbers
	}

	// Calculating size of health bar
	max = ps->stats[STAT_MAX_HEALTH];
	lengthMax = 73;
	if (max > 0)
	{
		if (value < max)
		{
			xLength = lengthMax * (value/max);
		}
		else	// So the graphic doesn't extend past the cap
		{
			xLength = lengthMax;
		}
	}
	else
	{
		max = 0;
		xLength = 0;
	}

	// Health empty section
	interface_graphics[IG_HEALTH_SLIDEREMPTY].x = 72 + xLength;
	interface_graphics[IG_HEALTH_SLIDEREMPTY].width = lengthMax - xLength;

	// Health full section
	interface_graphics[IG_HEALTH_SLIDERFULL].width = xLength;

	// Print it
	CG_PrintInterfaceGraphics(IG_HEALTH_START + 1,IG_HEALTH_END);
}

/*
================
CG_DrawStatusBar

================
*/
static void CG_DrawStatusBar( void )
{
	centity_t	*cent;
	playerState_t	*ps;
	vec3_t		angles;
	int y=0;
	vec4_t	whiteA;
	static float colors[4][4] =
	{
		{ 1, 0.69, 0, 1.0 } ,		// normal
		{ 1.0, 0.2, 0.2, 1.0 },		// low health
		{0.5, 0.5, 0.5, 1},			// weapon firing
		{ 1, 1, 1, 1 } };			// health > 100

	whiteA[0] = whiteA[1] = whiteA[2] = 1.0f;	whiteA[3] = 0.3f;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	// draw the team background
	AspectCorrect_SetMode( HSCALE_STRETCH, VSCALE_BOTTOM );
	CG_DrawTeamBackground( 0, 420, 640, 60, 0.33, cg.snap->ps.persistant[PERS_TEAM], qfalse );

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
	AspectCorrect_SetMode( HSCALE_LEFT, VSCALE_BOTTOM );
	y = (SCREEN_HEIGHT - (4*ICON_SIZE) - 20);
	if (cg.predictedPlayerState.powerups[PW_REDFLAG])
	{	//fixme: move to powerup renderer?  make it pulse?
	//	CG_FillRect(      5, y, ICON_SIZE*2, ICON_SIZE*2, whiteA);
		CG_DrawFlagModel( 5, y, ICON_SIZE*2, ICON_SIZE*2, TEAM_RED );
	}
	else if (cg.predictedPlayerState.powerups[PW_BLUEFLAG])
	{
	//	CG_FillRect(      5, y, ICON_SIZE*2, ICON_SIZE*2, whiteA);
		CG_DrawFlagModel( 5, y, ICON_SIZE*2, ICON_SIZE*2, TEAM_BLUE );
	}

	// Do start
	AspectCorrect_ResetMode();	// just to be safe
	if (!cg.interfaceStartupDone)
	{
		CG_InterfaceStartup();
	}

	//
	// ammo
	//
	AspectCorrect_SetMode( HSCALE_RIGHT, VSCALE_BOTTOM );
	if ( cent->currentState.weapon )
	{
		CG_DrawAmmo(cent);
	}


	//
	// health
	//
	AspectCorrect_SetMode( HSCALE_LEFT, VSCALE_BOTTOM );
	CG_DrawHealth(cent);


	//
	// armor
	//
	CG_DrawArmor(cent);

	AspectCorrect_ResetMode();
}

/*
================
CG_InterfaceStartup
================
*/
static void CG_InterfaceStartup()
{

	// Turn on Health Graphics
	if ((interface_graphics[IG_HEALTH_START].timer < cg.time) && (interface_graphics[IG_HEALTH_BEGINCAP].type == SG_OFF))
	{
		trap_S_StartLocalSound( cgs.media.interfaceSnd1, CHAN_LOCAL_SOUND );

		interface_graphics[IG_HEALTH_BEGINCAP].type = SG_GRAPHIC;
		interface_graphics[IG_HEALTH_BOX1].type = SG_GRAPHIC;
		interface_graphics[IG_HEALTH_ENDCAP].type = SG_GRAPHIC;
	}

	// Turn on Armor Graphics
	if ((interface_graphics[IG_ARMOR_START].timer <	cg.time) && (interface_graphics[IG_ARMOR_BEGINCAP].type == SG_OFF))
	{
		if (interface_graphics[IG_ARMOR_BEGINCAP].type == SG_OFF)
		{
			trap_S_StartLocalSound( cgs.media.interfaceSnd1, CHAN_LOCAL_SOUND );
		}

		interface_graphics[IG_ARMOR_BEGINCAP].type = SG_GRAPHIC;
		interface_graphics[IG_ARMOR_BOX1].type = SG_GRAPHIC;
		interface_graphics[IG_ARMOR_ENDCAP].type = SG_GRAPHIC;

	}

	// Turn on Ammo Graphics
	if (interface_graphics[IG_AMMO_START].timer <	cg.time)
	{
		if (interface_graphics[IG_AMMO_UPPER_BEGINCAP].type == SG_OFF)
		{
			trap_S_StartLocalSound( cgs.media.interfaceSnd1, CHAN_LOCAL_SOUND );
			interface_graphics[IG_GROW].type = SG_VAR;
			interface_graphics[IG_GROW].timer = cg.time;
		}

		interface_graphics[IG_AMMO_UPPER_BEGINCAP].type = SG_GRAPHIC;
		interface_graphics[IG_AMMO_UPPER_ENDCAP].type = SG_GRAPHIC;
		interface_graphics[IG_AMMO_LOWER_BEGINCAP].type = SG_GRAPHIC;
		interface_graphics[IG_AMMO_LOWER_ENDCAP].type = SG_GRAPHIC;
	}

	if (interface_graphics[IG_GROW].type == SG_VAR)
	{
		interface_graphics[IG_HEALTH_ENDCAP].x += 2;
		interface_graphics[IG_ARMOR_ENDCAP].x += 2;
		interface_graphics[IG_AMMO_UPPER_ENDCAP].x -= 1;
		interface_graphics[IG_AMMO_LOWER_ENDCAP].x -= 1;

		if (interface_graphics[IG_HEALTH_ENDCAP].x >= interface_graphics[IG_HEALTH_ENDCAP].max)
		{
			interface_graphics[IG_HEALTH_ENDCAP].x = interface_graphics[IG_HEALTH_ENDCAP].max;
			interface_graphics[IG_ARMOR_ENDCAP].x = interface_graphics[IG_ARMOR_ENDCAP].max;

			interface_graphics[IG_AMMO_UPPER_ENDCAP].x = interface_graphics[IG_AMMO_UPPER_ENDCAP].max;
			interface_graphics[IG_AMMO_LOWER_ENDCAP].x = interface_graphics[IG_AMMO_LOWER_ENDCAP].max;
			interface_graphics[IG_GROW].type = SG_OFF;

			interface_graphics[IG_HEALTH_SLIDERFULL].type = SG_GRAPHIC;
			interface_graphics[IG_HEALTH_SLIDEREMPTY].type = SG_GRAPHIC;
			interface_graphics[IG_HEALTH_COUNT].type = SG_NUMBER;

			interface_graphics[IG_ARMOR_SLIDERFULL].type = SG_GRAPHIC;
			interface_graphics[IG_ARMOR_SLIDEREMPTY].type = SG_GRAPHIC;
			interface_graphics[IG_ARMOR_COUNT].type = SG_NUMBER;

			interface_graphics[IG_AMMO_SLIDERFULL].type = SG_GRAPHIC;
			interface_graphics[IG_AMMO_SLIDEREMPTY].type = SG_GRAPHIC;
			interface_graphics[IG_AMMO_COUNT].type = SG_NUMBER;

			trap_S_StartLocalSound( cgs.media.interfaceSnd1, CHAN_LOCAL_SOUND );
			cg.interfaceStartupDone = 1;	// All done
		}

		interface_graphics[IG_GROW].timer = cg.time + 10;
	}

	cg.interfaceStartupTime = cg.time;

	// kef -- init struct for post game awards
	InitPostGameMenuStruct();
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
	vec3_t		angles;
	const char	*info;
	const char	*name;
	int			clientNum;

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.attackerTime ) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead( 640 - size, y, size, size, clientNum, angles );

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
//	CG_DrawBigString( 640 - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );
	UI_DrawProportionalString( 635, y, name, UI_RIGHT | UI_SMALLFONT, colorTable[CT_LTGOLD1] );

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime,
		cg.latestSnapshotNum, cgs.serverCommandSequence );

	w = UI_ProportionalStringWidth(s,UI_BIGFONT);
	UI_DrawProportionalString(635 - w, y + 2, s, UI_BIGFONT, colorTable[CT_LTGOLD1]);

	return y + BIGCHAR_HEIGHT + 10;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );
		w = UI_ProportionalStringWidth(s,UI_BIGFONT);
		UI_DrawProportionalString(635 - w, y + 2, s, UI_BIGFONT, colorTable[CT_LTGOLD1]);
	}

	return y + BIGCHAR_HEIGHT + 10;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;

	if ( cgs.timelimit > 0 )
	{
		msec = fabs(cgs.timelimit*60000 - msec);
	}

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );

	w = UI_ProportionalStringWidth(s,UI_BIGFONT);
	UI_DrawProportionalString(635 - w, y + 2, s, UI_BIGFONT, colorTable[CT_LTGOLD1]);

	return y + BIGCHAR_HEIGHT + 10;
}
#define TINYPAD 1.25

/*
=================
CG_DrawTeamOverlay
=================
*/

#define TEAM_OVERLAY_MAXNAME_WIDTH	12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	16

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	int ret_y;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED &&
		cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE )
		return y; // Not on any team

	plyrs = 0;
	w = 0;

	// max player name width
	pwidth = 0;
	for (i = 0; i < numSortedTeamPlayers; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);

			if (len > pwidth)
				pwidth = len;
			if ( ci->pClass > PC_NOCLASS )//if any one of them has a class, then we alloc space for the icon
				w = 1;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w += (pwidth + lwidth + 4);
	w *= (TINYCHAR_WIDTH * TINYPAD);

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * (TINYCHAR_HEIGHT * TINYPAD);

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_CLASS] == PC_BORG ) {
		hcolor[0] = 0;
		hcolor[1] = 1;
		hcolor[2] = 0;
		hcolor[3] = 0.33;
	} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
		hcolor[3] = 0.33;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
		hcolor[3] = 0.33;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	for (i = 0; i < numSortedTeamPlayers; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			xx = x + TINYCHAR_WIDTH;
//Draw class icon if appropriate
			if ( ci->pClass > PC_NOCLASS )
			{
				qhandle_t	icon;

				//Special hack: if it's Borg who has regen going, must be Borg queen
				if ( ci->pClass == PC_BORG && (ci->powerups&(1<<PW_REGEN)) )
				{
					icon = cgs.media.borgQueenIconShader;
				}
				else
				{
					icon = cgs.media.pClassShaders[ci->pClass];
				}
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, icon );

				xx += (TINYCHAR_WIDTH * TINYPAD);
			}
//draw name
//			CG_DrawStringExt( xx, y,
//				ci->name, hcolor, qfalse, qfalse,
//				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);
			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;
			UI_DrawProportionalString( xx, y, ci->name, UI_TINYFONT, hcolor);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
				len = CG_DrawStrlen(p);
				if (len > lwidth)
					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth +
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
//				CG_DrawStringExt( xx, y,
//					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
//					TEAM_OVERLAY_MAXLOCATION_WIDTH);
				UI_DrawProportionalString( xx, y, p, UI_TINYFONT, hcolor);

			}

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 +
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

//			CG_DrawStringExt( xx, y,
//				st, hcolor, qfalse, qfalse,
//				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
			UI_DrawProportionalString( xx, y, st, UI_TINYFONT, hcolor);

			// draw weapon icon
			xx += (TINYCHAR_WIDTH * TINYPAD) * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j < PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {
					gitem_t	*item = BG_FindItemForPowerup( j );

					if (item)
					{
						CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
							trap_R_RegisterShader( item->icon ) );
					}
					if (right) {
						xx -= (TINYCHAR_WIDTH * TINYPAD);
					} else {
						xx += (TINYCHAR_WIDTH * TINYPAD);
					}
				}
			}

			y += (TINYCHAR_HEIGHT * TINYPAD);
		}
	}

	return ret_y;
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float	y;

	y = 0;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	}
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
	if ( cg_drawAttacker.integer ) {
		y = CG_DrawAttacker( y );
	}

}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
static float CG_DrawScores( float y )
{
	const char	*s;
	int			s1, s2, score;
	int			x, w;
	int			v;
	vec4_t		color;
	float		y1;
	gitem_t		*item;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -=  BIGCHAR_HEIGHT + 8;

	y1 = y;

	// draw from the right side to left
	if ( cgs.gametype >= GT_TEAM )
	{
		x = 640;

		color[0] = 0;
		color[1] = 0;
		color[2] = 1;
		color[3] = 0.33;
		s = va( "%2i", s2 );
//		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		w = UI_ProportionalStringWidth(s,UI_SMALLFONT) + 8;

		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		{
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}

//		CG_DrawBigString( x + 4, y, s, 1.0F);
		UI_DrawProportionalString( x + 4, y, s, UI_SMALLFONT, colorTable[CT_LTGOLD1] );

		if ( cgs.gametype == GT_CTF )
		{
			// Display flag status
			item = BG_FindItemForPowerup( PW_BLUEFLAG );

			if (item)
			{
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.blueflag >= 0 && cgs.blueflag <= 2 )
				{
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.blueFlagShader[cgs.blueflag] );
				}
#if 0
				CG_RegisterItemVisuals( ITEM_INDEX(item) );
				switch (cgs.blueflag) {
				case 0 :  // at base
					// Draw the icon
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, trap_R_RegisterShader( item->icon ) );
				case 1 : // taken
					CG_FillRect( x, y1-4,  w, BIGCHAR_HEIGHT+8, color );
					break;
				case 2 : // droppped, grey background
					color[0] = color[1] = color[2] = 1;
					CG_FillRect( x, y1-4,  w, BIGCHAR_HEIGHT+8, color );
					break;
				// Other values won't draw (-1 is disabled)
				}
#endif
			}
		}

		color[0] = 1;
		color[1] = 0;
		color[2] = 0;
		color[3] = 0.33;
		s = va( "%2i", s1 );
//		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		w = UI_ProportionalStringWidth(s,UI_SMALLFONT) + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED )
		{
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
//		CG_DrawBigString( x + 4, y, s, 1.0F);
		UI_DrawProportionalString( x + 4, y, s, UI_SMALLFONT, colorTable[CT_LTGOLD1] );

		if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_REDFLAG );

			if (item)
			{
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.redflag >= 0 && cgs.redflag <= 2 )
				{
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.redFlagShader[cgs.redflag] );
				}
#if 0
				CG_RegisterItemVisuals( ITEM_INDEX(item) );
				switch (cgs.redflag)
				{
				case 0 :  // at base
					// Draw the icon
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, trap_R_RegisterShader( item->icon ) );
					break;
				case 1 : // taken
					CG_FillRect( x, y1-4,  w, BIGCHAR_HEIGHT+8, color );
					break;
				case 2 : // droppped, grey background
					color[0] = color[1] = color[2] = 1;
					CG_FillRect( x, y1-4,  w, BIGCHAR_HEIGHT+8, color );
					break;
				// Other values won't draw (-1 is disabled)
				}
#endif
			}
		}


		if ( cgs.gametype == GT_CTF )
		{
			v = cgs.capturelimit;
		}
		else
		{
			v = cgs.fraglimit;
		}
		if ( v )
		{
			s = va( "%2i", v );
//			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			w = UI_ProportionalStringWidth(s,UI_SMALLFONT) + 8;
			x -= w;
//			CG_DrawBigString( x + 4, y, s, 1.0F);
			UI_DrawProportionalString( x + 4, y, s, UI_SMALLFONT, colorTable[CT_LTGOLD1] );
		}

	}
	else
	{
		qboolean	spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR );

		// always show your score in the second box if not in first place
		if ( s1 != score )
		{
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT )
		{
			s = va( "%2i", s2 );
//			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			w = UI_ProportionalStringWidth(s,UI_SMALLFONT) + 8;
			x -= w;
			if ( !spectator && score == s2 && score != s1 )
			{
				color[0] = 1;
				color[1] = 0;
				color[2] = 0;
				color[3] = 0.33;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			}
			else
			{
				color[0] = 0.5;
				color[1] = 0.5;
				color[2] = 0.5;
				color[3] = 0.33;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}
//			CG_DrawBigString( x + 4, y, s, 1.0F);
			UI_DrawProportionalString( x + 4, y, s, UI_SMALLFONT, colorTable[CT_LTGOLD1] );
		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT )
		{
			s = va( "%2i", s1 );
//			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			w = UI_ProportionalStringWidth(s,UI_SMALLFONT) + 8;
			x -= w;
			if ( !spectator && score == s1 )
			{
				color[0] = 0;
				color[1] = 0;
				color[2] = 1;
				color[3] = 0.33;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			}
			else
			{
				color[0] = 0.5;
				color[1] = 0.5;
				color[2] = 0.5;
				color[3] = 0.33;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}
//			CG_DrawBigString( x + 4, y, s, 1.0F);
			UI_DrawProportionalString( x + 4, y, s, UI_SMALLFONT, colorTable[CT_LTGOLD1] );
		}

		if ( cgs.fraglimit )
		{
			s = va( "%2i", cgs.fraglimit );
//			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			w = UI_ProportionalStringWidth(s,UI_SMALLFONT) + 8;
			x -= w;
//			CG_DrawBigString( x + 4, y, s, 1.0F);
			UI_DrawProportionalString( x + 4, y, s, UI_SMALLFONT, colorTable[CT_LTGOLD1] );

		}
	}

	return y1 - 8;
}

/*
================
CG_DrawPowerups
================
*/
static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	gitem_t	*item;
	int		x;
	int		color;
	float	size;
	float	f;
	static float colors[2][4] = {
		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 } };
	int		hasHoldable;

	hasHoldable = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t < 0 || t > 999000) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for ( i = 0 ; i < active ; i++ ) {

		// Don't draw almost timed out powerups if we have more than 3 and a holdable item
		if (!(hasHoldable && i<active-2))
		{
		item = BG_FindItemForPowerup( sorted[i] );

		if (NULL == item)
		{
			continue;
		}
		color = 1;

		y -= ICON_SIZE;

		trap_R_SetColor( colors[color] );
//		CG_DrawField( x, y, 2, sortedTime[ i ] / 1000 );
		CG_DrawNumField (x,y,2,sortedTime[ i ] / 1000,16,32,NUM_FONT_BIG);

		t = ps->powerups[ sorted[i] ];
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			trap_R_SetColor( NULL );
		} else {
			vec4_t	modulate;

			f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
			f -= (int)f;
			modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
			trap_R_SetColor( modulate );
		}

		if ( cg.powerupActive == sorted[i] &&
			cg.time - cg.powerupTime < PULSE_TIME ) {
			f = 1.0 - ( ( (float)cg.time - cg.powerupTime ) / PULSE_TIME );
			size = ICON_SIZE * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
		} else {
			size = ICON_SIZE;
		}

		CG_DrawPic( 640 - size, y + ICON_SIZE / 2 - size / 2,
			size, size, trap_R_RegisterShader( item->icon ) );
		}
	}
	trap_R_SetColor( NULL );

	return y;
}


/*
=====================
CG_DrawLowerRight

=====================
*/
static void CG_DrawLowerRight( void ) {
	float	y;

	y = LOWEROVERLAY_Y;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	}

	y = CG_DrawScores( y );
	y = CG_DrawPowerups( y );
}

/*
===================
CG_DrawPickupItem
===================
*/
static int CG_DrawPickupItem( int y ) {
	int		value;
	float	*fadeColor;

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	y -= ICON_SIZE;

	value = cg.itemPickup;
	if ( value ) {
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );
			trap_R_SetColor( fadeColor );
			CG_DrawPic( 8, y, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
			UI_DrawProportionalString( ICON_SIZE + 16, y + (ICON_SIZE/2 - BIGCHAR_HEIGHT/2), bg_itemlist[ value ].pickup_name, UI_SMALLFONT, fadeColor);

//			CG_DrawBigString( ICON_SIZE + 16, y + (ICON_SIZE/2 - BIGCHAR_HEIGHT/2), bg_itemlist[ value ].pickup_name, fadeColor[0] );
			trap_R_SetColor( NULL );
		}
	}

	return y;
}

/*
=====================
CG_DrawLowerLeft

=====================
*/
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = LOWEROVERLAY_Y;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3 ) {
		y = CG_DrawTeamOverlay( y, qfalse, qfalse );
	}


	y = CG_DrawPickupItem( y );
}



//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
static void CG_DrawTeamInfo( void ) {
	int w, h;
	int i, len;
	vec4_t		hcolor;
	int		chatHeight;

#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		w = 0;

		for (i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++) {
			len = CG_DrawStrlen(cgs.teamChatMsgs[i % chatHeight]);
			if (len > w)
				w = len;
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			hcolor[0] = 1;
			hcolor[1] = 0;
			hcolor[2] = 0;
			hcolor[3] = 0.33;
		} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 1;
			hcolor[3] = 0.33;
		} else {
			hcolor[0] = 0;
			hcolor[1] = 1;
			hcolor[2] = 0;
			hcolor[3] = 0.33;
		}

		AspectCorrect_SetMode( HSCALE_STRETCH, VSCALE_BOTTOM );

		trap_R_SetColor( hcolor );
		CG_DrawPic( CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
		hcolor[3] = 1.0;

		AspectCorrect_SetMode( HSCALE_LEFT, VSCALE_BOTTOM );

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
//			CG_DrawStringExt( CHATLOC_X + TINYCHAR_WIDTH,
//				CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT,
//				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
//				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
			UI_DrawProportionalString( CHATLOC_X + TINYCHAR_WIDTH,
				CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT,
				cgs.teamChatMsgs[i % chatHeight], UI_TINYFONT, hcolor);

		}

		AspectCorrect_ResetMode();
	}
}

/*
===================
CG_DrawHoldableItem
===================
*/
static void CG_DrawHoldableItem( void ) {
	int		value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if ( value )
	{
		CG_RegisterItemVisuals( value );
		if ( cg.snap->ps.stats[STAT_USEABLE_PLACED] && cg.snap->ps.stats[STAT_USEABLE_PLACED] != 2 )
		{//draw detpack... Borg 2-part teleporter will just draw the same until done
			CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE, cgs.media.detpackPlacedIcon );
		}
		else
		{
			CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
		}
	}
	else
	{//holding nothing...
		if ( cg.snap->ps.stats[STAT_USEABLE_PLACED] > 0 )
		{//it's a timed countdown to getting a holdable, display the number in seconds
			int		sec;
			char	*s;
			int		w;

			sec = cg.snap->ps.stats[STAT_USEABLE_PLACED];

			if ( sec < 0 )
			{
				sec = 0;
			}

			s = va( "%i", sec );

			w = UI_ProportionalStringWidth(s,UI_BIGFONT);
			UI_DrawProportionalString(640-(ICON_SIZE/2)-(w/2), (SCREEN_HEIGHT-ICON_SIZE)/2+(BIGCHAR_HEIGHT/2), s, UI_BIGFONT, colorTable[CT_WHITE]);
		}
	}
}


/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward( void ) {
	float	*color;
	int		i;
	float	x, y;

	if ( !cg_drawRewards.integer ) {
		return;
	}
	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );
	y = 56;
	x = 320 - cg.rewardCount * ICON_SIZE/2;
	for ( i = 0 ; i < cg.rewardCount ; i++ ) {
		CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader );
		x += ICON_SIZE;
	}
	trap_R_SetColor( NULL );
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	usercmd_t	cmd;
	const char		*s;
	int			w;

	// draw the phone jack if we are at least 1 second ahead of server
	if ( !trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd ) ||
			cg.snap->ps.commandTime + 1000 >= cmd.serverTime ) {
		return;
	}

	// also add text in center of screen
	AspectCorrect_ResetMode();
	s = ingame_text[IGT_CONNECTIONINTERRUPTED];
//	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	w = UI_ProportionalStringWidth(s,UI_BIGFONT);
//	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);
	UI_DrawProportionalString(320 - w/2, 100, s, UI_BIGFONT, colorTable[CT_LTGOLD1]);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	AspectCorrect_SetMode( HSCALE_RIGHT, VSCALE_BOTTOM );
	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
	AspectCorrect_ResetMode();
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

	AspectCorrect_SetMode( HSCALE_RIGHT, VSCALE_BOTTOM );

	//
	// draw the graph
	//
	x = 640 - 48 - 75;	//move it left of the ammo numbers
	y = 480 - 48;

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
//		CG_DrawBigString( ax, ay, "snc", 1.0 );
		UI_DrawProportionalString(ax, ay, "snc", UI_BIGFONT, colorTable[CT_LTGOLD1]);
	}

	CG_DrawDisconnect();
	AspectCorrect_ResetMode();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		x, y, w;
	float	*color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 60; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

//		w = cg.centerPrintCharWidth * CG_DrawStrlen( linebuffer );
		w = UI_ProportionalStringWidth(linebuffer,UI_BIGFONT);

		x = ( SCREEN_WIDTH - w ) / 2;

//		CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue,
//			cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0 );

		UI_DrawProportionalString( x, y, linebuffer, UI_BIGFONT|UI_DROPSHADOW, color);

		y += cg.centerPrintCharWidth * 1.5;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void) {
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || (cg.snap->ps.eFlags&EF_ELIMINATED) ) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	hShader = VMExt_GVCommandInt( "crosshair_get_current_shader", -1 );
	if ( hShader < 0 ) {
		// No engine crosshair support - load crosshair the traditional way
		if ( !cg_drawCrosshair.integer ) {
			return;
		}
		hShader = cgs.media.crosshairShader[ cg_drawCrosshair.integer % NUM_CROSSHAIRS ];
	} else if ( hShader == 0 ) {
		// Engine crosshair support, but crosshair disabled
		return;
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		trap_R_SetColor( NULL );
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	x = 320.0f + cg_crosshairX.value - ( w / 2.0f );
	y = 240.0f + cg_crosshairY.value - ( h / 2.0f );
	CG_AdjustFrom640( &x, &y, &w, &h );

	trap_R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, hShader );
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 8192, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
		cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// if the player is in fog, don't show it
	content = trap_CM_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	int			team;
	int			nameFlags = UI_CENTER|UI_SMALLFONT;

	if ( !cg_drawCrosshair.integer )
	{
		return;
	}
	if ( !cg_drawCrosshairNames.integer )
	{
		return;
	}
	if ( cg.renderingThirdPerson )
	{
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color )
	{
		trap_R_SetColor( NULL );
		return;
	}

	color[3] *= 0.9;

	// Draw in red if red team, blue if blue team
	if (cgs.gametype >= GT_TEAM)
	{
		nameFlags |= UI_FORCECOLOR;
		team = cgs.clientinfo[ cg.crosshairClientNum ].team;
		if ( cgs.clientinfo[ cg.crosshairClientNum ].pClass == PC_BORG )
		{
			color[0] = colorGreen[0];
			color[1] = colorGreen[1];
			color[2] = colorGreen[2];
		}
		else if (team==TEAM_RED)
		{
			color[0] = colorRed[0];
			color[1] = colorRed[1];
			color[2] = colorRed[2];
		}
		else
		{
			color[0] = colorBlue[0];
			color[1] = colorBlue[1];
			color[2] = colorBlue[2];
		}
	}
	else
	{
		color[0] = colorTable[CT_YELLOW][0];
		color[1] = colorTable[CT_YELLOW][1];
		color[2] = colorTable[CT_YELLOW][2];
	}

	UI_DrawProportionalString(320,170, cgs.clientinfo[ cg.crosshairClientNum ].name, nameFlags, color);

	//FIXME: need health (&armor?) of teammates (if not TEAM_FREE) or everyone (if SPECTATOR) (or just crosshairEnt?) sent to me
	if ( (cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_SPECTATOR || cgs.clientinfo[ cg.snap->ps.clientNum ].pClass == PC_MEDIC) )
	{//if I'm a spectator, draw colored health of target under crosshair
		CG_GetColorForHealth( cgs.clientinfo[ cg.crosshairClientNum ].health,
			cgs.clientinfo[ cg.crosshairClientNum ].armor, color );
		UI_DrawProportionalString(320,170+SMALLCHAR_HEIGHT,va( "%d", cgs.clientinfo[ cg.crosshairClientNum ].health ),UI_CENTER|UI_SMALLFONT,color);
	}
}



//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
	AspectCorrect_SetMode( HSCALE_CENTER, VSCALE_BOTTOM );
//	CG_DrawBigString(320 - 9 * 8, 440, ingame_text[IGT_SPECTATOR], 1.0F);
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR )
	{
		UI_DrawProportionalString(SCREEN_WIDTH/2, SCREEN_HEIGHT - ((BIGCHAR_HEIGHT * 1.50) * 2) , ingame_text[IGT_SPECTATOR], UI_BIGFONT|UI_CENTER, colorTable[CT_LTGOLD1]);
	}
	else if ( cg.snap->ps.eFlags&EF_ELIMINATED )
	{
		UI_DrawProportionalString(SCREEN_WIDTH/2, SCREEN_HEIGHT - ((BIGCHAR_HEIGHT * 1.50) * 2) , ingame_text[IGT_TITLEELIMINATED], UI_BIGFONT|UI_CENTER, colorTable[CT_LTGOLD1]);
	}
	if ( cgs.gametype == GT_TOURNAMENT ) {
//		CG_DrawBigString(320 - 15 * 8, 460, ingame_text[IGT_WAITINGTOPLAY], 1.0F);
		UI_DrawProportionalString(SCREEN_WIDTH/2,  SCREEN_HEIGHT - (BIGCHAR_HEIGHT * 1.5), ingame_text[IGT_WAITINGTOPLAY], UI_BIGFONT|UI_CENTER, colorTable[CT_LTGOLD1]);
	}
	if ( cgs.gametype == GT_TEAM || cgs.gametype == GT_CTF ) {
//		CG_DrawBigString(320 - 25 * 8, 460, ingame_text[IGT_USEDTEAMMENU], 1.0F);
		UI_DrawProportionalString(SCREEN_WIDTH/2,  SCREEN_HEIGHT - (BIGCHAR_HEIGHT * 1.5), ingame_text[IGT_USEDTEAMMENU], UI_BIGFONT|UI_CENTER, colorTable[CT_LTGOLD1]);
	}
	AspectCorrect_ResetMode();
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	char	*s;
	int		sec;

	if ( !cgs.voteTime ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("%s(%i):%s %s(F1):%i %s(F2):%i", ingame_text[IGT_VOTE],sec, cgs.voteString,ingame_text[IGT_YES], cgs.voteYes,ingame_text[IGT_NO]  ,cgs.voteNo);

//	CG_DrawSmallStringColor( 0, 58, s, colorTable[CT_YELLOW] );
	UI_DrawProportionalString( 0,  58, s, UI_SMALLFONT, colorTable[CT_YELLOW]);
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
	if (0)// cgs.gametype == GT_SINGLE_PLAYER )
	{
		CG_DrawCenterString();
		return;
	}

	cg.scoreFadeTime = cg.time;
	CG_DrawScoreboard();
}

/*
=================
CG_DrawAbridgedObjective
=================
*/
static void CG_DrawAbridgedObjective(void)
{
	int i,pixelLen,x,y;

	for (i=0;i<MAX_OBJECTIVES;i++)
	{
		if (cgs.objectives[i].abridgedText[0])
		{
			if (!cgs.objectives[i].complete)
			{
				pixelLen = UI_ProportionalStringWidth( cgs.objectives[i].abridgedText,UI_TINYFONT);

				x = 364 - (pixelLen/2);
				y = SCREEN_HEIGHT - PROP_TINY_HEIGHT;
				UI_DrawProportionalString(x, y, cgs.objectives[i].abridgedText, UI_TINYFONT, colorTable[CT_GREEN] );
				break;
			}
		}
	}

}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	float		y;
	vec4_t		color;
	const char	*name;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}
	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	y = 16;

	UI_DrawProportionalString((SCREEN_WIDTH/2), y, ingame_text[IGT_FOLLOWING], UI_BIGFONT|UI_CENTER, colorTable[CT_LTGOLD1]);

	name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

	y += (BIGCHAR_HEIGHT * 1.25);
	UI_DrawProportionalString(  (SCREEN_WIDTH/2), 40, name, UI_BIGFONT|UI_CENTER, color);

	return qtrue;
}



/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void )
{
	const char	*s;

	if ( cg_drawAmmoWarning.integer == 0 )
	{
		return;
	}

	if ( !cg.lowAmmoWarning )
	{
		return;
	}

	if ( cg.lowAmmoWarning >= 2 )
	{
		s = ingame_text[IGT_OUTOFAMMO];
	} else
	{
		s = ingame_text[IGT_LOWAMMO];
	}

	UI_DrawProportionalString(320, 64, s, UI_SMALLFONT | UI_CENTER, colorTable[CT_LTGOLD1]);

}

/*
=================
CG_DrawWarmup
=================
*/
extern void CG_AddGameModNameToGameName( char *gamename );
static void CG_DrawWarmup( void ) {
	int			w;
	int			sec;
	int			i;
	clientInfo_t	*ci1, *ci2;
	int			cw;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
		s = ingame_text[IGT_WAITINGFORPLAYERS];
//		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		w = UI_ProportionalStringWidth(s,UI_BIGFONT);
//		CG_DrawBigString(320 - w / 2, 40, s, 1.0F);
		UI_DrawProportionalString(320 - w / 2, 40, s, UI_BIGFONT, colorTable[CT_LTGOLD1]);

		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_TOURNAMENT) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s vs %s", ci1->name, ci2->name );
//			w = CG_DrawStrlen( s );
			w = UI_ProportionalStringWidth(s,UI_BIGFONT);

			if ( w > 640 / BIGCHAR_WIDTH ) {
				cw = 640 / w;
			} else {
				cw = BIGCHAR_WIDTH;
			}
//			CG_DrawStringExt( 320 - w * cw/2, 20,s, colorWhite,
//					qfalse, qtrue, cw, (int)(cw * 1.5), 0 );
			UI_DrawProportionalString( (SCREEN_WIDTH/2), 20,s, UI_BIGFONT|UI_CENTER, colorTable[CT_LTGOLD1]);

		}
	} else {
		char	gamename[1024];

		if ( cgs.gametype == GT_FFA ) {
			s = ingame_text[IGT_GAME_FREEFORALL];
		} else if ( cgs.gametype == GT_TEAM ) {
			s =ingame_text[IGT_GAME_TEAMHOLOMATCH];
		} else if ( cgs.gametype == GT_CTF ) {
			s = ingame_text[IGT_GAME_CAPTUREFLAG];
		} else {
			s = "";
		}

		Q_strncpyz( gamename, s, sizeof(gamename) );

		CG_AddGameModNameToGameName( gamename );

		w = UI_ProportionalStringWidth(s,UI_BIGFONT);

		if ( w > 640 / BIGCHAR_WIDTH ) {
			cw = 640 / w;
		} else {
			cw = BIGCHAR_WIDTH;
		}

		UI_DrawProportionalString((SCREEN_WIDTH/2) , 20,gamename, UI_BIGFONT|UI_CENTER, colorTable[CT_LTGOLD1]);
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va( "%s: %i",ingame_text[IGT_STARTSIN], sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
		switch ( sec ) {
		case 0:
			trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
			break;
		case 1:
			trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
			break;
		case 2:
			trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
			break;
		default:
			break;
		}
	}
	switch ( cg.warmupCount ) {
	case 0:
		cw = 28;
		break;
	case 1:
		cw = 24;
		break;
	case 2:
		cw = 20;
		break;
	default:
		cw = 16;
		break;
	}

//	w = CG_DrawStrlen( s );
//	CG_DrawStringExt( 320 - w * cw/2, 70, s, colorWhite,
//			qfalse, qtrue, cw, (int)(cw * 1.5), 0 );

	w = UI_ProportionalStringWidth(s,UI_BIGFONT);
	UI_DrawProportionalString(  (SCREEN_WIDTH/2), 70, s, UI_BIGFONT|UI_CENTER, colorTable[CT_LTGOLD1]);

}

/*
================
CG_DrawZoomMask

================
*/
static void CG_DrawZoomMask( void )
{
	float		amt = 1, size, val, start_x, start_y;
	int			width, height, i;
	vec4_t		color1;

	if ( cg.snap->ps.persistant[PERS_CLASS] != PC_NOCLASS && cg.snap->ps.persistant[PERS_CLASS] != PC_ACTIONHERO && cg.snap->ps.persistant[PERS_CLASS] != PC_SNIPER && cg.snap->ps.persistant[PERS_CLASS] != PC_BORG )
	{//in a class-based game, only the sniper can zoom
		cg.zoomed = qfalse;
		cg.zoomLocked = qfalse;
		return;
	}
	// Calc where to place the zoom mask...all calcs are based off of a virtual 640x480 screen
	size = cg_viewsize.integer;

	width = 640 * size * 0.01;
	width &= ~1;

	height = 480 * size * 0.01;
	height &= ~1;

	start_x = ( 640 - width ) * 0.5;
	start_y = ( 480 - height ) * 0.5;

	if ( cg.zoomed )
	{
		// Smoothly fade in..Turn this off for now since the zoom is set to snap to 30% or so...fade looks a bit weird when it does that
		if ( cg.time - cg.zoomTime <= ZOOM_OUT_TIME ) {
			amt = ( cg.time - cg.zoomTime ) / ZOOM_OUT_TIME;
		}

		// Fade mask in
		for ( i = 0; i < 4; i++ ) {
			color1[i] = amt;
		}

		// Set fade color
		trap_R_SetColor( color1 );
		AspectCorrect_SetMode( HSCALE_STRETCH, VSCALE_STRETCH );
		CG_DrawPic( start_x, start_y, width, height, cgs.media.zoomMaskShader );
		AspectCorrect_ResetMode();

		start_x = 210;
		start_y = 80;

		CG_DrawPic( 320 + start_x, 241, 35, -170, cgs.media.zoomBarShader);
		CG_DrawPic( 320 - start_x, 241, -35, -170, cgs.media.zoomBarShader);
		CG_DrawPic( 320 + start_x, 239, 35, 170, cgs.media.zoomBarShader);
		CG_DrawPic( 320 - start_x, 239, -35, 170, cgs.media.zoomBarShader);

		// Calculate a percent and clamp it
		val = 26 - ( cg_fov.value - cg_zoomFov.value ) / ( cg_fov.value - MAX_ZOOM_FOV ) * 26;

		if ( val > 17.0f )
			val = 17.0f;
		else if ( val < 0.0f )
			val = 0.0f;

		// pink
		color1[0] = 0.85f;
		color1[1] = 0.55f;
		color1[2] = 0.75f;
		color1[3] = 1.0f;

		CG_DrawPic( 320 + start_x + 12, 245, 10, 108, cgs.media.zoomInsertShader );
		CG_DrawPic( 320 + start_x + 12, 235, 10, -108, cgs.media.zoomInsertShader );
		CG_DrawPic( 320 - start_x - 12, 245, -10, 108, cgs.media.zoomInsertShader );
		CG_DrawPic( 320 - start_x - 12, 235, -10, -108, cgs.media.zoomInsertShader );

		trap_R_SetColor( color1 );
		i = ((int)val) * 6;

		CG_DrawPic( 320 + start_x + 10, 230 - i, 12, 5, cgs.media.ammoslider );
		CG_DrawPic( 320 + start_x + 10, 251 + i, 12, -5, cgs.media.ammoslider );
		CG_DrawPic( 320 - start_x - 10, 230 - i, -12, 5, cgs.media.ammoslider );
		CG_DrawPic( 320 - start_x - 10, 251 + i, -12, -5, cgs.media.ammoslider );

		// Convert zoom and view axis into some numbers to throw onto the screen
		CG_DrawNumField( 468, 100, 5, cg_zoomFov.value * 1000 + 9999, 18, 10 ,NUM_FONT_BIG );
		CG_DrawNumField( 468, 120, 5, cg.refdef.viewaxis[0][0] * 9999 + 20000, 18, 10,NUM_FONT_BIG );
		CG_DrawNumField( 468, 140, 5, cg.refdef.viewaxis[0][1] * 9999 + 20000, 18, 10,NUM_FONT_BIG );
		CG_DrawNumField( 468, 160, 5, cg.refdef.viewaxis[0][2] * 9999 + 20000, 18, 10,NUM_FONT_BIG );

		// Is it time to draw the little max zoom arrows?
		if ( val < 0.2f )
		{
			amt = sin( cg.time * 0.03 ) * 0.5 + 0.5;
			color1[0] = 0.592156f * amt;
			color1[1] = 0.592156f * amt;
			color1[2] = 0.850980f * amt;
			color1[3] = 1.0f * amt;

			trap_R_SetColor( color1 );

			CG_DrawPic( 320 + start_x, 240 - 6, 16, 12, cgs.media.zoomArrowShader );
			CG_DrawPic( 320 - start_x, 240 - 6, -16, 12, cgs.media.zoomArrowShader );
		}
	}
	else
	{
		if ( cg.time - cg.zoomTime <= ZOOM_OUT_TIME )
		{
			amt = 1.0f - ( cg.time - cg.zoomTime ) / ZOOM_OUT_TIME;

			// Fade mask away
			for ( i = 0; i < 4; i++ ) {
				color1[i] = amt;
			}

			trap_R_SetColor( color1 );
			AspectCorrect_SetMode( HSCALE_STRETCH, VSCALE_STRETCH );
			CG_DrawPic( start_x, start_y, width, height, cgs.media.zoomMaskShader );
			AspectCorrect_ResetMode();
		}
	}
}

//==================================================================================

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void ) {

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		AspectCorrect_SetMode( HSCALE_RIGHT, VSCALE_TOP );
		CG_DrawUpperRight();
		AspectCorrect_ResetMode();

		CG_DrawIntermission();
		return;
	}

	if ( !cg.renderingThirdPerson )
	{
		CG_DrawZoomMask();
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || (cg.snap->ps.eFlags&EF_ELIMINATED) ) {
//		CG_DrawSpectator();
		CG_DrawCrosshair();
		CG_DrawCrosshairNames();
	} else {
		// don't draw any status if dead
		if ( cg.snap->ps.stats[STAT_HEALTH] > 0 ) {
			CG_DrawStatusBar();
			AspectCorrect_SetMode( HSCALE_CENTER, VSCALE_TOP );
			CG_DrawAmmoWarning();
			AspectCorrect_ResetMode();
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			AspectCorrect_SetMode( HSCALE_CENTER, VSCALE_BOTTOM );
			CG_DrawWeaponSelect();
			AspectCorrect_SetMode( HSCALE_RIGHT, VSCALE_BOTTOM );
			CG_DrawHoldableItem();
			AspectCorrect_ResetMode();
			CG_DrawReward();
			CG_DrawAbridgedObjective();
		}
		if ( cgs.gametype >= GT_TEAM ) {
			CG_DrawTeamInfo();
		}
	}

	if (cg.showObjectives)
	{
		CG_DrawObjectiveInformation();
	}

	AspectCorrect_SetMode( HSCALE_LEFT, VSCALE_TOP );
	CG_DrawVote();

	CG_DrawLagometer();

	AspectCorrect_SetMode( HSCALE_RIGHT, VSCALE_TOP );
	CG_DrawUpperRight();

	AspectCorrect_SetMode( HSCALE_RIGHT, VSCALE_BOTTOM );
	CG_DrawLowerRight();

	AspectCorrect_SetMode( HSCALE_LEFT, VSCALE_BOTTOM );
	CG_DrawLowerLeft();

	AspectCorrect_SetMode( HSCALE_CENTER, VSCALE_TOP );
	if ( !CG_DrawFollow() ) {
		CG_DrawWarmup();
	}

	AspectCorrect_ResetMode();

	// don't draw center string if scoreboard is up
	if ( !CG_DrawScoreboard() ) {
		CG_DrawCenterString();
	}

#ifdef ADDON_RESPAWN_TIMER
	// display the respawn countdown if indicated by server
	if ( cgs.modConfig.respawnTimerStatIndex > 0 && cgs.modConfig.respawnTimerStatIndex < MAX_STATS - 1 ) {
		int respawnTime = ( ( cg.snap->ps.stats[cgs.modConfig.respawnTimerStatIndex] & 0xffff ) << 16 ) |
				( cg.snap->ps.stats[cgs.modConfig.respawnTimerStatIndex + 1] & 0xffff );
		if ( respawnTime && respawnTime > cg.time ) {
			vec4_t color = { 1, 1, 1, 1 };
			char buffer[256];
			int x;
			Com_sprintf( buffer, sizeof( buffer ), "RESPAWN IN: %i", ( respawnTime - cg.time - 1 ) / 1000 + 1 );
			x = ( SCREEN_WIDTH - UI_ProportionalStringWidth( buffer, UI_BIGFONT ) ) / 2;
			UI_DrawProportionalString( x, SCREEN_HEIGHT - 90, buffer, UI_BIGFONT | UI_DROPSHADOW | UI_BLINK, color );
		}
	}
#endif

	// kef -- need the "use TEAM menu to play" message to draw on top of the bottom bar of scoreboard
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || (cg.snap->ps.eFlags&EF_ELIMINATED) )
	{
		CG_DrawSpectator();
	}
}


/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	float		separation;
	vec3_t		baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || (cg.snap->ps.eFlags&EF_ELIMINATED))&&
		( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	switch ( stereoView ) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error( "CG_DrawActive: Undefined stereoView" );
	}


	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	// draw status bar and other floating elements
	CG_Draw2D();
}
