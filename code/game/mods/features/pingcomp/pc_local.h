/*
* Ping Compensation Header
* 
* Definitions shared only between ping compensation modules.
*/

#include "mods/g_mod_local.h"

// Initializations
void ModPCPositionShift_Init( void );
void ModPCDeadMove_Init( void );
void ModPCInstantWeapons_Init( void );
void ModPCProjectileImpact_Init( void );
void ModPCProjectileLaunch_Init( void );
void ModPCSmoothing_Init( void );
void ModPCSmoothingDebug_Init( void );
void ModPCSmoothingOffset_Init( void );
void ModPCClientPredict_Init( void );

// Main component - pc_main.c
qboolean ModPingcomp_Static_ProjectileCompensationEnabled( void );
qboolean ModPingcomp_Static_SmoothingEnabled( void );
qboolean ModPingcomp_Static_SmoothingEnabledForClient( int clientNum );
qboolean ModPingcomp_Static_PositionShiftEnabled( void );

// Position shifting - pc_position_shift.c
#define POSITION_SHIFT_MAX_CLIENTS 32

typedef struct {
	// 0 = client not shifted
	int shiftTime[POSITION_SHIFT_MAX_CLIENTS];
} positionShiftState_t;

void ModPCPositionShift_Shared_TimeShiftClient( int clientNum, int time );
void ModPCPositionShift_Shared_TimeShiftClients( int ignoreClient, int time );
positionShiftState_t ModPCPositionShift_Shared_GetShiftState( void );
void ModPCPositionShift_Shared_SetShiftState( positionShiftState_t *shiftState );

// Smoothing - pc_smoothing.c
#define MAX_SMOOTHING_CLIENTS 32

typedef struct {
	char mins[3];
	char maxs[3];
} Smoothing_ShiftInfo_t;

qboolean ModPCSmoothing_Static_ShiftClient( int clientNum, Smoothing_ShiftInfo_t *info_out );

// Smoothing debug - pc_smoothing_debug.c
void ModPCSmoothingDebug_Static_LogFrame( int clientNum, int targetTime, int resultTime );

// Smoothing offset calc - pc_smoothing_offset.c
int ModPCSmoothingOffset_Shared_GetOffset( int clientNum );

// Smoothing dead move - pc_dead_move.c
qboolean ModPCDeadMove_Static_DeadMoveActive( int clientNum );
qboolean ModPCDeadMove_Static_ShiftClient( int clientNum, Smoothing_ShiftInfo_t *info_out );
void ModPCDeadMove_Static_InitDeadMove( int clientNum, const vec3_t origin, const vec3_t velocity );
