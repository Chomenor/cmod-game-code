// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "ui_local.h"

typedef struct {
	qboolean initialized;

	// r_mode cvar only applies to windowed mode, not fullscreen
	qboolean windowed_r_mode;

	// use updated template set
	qboolean modern_templates;

	// if enabled, anisotropic filtering option is in video menu, otherwise in video additional
	// this is needed because their sometimes isn't space in video menu...
	qboolean anisotropicFilteringVideoMenu;

	// normally enabled options, but can be disabled by engine
	qboolean supportGlDriver;
	qboolean supportAllowExtensions;
	qboolean supportColorDepth;
	qboolean supportTextureBits;
	qboolean supportSimpleShaders;
	
	// new additional options
	qboolean supportRendererSelect;
	qboolean supportMultisample;
	qboolean supportVSync;
	qboolean supportMaxAnisotropy;
} videoEngineConfig_t;

static videoEngineConfig_t videoEngineConfig;

/*
=================
UI_InitVideoEngineConfig

Load settings in the videoEngineConfig structure.
=================
*/
static void UI_InitVideoEngineConfig( void ) {
	if ( !videoEngineConfig.initialized ) {
		int freeSlots = 0;

		videoEngineConfig.initialized = qtrue;
		videoEngineConfig.windowed_r_mode = VMExt_GVCommandInt( "ui_using_windowed_r_mode", 0 ) ? qtrue : qfalse;
		videoEngineConfig.modern_templates = VMExt_GVCommandInt( "ui_modern_video_templates", 0 ) ? qtrue : qfalse;
		videoEngineConfig.supportMaxAnisotropy = VMExt_GVCommandInt( "ui_support_r_ext_max_anisotropy", 0 ) ? qtrue : qfalse;

		if ( !VMExt_GVCommandInt( "ui_skip_r_glDriver", 0 ) ) {
			videoEngineConfig.supportGlDriver = qtrue;
		} else {
			++freeSlots;
		}

		if ( !VMExt_GVCommandInt( "ui_skip_r_allowExtensions", 0 ) ) {
			videoEngineConfig.supportAllowExtensions = qtrue;
		} else {
			++freeSlots;
		}

		if ( !( VMExt_GVCommandInt( "ui_skip_r_colorbits", 0 ) && VMExt_GVCommandInt( "ui_skip_r_depthbits", 0 ) &&
				VMExt_GVCommandInt( "ui_skip_r_stencilbits", 0 ) ) ) {
			videoEngineConfig.supportColorDepth = qtrue;
		} else {
			++freeSlots;
		}

		if ( !VMExt_GVCommandInt( "ui_skip_r_texturebits", 0 ) ) {
			videoEngineConfig.supportTextureBits = qtrue;
		} else {
			++freeSlots;
		}

		if ( !VMExt_GVCommandInt( "ui_skip_r_lowEndVideo", 0 ) ) {
			videoEngineConfig.supportSimpleShaders = qtrue;
		} else {
			++freeSlots;
		}

		if ( freeSlots > 0 && VMExt_GVCommandInt( "ui_support_cl_renderer_opengl1", 0 ) &&
				VMExt_GVCommandInt( "ui_support_cl_renderer_opengl2", 0 ) ) {
			videoEngineConfig.supportRendererSelect = qtrue;
			--freeSlots;
		}

		if ( freeSlots > 0 && VMExt_GVCommandInt( "ui_support_cmd_set_multisample", 0 ) &&
				VMExt_GVCommandInt( "ui_support_cmd_get_multisample", 0 ) ) {
			videoEngineConfig.supportMultisample = qtrue;
			--freeSlots;
		}

		if ( freeSlots > 0 && VMExt_GVCommandInt( "ui_support_r_swapInterval", 0 ) ) {
			videoEngineConfig.supportVSync = qtrue;
			--freeSlots;
		}

		if ( freeSlots > 0 ) {
			videoEngineConfig.anisotropicFilteringVideoMenu = qtrue;
			--freeSlots;
		}
	}
}

/*
=================
UI_SetAnisotropyLevel

Sets anisotropic filtering level on scale from 0 (disabled) to 4 (16x).

UI_InitVideoEngineConfig should already be called.
=================
*/
static void UI_SetAnisotropyLevel( int level ) {
	if ( level > 0 ) {
		trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "1" );
		if ( videoEngineConfig.supportMaxAnisotropy ) {
			trap_Cvar_Set( "r_ext_max_anisotropy", va( "%i", 1 << level ) );
		}
	} else {
		trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
	}
}

/*
=================
UI_GetAnisotropyLevel

Retrieves current anisotropic filtering level on scale from 0 (disabled) to 4 (16x),
or from 0 (disabled) to 1 (enabled) if videoEngineConfig.supportMaxAnisotropy is false.

UI_InitVideoEngineConfig should already be called.
=================
*/
static int UI_GetAnisotropyLevel( void ) {
	if ( trap_Cvar_VariableValue( "r_ext_texture_filter_anisotropic" ) ) {
		if ( videoEngineConfig.supportMaxAnisotropy ) {
			float level = trap_Cvar_VariableValue( "r_ext_max_anisotropy" );
			if ( level >= 16.0f )
				return 4;
			else if ( level >= 8.0f )
				return 3;
			else if ( level >= 4.0f )
				return 2;
		}
		return 1;
	}

	return 0;
}

void UI_VideoDriverMenu( void );
void VideoDriver_Lines(int increment);
void UI_VideoData2SettingsMenu( void );


extern void *holdControlPtr;
extern int holdControlEvent;
static void Video_MenuEvent (void* ptr, int event);

#define PIC_MONBAR2		"menu/common/monbar_2.tga"
#define PIC_SLIDER		"menu/common/slider.tga"

// Video Data
typedef struct
{
	menuframework_s	menu;

	qhandle_t	swooshTop;
	qhandle_t	swooshBottom;
	qhandle_t	swooshTopSmall;
	qhandle_t	swooshBottomSmall;

} videoddata_t;

static videoddata_t	s_videodata;


// Video Drivers
typedef struct
{
	menuframework_s	menu;

	qhandle_t	swooshTopSmall;
	qhandle_t	swooshBottomSmall;
} videodriver_t;


// Video Data 2
typedef struct
{
	menuframework_s	menu;

	menulist_s		aspectCorrection;
	menuslider_s	centerHud_slider;
	menuslider_s	fov_slider;
	menuslider_s	screensize_slider;
	menulist_s		flares;
	menulist_s		wallmarks;
	menulist_s		dynamiclights;
	menulist_s		simpleitems;
	menulist_s		synceveryframe;
	menulist_s		anisotropic;

	qhandle_t	top;
} videodata2_t;

static videodata2_t	s_videodata2;


#define NUM_VIDEO_TEMPLATES 4

static int s_graphics_options_Names[NUM_VIDEO_TEMPLATES + 2] =
{
	MNT_VIDEO_HIGH_QUALITY,
	MNT_VIDEO_NORMAL,
	MNT_VIDEO_FAST,
	MNT_VIDEO_FASTEST,
	MNT_VIDEO_CUSTOM,
	MNT_NONE
};

static int s_graphics_options_modern_Names[NUM_VIDEO_TEMPLATES + 2] =
{
	MNT_VIDEO_LOW_QUALITY,
	MNT_VIDEO_MEDIUM_QUALITY,
	MNT_VIDEO_HIGH_QUALITY,
	MNT_VIDEO_HIGHEST_QUALITY,
	MNT_VIDEO_CUSTOM,
	MNT_NONE
};

static int s_driver_Names[] =
{
	MNT_VIDEO_DRIVER_DEFAULT,
	MNT_VIDEO_DRIVER_VOODOO,
	MNT_NONE
};

static int s_renderer_Names[] =
{
	MNT_VIDEO_RENDERER_OPENGL1,
	MNT_VIDEO_RENDERER_OPENGL2,
	MNT_NONE
};

static int s_renderer_NamesUnknown[] =
{
	MNT_VIDEO_RENDERER_OPENGL1,
	MNT_VIDEO_RENDERER_OPENGL2,
	MNT_VIDEO_CUSTOM,
	MNT_NONE
};

extern int s_OffOnNone_Names[];

static int s_resolutions[] =
{
//	MNT_320X200,
//	MNT_400X300,
	MNT_512X384,
	MNT_640X480,
	MNT_800X600,
	MNT_960X720,
	MNT_1024X768,
	MNT_1152X864,
	MNT_1280X960,
	MNT_1600X1200,
	MNT_2048X1536,
	MNT_856x480WIDE,
	MNT_VIDEO_CUSTOM,
	MNT_NONE
};

#define NUM_RESOLUTIONS ( ( sizeof ( s_resolutions ) / sizeof ( *s_resolutions ) ) - 2 )

static int s_colordepth_Names[] =
{
	MNT_DEFAULT,
	MNT_16BIT,
	MNT_32BIT,
	MNT_NONE
};

static int s_lighting_Names[] =
{
	MNT_LIGHTMAP,
	MNT_VERTEX,
	MNT_NONE
};

static int s_quality_Names[] =
{
	MNT_LOW,
	MNT_MEDIUM,
	MNT_HIGH,
	MNT_NONE
};

static int s_4quality_Names[] =
{
	MNT_LOW,
	MNT_MEDIUM,
	MNT_HIGH,
	MNT_VERY_HIGH,
	MNT_NONE
};

static int s_tqbits_Names[] =
{
	MNT_DEFAULT,
	MNT_16BIT,
	MNT_32BIT,
	MNT_NONE
};

static int s_filter_Names[] =
{
	MNT_BILINEAR,
	MNT_TRILINEAR,
	MNT_NONE
};

static int s_anisotropic_Names[] =
{
	MNT_OFF,
	MNT_VIDEO_2X,
	MNT_VIDEO_4X,
	MNT_VIDEO_8X,
	MNT_VIDEO_16X,
	MNT_NONE
};

static int s_multisample_Names[] =
{
	MNT_OFF,
	MNT_VIDEO_2X,
	MNT_VIDEO_4X,
	MNT_NONE
};


static menubitmap_s			s_video_drivers;
static menubitmap_s			s_video_data;
static menubitmap_s			s_video_data2;
static menubitmap_s			s_video_brightness;

#define ID_MAINMENU		100
#define ID_CONTROLS		101
#define ID_VIDEO		102
#define ID_SOUND		103
#define ID_GAMEOPTIONS	104
#define ID_CDKEY		105
#define ID_VIDEODATA	110
#define ID_VIDEODATA2	111
#define ID_VIDEODRIVERS	112
#define ID_ARROWDWN		113
#define ID_ARROWUP		114
#define ID_INGAMEMENU	115
#define ID_VIDEOBRIGHTNESS	116

// Precache stuff for Video Driver
#define MAX_VID_DRIVERS 128
static struct
{
	menuframework_s		menu;

	char *drivers[MAX_VID_DRIVERS];
	char extensionsString[2*MAX_STRING_CHARS];

	menutext_s		line1;
	menutext_s		line2;
	menutext_s		line3;
	menutext_s		line4;
	menutext_s		line5;
	menutext_s		line6;
	menutext_s		line7;
	menutext_s		line8;
	menutext_s		line9;
	menutext_s		line10;
	menutext_s		line11;
	menutext_s		line12;
	menutext_s		line13;
	menutext_s		line14;
	menutext_s		line15;
	menutext_s		line16;
	menutext_s		line17;
	menutext_s		line18;
	menutext_s		line19;
	menutext_s		line20;
	menutext_s		line21;
	menutext_s		line22;
	menutext_s		line23;
	menutext_s		line24;

	qhandle_t	corner_ll_8_16;
	qhandle_t	corner_ll_16_16;
	qhandle_t	arrow_dn;
	menubitmap_s	arrowdwn;
	menubitmap_s	arrowup;
	int			currentDriverLine;
	int			driverCnt;

	int			activeArrowDwn;
	int			activeArrowUp;
} s_videodriver;


static void* g_videolines[] =
{
	&s_videodriver.line1,
	&s_videodriver.line2,
	&s_videodriver.line3,
	&s_videodriver.line4,
	&s_videodriver.line5,
	&s_videodriver.line6,
	&s_videodriver.line7,
	&s_videodriver.line8,
	&s_videodriver.line9,
	&s_videodriver.line10,
	&s_videodriver.line11,
	&s_videodriver.line12,
	&s_videodriver.line13,
	&s_videodriver.line14,
	&s_videodriver.line15,
	&s_videodriver.line16,
	&s_videodriver.line17,
	&s_videodriver.line18,
	&s_videodriver.line19,
	&s_videodriver.line20,
	&s_videodriver.line21,
	&s_videodriver.line22,
	&s_videodriver.line23,
	&s_videodriver.line24,
	NULL,
};

int video_sidebuttons[4][2] =
{
	30, 240,	// Video Data Button
	30, 240 + 6 + 24,	// Brightness Button
	30, 240 + (2 * (6 + 24)),	// Additional Button
	30, 240 + (3 * (6 + 24)),	// Video Drivers Button
};


void Video_SideButtons(menuframework_s *menu,int menuType);
static void GraphicsOptions_ApplyChanges( void *unused, int notification );


/*
=======================================================================

BRIGHTNESS MENU

=======================================================================
*/

typedef struct {
	float gamma;
	float lighting_gamma;
	float overbright_factor;
	float intensity;
} brightnessTemplate_t;

const brightnessTemplate_t brightnessTemplatePlain = { 14.0f, 5.0f, 15.0f, 0.0f };
const brightnessTemplate_t brightnessTemplateFullContrast = { 13.0f, 5.0f, 20.0f, 0.0f };
const brightnessTemplate_t brightnessTemplateExtraBright = { 14.0f, 10.0f, 15.0f, 4.0f };
const brightnessTemplate_t brightnessTemplateFullBright = { 11.0f, 20.0f, 10.0f, 16.0f };

struct
{
	menuframework_s menu;
	qboolean supportMapLightingGamma;
	qboolean supportOverBrightFactor;
	qboolean supportIntensity;
	qboolean supportIntensityFractional;

	menuslider_s	gamma_slider;
	menuaction_s	gamma_apply;

	brightnessTemplate_t	current_loaded_values;
	menuslider_s	lighting_gamma_slider;
	menuslider_s	overbright_slider;
	menuslider_s	intensity_slider;
	menubitmap_s	template_plain;
	menubitmap_s	template_full_contrast;
	menubitmap_s	template_extra_bright;
	menubitmap_s	template_full_bright;
	menuaction_s	additional_apply;

	qhandle_t	gamma;
	qhandle_t	top;
} s_brightness;

// Display additional brightness options in a box below gamma settings
#define SUPPORT_BRIGHTNESS_ADDITIONAL ( s_brightness.supportMapLightingGamma && s_brightness.supportOverBrightFactor )

/*
=================
UI_BrightnessEncodeMLG

Convert r_mapLightingGamma actual value to slider value. The conversion is intended to make
the slider positions more closely correspond to useful values.
=================
*/
static float UI_BrightnessEncodeMLG( float x ) {
	if ( x >= 10.0f )
		return 20.0f;
	if ( x >= 3.5f )
		return 15.0f + ( x - 3.5f ) / 1.3f;
	if ( x >= 1.75f )
		return 10.0f + ( x - 1.75f ) / 0.35f;
	if ( x >= 1.0f )
		return 5.0f + ( x - 1.0f ) / 0.15f;
	if ( x >= 0.7f )
		return ( x - 0.7f ) / 0.06f;
	return 0.0f;
}

/*
=================
UI_BrightnessDecodeMLG

Convert r_mapLightingGamma slider value to actual value.
=================
*/
static float UI_BrightnessDecodeMLG( float x ) {
	if ( x >= 15.0f )
		return 3.5f + ( x - 15.0f ) * 1.3f;
	if ( x >= 10.0f )
		return 1.75f + ( x - 10.0f ) * 0.35f;
	if ( x >= 5.0f )
		return 1.0f + ( x - 5.0f ) * 0.15f;
	return 0.7f + x * 0.06f;
}

