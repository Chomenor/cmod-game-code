#include "mods/g_mod_local.h"

void ModRazorBounds_Init( void );
void ModRazorDamage_Init( void );
void ModRazorDifficulty_Init( void );
void ModRazorGreeting_Init( void );
void ModRazorItems_Init( void );
void ModRazorPowerups_Init( void );
void ModRazorScoring_Init( void );
void ModRazorSounds_Init( void );
void ModRazorWeapons_Init( void );

qboolean ModRazorBounds_Static_CheckTraceHit( const trace_t *results, const vec3_t start,
		const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
const char *ModRazorDifficulty_Static_DifficultyString( void );
int ModRazorScoring_Static_LastPusher( int clientNum );
