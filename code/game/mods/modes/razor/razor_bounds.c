/*
* Razor Bounds
* 
* Determines when a player hits the map edge.
*/

#define MOD_NAME ModRazorBounds

#include "mods/modes/razor/razor_local.h"

typedef enum {
	RMM_BASIC,			// simple SURF_SKY check like original Pinball mod
	RMM_STANDARD,		// default WMT + VOT check for better map compatibility
	RMM_VOT_ONLY,		// VOT check only; good for certain maps like "hm_kba"
	RMM_MAP_MATRIX,		// special case for "matrix" map
} razorMapMode_t;

static struct {
	trackedCvar_t g_razor_boundsMode;		// Use enhanced bounds calculation. 0 = disabled, 1 = enabled (default: 1)
	trackedCvar_t g_razor_boundsDebug;		// Enable debug prints. 0 = disabled, 1 = enabled (default: 0)

	razorMapMode_t mode;
	float ceiling;
	float votRange;
	qboolean strictCeiling;
} *MOD_STATE;

#define BOUNDS_DEBUG_PRINT( args ) if ( MOD_STATE->g_razor_boundsDebug.integer ) G_Printf args
#define CEILING_TRACE_RANGE 65536.0f
#define WMT_TRACE_RANGE 256.0f

/*
================
ModRazorBounds_CalcMapCeiling

Determine Z coordinate of map ceiling.
================
*/
static float ModRazorBounds_CalcMapCeiling( void ) {
	int i;
	trace_t tr;
	vec3_t traceEnd;
	float ceiling = -CEILING_TRACE_RANGE;
	const char *classnames[] = { "info_player_deathmatch", "team_CTF_redplayer", "team_CTF_redspawn",
			"team_CTF_blueplayer", "team_CTF_bluespawn", NULL };
	const char **classname;

	for ( i = 0; i < MAX_GENTITIES; ++i ) {
		const gentity_t *ent = &g_entities[i];
		if ( ent->inuse ) {
			qboolean match = qfalse;

			// Search both item and player spawn points
			if ( ent->item ) {
				match = qtrue;
			} else {
				for ( classname = classnames; *classname; ++classname ) {
					if ( ent->classname && !Q_stricmp( ent->classname, *classname ) ) {
						match = qtrue;
					}
				}
			}

			if ( match ) {
				VectorSet( traceEnd, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] + CEILING_TRACE_RANGE );
				trap_Trace( &tr, ent->s.origin, NULL, NULL, traceEnd, -1, MASK_PLAYERSOLID );
				if ( tr.endpos[2] > ceiling ) {
					ceiling = tr.endpos[2];
				}
			}
		}
	}

	return ceiling;
}

/*
================
ModRazorBounds_VerticalOffsetTest

Performs a test trace at a certain height (typically the top of the map). Returns true if
the test trace does not go much further than the original trace (less than maxDistance).

   ---*   <- offset trace (low or zero distance means more likely main trace hit map edge)
   |
   |
   |
---*   <- original trace
================
*/
static qboolean ModRazorBounds_VerticalOffsetTest( const vec3_t start, const vec3_t normal,
		const vec3_t mins, const vec3_t maxs, float height, float maxDistance, int passEntityNum ) {
	trace_t tr;
	vec3_t traceStart;
	vec3_t traceEnd;
	float distance;

	VectorSet( traceStart, start[0], start[1], height );
	VectorMA( traceStart, -1024.0f, normal, traceEnd );
	trap_Trace( &tr, traceStart, mins, maxs, traceEnd, passEntityNum, MASK_PLAYERSOLID );
	if ( tr.startsolid ) {
		BOUNDS_DEBUG_PRINT(( "razor bounds: VOT miss due to startsolid\n" ));
		return qfalse;
	}

	distance = tr.fraction * 1024.0f;
	if ( distance <= maxDistance ) {
		BOUNDS_DEBUG_PRINT(( "razor bounds: VOT pass with distance %f\n", distance ));
		return qtrue;
	}
	BOUNDS_DEBUG_PRINT(( "razor bounds: VOT miss with distance %f\n", distance ));
	return qfalse;
}

