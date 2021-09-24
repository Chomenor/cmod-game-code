#ifdef MODULE_CGAME
#include "cg_local.h"
#endif
#ifdef MODULE_UI
#include "ui_local.h"
#endif

#define CLAMP_VALUE( value, min, max ) ( value < min ? min : ( value > max ? max : value ) )

// Determines aspect correction support for UI and loading screen, but not cgame
#define UI_ASPECT_CORRECT_ENABLED ( uix.ui_aspectCorrect.integer >= 0 ? uix.ui_aspectCorrect.integer : uix.cg_aspectCorrect.integer )

// Name of cvars used to coordinate loading screen support between cgame and ui
#define SUPPORT_CVAR_UI "_ui_AspectScalingSupport"
#define SUPPORT_CVAR_CGAME "_cg_AspectScalingSupport"

#ifdef MODULE_CGAME
#define ASPECT_CORRECT_ENABLED uix.cg_aspectCorrect.integer
#endif
#ifdef MODULE_UI
#define ASPECT_CORRECT_ENABLED UI_ASPECT_CORRECT_ENABLED
#endif

static struct {
	// Screen dimensions
	float width;
	float height;

	// Cvars
	vmCvar_t cg_aspectCorrect;	// enables aspect correction for cgame and hud (0 = disabled, 1 = enabled)
#ifdef MODULE_CGAME
	vmCvar_t cg_aspectCorrectGunPos;	// enables gun fov correction (0 = disabled, 1 = enabled, -1 = use cg_aspectCorrect value)
	vmCvar_t cg_aspectCorrectCenterHud;		// moves hud elements from edge towards center of screen (0 = disabled, 1 = max)
#endif
	vmCvar_t ui_aspectCorrect;	// enables aspect correction for ui and loading screen (0 = disabled, 1 = enabled, -1 = use cg_aspectCorrect value)

	// Current mode
	uiHorizontalMode_t hModeCurrent;
	uiVerticalMode_t vModeCurrent;

	//
	// Everything below this point is set by AspectCorrect_UpdateValues
	//

#ifdef MODULE_CGAME
	int centerHudModificationCount;
#endif

	// Offset from edges of the screen in real pixels to reach the center 4:3 region
	float XCenterOffset;
	float YCenterOffset;

	// Offset from edges of the screen in real pixels to display edge-aligned graphics
	float XSideOffset;
	float YSideOffset;

	// Factor to convert from 640x480 virtual pixels to real pixels when drawing aspect-corrected graphics
	float XScaledFactor;
	float YScaledFactor;

	// Factor to convert from 640x480 virtual pixels to real pixels when drawing stretched graphics
	float XStretchFactor;
	float YStretchFactor;
} uix;

/*
================
AspectCorrect_UpdateValues
================
*/
void AspectCorrect_UpdateValues( void ) {
#ifdef MODULE_CGAME
	uix.centerHudModificationCount = uix.cg_aspectCorrectCenterHud.modificationCount;
#endif

	uix.XStretchFactor = uix.width / 640.0f;
	uix.YStretchFactor = uix.height / 480.0f;

	if ( uix.width * 3 > uix.height * 4 ) {
		// wide screen
		uix.XScaledFactor = uix.YStretchFactor;
		uix.YScaledFactor = uix.YStretchFactor;
		uix.XCenterOffset = 320.0f * ( uix.XStretchFactor - uix.XScaledFactor );
		uix.YCenterOffset = 0.0f;

#ifdef MODULE_CGAME
		uix.XSideOffset = uix.XCenterOffset * CLAMP_VALUE( uix.cg_aspectCorrectCenterHud.value, 0.0f, 1.0f );
		uix.YSideOffset = 0;
#endif
	} else {
		// narrow screen
		uix.XScaledFactor = uix.XStretchFactor;
		uix.YScaledFactor = uix.XStretchFactor;
		uix.XCenterOffset = 0.0f;
		uix.YCenterOffset = 240.0f * ( uix.YStretchFactor - uix.YScaledFactor );

#ifdef MODULE_CGAME
		uix.YSideOffset = uix.YCenterOffset * CLAMP_VALUE( uix.cg_aspectCorrectCenterHud.value, 0.0f, 1.0f );
		uix.XSideOffset = 0;
#endif
	}
}

/*
================
AspectCorrect_SetMode

Configure the display mode for standard cgame/ui drawing.
================
*/
void AspectCorrect_SetMode( uiHorizontalMode_t hMode, uiVerticalMode_t vMode ) {
	if ( ASPECT_CORRECT_ENABLED ) {
		uix.hModeCurrent = hMode;
		uix.vModeCurrent = vMode;
	} else {
		uix.hModeCurrent = HSCALE_STRETCH;
		uix.vModeCurrent = VSCALE_STRETCH;
	}
}

/*
================
AspectCorrect_SetLoadingMode

Configure the display mode for loading screen drawing.
================
*/
void AspectCorrect_SetLoadingMode( uiHorizontalMode_t hMode, uiVerticalMode_t vMode, qboolean overlay ) {
	// Use UI cvar to determine whether loading screen scaling is enabled, regardless
	// of whether the draw operation originates from ui or cgame.
	qboolean scalingEnabled = UI_ASPECT_CORRECT_ENABLED ? qtrue : qfalse;

	if ( scalingEnabled && overlay ) {
		// If both cgame and ui are drawing to screen, verify both support scaling.
		char buf1[256];
		char buf2[256];
		trap_Cvar_VariableStringBuffer( SUPPORT_CVAR_UI, buf1, sizeof( buf1 ) );
		trap_Cvar_VariableStringBuffer( SUPPORT_CVAR_CGAME, buf2, sizeof( buf2 ) );
		if ( buf1[0] != '1' || buf2[0] != '1' ) {
			scalingEnabled = qfalse;
		}
	}

	if ( scalingEnabled ) {
		uix.hModeCurrent = hMode;
		uix.vModeCurrent = vMode;
	} else {
		uix.hModeCurrent = HSCALE_STRETCH;
		uix.vModeCurrent = VSCALE_STRETCH;
	}
}

