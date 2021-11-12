#include "ui_local.h"


//===================================================================
//
// Functions to set Cvars from menus
//
//===================================================================


/*
=================
InvertMouseCallback
=================
*/
void InvertMouseCallback( void *s, int notification )
{
	menulist_s *box = (menulist_s *) s;

	if (notification != QM_ACTIVATED)
		return;

	if ( box->curvalue )
		trap_Cvar_SetValue( "m_pitch", -fabs( trap_Cvar_VariableValue( "m_pitch" ) ) );
	else
		trap_Cvar_SetValue( "m_pitch", fabs( trap_Cvar_VariableValue( "m_pitch" ) ) );

}

/*
=================
MouseSpeedCallback
=================
*/
void MouseSpeedCallback( void *s, int notification )
{
	menuslider_s	*slider = (menuslider_s *) s;

	if (notification != QM_ACTIVATED)
		return;

	trap_Cvar_SetValue( "sensitivity", slider->curvalue );
}

/*
=================
SmoothMouseCallback
=================
*/
void SmoothMouseCallback( void *s, int notification )
{
	menulist_s *box = (menulist_s *) s;

	if (notification != QM_ACTIVATED)
		return;

	trap_Cvar_SetValue( "m_filter", box->curvalue );
}

/*
=================
GammaCallback
=================
*/
void GammaCallback( void *s, int notification )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	if (notification != QM_ACTIVATED)
		return;

	trap_Cvar_SetValue( "r_gamma", slider->curvalue / 10.0f );
}


/*
=================
AlwaysRunCallback
=================
*/
void AlwaysRunCallback( void *s, int notification )
{
	menulist_s *s_alwaysrun_box = ( menulist_s * ) s;

	if (notification != QM_ACTIVATED)
		return;

	trap_Cvar_SetValue( "cl_run", s_alwaysrun_box->curvalue );
}


/*
=================
AutoswitchCallback
=================
*/
void AutoswitchCallback( void *unused, int notification )
{
	static menulist_s	s_autoswitch_box;

	if (notification != QM_ACTIVATED)
		return;

	trap_Cvar_SetValue( "cg_autoswitch", s_autoswitch_box.curvalue );
}


/*
=================
JoyXButtonCallback
=================
*/
void JoyXButtonCallback( void *s, int notification )
{
	menulist_s *box = (menulist_s *) s;

	if (notification != QM_ACTIVATED)
		return;

	trap_Cvar_SetValue( "joy_xbutton", box->curvalue );
}

/*
=================
JoyYButtonCallback
=================
*/
void JoyYButtonCallback( void *s, int notification )
{
	menulist_s *box = (menulist_s *) s;

	if (notification != QM_ACTIVATED)
		return;

	trap_Cvar_SetValue( "joy_ybutton", box->curvalue );
}