/*
=================
UI_BrightnessMenu_MenuDraw
=================
*/
static void UI_BrightnessMenu_MenuDraw (void)
{
	int y;

	UI_MenuFrame(&s_brightness.menu);

	UI_DrawProportionalString(  74,  66, "815",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  84, "9047",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  188, "1596",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  206, "7088",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  395, "2-9831",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	UI_Setup_MenuButtons();

	y = 191;
	if ( SUPPORT_BRIGHTNESS_ADDITIONAL )
	{
		y -= 12;
	}
	if (uis.glconfig.deviceSupportsGamma)
	{
		trap_R_SetColor( colorTable[CT_DKGREY]);
		UI_DrawHandlePic(  178, y, 68, 68, uis.whiteShader);	//
		trap_R_SetColor( colorTable[CT_WHITE]);
		UI_DrawHandlePic(  180, y+2, 64, 64, s_brightness.gamma);	// Starfleet graphic

		UI_DrawProportionalString( 256,  y + 5, menu_normal_text[MNT_GAMMA_LINE1],UI_SMALLFONT, colorTable[CT_LTGOLD1]);
		UI_DrawProportionalString( 256,  y + 25, menu_normal_text[MNT_GAMMA_LINE2],UI_SMALLFONT, colorTable[CT_LTGOLD1]);
		UI_DrawProportionalString( 256,  y + 45, menu_normal_text[MNT_GAMMA_LINE3],UI_SMALLFONT,colorTable[CT_LTGOLD1]);
	}
	else
	{
		UI_DrawProportionalString( 178,  y + 5, menu_normal_text[MNT_GAMMA2_LINE1],UI_SMALLFONT, colorTable[CT_LTGOLD1]);
		UI_DrawProportionalString( 178,  y + 25,menu_normal_text[MNT_GAMMA2_LINE2],UI_SMALLFONT, colorTable[CT_LTGOLD1]);
	}


	if ( SUPPORT_BRIGHTNESS_ADDITIONAL ) {
		int y, h;
		y = 163;

		// Brackets around gamma
		h = 92;
		trap_R_SetColor( colorTable[CT_LTPURPLE1]);
		UI_DrawHandlePic(158,y, 16, 16, uis.graphicBracket1CornerLU);
		UI_DrawHandlePic(158,y + 16,  8, h, uis.whiteShader);
		UI_DrawHandlePic(158,y + h + 16, 16, -16, uis.graphicBracket1CornerLU);	//LD

		UI_DrawHandlePic(174,y, 408, 8, uis.whiteShader);	// Top line
		UI_DrawHandlePic(579,y, 32, 16, s_brightness.top);	// Corner, UR
		UI_DrawHandlePic(581,y + 16, 30, h, uis.whiteShader);	// Right column
		UI_DrawHandlePic(579,y + h + 16, 32, -16, s_brightness.top);	// Corner, LR
		UI_DrawHandlePic(174,y + h + 24, 408, 8, uis.whiteShader);	// Bottom line

		// Brackets around map lighting options
		y = 292;
		h = 104;
		trap_R_SetColor( colorTable[CT_LTPURPLE1]);
		UI_DrawHandlePic(158,y, 16, 16, uis.graphicBracket1CornerLU);
		UI_DrawHandlePic(158,y + 16,  8, h, uis.whiteShader);
		UI_DrawHandlePic(158,y + h + 16, 16, -16, uis.graphicBracket1CornerLU);	//LD

		UI_DrawHandlePic(174,y, 408, 8, uis.whiteShader);	// Top line
		UI_DrawHandlePic(579,y, 32, 16, s_brightness.top);	// Corner, UR
		UI_DrawHandlePic(581,y + 16, 30, h, uis.whiteShader);	// Right column
		UI_DrawHandlePic(579,y + h + 16, 32, -16, s_brightness.top);	// Corner, LR
		UI_DrawHandlePic(174,y + h + 24, 408, 8, uis.whiteShader);	// Bottom line

		UI_DrawProportionalString( 190, 304, menu_normal_text[MNT_VIDEO_ADDITIONAL_OPTIONS], UI_SMALLFONT, colorTable[CT_LTGOLD1]);
	} else {
		// Brackets around gamma
		trap_R_SetColor( colorTable[CT_LTPURPLE1]);
		UI_DrawHandlePic(158,163, 16, 16, uis.graphicBracket1CornerLU);
		UI_DrawHandlePic(158,179,  8, 233, uis.whiteShader);
		UI_DrawHandlePic(158,412, 16, -16, uis.graphicBracket1CornerLU);	//LD

		UI_DrawHandlePic(174,163, 408, 8, uis.whiteShader);	// Top line

		UI_DrawHandlePic(579,163, 32, 16, s_brightness.top);	// Corner, UR
		UI_DrawHandlePic(581,179, 30, 121, uis.whiteShader);	// Top right column
		UI_DrawHandlePic(581,303, 30, 109, uis.whiteShader);	// Bottom right column
		UI_DrawHandlePic(579,412, 32, -16, s_brightness.top);	// Corner, LR

		UI_DrawHandlePic(174,420, 408, 8, uis.whiteShader);	// Bottom line
	}

	Menu_Draw( &s_brightness.menu );
}

/*
=================
UI_BrightnessMenu_TemplateMatchesCurrent

Returns qtrue if template matches current settings.
=================
*/
static qboolean UI_BrightnessMenu_TemplateMatchesCurrent( const brightnessTemplate_t *template ) {
	if ( fabs( s_brightness.gamma_slider.curvalue - template->gamma ) > 0.01 )
		return qfalse;
	if ( s_brightness.supportMapLightingGamma && fabs( s_brightness.lighting_gamma_slider.curvalue - template->lighting_gamma ) > 0.01 )
		return qfalse;
	if ( s_brightness.supportOverBrightFactor && fabs( s_brightness.overbright_slider.curvalue - template->overbright_factor ) > 0.01 )
		return qfalse;
	if ( s_brightness.supportIntensity && fabs( s_brightness.intensity_slider.curvalue - template->intensity ) > 0.01 )
		return qfalse;
	return qtrue;
}

/*
=================
UI_BrightnessMenu_CheckTemplateButtonEnabled

Enable or disable the template button depending on whether it already matches the current settings.
=================
*/
static void UI_BrightnessMenu_CheckTemplateButtonEnabled( const brightnessTemplate_t *template, menubitmap_s *button ) {
	if ( UI_BrightnessMenu_TemplateMatchesCurrent( template ) ) {
		button->textcolor = CT_WHITE;
		button->textcolor2 = CT_WHITE;
		button->color = CT_DKGREY;
		button->color2 = CT_DKGREY;
		button->generic.flags |= QMF_INACTIVE;
	} else {
		button->textcolor = CT_BLACK;
		button->textcolor2 = CT_WHITE;
		button->color = CT_DKORANGE;
		button->color2 = CT_LTORANGE;
		button->generic.flags &= ~QMF_INACTIVE;
	}
}

/*
=================
UI_BrightnessMenu_CheckAdditionalApplyButtonEnabled

Enable or disable the additional options apply button.
=================
*/
static void UI_BrightnessMenu_CheckAdditionalApplyButtonEnabled( void ) {
	if ( UI_BrightnessMenu_TemplateMatchesCurrent( &s_brightness.current_loaded_values ) ) {
		s_brightness.additional_apply.generic.flags |= QMF_GRAYED;
		s_brightness.additional_apply.generic.flags &= ~QMF_BLINK;
	} else {
		s_brightness.additional_apply.generic.flags &= ~QMF_GRAYED;
		s_brightness.additional_apply.generic.flags |= QMF_BLINK;
	}
}

/*
=================
UI_BrightnessMenu_CheckButtonsEnabled

Enable or disable template and apply buttons depending on whether they already match the current settings.
=================
*/
static void UI_BrightnessMenu_CheckButtonsEnabled( void ) {
	UI_BrightnessMenu_CheckTemplateButtonEnabled( &brightnessTemplatePlain, &s_brightness.template_plain );
	UI_BrightnessMenu_CheckTemplateButtonEnabled( &brightnessTemplateFullContrast, &s_brightness.template_full_contrast );
	UI_BrightnessMenu_CheckTemplateButtonEnabled( &brightnessTemplateExtraBright, &s_brightness.template_extra_bright );
	UI_BrightnessMenu_CheckTemplateButtonEnabled( &brightnessTemplateFullBright, &s_brightness.template_full_bright );
	UI_BrightnessMenu_CheckAdditionalApplyButtonEnabled();
}

/*
=================
UI_BrightnessMenu_ApplyTemplate
=================
*/
static void UI_BrightnessMenu_ApplyTemplate( const brightnessTemplate_t *template ) {
	// Set the gamma slider position, but don't actually apply it yet, so if the user cancels
	// it doesn't leave a situation where gamma is modified but not the other template settings.
	s_brightness.gamma_slider.curvalue = template->gamma;
	s_brightness.lighting_gamma_slider.curvalue = template->lighting_gamma;
	s_brightness.overbright_slider.curvalue = template->overbright_factor;
	s_brightness.intensity_slider.curvalue = template->intensity;
	UI_BrightnessMenu_CheckButtonsEnabled();
}

/*
=================
UI_BrightnessCallback
=================
*/
static void UI_BrightnessCallback( void *s, int notification )
{
	if (notification != QM_ACTIVATED)
		return;

	if ( s == &s_brightness.gamma_apply ) {
		trap_Cvar_SetValue( "r_gamma", s_brightness.gamma_slider.curvalue / 10.0f );
		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );

	} else if ( s == &s_brightness.gamma_slider ) {
		if ( uis.glconfig.deviceSupportsGamma ) {
			// Update gamma in real time
			trap_Cvar_SetValue( "r_gamma", s_brightness.gamma_slider.curvalue / 10.0f );
			s_brightness.current_loaded_values.gamma = s_brightness.gamma_slider.curvalue;
		} else {
			// Blink the apply button
			s_brightness.gamma_apply.generic.flags &= ~QMF_GRAYED;
			s_brightness.gamma_apply.generic.flags |= QMF_BLINK;
		}
		UI_BrightnessMenu_CheckButtonsEnabled();

	} else if ( s == &s_brightness.additional_apply ) {
		trap_Cvar_SetValue( "r_gamma", s_brightness.gamma_slider.curvalue / 10.0f );
		if ( s_brightness.supportMapLightingGamma ) {
			// Also set min clamp value along with high r_mapLightingGamma values
			float value = UI_BrightnessDecodeMLG( s_brightness.lighting_gamma_slider.curvalue );
			float min = ( value - 2.0f ) * 0.125f;
			if ( min < 0.0f )
				min = 0.0f;
			if ( min > 1.0f )
				min = 1.0f;
			trap_Cvar_SetValue( "r_mapLightingGamma", value );
			trap_Cvar_SetValue( "r_mapLightingClampMin", min );
		}
		if ( s_brightness.supportOverBrightFactor ) {
			trap_Cvar_SetValue( "r_overBrightFactor", s_brightness.overbright_slider.curvalue / 10.0f );
		}
		if ( s_brightness.supportIntensity ) {
			float value = s_brightness.intensity_slider.curvalue / 40.0f + 1.0f;
			if ( value != 1.0f && s_brightness.supportIntensityFractional ) {
				// Set fractional 0.5 value which helps reduce overbrightening, if supported by engine
				char buffer[256];
				int frac = value * 1000;
				Com_sprintf( buffer, sizeof( buffer ), "%i.%i:0.5", frac / 1000, frac % 1000 );
				trap_Cvar_Set( "r_intensity", buffer );
			} else {
				trap_Cvar_SetValue( "r_intensity", value );
			}
		}

		// Reset other brightness settings to avoid stacking
		if ( !s_brightness.supportMapLightingGamma ) {
			trap_Cvar_Set( "r_mapLightingClampMin", "0" );
		}
		trap_Cvar_Set( "r_mapLightingClampMax", "1" );
		trap_Cvar_Set( "r_ignorehwgamma", "0" );
		trap_Cvar_Set( "r_textureGamma", "1" );
		trap_Cvar_Set( "r_mapLightingFactor", "2" );

		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );

	} else if ( s == &s_brightness.template_plain ) {
		UI_BrightnessMenu_ApplyTemplate( &brightnessTemplatePlain );

	} else if ( s == &s_brightness.template_full_contrast ) {
		UI_BrightnessMenu_ApplyTemplate( &brightnessTemplateFullContrast );

	} else if ( s == &s_brightness.template_extra_bright ) {
		UI_BrightnessMenu_ApplyTemplate( &brightnessTemplateExtraBright );

	} else if ( s == &s_brightness.template_full_bright ) {
		UI_BrightnessMenu_ApplyTemplate( &brightnessTemplateFullBright );

	} else if ( s == &s_brightness.lighting_gamma_slider || s == &s_brightness.overbright_slider || s == &s_brightness.intensity_slider ) {
		UI_BrightnessMenu_CheckButtonsEnabled();
	}
}