/*
================
AspectCorrect_ResetMode
================
*/
void AspectCorrect_ResetMode( void ) {
	AspectCorrect_SetMode( HSCALE_CENTER, VSCALE_CENTER );
}

/*
================
AspectCorrect_AdjustFrom640
================
*/
void AspectCorrect_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	if ( uix.hModeCurrent == HSCALE_STRETCH ) {
		*x *= uix.XStretchFactor;
		*w *= uix.XStretchFactor;
	} else if ( uix.hModeCurrent == HSCALE_CENTER ) {
		*x = uix.XCenterOffset + *x * uix.XScaledFactor;
		*w *= uix.XScaledFactor;
	} else {
		if ( uix.hModeCurrent == HSCALE_LEFT ) {
			*x = *x * uix.XScaledFactor + uix.XSideOffset;
		} else {
			*x = uix.width - ( 640.0f - *x ) * uix.XScaledFactor - uix.XSideOffset;
		}
		*w *= uix.XScaledFactor;
	}

	if ( uix.vModeCurrent == VSCALE_STRETCH ) {
		*y *= uix.YStretchFactor;
		*h *= uix.YStretchFactor;
	} else if ( uix.vModeCurrent == VSCALE_CENTER ) {
		*y = uix.YCenterOffset + *y * uix.YScaledFactor;
		*h *= uix.YScaledFactor;
	} else {
		if ( uix.vModeCurrent == VSCALE_TOP ) {
			*y = *y * uix.YScaledFactor + uix.YSideOffset;
		} else {
			*y = uix.height - ( 480.0f - *y ) * uix.YScaledFactor - uix.YSideOffset;
		}
		*h *= uix.YScaledFactor;
	}
}

/*
================
AspectCorrect_DrawAdjustedStretchPic
================
*/
void AspectCorrect_DrawAdjustedStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	AspectCorrect_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}

#ifdef MODULE_CGAME
/*
================
AspectCorrect_UseCorrectedGunPosition
================
*/
qboolean AspectCorrect_UseCorrectedGunPosition( void ) {
	return uix.cg_aspectCorrectGunPos.integer >= 0 ? uix.cg_aspectCorrectGunPos.integer : uix.cg_aspectCorrect.integer;
}
#endif

/*
================
AspectCorrect_RunFrame
================
*/
void AspectCorrect_RunFrame( void ) {
	trap_Cvar_Update( &uix.cg_aspectCorrect );
#ifdef MODULE_CGAME
	trap_Cvar_Update( &uix.cg_aspectCorrectGunPos );
	trap_Cvar_Update( &uix.cg_aspectCorrectCenterHud );
#endif
	trap_Cvar_Update( &uix.ui_aspectCorrect );

#ifdef MODULE_CGAME
	if ( uix.centerHudModificationCount != uix.cg_aspectCorrectCenterHud.modificationCount ) {
		AspectCorrect_UpdateValues();
	}
#endif
}

/*
================
AspectCorrect_Shutdown
================
*/
void AspectCorrect_Shutdown( void ) {
#ifdef MODULE_UI
	trap_Cvar_Set( SUPPORT_CVAR_UI, "0" );
#endif
#ifdef MODULE_CGAME
	trap_Cvar_Set( SUPPORT_CVAR_CGAME, "0" );
#endif
}

/*
================
AspectCorrect_Init

Should be called before any drawing calls are issued.
================
*/
void AspectCorrect_Init( int width, int height ) {
	vmCvar_t temp;

	trap_Cvar_Register( &uix.cg_aspectCorrect, "cg_aspectCorrect", "0", CVAR_ARCHIVE );
#ifdef MODULE_CGAME
	trap_Cvar_Register( &uix.cg_aspectCorrectGunPos, "cg_aspectCorrectGunPos", "-1", CVAR_ARCHIVE );
	trap_Cvar_Register( &uix.cg_aspectCorrectCenterHud, "cg_aspectCorrectCenterHud", "0", CVAR_ARCHIVE );
#endif
	trap_Cvar_Register( &uix.ui_aspectCorrect, "ui_aspectCorrect", "-1", CVAR_ARCHIVE );

	uix.width = width;
	uix.height = height;
	AspectCorrect_UpdateValues();
	AspectCorrect_ResetMode();

	// Set cvars used to determine support for scaling during the loading screen.
	// Both ui and cgame modules draw to the screen during loading, so scaling should
	// only be enabled if both modules support it.
#ifdef MODULE_UI
	trap_Cvar_Register( &temp, SUPPORT_CVAR_UI, "1", CVAR_ROM );
	trap_Cvar_Set( SUPPORT_CVAR_UI, "1" );
#endif
#ifdef MODULE_CGAME
	trap_Cvar_Register( &temp, SUPPORT_CVAR_CGAME, "1", CVAR_ROM );
	trap_Cvar_Set( SUPPORT_CVAR_CGAME, "1" );
#endif
}