/*
================
ModRazorBounds_WeaponMaskTest

Performs a test trace past the plane that was hit, using MASK_SHOT contents and checking
for either SURF_SKY, SURF_NOIMPACT, or empty space.
================
*/
static qboolean ModRazorBounds_WeaponMaskTest( const vec3_t start, const vec3_t normal,
		const vec3_t mins, const vec3_t maxs, int passEntityNum ) {
	trace_t tr;
	vec3_t newEnd;

	VectorMA( start, -WMT_TRACE_RANGE, normal, newEnd );
	trap_Trace( &tr, start, mins, maxs, newEnd, passEntityNum, MASK_SHOT );
	if ( tr.surfaceFlags & ( SURF_SKY | SURF_NOIMPACT ) || ( tr.fraction == 1.0f && !tr.startsolid ) ) {
		BOUNDS_DEBUG_PRINT(( "razor bounds: WMT hit with distance %f\n", WMT_TRACE_RANGE * tr.fraction ));
		return qtrue;
	} else {
		BOUNDS_DEBUG_PRINT(( "razor bounds: WMT miss with distance %f\n", WMT_TRACE_RANGE * tr.fraction ));
		return qfalse;
	}
}

/*
================
ModRazorBounds_Static_CheckTraceHit

Returns qtrue if player movement trace encountered map edge.
================
*/
qboolean ModRazorBounds_Static_CheckTraceHit( const trace_t *results, const vec3_t start,
		const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask ) {
	if ( results->surfaceFlags & SURF_SKY ) {
		BOUNDS_DEBUG_PRINT(( "razor bounds: Hit due to direct SURF_SKY\n" ));
		return qtrue;
	}

	if ( MOD_STATE && MOD_STATE->g_razor_boundsMode.integer && results->fraction != 1.0f && !results->allsolid &&
			EF_WARN_ASSERT( VectorLengthSquared( results->plane.normal ) > 0.99f ) ) {
		float adjustedCeiling = MOD_STATE->ceiling - maxs[2] - 1.0f;

		if ( MOD_STATE->strictCeiling && results->endpos[2] > adjustedCeiling ) {
			BOUNDS_DEBUG_PRINT(( "razor bounds: Hit due to strict ceiling - endpos(%f) adjustedCeiling(%f)\n",
					results->endpos[2], adjustedCeiling ));
			return qtrue;
		}

		if ( MOD_STATE->mode == RMM_STANDARD &&
				( results->surfaceFlags & SURF_NOIMPACT ) &&
				results->plane.normal[2] != 1.0f &&		// ignore flat ground
				( results->endpos[2] > adjustedCeiling || results->plane.normal[2] == 0.0f ) &&		// optimize
				ModRazorBounds_WeaponMaskTest( results->endpos, results->plane.normal, mins, maxs, passEntityNum ) ) {
			if ( results->endpos[2] > adjustedCeiling ) {
				BOUNDS_DEBUG_PRINT(( "razor bounds: Hit due to WMT + ceiling - endpos(%f) adjustedCeiling(%f)\n",
						results->endpos[2], adjustedCeiling ));
				return qtrue;
			}
			if ( results->plane.normal[2] == 0.0f && ModRazorBounds_VerticalOffsetTest( results->endpos,
					results->plane.normal, mins, maxs, adjustedCeiling, MOD_STATE->votRange, passEntityNum ) ) {
				BOUNDS_DEBUG_PRINT(( "razor bounds: Hit due to WMT + VOT\n" ));
				return qtrue;
			}
		}

		if ( MOD_STATE->mode == RMM_VOT_ONLY && results->plane.normal[2] == 0.0f &&
				ModRazorBounds_VerticalOffsetTest( results->endpos, results->plane.normal,
				mins, maxs, adjustedCeiling, MOD_STATE->votRange, passEntityNum ) ) {
			BOUNDS_DEBUG_PRINT(( "razor bounds: Hit due to VOT only mode\n" ));
			return qtrue;
		}

		if ( MOD_STATE->mode == RMM_MAP_MATRIX && results->plane.normal[2] == 0.0f ) {
			vec3_t start;
			vec3_t end;
			trace_t tr;

			VectorMA( results->endpos, -32.0f, results->plane.normal, start );
			VectorMA( start, -65536.0f, results->plane.normal, end );
			trap_Trace( &tr, start, mins, maxs, end, passEntityNum, contentMask );

			BOUNDS_DEBUG_PRINT(( "razor bounds: Matrix trace - fraction(%f) startsolid(%i) hitSky(%i)\n",
					tr.fraction, tr.startsolid, ( tr.surfaceFlags & SURF_SKY ) ? 1 : 0 ));
			if ( tr.surfaceFlags & SURF_SKY ) {
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
================
ModRazorBounds_VerifyMapDescription

Verify description matches current map, as an extra check to avoid map name conflicts.
================
*/
static qboolean ModRazorBounds_VerifyMapDescription( const char *description ) {
	char buffer[256];
	trap_GetConfigstring( CS_MESSAGE, buffer, sizeof( buffer ) );
	return !Q_stricmp( buffer, description ) ? qtrue : qfalse;
}

/*
================
ModRazorBounds_InitConfig

Load map-specific configuration.
================
*/
static void ModRazorBounds_InitConfig( void ) {
	const char *mapname = G_ModUtils_GetMapName();

	MOD_STATE->votRange = 32.0f;

	if ( !Q_stricmp( mapname, "ctf_voy1" ) || !Q_stricmp( mapname, "heffdm17" ) ) {
		// Disable extra checks on some maps that don't need it, to be safe.
		BOUNDS_DEBUG_PRINT(( "razor bounds: Using basic profile for map '%s'\n", mapname ));
		MOD_STATE->mode = RMM_BASIC;
	} else if ( !Q_stricmp( mapname, "hm_kba" ) || !Q_stricmp( mapname, "forestfrag" ) ) {
		BOUNDS_DEBUG_PRINT(( "razor bounds: Using VOT only profile for map '%s'\n", mapname ));
		MOD_STATE->mode = RMM_VOT_ONLY;
		MOD_STATE->strictCeiling = qtrue;
	} else if ( !Q_stricmp( mapname, "ctf_akilo_f4g" ) || !Q_stricmp( mapname, "ctf_fsceleg" ) ||
			!Q_stricmp( mapname, "skunkysbitch" ) ) {
		BOUNDS_DEBUG_PRINT(( "razor bounds: Using special profile for map '%s'\n", mapname ));
		MOD_STATE->mode = RMM_STANDARD;
		MOD_STATE->votRange = 256.0f;
	} else if ( !Q_stricmp( mapname, "matrix" ) && ModRazorBounds_VerifyMapDescription( "Loading Jump Program" ) ) {
		BOUNDS_DEBUG_PRINT(( "razor bounds: Using special profile for map '%s'\n", mapname ));
		MOD_STATE->mode = RMM_MAP_MATRIX;
	} else {
		BOUNDS_DEBUG_PRINT(( "razor bounds: Using standard profile\n" ));
		MOD_STATE->mode = RMM_STANDARD;
	}

	if ( MOD_STATE->mode != RMM_BASIC ) {
		MOD_STATE->ceiling = ModRazorBounds_CalcMapCeiling();
		BOUNDS_DEBUG_PRINT(( "razor bounds: Calculated map ceiling %f\n", MOD_STATE->ceiling ));
	}
}

/*
================
(ModFN) GeneralInit
================
*/
static void MOD_PREFIX(GeneralInit)( MODFN_CTV ) {
	MODFN_NEXT( GeneralInit, ( MODFN_NC ) );
	ModRazorBounds_InitConfig();
}

/*
================
ModRazorBounds_Init
================
*/
void ModRazorBounds_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		G_RegisterTrackedCvar( &MOD_STATE->g_razor_boundsMode, "g_razor_boundsMode", "1", 0, qfalse );
		G_RegisterTrackedCvar( &MOD_STATE->g_razor_boundsDebug, "g_razor_boundsDebug", "0", 0, qfalse );

		MODFN_REGISTER( GeneralInit, ++modePriorityLevel );
	}
}