/*
=================
UI_BrightnessMenu_Init
=================
*/
void UI_BrightnessMenu_Init( void )
{
	int x, y;

	memset( &s_brightness, 0, sizeof( s_brightness ) );

	// Determine engine support for map lighting options
	s_brightness.supportMapLightingGamma = VMExt_GVCommandInt( "ui_support_r_ext_mapLightingGamma", 0 ) ? qtrue : qfalse;
	s_brightness.supportOverBrightFactor = VMExt_GVCommandInt( "ui_support_r_ext_overBrightFactor", 0 ) ? qtrue : qfalse;
	s_brightness.supportIntensity = VMExt_GVCommandInt( "ui_support_r_intensity", 1 ) ? qtrue : qfalse;
	s_brightness.supportIntensityFractional = VMExt_GVCommandInt( "ui_support_r_intensity_fractional", 0 ) ? qtrue : qfalse;

	s_brightness.top = trap_R_RegisterShaderNoMip("menu/common/corner_ur_8_30.tga");
	s_brightness.gamma = trap_R_RegisterShaderNoMip("menu/special/gamma_test.tga");

	// Menu Data
	s_brightness.menu.wrapAround					= qtrue;
	s_brightness.menu.draw							= UI_BrightnessMenu_MenuDraw;
	s_brightness.menu.fullscreen					= qtrue;
	s_brightness.menu.descX							= MENU_DESC_X;
	s_brightness.menu.descY							= MENU_DESC_Y;
	s_brightness.menu.listX							= 230;
	s_brightness.menu.listY							= 188;
	s_brightness.menu.titleX						= MENU_TITLE_X;
	s_brightness.menu.titleY						= MENU_TITLE_Y;
	s_brightness.menu.titleI						= MNT_CONTROLSMENU_TITLE;
	s_brightness.menu.footNoteEnum					= MNT_VIDEOSETUP;

	SetupMenu_TopButtons(&s_brightness.menu,MENU_VIDEO,NULL);

	Video_SideButtons(&s_brightness.menu,ID_VIDEOBRIGHTNESS);

	x = 180;
	y = SUPPORT_BRIGHTNESS_ADDITIONAL ? 267 - 14 : 267;
	s_brightness.gamma_slider.generic.type		= MTYPE_SLIDER;
	s_brightness.gamma_slider.generic.x			= x + 162;
	s_brightness.gamma_slider.generic.y			= y;
	s_brightness.gamma_slider.generic.flags		= QMF_SMALLFONT;
	s_brightness.gamma_slider.generic.callback	= UI_BrightnessCallback;
	s_brightness.gamma_slider.minvalue			= 5;
	s_brightness.gamma_slider.maxvalue			= 30;
	s_brightness.gamma_slider.color				= CT_DKPURPLE1;
	s_brightness.gamma_slider.color2			= CT_LTPURPLE1;
	s_brightness.gamma_slider.generic.name		= PIC_MONBAR2;
	s_brightness.gamma_slider.width				= 256;
	s_brightness.gamma_slider.height			= 32;
	s_brightness.gamma_slider.focusWidth		= 145;
	s_brightness.gamma_slider.focusHeight		= 18;
	s_brightness.gamma_slider.picName			= GRAPHIC_SQUARE;
	s_brightness.gamma_slider.picX				= x;
	s_brightness.gamma_slider.picY				= y;
	s_brightness.gamma_slider.picWidth			= MENU_BUTTON_MED_WIDTH + 21;
	s_brightness.gamma_slider.picHeight			= MENU_BUTTON_MED_HEIGHT;
	s_brightness.gamma_slider.textX				= MENU_BUTTON_TEXT_X;
	s_brightness.gamma_slider.textY				= MENU_BUTTON_TEXT_Y;
	s_brightness.gamma_slider.textEnum			= MBT_BRIGHTNESS;
	s_brightness.gamma_slider.textcolor			= CT_BLACK;
	s_brightness.gamma_slider.textcolor2		= CT_WHITE;
	s_brightness.gamma_slider.thumbName			= PIC_SLIDER;
	s_brightness.gamma_slider.thumbHeight		= 32;
	s_brightness.gamma_slider.thumbWidth		= 16;
	s_brightness.gamma_slider.thumbGraphicWidth	= 9;
	s_brightness.gamma_slider.thumbColor		= CT_DKBLUE1;
	s_brightness.gamma_slider.thumbColor2		= CT_LTBLUE1;

	s_brightness.gamma_apply.generic.type			= MTYPE_ACTION;
	s_brightness.gamma_apply.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS|QMF_GRAYED;
	s_brightness.gamma_apply.generic.x				= 490;
	s_brightness.gamma_apply.generic.y				= SUPPORT_BRIGHTNESS_ADDITIONAL ? 191 - 10 : 191;
	s_brightness.gamma_apply.generic.callback		= UI_BrightnessCallback;
	s_brightness.gamma_apply.textEnum				= MBT_ACCEPT;
	s_brightness.gamma_apply.textcolor				= CT_BLACK;
	s_brightness.gamma_apply.textcolor2				= CT_WHITE;
	s_brightness.gamma_apply.textcolor3				= CT_LTGREY;
	s_brightness.gamma_apply.color					= CT_DKPURPLE1;
	s_brightness.gamma_apply.color2					= CT_LTPURPLE1;
	s_brightness.gamma_apply.color3					= CT_DKGREY;
	s_brightness.gamma_apply.textX					= 5;
	s_brightness.gamma_apply.textY					= 30;
	s_brightness.gamma_apply.width					= 82;
	s_brightness.gamma_apply.height					= 70;

	y = 324;

	if ( s_brightness.supportMapLightingGamma ) {
		y += 24;
		s_brightness.lighting_gamma_slider.generic.type			= MTYPE_SLIDER;
		s_brightness.lighting_gamma_slider.generic.x			= x + 162;
		s_brightness.lighting_gamma_slider.generic.y			= y;
		s_brightness.lighting_gamma_slider.generic.flags		= QMF_SMALLFONT;
		s_brightness.lighting_gamma_slider.generic.callback		= UI_BrightnessCallback;
		s_brightness.lighting_gamma_slider.minvalue				= 0;
		s_brightness.lighting_gamma_slider.maxvalue				= 20;
		s_brightness.lighting_gamma_slider.color				= CT_DKPURPLE1;
		s_brightness.lighting_gamma_slider.color2				= CT_LTPURPLE1;
		s_brightness.lighting_gamma_slider.generic.name			= PIC_MONBAR2;
		s_brightness.lighting_gamma_slider.width				= 256;
		s_brightness.lighting_gamma_slider.height				= 32;
		s_brightness.lighting_gamma_slider.focusWidth			= 145;
		s_brightness.lighting_gamma_slider.focusHeight			= 18;
		s_brightness.lighting_gamma_slider.picName				= GRAPHIC_SQUARE;
		s_brightness.lighting_gamma_slider.picX					= x;
		s_brightness.lighting_gamma_slider.picY					= y;
		s_brightness.lighting_gamma_slider.picWidth				= MENU_BUTTON_MED_WIDTH + 21;
		s_brightness.lighting_gamma_slider.picHeight			= MENU_BUTTON_MED_HEIGHT;
		s_brightness.lighting_gamma_slider.textX				= MENU_BUTTON_TEXT_X;
		s_brightness.lighting_gamma_slider.textY				= MENU_BUTTON_TEXT_Y;
		s_brightness.lighting_gamma_slider.textEnum				= MBT_VIDEO_LIGHTING_LEVEL;
		s_brightness.lighting_gamma_slider.textcolor			= CT_BLACK;
		s_brightness.lighting_gamma_slider.textcolor2			= CT_WHITE;
		s_brightness.lighting_gamma_slider.thumbName			= PIC_SLIDER;
		s_brightness.lighting_gamma_slider.thumbHeight			= 32;
		s_brightness.lighting_gamma_slider.thumbWidth			= 16;
		s_brightness.lighting_gamma_slider.thumbGraphicWidth	= 9;
		s_brightness.lighting_gamma_slider.thumbColor			= CT_DKBLUE1;
		s_brightness.lighting_gamma_slider.thumbColor2			= CT_LTBLUE1;

		s_brightness.lighting_gamma_slider.curvalue = UI_BrightnessEncodeMLG( trap_Cvar_VariableValue( "r_mapLightingGamma" ) );
	}

	if ( s_brightness.supportOverBrightFactor ) {
		y += 24;
		s_brightness.overbright_slider.generic.type			= MTYPE_SLIDER;
		s_brightness.overbright_slider.generic.x			= x + 162;
		s_brightness.overbright_slider.generic.y			= y;
		s_brightness.overbright_slider.generic.flags		= QMF_SMALLFONT;
		s_brightness.overbright_slider.generic.callback		= UI_BrightnessCallback;
		s_brightness.overbright_slider.minvalue				= 10;
		s_brightness.overbright_slider.maxvalue				= 20;
		s_brightness.overbright_slider.color				= CT_DKPURPLE1;
		s_brightness.overbright_slider.color2				= CT_LTPURPLE1;
		s_brightness.overbright_slider.generic.name			= PIC_MONBAR2;
		s_brightness.overbright_slider.width				= 256;
		s_brightness.overbright_slider.height				= 32;
		s_brightness.overbright_slider.focusWidth			= 145;
		s_brightness.overbright_slider.focusHeight			= 18;
		s_brightness.overbright_slider.picName				= GRAPHIC_SQUARE;
		s_brightness.overbright_slider.picX					= x;
		s_brightness.overbright_slider.picY					= y;
		s_brightness.overbright_slider.picWidth				= MENU_BUTTON_MED_WIDTH + 21;
		s_brightness.overbright_slider.picHeight			= MENU_BUTTON_MED_HEIGHT;
		s_brightness.overbright_slider.textX				= MENU_BUTTON_TEXT_X;
		s_brightness.overbright_slider.textY				= MENU_BUTTON_TEXT_Y;
		s_brightness.overbright_slider.textEnum				= MBT_VIDEO_LIGHTING_CONTRAST;
		s_brightness.overbright_slider.textcolor			= CT_BLACK;
		s_brightness.overbright_slider.textcolor2			= CT_WHITE;
		s_brightness.overbright_slider.thumbName			= PIC_SLIDER;
		s_brightness.overbright_slider.thumbHeight			= 32;
		s_brightness.overbright_slider.thumbWidth			= 16;
		s_brightness.overbright_slider.thumbGraphicWidth	= 9;
		s_brightness.overbright_slider.thumbColor			= CT_DKBLUE1;
		s_brightness.overbright_slider.thumbColor2			= CT_LTBLUE1;

		{
			float obf = trap_Cvar_VariableValue( "r_overBrightFactor" );
			if ( obf < 1.0f ) {
				obf = 1.0f;
			}
			if ( obf > 2.0f ) {
				obf = 2.0f;
			}
			s_brightness.overbright_slider.curvalue = obf * 10.0f;
		}
	}

	if ( s_brightness.supportIntensity ) {
		y += 24;
		s_brightness.intensity_slider.generic.type			= MTYPE_SLIDER;
		s_brightness.intensity_slider.generic.x				= x + 162;
		s_brightness.intensity_slider.generic.y				= y;
		s_brightness.intensity_slider.generic.flags			= QMF_SMALLFONT;
		s_brightness.intensity_slider.generic.callback		= UI_BrightnessCallback;
		s_brightness.intensity_slider.minvalue				= 0;
		s_brightness.intensity_slider.maxvalue				= 16;
		s_brightness.intensity_slider.color					= CT_DKPURPLE1;
		s_brightness.intensity_slider.color2				= CT_LTPURPLE1;
		s_brightness.intensity_slider.generic.name			= PIC_MONBAR2;
		s_brightness.intensity_slider.width					= 256;
		s_brightness.intensity_slider.height				= 32;
		s_brightness.intensity_slider.focusWidth			= 145;
		s_brightness.intensity_slider.focusHeight			= 18;
		s_brightness.intensity_slider.picName				= GRAPHIC_SQUARE;
		s_brightness.intensity_slider.picX					= x;
		s_brightness.intensity_slider.picY					= y;
		s_brightness.intensity_slider.picWidth				= MENU_BUTTON_MED_WIDTH + 21;
		s_brightness.intensity_slider.picHeight				= MENU_BUTTON_MED_HEIGHT;
		s_brightness.intensity_slider.textX					= MENU_BUTTON_TEXT_X;
		s_brightness.intensity_slider.textY					= MENU_BUTTON_TEXT_Y;
		s_brightness.intensity_slider.textEnum				= MBT_VIDEO_INTENSITY;
		s_brightness.intensity_slider.textcolor				= CT_BLACK;
		s_brightness.intensity_slider.textcolor2			= CT_WHITE;
		s_brightness.intensity_slider.thumbName				= PIC_SLIDER;
		s_brightness.intensity_slider.thumbHeight			= 32;
		s_brightness.intensity_slider.thumbWidth			= 16;
		s_brightness.intensity_slider.thumbGraphicWidth		= 9;
		s_brightness.intensity_slider.thumbColor			= CT_DKBLUE1;
		s_brightness.intensity_slider.thumbColor2			= CT_LTBLUE1;

		{
			float intensity = trap_Cvar_VariableValue( "r_intensity" );
			if ( intensity < 1.0f ) {
				intensity = 1.0f;
			}
			if ( intensity > 1.4f ) {
				intensity = 1.4f;
			}
			s_brightness.intensity_slider.curvalue = ( intensity - 1.0f ) * 40.0f;
		}
	}

	s_brightness.template_plain.generic.type			= MTYPE_BITMAP;
	s_brightness.template_plain.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_brightness.template_plain.generic.x				= x - 8;
	s_brightness.template_plain.generic.y				= 324;
	s_brightness.template_plain.generic.name			= GRAPHIC_SQUARE;
	s_brightness.template_plain.generic.callback		= UI_BrightnessCallback;
	s_brightness.template_plain.textEnum				= MBT_VIDEO_LIGHTING_PLAIN;
	s_brightness.template_plain.textcolor				= CT_BLACK;
	s_brightness.template_plain.textcolor2				= CT_WHITE;
	s_brightness.template_plain.color					= CT_DKORANGE;
	s_brightness.template_plain.color2					= CT_LTORANGE;
	s_brightness.template_plain.textX					= 2;
	s_brightness.template_plain.textY					= 2;
	s_brightness.template_plain.width					= 60;
	s_brightness.template_plain.height					= 18;

	s_brightness.template_full_contrast.generic.type		= MTYPE_BITMAP;
	s_brightness.template_full_contrast.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_brightness.template_full_contrast.generic.x			= s_brightness.template_plain.generic.x + s_brightness.template_plain.width + 5;
	s_brightness.template_full_contrast.generic.y			= s_brightness.template_plain.generic.y;
	s_brightness.template_full_contrast.generic.name		= GRAPHIC_SQUARE;
	s_brightness.template_full_contrast.generic.callback	= UI_BrightnessCallback;
	s_brightness.template_full_contrast.textEnum			= MBT_VIDEO_LIGHTING_FULL_CONTRAST;
	s_brightness.template_full_contrast.textcolor			= CT_BLACK;
	s_brightness.template_full_contrast.textcolor2			= CT_WHITE;
	s_brightness.template_full_contrast.color				= CT_DKORANGE;
	s_brightness.template_full_contrast.color2				= CT_LTORANGE;
	s_brightness.template_full_contrast.textX				= 2;
	s_brightness.template_full_contrast.textY				= 2;
	s_brightness.template_full_contrast.width				= 108;
	s_brightness.template_full_contrast.height				= 18;

	s_brightness.template_extra_bright.generic.type			= MTYPE_BITMAP;
	s_brightness.template_extra_bright.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_brightness.template_extra_bright.generic.x			= s_brightness.template_full_contrast.generic.x + s_brightness.template_full_contrast.width + 5;
	s_brightness.template_extra_bright.generic.y			= s_brightness.template_full_contrast.generic.y;
	s_brightness.template_extra_bright.generic.name			= GRAPHIC_SQUARE;
	s_brightness.template_extra_bright.generic.callback		= UI_BrightnessCallback;
	s_brightness.template_extra_bright.textEnum				= MBT_VIDEO_LIGHTING_EXTRA_BRIGHT;
	s_brightness.template_extra_bright.textcolor			= CT_BLACK;
	s_brightness.template_extra_bright.textcolor2			= CT_WHITE;
	s_brightness.template_extra_bright.color				= CT_DKORANGE;
	s_brightness.template_extra_bright.color2				= CT_LTORANGE;
	s_brightness.template_extra_bright.textX				= 2;
	s_brightness.template_extra_bright.textY				= 2;
	s_brightness.template_extra_bright.width				= 105;
	s_brightness.template_extra_bright.height				= 18;

	s_brightness.template_full_bright.generic.type			= MTYPE_BITMAP;
	s_brightness.template_full_bright.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_brightness.template_full_bright.generic.x				= s_brightness.template_extra_bright.generic.x + s_brightness.template_extra_bright.width + 5;
	s_brightness.template_full_bright.generic.y				= s_brightness.template_extra_bright.generic.y;
	s_brightness.template_full_bright.generic.name			= GRAPHIC_SQUARE;
	s_brightness.template_full_bright.generic.callback		= UI_BrightnessCallback;
	s_brightness.template_full_bright.textEnum				= MBT_VIDEO_LIGHTING_FULL_BRIGHT;
	s_brightness.template_full_bright.textcolor				= CT_BLACK;
	s_brightness.template_full_bright.textcolor2			= CT_WHITE;
	s_brightness.template_full_bright.color					= CT_DKORANGE;
	s_brightness.template_full_bright.color2				= CT_LTORANGE;
	s_brightness.template_full_bright.textX					= 2;
	s_brightness.template_full_bright.textY					= 2;
	s_brightness.template_full_bright.width					= 115;
	s_brightness.template_full_bright.height				= 18;

	s_brightness.additional_apply.generic.type			= MTYPE_ACTION;
	s_brightness.additional_apply.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS|QMF_GRAYED;
	s_brightness.additional_apply.generic.x				= 503;
	s_brightness.additional_apply.generic.y				= 350;
	s_brightness.additional_apply.generic.callback		= UI_BrightnessCallback;
	s_brightness.additional_apply.textEnum				= MBT_VIDEO_LIGHTING_APPLY;
	s_brightness.additional_apply.textcolor				= CT_BLACK;
	s_brightness.additional_apply.textcolor2			= CT_WHITE;
	s_brightness.additional_apply.textcolor3			= CT_LTGREY;
	s_brightness.additional_apply.color					= CT_DKPURPLE1;
	s_brightness.additional_apply.color2				= CT_LTPURPLE1;
	s_brightness.additional_apply.color3				= CT_DKGREY;
	s_brightness.additional_apply.textX					= 5;
	s_brightness.additional_apply.textY					= 5;
	s_brightness.additional_apply.width					= 68;
	s_brightness.additional_apply.height				= 60;

	Menu_AddItem( &s_brightness.menu, ( void * )&s_brightness.gamma_slider);
	if (!uis.glconfig.deviceSupportsGamma)
	{
		Menu_AddItem( &s_brightness.menu, ( void * )&s_brightness.gamma_apply);
	}
	s_brightness.gamma_slider.curvalue = trap_Cvar_VariableValue( "r_gamma" ) * 10.0f;

	if ( SUPPORT_BRIGHTNESS_ADDITIONAL )
	{
		s_brightness.current_loaded_values.gamma = s_brightness.gamma_slider.curvalue;
		s_brightness.current_loaded_values.lighting_gamma = s_brightness.lighting_gamma_slider.curvalue;
		s_brightness.current_loaded_values.overbright_factor = s_brightness.overbright_slider.curvalue;
		s_brightness.current_loaded_values.intensity = s_brightness.intensity_slider.curvalue;

		if ( s_brightness.supportMapLightingGamma ) {
			Menu_AddItem( &s_brightness.menu, ( void * )&s_brightness.lighting_gamma_slider);
		}
		if ( s_brightness.supportOverBrightFactor ) {
			Menu_AddItem( &s_brightness.menu, ( void * )&s_brightness.overbright_slider);
		}
		if ( s_brightness.supportIntensity ) {
			Menu_AddItem( &s_brightness.menu, ( void * )&s_brightness.intensity_slider);
		}
		Menu_AddItem( &s_brightness.menu, ( void * )&s_brightness.template_plain);
		Menu_AddItem( &s_brightness.menu, ( void * )&s_brightness.template_full_contrast);
		Menu_AddItem( &s_brightness.menu, ( void * )&s_brightness.template_extra_bright);
		Menu_AddItem( &s_brightness.menu, ( void * )&s_brightness.template_full_bright);
		Menu_AddItem( &s_brightness.menu, ( void * )&s_brightness.additional_apply);

		UI_BrightnessMenu_CheckButtonsEnabled();
	}

	UI_PushMenu( &s_brightness.menu );
}

/*
=======================================================================

DRIVER INFORMATION MENU

=======================================================================
*/



#define ID_DRIVERINFOBACK	100

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	back;
	menubitmap_s	framel;
	menubitmap_s	framer;
	char			stringbuff[2*MAX_STRING_CHARS];
	char*			strings[64];
	int				numstrings;
} driverinfo_t;

static driverinfo_t	s_driverinfo;


/*
=================
DriverInfo_MenuDraw
=================
*/
static void DriverInfo_MenuDraw( void )
{
	int	i;
	int	y;

	Menu_Draw( &s_driverinfo.menu );

	UI_DrawString( 320, 80, "VENDOR", UI_CENTER|UI_SMALLFONT, color_red );
	UI_DrawString( 320, 152, "PIXELFORMAT", UI_CENTER|UI_SMALLFONT, color_red );
	UI_DrawString( 320, 192, "EXTENSIONS", UI_CENTER|UI_SMALLFONT, color_red );

	UI_DrawString( 320, 80+16, uis.glconfig.vendor_string, UI_CENTER|UI_SMALLFONT, text_color_normal );
	UI_DrawString( 320, 96+16, uis.glconfig.version_string, UI_CENTER|UI_SMALLFONT, text_color_normal );
	UI_DrawString( 320, 112+16, uis.glconfig.renderer_string, UI_CENTER|UI_SMALLFONT, text_color_normal );
	UI_DrawString( 320, 152+16, va ("color(%d-bits) Z(%d-bits) stencil(%d-bits)", uis.glconfig.colorBits, uis.glconfig.depthBits, uis.glconfig.stencilBits), UI_CENTER|UI_SMALLFONT, text_color_normal );

	// double column
	y = 192+16;
	for (i=0; i<s_driverinfo.numstrings/2; i++) {
		UI_DrawString( 320-4, y, s_driverinfo.strings[i*2], UI_RIGHT|UI_SMALLFONT, text_color_normal );
		UI_DrawString( 320+4, y, s_driverinfo.strings[i*2+1], UI_LEFT|UI_SMALLFONT, text_color_normal );
		y += SMALLCHAR_HEIGHT;
	}

	if (s_driverinfo.numstrings & 1)
		UI_DrawString( 320, y, s_driverinfo.strings[s_driverinfo.numstrings-1], UI_CENTER|UI_SMALLFONT, text_color_normal );
}

/*
=================
DriverInfo_Cache
=================
*/
void DriverInfo_Cache( void )
{

}

/*
=================
UI_DriverInfo_Menu
=================
*/
static void UI_DriverInfo_Menu( void )
{
	char*	eptr;
	int		i;
	int		len;

	// zero set all our globals
	memset( &s_driverinfo, 0 ,sizeof(driverinfo_t) );

	DriverInfo_Cache();

	s_driverinfo.menu.fullscreen = qtrue;
	s_driverinfo.menu.draw       = DriverInfo_MenuDraw;
/*
	s_driverinfo.banner.generic.type  = MTYPE_BTEXT;
	s_driverinfo.banner.generic.x	  = 320;
	s_driverinfo.banner.generic.y	  = 16;
	s_driverinfo.banner.string		  = "DRIVER INFO";
	s_driverinfo.banner.color	      = color_white;
	s_driverinfo.banner.style	      = UI_CENTER;

	s_driverinfo.framel.generic.type  = MTYPE_BITMAP;
	s_driverinfo.framel.generic.name  = DRIVERINFO_FRAMEL;
	s_driverinfo.framel.generic.flags = QMF_INACTIVE;
	s_driverinfo.framel.generic.x	  = 0;
	s_driverinfo.framel.generic.y	  = 78;
	s_driverinfo.framel.width  	      = 256;
	s_driverinfo.framel.height  	  = 329;

	s_driverinfo.framer.generic.type  = MTYPE_BITMAP;
	s_driverinfo.framer.generic.name  = DRIVERINFO_FRAMER;
	s_driverinfo.framer.generic.flags = QMF_INACTIVE;
	s_driverinfo.framer.generic.x	  = 376;
	s_driverinfo.framer.generic.y	  = 76;
	s_driverinfo.framer.width  	      = 256;
	s_driverinfo.framer.height  	  = 334;

	s_driverinfo.back.generic.type	   = MTYPE_BITMAP;
	s_driverinfo.back.generic.name     = DRIVERINFO_BACK0;
	s_driverinfo.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_driverinfo.back.generic.callback = DriverInfo_Event;
	s_driverinfo.back.generic.id	   = ID_DRIVERINFOBACK;
	s_driverinfo.back.generic.x		   = 0;
	s_driverinfo.back.generic.y		   = 480-64;
	s_driverinfo.back.width  		   = 128;
	s_driverinfo.back.height  		   = 64;
	s_driverinfo.back.focuspic         = DRIVERINFO_BACK1;
*/
	strcpy( s_driverinfo.stringbuff, uis.glconfig.extensions_string );

	// build null terminated extension strings
	eptr = s_driverinfo.stringbuff;
	while ( s_driverinfo.numstrings<40 && *eptr )
	{
		while ( *eptr && *eptr == ' ' )
			*eptr++ = '\0';

		// track start of valid string
		if (*eptr && *eptr != ' ')
			s_driverinfo.strings[s_driverinfo.numstrings++] = eptr;

		while ( *eptr && *eptr != ' ' )
			eptr++;
	}

	// safety length strings for display
	for (i=0; i<s_driverinfo.numstrings; i++) {
		len = strlen(s_driverinfo.strings[i]);
		if (len > 32) {
			s_driverinfo.strings[i][len-1] = '>';
			s_driverinfo.strings[i][len]   = '\0';
		}
	}

	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.banner );
	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.framel );
	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.framer );
	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.back );

	UI_PushMenu( &s_driverinfo.menu );
}

/*
=======================================================================

GRAPHICS OPTIONS MENU

=======================================================================
*/


static const char *s_drivers[] =
{
	OPENGL_DRIVER_NAME,
	_3DFX_DRIVER_NAME,
	0
};

#define ID_BACK2		101
#define ID_FULLSCREEN	102
#define ID_LIST			103
#define ID_MODE			104
#define ID_DRIVERINFO	105
#define ID_GRAPHICS		106
#define ID_DISPLAY		107
//#define ID_SOUND		108
#define ID_NETWORK		109

typedef struct {
	menuframework_s	menu;

	menulist_s		list;
	menulist_s		mode;
	menulist_s		driver;
	menulist_s		renderer;
	menulist_s		tq;
	menulist_s  	fs;
	menulist_s  	lighting;
	menulist_s  	allow_extensions;
	menulist_s  	texturebits;
	menulist_s  	colordepth;
	menulist_s  	geometry;
	menulist_s  	filter;
	menutext_s		driverinfo;
	menulist_s		simpleshaders;
	menulist_s		compresstextures;
	menulist_s		anisotropic;
	menulist_s		multisample;
	menulist_s		vsync;

	menuaction_s	apply;

	menubitmap_s	back;
} graphicsoptions_t;

typedef struct
{
	int mode;
	qboolean fullscreen;
	int tq;
	int lighting;
	int colordepth;
	int texturebits;
	int geometry;
	int filter;
	int driver;
	qboolean extensions;
	int simpleshaders;
	int compresstextures;
	int anisotropic;
	int multisample;
	int renderer;
	int vsync;
} InitialVideoOptions_s;

