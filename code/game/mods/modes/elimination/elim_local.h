#include "mods/g_mod_local.h"

void ModElimMisc_Init( void );
void ModElimTimelimit_Init( void );
void ModElimTweaks_Init( void );
void ModElimMultiRound_Init( void );

int ModElimination_Shared_CountPlayersAliveTeam( team_t team, int ignoreClientNum );
qboolean ModElimination_Static_IsPlayerEliminated( int clientNum );
