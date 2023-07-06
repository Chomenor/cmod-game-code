#include "mods/g_mod_local.h"

void ModUAMAmmo_Init( void );
void ModUAMDeathEffects_Init( void );
void ModUAMGreeting_Init( void );
void ModUAMInstagib_Init( void );
void ModUAMInstagibSaveWep_Init( void );
void ModUAMMusic_Init( void );
void ModUAMPowerups_Init( void );
void ModUAMWarmupMode_Init( void );
void ModUAMWarmupSequence_Init( void );
void ModUAMWeaponEffects_Init( void );
void ModUAMWeaponSpawn_Init( qboolean allowWeaponCvarChanges );
void ModUAMWeapons_Init( qboolean allowWeaponCvarChanges );
void ModUAMWinnerTaunt_Init( void );

//
// UAM Warmup Sequence (uam_warmup_sequence.c)
//

qboolean ModUAMWarmupSequence_Shared_DetpackEnabled( void );