static InitialVideoOptions_s	s_ivo;
static graphicsoptions_t		s_graphicsoptions;

static InitialVideoOptions_s s_ivo_templates[NUM_VIDEO_TEMPLATES] =
{
	{
		2, qtrue, 3, 0, 2, 2, 2, 1, 0, qtrue, 0, 0,	// JDC: this was tq 3
	},
	{
		1, qtrue, 2, 0, 0, 0, 2, 0, 0, qtrue, 0, 0,
	},
	{
		0, qtrue, 1, 0, 1, 0, 0, 0, 0, qtrue, 0, 0,
	},
	{
		0, qtrue, 1, 1, 1, 0, 0, 0, 0, qtrue, 1, 0,
	},
	//{
	//	1, qtrue, 1, 0, 0, 0, 1, 0, 0, qtrue, 0, 0,
	//}
};

static InitialVideoOptions_s s_ivo_modern_templates[NUM_VIDEO_TEMPLATES] =
{
	{
		0, qtrue, 1, 0, 1, 0, 1, 0, 0, qtrue, 0, 0, 0, 0,
	},
	{
		1, qtrue, 2, 0, 0, 0, 2, 1, 0, qtrue, 0, 0, 2, 0,
	},
	{
		2, qtrue, 3, 0, 2, 2, 2, 1, 0, qtrue, 0, 0, 4, 0,
	},
	{
		2, qtrue, 3, 0, 2, 2, 2, 1, 0, qtrue, 0, 0,	4, 2,
	},
};

// If max anisotropy is unsupported, convert template anisotropy levels to 1 or 0.
#define ANISOTROPIC_TEMPLATE_VALUE( value ) ( videoEngineConfig.supportMaxAnisotropy ? value : ( value ? 1 : 0 ) )

/*
=================
GraphicsOptions_GetInitialVideo
=================
*/
static void GraphicsOptions_GetInitialVideo( void )
{
	s_ivo.colordepth  = s_graphicsoptions.colordepth.curvalue;
	s_ivo.driver      = s_graphicsoptions.driver.curvalue;
	s_ivo.mode        = s_graphicsoptions.mode.curvalue;
	s_ivo.fullscreen  = s_graphicsoptions.fs.curvalue;
	s_ivo.extensions  = s_graphicsoptions.allow_extensions.curvalue;
	s_ivo.tq          = s_graphicsoptions.tq.curvalue;
//	s_ivo.lighting    = s_graphicsoptions.lighting.curvalue;
	s_ivo.geometry    = s_graphicsoptions.geometry.curvalue;
	s_ivo.filter      = s_graphicsoptions.filter.curvalue;
	s_ivo.texturebits = s_graphicsoptions.texturebits.curvalue;
	s_ivo.simpleshaders = s_graphicsoptions.simpleshaders.curvalue;
	s_ivo.compresstextures = s_graphicsoptions.compresstextures.curvalue;
	s_ivo.renderer    = s_graphicsoptions.renderer.curvalue;
	s_ivo.anisotropic = s_graphicsoptions.anisotropic.curvalue;
	s_ivo.multisample = s_graphicsoptions.multisample.curvalue;
	s_ivo.vsync = s_graphicsoptions.vsync.curvalue;
}

/*
=================
GraphicsOptions_CompareTemplate

Returns qtrue if the config matches the current button configuration.
=================
*/
static qboolean GraphicsOptions_CompareTemplate( InitialVideoOptions_s template ) {
	// Apply some forced values from GraphicsOptions_UpdateMenuItems to template ahead of comparison
	// Otherwise in some cases template will never match and template button will get stuck
	if ( videoEngineConfig.supportGlDriver && s_graphicsoptions.driver.curvalue == 1 ) {
		template.fullscreen = 1;
	}
	if ( s_graphicsoptions.fs.curvalue == 0 || ( videoEngineConfig.supportGlDriver && s_graphicsoptions.driver.curvalue == 1 ) ) {
		template.colordepth = 0;
	}
	if ( videoEngineConfig.supportAllowExtensions && s_graphicsoptions.allow_extensions.curvalue == 0 )
	{
		if ( template.texturebits == 0 )
		{
			template.texturebits = 1;
		}
	}

	if ( !videoEngineConfig.modern_templates && template.mode != s_graphicsoptions.mode.curvalue )
		return qfalse;
	if ( !videoEngineConfig.modern_templates && template.fullscreen != s_graphicsoptions.fs.curvalue )
		return qfalse;
	if ( template.tq != s_graphicsoptions.tq.curvalue )
		return qfalse;
	if ( videoEngineConfig.supportColorDepth && template.colordepth != s_graphicsoptions.colordepth.curvalue )
		return qfalse;
	if ( videoEngineConfig.supportTextureBits && template.texturebits != s_graphicsoptions.texturebits.curvalue )
		return qfalse;
	if ( template.geometry != s_graphicsoptions.geometry.curvalue )
		return qfalse;
	if ( template.filter != s_graphicsoptions.filter.curvalue )
		return qfalse;
	if ( videoEngineConfig.supportSimpleShaders && template.simpleshaders != s_graphicsoptions.simpleshaders.curvalue )
		return qfalse;
	if ( template.compresstextures != s_graphicsoptions.compresstextures.curvalue )
		return qfalse;
	if ( videoEngineConfig.anisotropicFilteringVideoMenu && videoEngineConfig.modern_templates &&
			ANISOTROPIC_TEMPLATE_VALUE( template.anisotropic ) != s_graphicsoptions.anisotropic.curvalue )
		return qfalse;
	if ( videoEngineConfig.supportMultisample && videoEngineConfig.modern_templates &&
			template.multisample != s_graphicsoptions.multisample.curvalue )
		return qfalse;
	return qtrue;
}

/*
=================
GraphicsOptions_ApplyTemplate

Copies values from template to current button configuration.
=================
*/
static void GraphicsOptions_ApplyTemplate( const InitialVideoOptions_s *template ) {
	if ( !videoEngineConfig.modern_templates )
		s_graphicsoptions.mode.curvalue			= template->mode;
	if ( !videoEngineConfig.modern_templates )
		s_graphicsoptions.fs.curvalue			= template->fullscreen;
	s_graphicsoptions.tq.curvalue				= template->tq;
	s_graphicsoptions.colordepth.curvalue		= template->colordepth;
	s_graphicsoptions.texturebits.curvalue		= template->texturebits;
	s_graphicsoptions.geometry.curvalue			= template->geometry;
	s_graphicsoptions.filter.curvalue			= template->filter;
	s_graphicsoptions.simpleshaders.curvalue	= template->simpleshaders;
	s_graphicsoptions.compresstextures.curvalue	= template->compresstextures;
	if ( videoEngineConfig.modern_templates )
		s_graphicsoptions.anisotropic.curvalue	= ANISOTROPIC_TEMPLATE_VALUE( template->anisotropic );
	if ( videoEngineConfig.modern_templates )
		s_graphicsoptions.multisample.curvalue	= template->multisample;
}

/*
=================
GraphicsOptions_CheckConfig
=================
*/
static void GraphicsOptions_CheckConfig( void )
{
	int i;
	const InitialVideoOptions_s *template = videoEngineConfig.modern_templates ? s_ivo_modern_templates : s_ivo_templates;

	for ( i = NUM_VIDEO_TEMPLATES - 1; i >= 0; i-- )
	{
		if ( GraphicsOptions_CompareTemplate( template[i] ) ) {
			s_graphicsoptions.list.curvalue = i;
			return;
		}
	}
	s_graphicsoptions.list.curvalue = NUM_VIDEO_TEMPLATES;
}

/*
=================
GraphicsOptions_UpdateMenuItems
=================
*/
static void GraphicsOptions_UpdateMenuItems( void )
{
	if ( s_graphicsoptions.mode.curvalue == NUM_RESOLUTIONS && s_ivo.mode != NUM_RESOLUTIONS ) {
		// skip custom resolution if initial resolution wasn't custom
		s_graphicsoptions.mode.curvalue = 0;
	}

	if ( videoEngineConfig.supportGlDriver && s_graphicsoptions.driver.curvalue == 1 )
	{
		s_graphicsoptions.fs.curvalue = 1;
		s_graphicsoptions.fs.generic.flags |= QMF_GRAYED;
		s_graphicsoptions.colordepth.curvalue = 1;
	}
	else
	{
		s_graphicsoptions.fs.generic.flags &= ~QMF_GRAYED;
	}

	if ( s_graphicsoptions.fs.curvalue == 0 || ( videoEngineConfig.supportGlDriver && s_graphicsoptions.driver.curvalue == 1 ) )
	{
		s_graphicsoptions.colordepth.curvalue = 0;
		s_graphicsoptions.colordepth.generic.flags |= QMF_GRAYED;
	}
	else
	{
		s_graphicsoptions.colordepth.generic.flags &= ~QMF_GRAYED;
	}

	if ( videoEngineConfig.supportAllowExtensions && s_graphicsoptions.allow_extensions.curvalue == 0 )
	{
		if ( s_graphicsoptions.texturebits.curvalue == 0 )
		{
			s_graphicsoptions.texturebits.curvalue = 1;
		}
	}

	s_graphicsoptions.apply.generic.flags	|= QMF_GRAYED;
	s_graphicsoptions.apply.generic.flags&= ~ QMF_BLINK;

	if ( s_ivo.mode != s_graphicsoptions.mode.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	if ( s_ivo.fullscreen != s_graphicsoptions.fs.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	if ( videoEngineConfig.supportAllowExtensions && s_ivo.extensions != s_graphicsoptions.allow_extensions.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}
	if ( s_ivo.tq != s_graphicsoptions.tq.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

/*	if ( s_ivo.lighting != s_graphicsoptions.lighting.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}
*/
	if ( videoEngineConfig.supportColorDepth && s_ivo.colordepth != s_graphicsoptions.colordepth.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	if ( videoEngineConfig.supportGlDriver && s_ivo.driver != s_graphicsoptions.driver.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}
	if ( videoEngineConfig.supportRendererSelect && s_ivo.renderer != s_graphicsoptions.renderer.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}
	if ( videoEngineConfig.supportTextureBits && s_ivo.texturebits != s_graphicsoptions.texturebits.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	if ( videoEngineConfig.supportSimpleShaders && s_ivo.simpleshaders != s_graphicsoptions.simpleshaders.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	if ( s_ivo.compresstextures != s_graphicsoptions.compresstextures.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	if ( s_ivo.geometry != s_graphicsoptions.geometry.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	if ( s_ivo.filter != s_graphicsoptions.filter.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	if ( videoEngineConfig.anisotropicFilteringVideoMenu && s_ivo.anisotropic != s_graphicsoptions.anisotropic.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	if ( videoEngineConfig.supportMultisample && s_ivo.multisample != s_graphicsoptions.multisample.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	if ( videoEngineConfig.supportVSync && s_ivo.vsync != s_graphicsoptions.vsync.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~QMF_GRAYED;
		s_graphicsoptions.apply.generic.flags |= QMF_BLINK;
	}

	GraphicsOptions_CheckConfig();
}

/*
=================
ApplyChanges - Apply the changes from the video data screen
=================
*/
static void ApplyChanges2( void *unused, int notification )
{
	if (notification != QM_ACTIVATED)
		return;

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}
/*
=================
GraphicsOptions_ApplyChanges
=================
*/
static void GraphicsOptions_ApplyChanges( void *unused, int notification )
{
	if (notification != QM_ACTIVATED)
		return;

	if ( videoEngineConfig.supportTextureBits ) {
		switch ( s_graphicsoptions.texturebits.curvalue  )
		{
		case 0:
			trap_Cvar_SetValue( "r_texturebits", 0 );
			break;
		case 1:
			trap_Cvar_SetValue( "r_texturebits", 16 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_texturebits", 32 );
			break;
		}
	}
	trap_Cvar_SetValue( "r_picmip", 3 - s_graphicsoptions.tq.curvalue );
	if ( videoEngineConfig.supportAllowExtensions ) {
		trap_Cvar_SetValue( "r_allowExtensions", s_graphicsoptions.allow_extensions.curvalue );
	}
	if ( s_graphicsoptions.mode.curvalue >= 0 && s_graphicsoptions.mode.curvalue < NUM_RESOLUTIONS ) {
		trap_Cvar_SetValue( "r_mode", (s_graphicsoptions.mode.curvalue + 2) );
	}
	trap_Cvar_SetValue( "r_fullscreen", s_graphicsoptions.fs.curvalue );
	if ( videoEngineConfig.supportGlDriver ) {
		trap_Cvar_Set( "r_glDriver", ( char * ) s_drivers[s_graphicsoptions.driver.curvalue] );
	}
	if ( videoEngineConfig.supportRendererSelect ) {
		if ( s_graphicsoptions.renderer.curvalue == 0 ) {
			trap_Cvar_Set( "cl_renderer", "opengl1" );
		} else if ( s_graphicsoptions.renderer.curvalue == 1 ) {
			trap_Cvar_Set( "cl_renderer", "opengl2" );
		}
	}

	if ( videoEngineConfig.supportSimpleShaders ) {
		trap_Cvar_SetValue( "r_lowEndVideo", s_graphicsoptions.simpleshaders.curvalue );
	}

	trap_Cvar_SetValue( "r_ext_compress_textures", s_graphicsoptions.compresstextures.curvalue );

	if ( videoEngineConfig.supportColorDepth ) {
		switch ( s_graphicsoptions.colordepth.curvalue )
		{
		case 0:
			trap_Cvar_SetValue( "r_colorbits", 0 );
			trap_Cvar_SetValue( "r_depthbits", 0 );
			trap_Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 1:
			trap_Cvar_SetValue( "r_colorbits", 16 );
			trap_Cvar_SetValue( "r_depthbits", 16 );
			trap_Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_colorbits", 32 );
			trap_Cvar_SetValue( "r_depthbits", 24 );
			break;
		}
	}
//	trap_Cvar_SetValue( "r_vertexLight", s_graphicsoptions.lighting.curvalue );

	if ( s_graphicsoptions.geometry.curvalue == 2 )
	{
		trap_Cvar_SetValue( "r_lodBias", 0 );
		trap_Cvar_SetValue( "r_subdivisions", 4 );
	}
	else if ( s_graphicsoptions.geometry.curvalue == 1 )
	{
		trap_Cvar_SetValue( "r_lodBias", 1 );
		trap_Cvar_SetValue( "r_subdivisions", 12 );
	}
	else
	{
		trap_Cvar_SetValue( "r_lodBias", 1 );
		trap_Cvar_SetValue( "r_subdivisions", 20 );
	}

	if ( s_graphicsoptions.filter.curvalue )
	{
		trap_Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR" );
	}
	else
	{
		trap_Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
	}

	if ( videoEngineConfig.anisotropicFilteringVideoMenu ) {
		UI_SetAnisotropyLevel( s_graphicsoptions.anisotropic.curvalue );
	}

	if ( videoEngineConfig.supportMultisample ) {
		int value = s_graphicsoptions.multisample.curvalue > 0 ? 1 << s_graphicsoptions.multisample.curvalue : 0;
		VMExt_GVCommandInt( va( "cmd_set_multisample %i", value ), 0 );
	}

	if ( videoEngineConfig.supportVSync ) {
		trap_Cvar_Set( "r_swapInterval", s_graphicsoptions.vsync.curvalue ? "1" : "0" );
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

/*
=================
GraphicsOptions_Event
=================
*/
static void GraphicsOptions_Event( void* ptr, int event )
{
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_MODE:
		// clamp 3dfx video modes
		if ( s_graphicsoptions.driver.curvalue == 1 )
		{
			if ( s_graphicsoptions.mode.curvalue < 2 )
				s_graphicsoptions.mode.curvalue = 2;
			else if ( s_graphicsoptions.mode.curvalue > 6 )
				s_graphicsoptions.mode.curvalue = 6;
		}
		break;

	case ID_LIST:
		{
			const InitialVideoOptions_s *template = videoEngineConfig.modern_templates ? s_ivo_modern_templates : s_ivo_templates;
			if ( s_graphicsoptions.list.curvalue >= NUM_VIDEO_TEMPLATES ) {
				// Trying to set "custom" - revert to original settings
				GraphicsOptions_ApplyTemplate( &s_ivo );

				// If original settings match one of the other templates, go to the first template instead
				GraphicsOptions_CheckConfig();
				if ( s_graphicsoptions.list.curvalue < NUM_VIDEO_TEMPLATES ) {
					s_graphicsoptions.list.curvalue = 0;
					GraphicsOptions_ApplyTemplate( &template[0] );
				}
			} else {
				GraphicsOptions_ApplyTemplate( &template[s_graphicsoptions.list.curvalue] );
			}
			break;
		}

	case ID_DRIVERINFO:
		UI_DriverInfo_Menu();
		break;

	case ID_BACK2:
		UI_PopMenu();
		break;

	case ID_GRAPHICS:
		break;

	case ID_DISPLAY:
		UI_PopMenu();
		UI_VideoDataMenu();		// Move to the Video Menu
//		UI_DisplayOptionsMenu();
		break;

//	case ID_SOUND:
//		UI_PopMenu();
//		UI_SoundOptionsMenu();
//		break;

	case ID_NETWORK:
		UI_PopMenu();
		UI_NetworkOptionsMenu();
		break;
	}
}


/*
================
GraphicsOptions_MenuDraw
================
*/
void GraphicsOptions_MenuDraw (void)
{
//APSFIX - rework this
	GraphicsOptions_UpdateMenuItems();

	Menu_Draw( &s_graphicsoptions.menu );
}

/*
=================
GraphicsOptions_SetMenuItems
=================
*/
static void GraphicsOptions_SetMenuItems( void )
{
	s_graphicsoptions.mode.curvalue = (trap_Cvar_VariableValue( "r_mode" ) - 2);
	if ( s_graphicsoptions.mode.curvalue < 0 || s_graphicsoptions.mode.curvalue >= NUM_RESOLUTIONS )
	{
		s_graphicsoptions.mode.curvalue = NUM_RESOLUTIONS;	// custom
	}
	s_graphicsoptions.fs.curvalue = trap_Cvar_VariableValue("r_fullscreen") ? 1 : 0;
	s_graphicsoptions.allow_extensions.curvalue = trap_Cvar_VariableValue("r_allowExtensions") ? 1 : 0;
	s_graphicsoptions.simpleshaders.curvalue = trap_Cvar_VariableValue("r_lowEndVideo") ? 1 : 0;
	s_graphicsoptions.compresstextures.curvalue = trap_Cvar_VariableValue("r_ext_compress_textures") ? 1 : 0;

	s_graphicsoptions.tq.curvalue = 3-trap_Cvar_VariableValue( "r_picmip");
	if ( s_graphicsoptions.tq.curvalue < 0 )
	{
		s_graphicsoptions.tq.curvalue = 0;
	}
	else if ( s_graphicsoptions.tq.curvalue > 3 )
	{
		s_graphicsoptions.tq.curvalue = 3;
	}

//	s_graphicsoptions.lighting.curvalue = trap_Cvar_VariableValue( "r_vertexLight" ) != 0;
	switch ( ( int ) trap_Cvar_VariableValue( "r_texturebits" ) )
	{
	default:
	case 0:
		s_graphicsoptions.texturebits.curvalue = 0;
		break;
	case 16:
		s_graphicsoptions.texturebits.curvalue = 1;
		break;
	case 32:
		s_graphicsoptions.texturebits.curvalue = 2;
		break;
	}

	if ( !Q_stricmp( UI_Cvar_VariableString( "r_textureMode" ), "GL_LINEAR_MIPMAP_NEAREST" ) )
	{
		s_graphicsoptions.filter.curvalue = 0;
	}
	else
	{
		s_graphicsoptions.filter.curvalue = 1;
	}

	if ( trap_Cvar_VariableValue( "r_lodBias" ) > 0 )
	{
		if ( trap_Cvar_VariableValue( "r_subdivisions" ) >= 20 )
		{
			s_graphicsoptions.geometry.curvalue = 0;
		}
		else
		{
			s_graphicsoptions.geometry.curvalue = 1;
		}
	}
	else
	{
		s_graphicsoptions.geometry.curvalue = 2;
	}

	switch ( ( int ) trap_Cvar_VariableValue( "r_colorbits" ) )
	{
	default:
	case 0:
		s_graphicsoptions.colordepth.curvalue = 0;
		break;
	case 16:
		s_graphicsoptions.colordepth.curvalue = 1;
		break;
	case 32:
		s_graphicsoptions.colordepth.curvalue = 2;
		break;
	}

	if ( videoEngineConfig.supportRendererSelect ) {
		char buffer[256];
		trap_Cvar_VariableStringBuffer( "cl_renderer", buffer, sizeof( buffer ) );
		if ( !Q_stricmp( buffer, "opengl1" ) ) {
			s_graphicsoptions.renderer.listnames = s_renderer_Names;
			s_graphicsoptions.renderer.curvalue = 0;
		} else if ( !Q_stricmp( buffer, "opengl2" ) ) {
			s_graphicsoptions.renderer.listnames = s_renderer_Names;
			s_graphicsoptions.renderer.curvalue = 1;
		} else {
			// set button to "custom", meaning to leave the existing value unchanged
			s_graphicsoptions.renderer.listnames = s_renderer_NamesUnknown;
			s_graphicsoptions.renderer.curvalue = 2;
		}
	}

	if ( videoEngineConfig.anisotropicFilteringVideoMenu ) {
		s_graphicsoptions.anisotropic.curvalue = UI_GetAnisotropyLevel();
	}

	if ( videoEngineConfig.supportMultisample ) {
		int level = VMExt_GVCommandInt( "cmd_get_multisample", 0 );
		if ( level >= 4 )
			s_graphicsoptions.multisample.curvalue = 2;
		else if ( level >= 2 )
			s_graphicsoptions.multisample.curvalue = 1;
		else
			s_graphicsoptions.multisample.curvalue = 0;
	}

	if ( videoEngineConfig.supportVSync ) {
		s_graphicsoptions.vsync.curvalue = trap_Cvar_VariableValue( "r_swapInterval" ) ? 1 : 0;
	}

	if ( s_graphicsoptions.fs.curvalue == 0 )
	{
		s_graphicsoptions.colordepth.curvalue = 0;
	}
	if ( s_graphicsoptions.driver.curvalue == 1 )
	{
		s_graphicsoptions.colordepth.curvalue = 1;
	}
}



void VideoSideButtonsAction( qboolean result )
{
	if ( result )	// Yes - do it
	{
		Video_MenuEvent(holdControlPtr, holdControlEvent);
	}
}

/*
=================
VideoSideButtons_MenuEvent
=================
*/
static void VideoSideButtons_MenuEvent (void* ptr, int event)
{

	if (event != QM_ACTIVATED)
		return;

	holdControlPtr = ptr;
	holdControlEvent = event;

	if (s_graphicsoptions.apply.generic.flags & QMF_BLINK)	// Video apply changes button is flashing
	{
		UI_ConfirmMenu(menu_normal_text[MNT_LOOSEVIDSETTINGS], 0, VideoSideButtonsAction);
	}
	else	// Go ahead, act normal
	{
		Video_MenuEvent (holdControlPtr, holdControlEvent);
	}
}

/*
=================
Video_MenuEvent
=================
*/
static void Video_MenuEvent (void* ptr, int event)
{
	menuframework_s*	m;

	if (event != QM_ACTIVATED)
		return;

	m = ((menucommon_s*)ptr)->parent;

	switch (((menucommon_s*)ptr)->id)
	{
		case ID_VIDEO:				// You're already in video menus, doofus
			break;

		case ID_ARROWDWN:
			VideoDriver_Lines(1);
			break;

		case ID_ARROWUP:
			VideoDriver_Lines(-1);
			break;

		case ID_VIDEODRIVERS:
			if (m != &s_videodriver.menu)	//	Not already in menu?
			{
				UI_PopMenu();			// Get rid of whatever is ontop
				UI_VideoDriverMenu();	// Move to the Controls Menu
			}
			break;

		case ID_VIDEODATA:
			if (m != &s_videodata.menu)	//	Not already in menu?
			{
				UI_PopMenu();			// Get rid of whatever is ontop
				UI_VideoDataMenu();		// Move to the Controls Menu
			}
			break;

		case ID_VIDEODATA2:
			if (m != &s_videodata2.menu)	//	Not already in menu?
			{
				UI_PopMenu();				// Get rid of whatever is ontop
				UI_VideoData2SettingsMenu();	// Move to the Controls Menu
			}
			break;

		case ID_VIDEOBRIGHTNESS:
			if (m != &s_brightness.menu)	//	Not already in menu?
			{
				UI_PopMenu();				// Get rid of whatever is ontop
				UI_BrightnessMenu_Init();	// Move to the Brightness Menu
			}
			break;

		case ID_CONTROLS:
			UI_PopMenu();			// Get rid of whatever is ontop
//			UI_SetupWeaponsMenu();	// Move to the Controls Menu
			break;

		case ID_SOUND:
			UI_PopMenu();			// Get rid of whatever is ontop
			UI_SoundMenu();			// Move to the Sound Menu
			break;

		case ID_GAMEOPTIONS:
			UI_PopMenu();			// Get rid of whatever is ontop
			UI_GameOptionsMenu();	// Move to the Game Options Menu
			break;

		case ID_CDKEY:
			UI_PopMenu();			// Get rid of whatever is ontop
			UI_CDKeyMenu();			// Move to the CD Key Menu
			break;

		case ID_MAINMENU:
			UI_PopMenu();
			break;

		case ID_INGAMEMENU :
			UI_PopMenu();
			break;

	}
}

/*
=================
M_VideoDataMenu_Graphics
=================
*/
void M_VideoDataMenu_Graphics (void)
{
	UI_MenuFrame(&s_videodata.menu);

	UI_DrawProportionalString(  74,  66, "207",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  84, "44909",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  188, "357",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  206, "250624",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  395, "456730-1",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	UI_Setup_MenuButtons();

	// Rest of Bottom1_Graphics
//	trap_R_SetColor( colorTable[CT_LTORANGE]);
//	UI_DrawHandlePic(  30, 203, 47, 69, uis.whiteShader);	// Top Left column above two buttons
//	UI_DrawHandlePic(  30, 344, 47, 45, uis.whiteShader);	// Top Left column below two buttons

	// Brackets around Video Data
	trap_R_SetColor( colorTable[CT_LTPURPLE1]);
	UI_DrawHandlePic(158,163, 16, 16, uis.graphicBracket1CornerLU);
	UI_DrawHandlePic(158,179,  8, 233, uis.whiteShader);
	UI_DrawHandlePic(158,412, 16, -16, uis.graphicBracket1CornerLU);	//LD

	UI_DrawHandlePic(174,163, 320, 8, uis.whiteShader);	// Top line

	UI_DrawHandlePic(494,163, 128, 128, s_videodata.swooshTop);			// Top swoosh

	UI_DrawHandlePic(501,188, 110, 54, uis.whiteShader);	// Top right column

	UI_DrawHandlePic(501,348, 110, 55, uis.whiteShader);	// Bottom right column

	UI_DrawHandlePic(494,406, 128, 128, s_videodata.swooshBottom);		// Bottom swoosh

	UI_DrawHandlePic(174,420, 320, 8, uis.whiteShader);	// Bottom line
}
/*
=================
VideoData_MenuDraw
=================
*/
static void VideoData_MenuDraw (void)
{
	GraphicsOptions_UpdateMenuItems();

	M_VideoDataMenu_Graphics();

	Menu_Draw( &s_videodata.menu );
}

/*
=================
UI_VideoDataMenu_Cache
=================
*/
void UI_VideoDataMenu_Cache(void)
{
	s_videodata.swooshTop = trap_R_RegisterShaderNoMip("menu/common/swoosh_top.tga");
	s_videodata.swooshBottom= trap_R_RegisterShaderNoMip("menu/common/swoosh_bottom.tga");
	s_videodata.swooshTopSmall= trap_R_RegisterShaderNoMip("menu/common/swoosh_topsmall.tga");
	s_videodata.swooshBottomSmall= trap_R_RegisterShaderNoMip("menu/common/swoosh_bottomsmall.tga");
}


/*
=================
VideoData_MenuInit
=================
*/
static void VideoData_MenuInit( void )
{
	int x,y,width,inc;

	UI_InitVideoEngineConfig();

	UI_VideoDataMenu_Cache();

	// Menu Data
	s_videodata.menu.nitems						= 0;
	s_videodata.menu.wrapAround					= qtrue;
	s_videodata.menu.draw						= VideoData_MenuDraw;
	s_videodata.menu.fullscreen					= qtrue;
	s_videodata.menu.descX						= MENU_DESC_X;
	s_videodata.menu.descY						= MENU_DESC_Y;
	s_videodata.menu.listX						= 230;
	s_videodata.menu.listY						= 188;
	s_videodata.menu.titleX						= MENU_TITLE_X;
	s_videodata.menu.titleY						= MENU_TITLE_Y;
	s_videodata.menu.titleI						= MNT_CONTROLSMENU_TITLE;
	s_videodata.menu.footNoteEnum				= MNT_VIDEOSETUP;

	x = 170;
	y = 178;
	width = 145;

	s_graphicsoptions.list.generic.type			= MTYPE_SPINCONTROL;
	s_graphicsoptions.list.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_graphicsoptions.list.generic.x			= x;
	s_graphicsoptions.list.generic.y			= y;
	s_graphicsoptions.list.generic.callback		=GraphicsOptions_Event;
	s_graphicsoptions.list.generic.id			= ID_LIST;
	s_graphicsoptions.list.textEnum				= MBT_VIDEOOPTIONS;
	s_graphicsoptions.list.textcolor			= CT_BLACK;
	s_graphicsoptions.list.textcolor2			= CT_WHITE;
	s_graphicsoptions.list.color				= CT_DKPURPLE1;
	s_graphicsoptions.list.color2				= CT_LTPURPLE1;
	s_graphicsoptions.list.textX				= 5;
	s_graphicsoptions.list.textY				= 2;
	s_graphicsoptions.list.listnames			=
			videoEngineConfig.modern_templates ? s_graphics_options_modern_Names : s_graphics_options_Names;
	s_graphicsoptions.list.width				= width;

	inc = 20;

	if ( videoEngineConfig.supportGlDriver ) {
		y += inc;
		s_graphicsoptions.driver.generic.type		= MTYPE_SPINCONTROL;
		s_graphicsoptions.driver.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
		s_graphicsoptions.driver.generic.x			= x;
		s_graphicsoptions.driver.generic.y			= y;
		s_graphicsoptions.driver.textEnum			= MBT_VIDEODRIVER;
		s_graphicsoptions.driver.textcolor			= CT_BLACK;
		s_graphicsoptions.driver.textcolor2			= CT_WHITE;
		s_graphicsoptions.driver.color				= CT_DKPURPLE1;
		s_graphicsoptions.driver.color2				= CT_LTPURPLE1;
		s_graphicsoptions.driver.textX				= 5;
		s_graphicsoptions.driver.textY				= 2;
		s_graphicsoptions.driver.listnames			= s_driver_Names;
		s_graphicsoptions.driver.curvalue			= (uis.glconfig.driverType == GLDRV_VOODOO);
		s_graphicsoptions.driver.width				= width;
	}

	if ( videoEngineConfig.supportRendererSelect ) {
		y += inc;
		s_graphicsoptions.renderer.generic.type		= MTYPE_SPINCONTROL;
		s_graphicsoptions.renderer.generic.flags	= QMF_HIGHLIGHT_IF_FOCUS;
		s_graphicsoptions.renderer.generic.x		= x;
		s_graphicsoptions.renderer.generic.y		= y;
		s_graphicsoptions.renderer.textEnum			= MBT_VIDEO_RENDERER;
		s_graphicsoptions.renderer.textcolor		= CT_BLACK;
		s_graphicsoptions.renderer.textcolor2		= CT_WHITE;
		s_graphicsoptions.renderer.color			= CT_DKPURPLE1;
		s_graphicsoptions.renderer.color2			= CT_LTPURPLE1;
		s_graphicsoptions.renderer.textX			= 5;
		s_graphicsoptions.renderer.textY			= 2;
		s_graphicsoptions.renderer.listnames		= s_renderer_Names;
		s_graphicsoptions.renderer.width			= width;
	}

	if ( videoEngineConfig.supportAllowExtensions ) {
		y += inc;
		s_graphicsoptions.allow_extensions.generic.type		= MTYPE_SPINCONTROL;
		s_graphicsoptions.allow_extensions.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
		s_graphicsoptions.allow_extensions.generic.x			= x;
		s_graphicsoptions.allow_extensions.generic.y			= y;
		s_graphicsoptions.allow_extensions.textEnum			= MBT_VIDEOGLEXTENTIONS;
		s_graphicsoptions.allow_extensions.textcolor			= CT_BLACK;
		s_graphicsoptions.allow_extensions.textcolor2		= CT_WHITE;
		s_graphicsoptions.allow_extensions.color				= CT_DKPURPLE1;
		s_graphicsoptions.allow_extensions.color2			= CT_LTPURPLE1;
		s_graphicsoptions.allow_extensions.textX				= 5;
		s_graphicsoptions.allow_extensions.textY				= 2;
		s_graphicsoptions.allow_extensions.listnames			= s_OffOnNone_Names;
		s_graphicsoptions.allow_extensions.width				= width;
	}

	y += inc;
	// references/modifies "r_mode"
	s_graphicsoptions.mode.generic.type					= MTYPE_SPINCONTROL;
	s_graphicsoptions.mode.generic.flags					= QMF_HIGHLIGHT_IF_FOCUS;
	s_graphicsoptions.mode.generic.x						= x;
	s_graphicsoptions.mode.generic.y						= y;
	s_graphicsoptions.mode.generic.callback				= GraphicsOptions_Event;
	s_graphicsoptions.mode.textEnum						=
			videoEngineConfig.windowed_r_mode ? MBT_VIDEO_WINDOW_SIZE : MBT_VIDEOMODE;
	s_graphicsoptions.mode.textcolor						= CT_BLACK;
	s_graphicsoptions.mode.textcolor2					= CT_WHITE;
	s_graphicsoptions.mode.color							= CT_DKPURPLE1;
	s_graphicsoptions.mode.color2						= CT_LTPURPLE1;
	s_graphicsoptions.mode.textX							= 5;
	s_graphicsoptions.mode.textY							= 2;
	s_graphicsoptions.mode.listnames						= s_resolutions;
	s_graphicsoptions.mode.width						= width;

	if ( videoEngineConfig.supportColorDepth ) {
		y += inc;
		s_graphicsoptions.colordepth.generic.type			= MTYPE_SPINCONTROL;
		s_graphicsoptions.colordepth.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
		s_graphicsoptions.colordepth.generic.x				= x;
		s_graphicsoptions.colordepth.generic.y				= y;
		s_graphicsoptions.colordepth.textEnum				= MBT_VIDEOCOLORDEPTH;
		s_graphicsoptions.colordepth.textcolor				= CT_BLACK;
		s_graphicsoptions.colordepth.textcolor2				= CT_WHITE;
		s_graphicsoptions.colordepth.color					= CT_DKPURPLE1;
		s_graphicsoptions.colordepth.color2					= CT_LTPURPLE1;
		s_graphicsoptions.colordepth.textX					= 5;
		s_graphicsoptions.colordepth.textY					= 2;
		s_graphicsoptions.colordepth.listnames				= s_colordepth_Names;
		s_graphicsoptions.colordepth.width						= width;
	}

	y += inc;
	s_graphicsoptions.fs.generic.type			= MTYPE_SPINCONTROL;
	s_graphicsoptions.fs.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_graphicsoptions.fs.generic.x				= x;
	s_graphicsoptions.fs.generic.y				= y;
	s_graphicsoptions.fs.textEnum				= MBT_VIDEOFULLSCREEN;
	s_graphicsoptions.fs.textcolor				= CT_BLACK;
	s_graphicsoptions.fs.textcolor2				= CT_WHITE;
	s_graphicsoptions.fs.color					= CT_DKPURPLE1;
	s_graphicsoptions.fs.color2					= CT_LTPURPLE1;
	s_graphicsoptions.fs.textX					= 5;
	s_graphicsoptions.fs.textY					= 2;
	s_graphicsoptions.fs.listnames				= s_OffOnNone_Names;
	s_graphicsoptions.fs.width						= width;
/*
	y += inc;
	s_graphicsoptions.lighting.generic.type				= MTYPE_SPINCONTROL;
	s_graphicsoptions.lighting.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_graphicsoptions.lighting.generic.x				= x;
	s_graphicsoptions.lighting.generic.y				= y;
	s_graphicsoptions.lighting.textEnum					= MBT_VIDEOLIGHTING;
	s_graphicsoptions.lighting.textcolor				= CT_BLACK;
	s_graphicsoptions.lighting.textcolor2				= CT_WHITE;
	s_graphicsoptions.lighting.color					= CT_DKPURPLE1;
	s_graphicsoptions.lighting.color2					= CT_LTPURPLE1;
	s_graphicsoptions.lighting.textX					= 5;
	s_graphicsoptions.lighting.textY					= 2;
	s_graphicsoptions.lighting.listnames				= s_lighting_Names;
	s_graphicsoptions.lighting.width						= width;
*/
	y += inc;
	// references/modifies "r_lodBias" & "subdivisions"
	s_graphicsoptions.geometry.generic.type				= MTYPE_SPINCONTROL;
	s_graphicsoptions.geometry.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_graphicsoptions.geometry.generic.x				= x;
	s_graphicsoptions.geometry.generic.y				= y;
	s_graphicsoptions.geometry.textEnum					= MBT_VIDEOGEOMETRY;
	s_graphicsoptions.geometry.textcolor				= CT_BLACK;
	s_graphicsoptions.geometry.textcolor2				= CT_WHITE;
	s_graphicsoptions.geometry.color					= CT_DKPURPLE1;
	s_graphicsoptions.geometry.color2					= CT_LTPURPLE1;
	s_graphicsoptions.geometry.textX					= 5;
	s_graphicsoptions.geometry.textY					= 2;
	s_graphicsoptions.geometry.listnames				= s_quality_Names;
	s_graphicsoptions.geometry.width					= width;

	y += inc;
	s_graphicsoptions.tq.generic.type		= MTYPE_SPINCONTROL;
	s_graphicsoptions.tq.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_graphicsoptions.tq.generic.x			= x;
	s_graphicsoptions.tq.generic.y			= y;
	s_graphicsoptions.tq.textEnum			= MBT_VIDEOTEXTUREDETAIL;
	s_graphicsoptions.tq.textcolor			= CT_BLACK;
	s_graphicsoptions.tq.textcolor2			= CT_WHITE;
	s_graphicsoptions.tq.color				= CT_DKPURPLE1;
	s_graphicsoptions.tq.color2				= CT_LTPURPLE1;
	s_graphicsoptions.tq.textX				= 5;
	s_graphicsoptions.tq.textY				= 2;
	s_graphicsoptions.tq.listnames			= s_4quality_Names;
	s_graphicsoptions.tq.width					= width;

	if ( videoEngineConfig.supportTextureBits ) {
		y += inc;
		// references/modifies "r_textureBits"
		s_graphicsoptions.texturebits.generic.type				= MTYPE_SPINCONTROL;
		s_graphicsoptions.texturebits.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
		s_graphicsoptions.texturebits.generic.x					= x;
		s_graphicsoptions.texturebits.generic.y					= y;
		s_graphicsoptions.texturebits.textEnum					= MBT_VIDEOTEXTUREBITS;
		s_graphicsoptions.texturebits.textcolor					= CT_BLACK;
		s_graphicsoptions.texturebits.textcolor2				= CT_WHITE;
		s_graphicsoptions.texturebits.color						= CT_DKPURPLE1;
		s_graphicsoptions.texturebits.color2					= CT_LTPURPLE1;
		s_graphicsoptions.texturebits.textX						= 5;
		s_graphicsoptions.texturebits.textY						= 2;
		s_graphicsoptions.texturebits.listnames					= s_tqbits_Names;
		s_graphicsoptions.texturebits.width					= width;
	}

	y += inc;
	// references/modifies "r_textureMode"
	s_graphicsoptions.filter.generic.type					= MTYPE_SPINCONTROL;
	s_graphicsoptions.filter.generic.flags					= QMF_HIGHLIGHT_IF_FOCUS;
	s_graphicsoptions.filter.generic.x						= x;
	s_graphicsoptions.filter.generic.y						= y;
	s_graphicsoptions.filter.textEnum						= MBT_VIDEOTEXTUREFILTER;
	s_graphicsoptions.filter.textcolor						= CT_BLACK;
	s_graphicsoptions.filter.textcolor2						= CT_WHITE;
	s_graphicsoptions.filter.color							= CT_DKPURPLE1;
	s_graphicsoptions.filter.color2							= CT_LTPURPLE1;
	s_graphicsoptions.filter.textX							= 5;
	s_graphicsoptions.filter.textY							= 2;
	s_graphicsoptions.filter.listnames						= s_filter_Names;
	s_graphicsoptions.filter.width					= width;

	if ( videoEngineConfig.supportSimpleShaders ) {
		y += inc;
		// references/modifies "r_lowEndVideo"
		s_graphicsoptions.simpleshaders.generic.type				= MTYPE_SPINCONTROL;
		s_graphicsoptions.simpleshaders.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
		s_graphicsoptions.simpleshaders.generic.x					= x;
		s_graphicsoptions.simpleshaders.generic.y					= y;
		s_graphicsoptions.simpleshaders.textEnum					= MBT_SIMPLESHADER;
		s_graphicsoptions.simpleshaders.textcolor					= CT_BLACK;
		s_graphicsoptions.simpleshaders.textcolor2				= CT_WHITE;
		s_graphicsoptions.simpleshaders.color						= CT_DKPURPLE1;
		s_graphicsoptions.simpleshaders.color2					= CT_LTPURPLE1;
		s_graphicsoptions.simpleshaders.textX						= 5;
		s_graphicsoptions.simpleshaders.textY						= 2;
		s_graphicsoptions.simpleshaders.listnames					= s_OffOnNone_Names;
		s_graphicsoptions.simpleshaders.width					= width;
	}

	y += inc;
	// references/modifies "r_ext_compress_textures"
	s_graphicsoptions.compresstextures.generic.type				= MTYPE_SPINCONTROL;
	s_graphicsoptions.compresstextures.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_graphicsoptions.compresstextures.generic.x				= x;
	s_graphicsoptions.compresstextures.generic.y				= y;
	s_graphicsoptions.compresstextures.textEnum					= MBT_COMPRESSEDTEXTURES;
	s_graphicsoptions.compresstextures.textcolor				= CT_BLACK;
	s_graphicsoptions.compresstextures.textcolor2				= CT_WHITE;
	s_graphicsoptions.compresstextures.color					= CT_DKPURPLE1;
	s_graphicsoptions.compresstextures.color2					= CT_LTPURPLE1;
	s_graphicsoptions.compresstextures.textX					= 5;
	s_graphicsoptions.compresstextures.textY					= 2;
	s_graphicsoptions.compresstextures.listnames				= s_OffOnNone_Names;
	s_graphicsoptions.compresstextures.width					= width;

	if ( videoEngineConfig.anisotropicFilteringVideoMenu ) {
		y += inc;
		s_graphicsoptions.anisotropic.generic.type			= MTYPE_SPINCONTROL;
		s_graphicsoptions.anisotropic.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
		s_graphicsoptions.anisotropic.generic.x				= x;
		s_graphicsoptions.anisotropic.generic.y				= y;
		s_graphicsoptions.anisotropic.textEnum				= MBT_VIDEO_ANISOTROPIC_LEVEL;
		s_graphicsoptions.anisotropic.textcolor				= CT_BLACK;
		s_graphicsoptions.anisotropic.textcolor2			= CT_WHITE;
		s_graphicsoptions.anisotropic.color					= CT_DKPURPLE1;
		s_graphicsoptions.anisotropic.color2				= CT_LTPURPLE1;
		s_graphicsoptions.anisotropic.textX					= 5;
		s_graphicsoptions.anisotropic.textY					= 2;
		s_graphicsoptions.anisotropic.listnames				= videoEngineConfig.supportMaxAnisotropy ? s_anisotropic_Names : s_OffOnNone_Names;
		s_graphicsoptions.anisotropic.width					= width;
	}

	if ( videoEngineConfig.supportMultisample ) {
		y += inc;
		s_graphicsoptions.multisample.generic.type			= MTYPE_SPINCONTROL;
		s_graphicsoptions.multisample.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
		s_graphicsoptions.multisample.generic.x				= x;
		s_graphicsoptions.multisample.generic.y				= y;
		s_graphicsoptions.multisample.textEnum				= MBT_VIDEO_ANTI_ALIASING;
		s_graphicsoptions.multisample.textcolor				= CT_BLACK;
		s_graphicsoptions.multisample.textcolor2			= CT_WHITE;
		s_graphicsoptions.multisample.color					= CT_DKPURPLE1;
		s_graphicsoptions.multisample.color2				= CT_LTPURPLE1;
		s_graphicsoptions.multisample.textX					= 5;
		s_graphicsoptions.multisample.textY					= 2;
		s_graphicsoptions.multisample.listnames				= s_multisample_Names;
		s_graphicsoptions.multisample.width					= width;
	}

	if ( videoEngineConfig.supportVSync ) {
		y += inc;
		s_graphicsoptions.vsync.generic.type			= MTYPE_SPINCONTROL;
		s_graphicsoptions.vsync.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
		s_graphicsoptions.vsync.generic.x				= x;
		s_graphicsoptions.vsync.generic.y				= y;
		s_graphicsoptions.vsync.textEnum				= MBT_VIDEO_VERTICAL_SYNC;
		s_graphicsoptions.vsync.textcolor				= CT_BLACK;
		s_graphicsoptions.vsync.textcolor2				= CT_WHITE;
		s_graphicsoptions.vsync.color					= CT_DKPURPLE1;
		s_graphicsoptions.vsync.color2					= CT_LTPURPLE1;
		s_graphicsoptions.vsync.textX					= 5;
		s_graphicsoptions.vsync.textY					= 2;
		s_graphicsoptions.vsync.listnames				= s_OffOnNone_Names;
		s_graphicsoptions.vsync.width					= width;
	}

	s_graphicsoptions.apply.generic.type				= MTYPE_ACTION;
	s_graphicsoptions.apply.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS|QMF_GRAYED;
	s_graphicsoptions.apply.generic.x					= 501;
	s_graphicsoptions.apply.generic.y					= 245;
	s_graphicsoptions.apply.generic.callback			= GraphicsOptions_ApplyChanges;
	s_graphicsoptions.apply.textEnum					= MBT_VIDEOAPPLYCHANGES;
	s_graphicsoptions.apply.textcolor					= CT_BLACK;
	s_graphicsoptions.apply.textcolor2					= CT_WHITE;
	s_graphicsoptions.apply.textcolor3					= CT_LTGREY;
	s_graphicsoptions.apply.color						= CT_DKPURPLE1;
	s_graphicsoptions.apply.color2						= CT_LTPURPLE1;
	s_graphicsoptions.apply.color3						= CT_DKGREY;
	s_graphicsoptions.apply.textX						= 5;
	s_graphicsoptions.apply.textY						= 80;
	s_graphicsoptions.apply.width						= 110;
	s_graphicsoptions.apply.height						= 100;

	GraphicsOptions_SetMenuItems();
	GraphicsOptions_GetInitialVideo();

	SetupMenu_TopButtons(&s_videodata.menu,MENU_VIDEODATA,&s_graphicsoptions.apply);

	Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.list);
	if ( videoEngineConfig.supportGlDriver ) {
		Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.driver);
	}
	if ( videoEngineConfig.supportRendererSelect ) {
		Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.renderer);
	}
	if ( videoEngineConfig.supportAllowExtensions ) {
		Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.allow_extensions);
	}
	Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.mode);
	if ( videoEngineConfig.supportColorDepth ) {
		Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.colordepth);
	}
	Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.fs);
//	Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.lighting);
	Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.geometry);
	Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.tq);
	if ( videoEngineConfig.supportTextureBits ) {
		Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.texturebits);
	}
	Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.filter);
	if ( videoEngineConfig.supportSimpleShaders ) {
		Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.simpleshaders);
	}
	Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.compresstextures);
	if ( videoEngineConfig.anisotropicFilteringVideoMenu ) {
		Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.anisotropic);
	}
	if ( videoEngineConfig.supportMultisample ) {
		Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.multisample);
	}
	if ( videoEngineConfig.supportVSync ) {
		Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.vsync);
	}
	Menu_AddItem( &s_videodata.menu, ( void * )&s_graphicsoptions.apply);


	Video_SideButtons(&s_videodata.menu,ID_VIDEODATA);

	if ( uis.glconfig.driverType == GLDRV_ICD &&
		uis.glconfig.hardwareType == GLHW_3DFX_2D3D )
	{
		s_graphicsoptions.driver.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	}
}


/*
=================
UI_VideoDataMenu
=================
*/
void UI_VideoDataMenu( void )
{
	VideoData_MenuInit();

	UI_PushMenu( &s_videodata.menu );
}

/*
=================
Video_SideButtons
=================
*/
void Video_SideButtons(menuframework_s *menu,int menuType)
{

	// Button Data
	s_video_data.generic.type				= MTYPE_BITMAP;
	s_video_data.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_video_data.generic.x					= video_sidebuttons[0][0];
	s_video_data.generic.y					= video_sidebuttons[0][1];
	s_video_data.generic.name				= GRAPHIC_SQUARE;
	s_video_data.generic.id					= ID_VIDEODATA;
	if (menuType == ID_VIDEODATA)
	{
		s_video_data.generic.callback			= VideoSideButtons_MenuEvent;
	}
	else
	{
		s_video_data.generic.callback			= Video_MenuEvent;
	}
	s_video_data.width						= MENU_BUTTON_MED_WIDTH - 10;
	s_video_data.height						= MENU_BUTTON_MED_HEIGHT;
	s_video_data.color						= CT_DKPURPLE1;
	s_video_data.color2						= CT_LTPURPLE1;
	s_video_data.textX						= 5;
	s_video_data.textY						= 2;
	s_video_data.textEnum					= MBT_VIDEODATA;
	if (menuType == ID_VIDEODATA)
	{
		s_video_data.textcolor				= CT_WHITE;
		s_video_data.textcolor2				= CT_WHITE;
		s_video_data.generic.flags			= QMF_GRAYED;
	}
	else
	{
		s_video_data.textcolor				= CT_BLACK;
		s_video_data.textcolor2				= CT_WHITE;
	}

	s_video_data2.generic.type				= MTYPE_BITMAP;
	s_video_data2.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_video_data2.generic.x					= video_sidebuttons[1][0];
	s_video_data2.generic.y					= video_sidebuttons[1][1];
	s_video_data2.generic.name				= GRAPHIC_SQUARE;
	s_video_data2.generic.id				= ID_VIDEODATA2;
	if (menuType == ID_VIDEODATA)
	{
		s_video_data2.generic.callback			= VideoSideButtons_MenuEvent;
	}
	else
	{
		s_video_data2.generic.callback			= Video_MenuEvent;
	}
	s_video_data2.width						= MENU_BUTTON_MED_WIDTH - 10;
	s_video_data2.height					= MENU_BUTTON_MED_HEIGHT;
	s_video_data2.color						= CT_DKPURPLE1;
	s_video_data2.color2					= CT_LTPURPLE1;
	s_video_data2.textX						= 5;
	s_video_data2.textY						= 2;
	s_video_data2.textEnum					= MBT_VIDEODATA2;
	s_video_data2.textcolor					= CT_WHITE;
	s_video_data2.textcolor2				= CT_WHITE;
	if (menuType == ID_VIDEODATA2)
	{
		s_video_data2.textcolor				= CT_WHITE;
		s_video_data2.textcolor2			= CT_WHITE;
		s_video_data2.generic.flags			= QMF_GRAYED;
	}
	else
	{
		s_video_data2.textcolor				= CT_BLACK;
		s_video_data2.textcolor2			= CT_WHITE;
	}

	s_video_brightness.generic.type				= MTYPE_BITMAP;
	s_video_brightness.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_video_brightness.generic.x				= video_sidebuttons[2][0];
	s_video_brightness.generic.y				= video_sidebuttons[2][1];
	s_video_brightness.generic.name				= GRAPHIC_SQUARE;
	s_video_brightness.generic.id				= ID_VIDEOBRIGHTNESS;
	if (menuType == ID_VIDEODATA)
	{
		s_video_brightness.generic.callback		= VideoSideButtons_MenuEvent;
	}
	else
	{
		s_video_brightness.generic.callback		= Video_MenuEvent;
	}
	s_video_brightness.width					= MENU_BUTTON_MED_WIDTH - 10;
	s_video_brightness.height					= MENU_BUTTON_MED_HEIGHT;
	s_video_brightness.color					= CT_DKPURPLE1;
	s_video_brightness.color2					= CT_LTPURPLE1;
	s_video_brightness.textX					= 5;
	s_video_brightness.textY					= 2;
	s_video_brightness.textEnum					= MBT_BRIGHTNESS;
	s_video_brightness.textcolor				= CT_WHITE;
	s_video_brightness.textcolor2				= CT_WHITE;
	if (menuType == ID_VIDEOBRIGHTNESS)
	{
		s_video_brightness.textcolor			= CT_WHITE;
		s_video_brightness.textcolor2			= CT_WHITE;
		s_video_brightness.generic.flags		= QMF_GRAYED;
	}
	else
	{
		s_video_brightness.textcolor			= CT_BLACK;
		s_video_brightness.textcolor2			= CT_WHITE;
	}

	s_video_drivers.generic.type			= MTYPE_BITMAP;
	s_video_drivers.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_video_drivers.generic.x				= video_sidebuttons[3][0];
	s_video_drivers.generic.y				= video_sidebuttons[3][1];
	s_video_drivers.generic.name			= GRAPHIC_SQUARE;
	s_video_drivers.generic.id				= ID_VIDEODRIVERS;
	if (menuType == ID_VIDEODATA)
	{
		s_video_drivers.generic.callback			= VideoSideButtons_MenuEvent;
	}
	else
	{
		s_video_drivers.generic.callback			= Video_MenuEvent;
	}
	s_video_drivers.width					= MENU_BUTTON_MED_WIDTH - 10;
	s_video_drivers.height					= MENU_BUTTON_MED_HEIGHT;
	s_video_drivers.color					= CT_DKPURPLE1;
	s_video_drivers.color2					= CT_LTPURPLE1;
	s_video_drivers.textX					= 5;
	s_video_drivers.textY					= 2;
	s_video_drivers.textEnum				= MBT_VIDEODRIVERS;
	if (menuType == ID_VIDEODRIVERS)
	{
		s_video_drivers.textcolor			= CT_WHITE;
		s_video_drivers.textcolor2			= CT_WHITE;
		s_video_drivers.generic.flags		= QMF_GRAYED;
	}
	else
	{
		s_video_drivers.textcolor			= CT_BLACK;
		s_video_drivers.textcolor2			= CT_WHITE;
	}

	Menu_AddItem( menu, ( void * )&s_video_data);
	Menu_AddItem( menu, ( void * )&s_video_data2);
	Menu_AddItem( menu, ( void * )&s_video_brightness);
	Menu_AddItem( menu, ( void * )&s_video_drivers);

}


/*
=================
VideoDriver_Lines
=================
*/
void VideoDriver_Lines(int increment)
{
	int		i,i2;

	s_videodriver.currentDriverLine += increment;

	i=0;
	i2 = 0;

	i = (s_videodriver.currentDriverLine * 2);
	if (i<0)
	{
		s_videodriver.currentDriverLine = 0;
		return;
	}

	if (i>s_videodriver.driverCnt)
	{
		s_videodriver.currentDriverLine = (s_videodriver.driverCnt/2);
		return;
	}
	else if (i==s_videodriver.driverCnt)
	{
		s_videodriver.currentDriverLine = (s_videodriver.driverCnt/2) - 1;
		return;
	}

	if (!s_videodriver.drivers[i + 22])
	{
		s_videodriver.currentDriverLine -= increment;
		s_videodriver.activeArrowDwn = qfalse;
		return;
	}

	for (; i < MAX_VID_DRIVERS; i++)
	{
		if (s_videodriver.drivers[i])
		{
			if (i2<24)
			{
				((menutext_s *)g_videolines[i2])->string	= s_videodriver.drivers[i];
				i2++;
			}
		}
		else
		{
			if (i2<24)
			{
				((menutext_s *)g_videolines[i2])->string	= NULL;
				i2++;
			}
			else
			{
				break;
			}
		}
	}

	// Set up arrows

	if (increment > 0)
	{
		s_videodriver.activeArrowUp = qtrue;
	}

	if (s_videodriver.currentDriverLine < 1)
	{
		s_videodriver.activeArrowUp = qfalse;
	}

	if (i2>= 24)
	{
		s_videodriver.activeArrowDwn = qtrue;
	}

	i = (s_videodriver.currentDriverLine * 2);
	if (!s_videodriver.drivers[i + 24])
	{
		s_videodriver.activeArrowDwn = qfalse;
		return;
	}

}

/*
=================
VideoDriver_LineSetup
=================
*/
void VideoDriver_LineSetup(void)
{
	char	*bufhold;
	char	*eptr;
	int		i;

	strcpy( s_videodriver.extensionsString, uis.glconfig.extensions_string );
	eptr = s_videodriver.extensionsString;
	i=0;

	s_videodriver.driverCnt = 0;

	while ( i < MAX_VID_DRIVERS && *eptr )
	{
		while ( *eptr )
		{
			bufhold = eptr;

			while(*bufhold !=  ' ')
			{
				++bufhold;
			}
			*bufhold = 0;

			s_videodriver.drivers[i] = eptr;

			if (i<24)
			{
				((menutext_s *)g_videolines[i])->string	= eptr;
			}

			bufhold++;
			eptr = bufhold;
			s_videodriver.driverCnt++;
			i++;
		}
	}

	// Set down arrows
	if (i> 24)
	{
		s_videodriver.activeArrowDwn = qtrue;
	}

	s_videodriver.currentDriverLine = 0;

}

/*
=================
VideoDriver_MenuKey
=================
*/
sfxHandle_t VideoDriver_MenuKey (int key)
{
	return ( Menu_DefaultKey( &s_videodriver.menu, key ) );
}

/*
=================
M_VideoDriverMenu_Graphics
=================
*/
void M_VideoDriverMenu_Graphics (void)
{
	float labelColor[] = { 0, 1.0, 0, 1.0 };
	float textColor[] = { 1, 1, 1, 1 };
	int x,y,x2,x3;

	UI_MenuFrame(&s_videodriver.menu);

	UI_DrawProportionalString(  74,  66, "207",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  84, "44909",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  188, "357",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  206, "250624",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  395, "456730-1",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	UI_Setup_MenuButtons();

//	trap_R_SetColor( colorTable[CT_DKPURPLE1]);
//	UI_DrawHandlePic(  30, 203, 47, 69, uis.whiteShader);	// Top Left column above two buttons
//	UI_DrawHandlePic(  30, 344, 47, 45, uis.whiteShader);	// Top Left column below two buttons

	// Top Frame
	UI_DrawHandlePic( 178, 136,  32, -32, s_videodriver.corner_ll_8_16);	// UL
	UI_DrawHandlePic( 178, 221,  32,  32, s_videodriver.corner_ll_8_16);	// LL
	UI_DrawHandlePic( 556, 136, -32, -32, s_videodriver.corner_ll_8_16);	// UR
	UI_DrawHandlePic( 556, 221, -32,  32, s_videodriver.corner_ll_8_16);	// LR
	UI_DrawHandlePic(194,157, 378,  8, uis.whiteShader);	// Top line
	UI_DrawHandlePic(178,165,  16, 60, uis.whiteShader);	// Left side
	UI_DrawHandlePic(572,165,  16, 60, uis.whiteShader);	// Right side
	UI_DrawHandlePic(194,224, 378,  8, uis.whiteShader);	// Bottom line


	// Lower Frame
	UI_DrawHandlePic( 178, 226,  32, -32, s_videodriver.corner_ll_16_16);	// UL
	UI_DrawHandlePic( 178, 414,  32,  32, s_videodriver.corner_ll_16_16);	// LL
	UI_DrawHandlePic( 556, 226, -32, -32, s_videodriver.corner_ll_16_16);	// UR
	UI_DrawHandlePic( 556, 414, -32,  32, s_videodriver.corner_ll_16_16);	// LR

	UI_DrawHandlePic( 194, 237, 378,  16, uis.whiteShader);	// Top line
	UI_DrawHandlePic( 178, 252,  16, 168, uis.whiteShader);	// Left side

	UI_DrawHandlePic( 572, 261,  16,  15, uis.whiteShader);	// Right side
	UI_DrawHandlePic( 572, 279,  16, 114, uis.whiteShader);	// Right side
	UI_DrawHandlePic( 572, 396,  16,  15, uis.whiteShader);	// Right side

	UI_DrawHandlePic( 194, 419, 378,  16, uis.whiteShader);	// Bottom line

	trap_R_SetColor( colorTable[CT_LTGOLD1]);
	if (s_videodriver.activeArrowUp)
	{
		UI_DrawHandlePic( 382, 237, 32,  -14, s_videodriver.arrow_dn);
	}

	if (s_videodriver.activeArrowDwn)
	{
		UI_DrawHandlePic( 382, 422, 32,   14, s_videodriver.arrow_dn);
	}

	x = 204;
	x2 = 259;
	x3 = x2 + 150;
	y = 168;
	UI_DrawProportionalString( x, y, menu_normal_text[MNT_VENDOR], UI_LEFT|UI_TINYFONT, labelColor );
	UI_DrawProportionalString( x2, y, uis.glconfig.vendor_string, UI_LEFT|UI_TINYFONT, textColor );
	y += 14;
	UI_DrawProportionalString( x, y, menu_normal_text[MNT_VERSION], UI_LEFT|UI_TINYFONT, labelColor );
	UI_DrawProportionalString( x2, y, uis.glconfig.version_string, UI_LEFT|UI_TINYFONT, textColor );
	y += 14;
	UI_DrawProportionalString( x, y, menu_normal_text[MNT_RENDERER], UI_LEFT|UI_TINYFONT, labelColor );
	UI_DrawProportionalString( x2, y, uis.glconfig.renderer_string, UI_LEFT|UI_TINYFONT, textColor );
	y += 14;
	UI_DrawProportionalString( x, y, menu_normal_text[MNT_PIXELFORMAT], UI_LEFT|UI_TINYFONT, labelColor );
	UI_DrawProportionalString( x2, y, va("color(%d-bits) Z(%d-bit) stencil(%d-bits)", uis.glconfig.colorBits, uis.glconfig.depthBits, uis.glconfig.stencilBits), UI_LEFT|UI_TINYFONT, textColor );

}

/*
=================
VideoDriver_MenuDraw
=================
*/
static void VideoDriver_MenuDraw (void)
{

	M_VideoDriverMenu_Graphics();

	Menu_Draw( &s_videodriver.menu );
}

/*
=================
UI_VideoDriverMenu_Cache
=================
*/
void UI_VideoDriverMenu_Cache(void)
{
	s_videodriver.corner_ll_16_16 = trap_R_RegisterShaderNoMip("menu/common/corner_ll_16_16.tga");
	s_videodriver.corner_ll_8_16 = trap_R_RegisterShaderNoMip("menu/common/corner_ll_8_16.tga");
	s_videodriver.arrow_dn = trap_R_RegisterShaderNoMip("menu/common/arrow_dn_16.tga");
}


/*
=================
Video_MenuInit
=================
*/
static void VideoDriver_MenuInit( void )
{
	int		i,x,y,x2;

	UI_VideoDriverMenu_Cache();

	s_videodriver.menu.nitems					= 0;
	s_videodriver.menu.wrapAround				= qtrue;
//	s_videodriver.menu.opening					= NULL;
//	s_videodriver.menu.closing					= NULL;
	s_videodriver.menu.draw						= VideoDriver_MenuDraw;
	s_videodriver.menu.key						= VideoDriver_MenuKey;
	s_videodriver.menu.fullscreen				= qtrue;
	s_videodriver.menu.descX					= MENU_DESC_X;
	s_videodriver.menu.descY					= MENU_DESC_Y;
	s_videodriver.menu.listX					= 230;
	s_videodriver.menu.listY					= 188;
	s_videodriver.menu.titleX					= MENU_TITLE_X;
	s_videodriver.menu.titleY					= MENU_TITLE_Y;
	s_videodriver.menu.titleI					= MNT_CONTROLSMENU_TITLE;
	s_videodriver.menu.footNoteEnum				= MNT_VIDEODRIVER;

	SetupMenu_TopButtons(&s_videodriver.menu,MENU_VIDEO,NULL);

	Video_SideButtons(&s_videodriver.menu,ID_VIDEODRIVERS);

	s_videodriver.arrowup.generic.type				= MTYPE_BITMAP;
	s_videodriver.arrowup.generic.flags				= QMF_HIGHLIGHT_IF_FOCUS;
	s_videodriver.arrowup.generic.x					= 572;
	s_videodriver.arrowup.generic.y					= 262;
	s_videodriver.arrowup.generic.name				= "menu/common/arrow_up_16.tga";
	s_videodriver.arrowup.generic.id				= ID_ARROWUP;
	s_videodriver.arrowup.generic.callback			= Video_MenuEvent;
	s_videodriver.arrowup.width						= 16;
	s_videodriver.arrowup.height					= 16;
	s_videodriver.arrowup.color						= CT_DKBLUE1;
	s_videodriver.arrowup.color2					= CT_LTBLUE1;
	s_videodriver.arrowup.textX						= 0;
	s_videodriver.arrowup.textY						= 0;
	s_videodriver.arrowup.textEnum					= MBT_NONE;
	s_videodriver.arrowup.textcolor					= CT_BLACK;
	s_videodriver.arrowup.textcolor2				= CT_WHITE;
	Menu_AddItem( &s_videodriver.menu,( void * ) &s_videodriver.arrowup);

	s_videodriver.arrowdwn.generic.type				= MTYPE_BITMAP;
	s_videodriver.arrowdwn.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_videodriver.arrowdwn.generic.x				= 572;
	s_videodriver.arrowdwn.generic.y				= 397;
	s_videodriver.arrowdwn.generic.name				= "menu/common/arrow_dn_16.tga";
	s_videodriver.arrowdwn.generic.id				= ID_ARROWDWN;
	s_videodriver.arrowdwn.generic.callback			= Video_MenuEvent;
	s_videodriver.arrowdwn.width					= 16;
	s_videodriver.arrowdwn.height					= 16;
	s_videodriver.arrowdwn.color					= CT_DKBLUE1;
	s_videodriver.arrowdwn.color2					= CT_LTBLUE1;
	s_videodriver.arrowdwn.textX					= 0;
	s_videodriver.arrowdwn.textY					= 0;
	s_videodriver.arrowdwn.textEnum					= MBT_NONE;
	s_videodriver.arrowdwn.textcolor				= CT_BLACK;
	s_videodriver.arrowdwn.textcolor2				= CT_WHITE;
	Menu_AddItem( &s_videodriver.menu, ( void * ) &s_videodriver.arrowdwn);

	s_videodriver.activeArrowDwn = qfalse;
	s_videodriver.activeArrowUp = qfalse;

	x = 204;
	x2 = 404;
	y = 260;

	for (i=0;i<24;i++)
	{
		((menutext_s *)g_videolines[i])->generic.type		= MTYPE_TEXT;
		((menutext_s *)g_videolines[i])->generic.flags		= QMF_LEFT_JUSTIFY | QMF_INACTIVE;
		((menutext_s *)g_videolines[i])->generic.y			= y;
		if ((i % 2 ) == 0)
		{
			((menutext_s *)g_videolines[i])->generic.x			= x;
		}
		else
		{
			((menutext_s *)g_videolines[i])->generic.x			= x2;
			y +=13;
		}


		((menutext_s *)g_videolines[i])->buttontextEnum		= MBT_NONE;
		((menutext_s *)g_videolines[i])->style				= UI_TINYFONT | UI_LEFT;
		((menutext_s *)g_videolines[i])->color				= colorTable[CT_LTPURPLE1];
		Menu_AddItem( &s_videodriver.menu, ( void * )g_videolines[i]);

	}

	// Print extensions
	VideoDriver_LineSetup();
}


/*
=================
UI_VideoDriverMenu
=================
*/
void UI_VideoDriverMenu( void )
{
	if (!s_videodriver.menu.initialized)
	{
		VideoDriver_MenuInit();
	}

	UI_PushMenu( &s_videodriver.menu );
}


/*
=================
M_VideoData2Menu_Graphics
=================
*/
void M_VideoData2Menu_Graphics (void)
{
	UI_MenuFrame(&s_videodata2.menu);

	UI_DrawProportionalString(  74,  66, "925",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  84, "88PK",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  188, "8125",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  206, "358677",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);
	UI_DrawProportionalString(  74,  395, "3-679",UI_RIGHT|UI_TINYFONT, colorTable[CT_BLACK]);

	UI_Setup_MenuButtons();

	// Brackets around Video Data
	trap_R_SetColor( colorTable[CT_LTPURPLE1]);
	UI_DrawHandlePic(158,163, 16, 16, uis.graphicBracket1CornerLU);
	UI_DrawHandlePic(158,179,  8, 233, uis.whiteShader);
	UI_DrawHandlePic(158,412, 16, -16, uis.graphicBracket1CornerLU);	//LD

	UI_DrawHandlePic(174,163, 408, 8, uis.whiteShader);	// Top line

	UI_DrawHandlePic(579,163, 32, 16, s_videodata2.top);	// Corner, UR
	UI_DrawHandlePic(581,179, 30, 121, uis.whiteShader);	// Top right column
	UI_DrawHandlePic(581,303, 30, 109, uis.whiteShader);	// Bottom right column
	UI_DrawHandlePic(579,412, 32, -16, s_videodata2.top);	// Corner, LR

	UI_DrawHandlePic(174,420, 408, 8, uis.whiteShader);	// Bottom line
}

/*
=================
VideoData2_MenuDraw
=================
*/
static void VideoData2_MenuDraw (void)
{

	M_VideoData2Menu_Graphics();

	Menu_Draw( &s_videodata2.menu );
}


/*
=================
UI_VideoData2Menu_Cache
=================
*/
void UI_VideoData2Menu_Cache(void)
{
	s_videodata2.top = trap_R_RegisterShaderNoMip("menu/common/corner_ur_8_30.tga");
	trap_R_RegisterShaderNoMip(PIC_MONBAR2);
	trap_R_RegisterShaderNoMip(PIC_SLIDER);
}

/*
=================
VideoData2_EnabledDisableCenterHudSlider

Only enable the center HUD slider if aspect correction is enabled.
=================
*/
void VideoData2_EnabledDisableCenterHudSlider( void ) {
	if ( s_videodata2.aspectCorrection.curvalue ) {
		s_videodata2.centerHud_slider.generic.flags	&= ~QMF_GRAYED;
	} else {
		s_videodata2.centerHud_slider.generic.flags	|= QMF_GRAYED;
	}
}

/*
=================
VideoData2_Event
=================
*/
static void VideoData2_Event( void* ptr, int notification )
{
	if( notification != QM_ACTIVATED )
	{
		return;
	}

	if ( ptr == &s_videodata2.aspectCorrection ) {
		trap_Cvar_SetValue( "cg_aspectCorrect", s_videodata2.aspectCorrection.curvalue );
		VideoData2_EnabledDisableCenterHudSlider();
	}

	else if ( ptr == &s_videodata2.centerHud_slider ) {
		trap_Cvar_SetValue( "cg_aspectCorrectCenterHud", s_videodata2.centerHud_slider.curvalue / 10.0f );
	}

	else if ( ptr == &s_videodata2.fov_slider ) {
		// Use asterisk notation to enable horizontal scaling.
		trap_Cvar_Set( "cg_fov", va( "%f*", s_videodata2.fov_slider.curvalue ) );
	}

	else if ( ptr == &s_videodata2.screensize_slider ) {
		trap_Cvar_SetValue( "cg_viewsize", s_videodata2.screensize_slider.curvalue );
	}

	else if ( ptr == &s_videodata2.flares ) {
		trap_Cvar_SetValue( "r_flares", s_videodata2.flares.curvalue );
	}

	else if ( ptr == &s_videodata2.wallmarks ) {
		trap_Cvar_SetValue( "cg_marks", s_videodata2.wallmarks.curvalue );
	}

	else if ( ptr == &s_videodata2.dynamiclights ) {
		trap_Cvar_SetValue( "r_dynamiclight", s_videodata2.dynamiclights.curvalue );
	}

	else if ( ptr == &s_videodata2.simpleitems ) {
		trap_Cvar_SetValue( "cg_simpleItems", s_videodata2.simpleitems.curvalue );
	}

	else if ( ptr == &s_videodata2.synceveryframe ) {
		trap_Cvar_SetValue( "r_finish", s_videodata2.synceveryframe.curvalue );
	}

	else if ( ptr == &s_videodata2.anisotropic ) {
		UI_SetAnisotropyLevel( s_videodata2.anisotropic.curvalue );
	}
}

/*
=================
VideoData2_MenuInit
=================
*/
static void VideoData2_MenuInit( void )
{
	int x,y;
	int inc = 22;
	qboolean suppressViewSize = VMExt_GVCommandInt( "ui_suppress_cg_viewsize", 0 )
			&& trap_Cvar_VariableValue( "cg_viewsize" ) >= 100.0f ? qtrue : qfalse;

	UI_InitVideoEngineConfig();

	UI_VideoData2Menu_Cache();

	// Menu Data
	s_videodata2.menu.nitems						= 0;
	s_videodata2.menu.wrapAround					= qtrue;
//	s_videodata2.menu.opening						= NULL;
//	s_videodata2.menu.closing						= NULL;
	s_videodata2.menu.draw							= VideoData2_MenuDraw;
	s_videodata2.menu.fullscreen					= qtrue;
	s_videodata2.menu.descX							= MENU_DESC_X;
	s_videodata2.menu.descY							= MENU_DESC_Y;
	s_videodata2.menu.listX							= 230;
	s_videodata2.menu.listY							= 188;
	s_videodata2.menu.titleX						= MENU_TITLE_X;
	s_videodata2.menu.titleY						= MENU_TITLE_Y;
	s_videodata2.menu.titleI						= MNT_CONTROLSMENU_TITLE;
	s_videodata2.menu.footNoteEnum					= MNT_VIDEOSETUP;

	SetupMenu_TopButtons(&s_videodata2.menu,MENU_VIDEO,NULL);

	Video_SideButtons(&s_videodata2.menu,ID_VIDEODATA2);

	x = 175;
	y = 183;
	s_videodata2.aspectCorrection.generic.type			= MTYPE_SPINCONTROL;
	s_videodata2.aspectCorrection.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_videodata2.aspectCorrection.generic.x				= x;
	s_videodata2.aspectCorrection.generic.y				= y;
	s_videodata2.aspectCorrection.generic.name			= GRAPHIC_BUTTONRIGHT;
	s_videodata2.aspectCorrection.generic.callback		= VideoData2_Event;
	s_videodata2.aspectCorrection.color					= CT_DKPURPLE1;
	s_videodata2.aspectCorrection.color2				= CT_LTPURPLE1;
	s_videodata2.aspectCorrection.textX					= MENU_BUTTON_TEXT_X;
	s_videodata2.aspectCorrection.textY					= MENU_BUTTON_TEXT_Y;
	s_videodata2.aspectCorrection.textEnum				= MBT_ASPECTCORRECTION;
	s_videodata2.aspectCorrection.textcolor				= CT_BLACK;
	s_videodata2.aspectCorrection.textcolor2			= CT_WHITE;
	s_videodata2.aspectCorrection.listnames				= s_OffOnNone_Names;

	y += inc;
	s_videodata2.centerHud_slider.generic.type		= MTYPE_SLIDER;
	s_videodata2.centerHud_slider.generic.x			= x + 162;
	s_videodata2.centerHud_slider.generic.y			= y;
	s_videodata2.centerHud_slider.generic.flags		= QMF_SMALLFONT;
	s_videodata2.centerHud_slider.generic.callback	= VideoData2_Event;
	s_videodata2.centerHud_slider.minvalue			= 0;
	s_videodata2.centerHud_slider.maxvalue			= 10;
	s_videodata2.centerHud_slider.color				= CT_DKPURPLE1;
	s_videodata2.centerHud_slider.color2			= CT_LTPURPLE1;
	s_videodata2.centerHud_slider.generic.name		= PIC_MONBAR2;
	s_videodata2.centerHud_slider.width				= 256;
	s_videodata2.centerHud_slider.height			= 32;
	s_videodata2.centerHud_slider.focusWidth		= 145;
	s_videodata2.centerHud_slider.focusHeight		= 18;
	s_videodata2.centerHud_slider.picName			= GRAPHIC_SQUARE;
	s_videodata2.centerHud_slider.picX				= x;
	s_videodata2.centerHud_slider.picY				= y;
	s_videodata2.centerHud_slider.picWidth			= MENU_BUTTON_MED_WIDTH + 21;
	s_videodata2.centerHud_slider.picHeight			= MENU_BUTTON_MED_HEIGHT;
	s_videodata2.centerHud_slider.textX				= MENU_BUTTON_TEXT_X;
	s_videodata2.centerHud_slider.textY				= MENU_BUTTON_TEXT_Y;
	s_videodata2.centerHud_slider.textEnum			= MBT_CENTERHUD;
	s_videodata2.centerHud_slider.textcolor			= CT_BLACK;
	s_videodata2.centerHud_slider.textcolor2		= CT_WHITE;
	s_videodata2.centerHud_slider.thumbName			= PIC_SLIDER;
	s_videodata2.centerHud_slider.thumbHeight		= 32;
	s_videodata2.centerHud_slider.thumbWidth		= 16;
	s_videodata2.centerHud_slider.thumbGraphicWidth= 9;
	s_videodata2.centerHud_slider.thumbColor		= CT_DKBLUE1;
	s_videodata2.centerHud_slider.thumbColor2		= CT_LTBLUE1;

	y += inc;
	s_videodata2.fov_slider.generic.type		= MTYPE_SLIDER;
	s_videodata2.fov_slider.generic.x			= x + 162;
	s_videodata2.fov_slider.generic.y			= y;
	s_videodata2.fov_slider.generic.flags		= QMF_SMALLFONT;
	s_videodata2.fov_slider.generic.callback	= VideoData2_Event;
	s_videodata2.fov_slider.minvalue			= 60;
	s_videodata2.fov_slider.maxvalue			= 120;
	s_videodata2.fov_slider.color				= CT_DKPURPLE1;
	s_videodata2.fov_slider.color2				= CT_LTPURPLE1;
	s_videodata2.fov_slider.generic.name		= PIC_MONBAR2;
	s_videodata2.fov_slider.width				= 256;
	s_videodata2.fov_slider.height				= 32;
	s_videodata2.fov_slider.focusWidth			= 145;
	s_videodata2.fov_slider.focusHeight			= 18;
	s_videodata2.fov_slider.picName				= GRAPHIC_SQUARE;
	s_videodata2.fov_slider.picX				= x;
	s_videodata2.fov_slider.picY				= y;
	s_videodata2.fov_slider.picWidth			= MENU_BUTTON_MED_WIDTH + 21;
	s_videodata2.fov_slider.picHeight			= MENU_BUTTON_MED_HEIGHT;
	s_videodata2.fov_slider.textX				= MENU_BUTTON_TEXT_X;
	s_videodata2.fov_slider.textY				= MENU_BUTTON_TEXT_Y;
	s_videodata2.fov_slider.textEnum			= MBT_FOV;
	s_videodata2.fov_slider.textcolor			= CT_BLACK;
	s_videodata2.fov_slider.textcolor2			= CT_WHITE;
	s_videodata2.fov_slider.thumbName			= PIC_SLIDER;
	s_videodata2.fov_slider.thumbHeight			= 32;
	s_videodata2.fov_slider.thumbWidth			= 16;
	s_videodata2.fov_slider.thumbGraphicWidth	= 9;
	s_videodata2.fov_slider.thumbColor			= CT_DKBLUE1;
	s_videodata2.fov_slider.thumbColor2			= CT_LTBLUE1;

	if ( !suppressViewSize ) {
		y += inc;
		s_videodata2.screensize_slider.generic.type		= MTYPE_SLIDER;
		s_videodata2.screensize_slider.generic.x		= x + 162;
		s_videodata2.screensize_slider.generic.y		= y;
		s_videodata2.screensize_slider.generic.flags	= QMF_SMALLFONT;
		s_videodata2.screensize_slider.generic.callback	= VideoData2_Event;
		s_videodata2.screensize_slider.minvalue			= 30;
		s_videodata2.screensize_slider.maxvalue			= 100;
		s_videodata2.screensize_slider.color			= CT_DKPURPLE1;
		s_videodata2.screensize_slider.color2			= CT_LTPURPLE1;
		s_videodata2.screensize_slider.generic.name		= PIC_MONBAR2;
		s_videodata2.screensize_slider.width			= 256;
		s_videodata2.screensize_slider.height			= 32;
		s_videodata2.screensize_slider.focusWidth		= 145;
		s_videodata2.screensize_slider.focusHeight		= 18;
		s_videodata2.screensize_slider.picName			= GRAPHIC_SQUARE;
		s_videodata2.screensize_slider.picX				= x;
		s_videodata2.screensize_slider.picY				= y;
		s_videodata2.screensize_slider.picWidth			= MENU_BUTTON_MED_WIDTH + 21;
		s_videodata2.screensize_slider.picHeight		= MENU_BUTTON_MED_HEIGHT;
		s_videodata2.screensize_slider.textX			= MENU_BUTTON_TEXT_X;
		s_videodata2.screensize_slider.textY			= MENU_BUTTON_TEXT_Y;
		s_videodata2.screensize_slider.textEnum			= MBT_SCREENSIZE;
		s_videodata2.screensize_slider.textcolor		= CT_BLACK;
		s_videodata2.screensize_slider.textcolor2		= CT_WHITE;
		s_videodata2.screensize_slider.thumbName		= PIC_SLIDER;
		s_videodata2.screensize_slider.thumbHeight		= 32;
		s_videodata2.screensize_slider.thumbWidth		= 16;
		s_videodata2.screensize_slider.thumbGraphicWidth= 9;
		s_videodata2.screensize_slider.thumbColor		= CT_DKBLUE1;
		s_videodata2.screensize_slider.thumbColor2		= CT_LTBLUE1;
	}

	y += inc;
	s_videodata2.flares.generic.type			= MTYPE_SPINCONTROL;
	s_videodata2.flares.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_videodata2.flares.generic.x				= x;
	s_videodata2.flares.generic.y				= y;
	s_videodata2.flares.generic.name			= GRAPHIC_BUTTONRIGHT;
	s_videodata2.flares.generic.callback		= VideoData2_Event;
	s_videodata2.flares.color					= CT_DKPURPLE1;
	s_videodata2.flares.color2					= CT_LTPURPLE1;
	s_videodata2.flares.textX					= MENU_BUTTON_TEXT_X;
	s_videodata2.flares.textY					= MENU_BUTTON_TEXT_Y;
	s_videodata2.flares.textEnum				= MBT_LIGHTFLARES;
	s_videodata2.flares.textcolor				= CT_BLACK;
	s_videodata2.flares.textcolor2				= CT_WHITE;
	s_videodata2.flares.listnames				= s_OffOnNone_Names;

	y += inc;
	s_videodata2.wallmarks.generic.type			= MTYPE_SPINCONTROL;
	s_videodata2.wallmarks.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_videodata2.wallmarks.generic.x			= x;
	s_videodata2.wallmarks.generic.y			= y;
	s_videodata2.wallmarks.generic.name			= GRAPHIC_BUTTONRIGHT;
	s_videodata2.wallmarks.generic.callback		= VideoData2_Event;
	s_videodata2.wallmarks.color				= CT_DKPURPLE1;
	s_videodata2.wallmarks.color2				= CT_LTPURPLE1;
	s_videodata2.wallmarks.textX				= MENU_BUTTON_TEXT_X;
	s_videodata2.wallmarks.textY				= MENU_BUTTON_TEXT_Y;
	s_videodata2.wallmarks.textEnum				= MBT_WALLMARKS1;
	s_videodata2.wallmarks.textcolor			= CT_BLACK;
	s_videodata2.wallmarks.textcolor2			= CT_WHITE;
	s_videodata2.wallmarks.listnames			= s_OffOnNone_Names;

	y += inc;
	s_videodata2.dynamiclights.generic.type			= MTYPE_SPINCONTROL;
	s_videodata2.dynamiclights.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_videodata2.dynamiclights.generic.x			= x;
	s_videodata2.dynamiclights.generic.y			= y;
	s_videodata2.dynamiclights.generic.name			= GRAPHIC_BUTTONRIGHT;
	s_videodata2.dynamiclights.generic.callback		= VideoData2_Event;
	s_videodata2.dynamiclights.color				= CT_DKPURPLE1;
	s_videodata2.dynamiclights.color2				= CT_LTPURPLE1;
	s_videodata2.dynamiclights.textX				= MENU_BUTTON_TEXT_X;
	s_videodata2.dynamiclights.textY				= MENU_BUTTON_TEXT_Y;
	s_videodata2.dynamiclights.textEnum				= MBT_DYNAMICLIGHTS1;
	s_videodata2.dynamiclights.textcolor			= CT_BLACK;
	s_videodata2.dynamiclights.textcolor2			= CT_WHITE;
	s_videodata2.dynamiclights.listnames			= s_OffOnNone_Names;
	
	y += inc;
	s_videodata2.simpleitems.generic.type			= MTYPE_SPINCONTROL;
	s_videodata2.simpleitems.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	s_videodata2.simpleitems.generic.x				= x;
	s_videodata2.simpleitems.generic.y				= y;
	s_videodata2.simpleitems.generic.name			= GRAPHIC_BUTTONRIGHT;
	s_videodata2.simpleitems.generic.callback		= VideoData2_Event;
	s_videodata2.simpleitems.color					= CT_DKPURPLE1;
	s_videodata2.simpleitems.color2					= CT_LTPURPLE1;
	s_videodata2.simpleitems.textX					= MENU_BUTTON_TEXT_X;
	s_videodata2.simpleitems.textY					= MENU_BUTTON_TEXT_Y;
	s_videodata2.simpleitems.textEnum				= MBT_SIMPLEITEMS;
	s_videodata2.simpleitems.textcolor				= CT_BLACK;
	s_videodata2.simpleitems.textcolor2				= CT_WHITE;
	s_videodata2.simpleitems.listnames				= s_OffOnNone_Names;

	y += inc;
	s_videodata2.synceveryframe.generic.type		= MTYPE_SPINCONTROL;
	s_videodata2.synceveryframe.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS;
	s_videodata2.synceveryframe.generic.x			= x;
	s_videodata2.synceveryframe.generic.y			= y;
	s_videodata2.synceveryframe.generic.name		= GRAPHIC_BUTTONRIGHT;
	s_videodata2.synceveryframe.generic.callback	= VideoData2_Event;
	s_videodata2.synceveryframe.color				= CT_DKPURPLE1;
	s_videodata2.synceveryframe.color2				= CT_LTPURPLE1;
	s_videodata2.synceveryframe.textX				= MENU_BUTTON_TEXT_X;
	s_videodata2.synceveryframe.textY				= MENU_BUTTON_TEXT_Y;
	s_videodata2.synceveryframe.textEnum			= MBT_SYNCEVERYFRAME1;
	s_videodata2.synceveryframe.textcolor			= CT_BLACK;
	s_videodata2.synceveryframe.textcolor2			= CT_WHITE;
	s_videodata2.synceveryframe.listnames			= s_OffOnNone_Names;

	if ( !videoEngineConfig.anisotropicFilteringVideoMenu ) {
		y += inc;
		s_videodata2.anisotropic.generic.type			= MTYPE_SPINCONTROL;
		s_videodata2.anisotropic.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
		s_videodata2.anisotropic.generic.x				= x;
		s_videodata2.anisotropic.generic.y				= y;
		s_videodata2.anisotropic.generic.name			= GRAPHIC_BUTTONRIGHT;
		s_videodata2.anisotropic.generic.callback		= VideoData2_Event;
		s_videodata2.anisotropic.color					= CT_DKPURPLE1;
		s_videodata2.anisotropic.color2					= CT_LTPURPLE1;
		s_videodata2.anisotropic.textX					= MENU_BUTTON_TEXT_X;
		s_videodata2.anisotropic.textY					= MENU_BUTTON_TEXT_Y;
		s_videodata2.anisotropic.textEnum				= MBT_VIDEO_ANISOTROPIC_LEVEL;
		s_videodata2.anisotropic.textcolor				= CT_BLACK;
		s_videodata2.anisotropic.textcolor2				= CT_WHITE;
		s_videodata2.anisotropic.listnames				= videoEngineConfig.supportMaxAnisotropy ? s_anisotropic_Names : s_OffOnNone_Names;
	}


	Menu_AddItem( &s_videodata2.menu, ( void * )&s_videodata2.aspectCorrection);
	Menu_AddItem( &s_videodata2.menu, ( void * )&s_videodata2.centerHud_slider);
	Menu_AddItem( &s_videodata2.menu, ( void * )&s_videodata2.fov_slider);
	if ( !suppressViewSize ) {
		Menu_AddItem( &s_videodata2.menu, ( void * )&s_videodata2.screensize_slider);
	}
	Menu_AddItem( &s_videodata2.menu, ( void * )&s_videodata2.flares);
	Menu_AddItem( &s_videodata2.menu, ( void * )&s_videodata2.wallmarks);
	Menu_AddItem( &s_videodata2.menu, ( void * )&s_videodata2.dynamiclights);
	Menu_AddItem( &s_videodata2.menu, ( void * )&s_videodata2.simpleitems);
	Menu_AddItem( &s_videodata2.menu, ( void * )&s_videodata2.synceveryframe);
	if ( !videoEngineConfig.anisotropicFilteringVideoMenu ) {
		Menu_AddItem( &s_videodata2.menu, ( void * )&s_videodata2.anisotropic);
	}

}

/*
=================
UI_VideoData2SettingsGetCurrentFov
=================
*/
static float UI_VideoData2SettingsGetCurrentFov( void )
{
	float fov;
	char buffer[256];

	trap_Cvar_VariableStringBuffer( "cg_fov", buffer, sizeof( buffer ) );
	fov = atof( buffer );

	if ( !strchr( buffer, '*' ) ) {
		// Convert existing unscaled fov to scaled version, which is what is used by UI.
		float x = uis.glconfig.vidWidth / tan( fov / 360 * M_PI );
		float fov_y = atan2( uis.glconfig.vidHeight, x );
		fov_y = fov_y * 360 / M_PI;

		x = 480.0 / tan( fov_y / 360 * M_PI );
		fov = atan2( 640.0, x );
		fov = fov * 360 / M_PI;
	}

	return fov;
}

/*
=================
UI_VideoData2SettingsGetCurrentCenterHud
=================
*/
static float UI_VideoData2SettingsGetCurrentCenterHud( void )
{
	float value;
	char buffer[256];

	trap_Cvar_VariableStringBuffer( "cg_aspectCorrectCenterHud", buffer, sizeof( buffer ) );
	value = atof( buffer );

	if ( strchr( buffer, '*' ) ) {
		// Convert aspect-ratio specifier to fraction.
		if ( uis.glconfig.vidWidth * 3 > uis.glconfig.vidHeight * 4 ) {
			float screenAspect = (float)uis.glconfig.vidWidth / (float)uis.glconfig.vidHeight;
			float max = screenAspect - 4.0f / 3.0f;		// available horizontal space
			float specified = screenAspect - value;		// horizontal space specified to use
			value = max > 0.01f ? specified / max : 1.0f;
		} else {
			float screenAspect = (float)uis.glconfig.vidHeight / (float)uis.glconfig.vidWidth;
			float max = screenAspect - 3.0f / 4.0f;		// available vertical space
			float specified = screenAspect - value;		// vertical space specified to use
			value = max > 0.01f ? specified / max : 1.0f;
		}
	}

	if ( value < 0.0f )
		value = 0.0f;
	if ( value > 1.0f )
		value = 1.0f;

	return value;
}

/*
=================
UI_VideoData2SettingsGetCvars
=================
*/
static void	UI_VideoData2SettingsGetCvars()
{
	s_videodata2.aspectCorrection.curvalue = trap_Cvar_VariableValue( "cg_aspectCorrect" ) ? 1 : 0;
	s_videodata2.centerHud_slider.curvalue = UI_VideoData2SettingsGetCurrentCenterHud() * 10.0f;
	s_videodata2.fov_slider.curvalue = UI_VideoData2SettingsGetCurrentFov();
	s_videodata2.screensize_slider.curvalue = trap_Cvar_VariableValue( "cg_viewsize" );
	s_videodata2.flares.curvalue = trap_Cvar_VariableValue( "r_flares" ) ? 1 : 0;
	s_videodata2.wallmarks.curvalue = trap_Cvar_VariableValue( "cg_marks" ) ? 1 : 0;
	s_videodata2.dynamiclights.curvalue = trap_Cvar_VariableValue( "r_dynamiclight" ) ? 1 : 0;
	s_videodata2.simpleitems.curvalue = trap_Cvar_VariableValue( "cg_simpleItems" ) ? 1 : 0;
	s_videodata2.synceveryframe.curvalue = trap_Cvar_VariableValue( "r_finish" ) ? 1 : 0;
	s_videodata2.anisotropic.curvalue = UI_GetAnisotropyLevel();
}

/*
=================
UI_VideoData2SettingsMenu
=================
*/
void UI_VideoData2SettingsMenu( void )
{
	UI_VideoData2SettingsGetCvars();

	if (!s_videodata2.menu.initialized)
	{
		VideoData2_MenuInit();
	}

	VideoData2_EnabledDisableCenterHudSlider();

	UI_PushMenu( &s_videodata2.menu );
}

