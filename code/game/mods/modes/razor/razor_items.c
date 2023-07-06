/*
* Razor Items
* 
* Suppress, remap, and/or adjust timing on item pickup spawns consistent with Pinball mod.
*/

#define MOD_NAME ModRazorItems

#include "mods/modes/razor/razor_local.h"

static struct {
	int _unused;
} *MOD_STATE;

/*
================
(ModFN) ItemInitialSpawnDelay
================
*/
static int MOD_PREFIX(ItemInitialSpawnDelay)( MODFN_CTV, gentity_t *ent ) {
	// powerup initial spawn time is effectively 0 - 120s
	if ( ent->item->giType == IT_POWERUP ) {
		float respawn = random() * 120;
		return respawn * 1000;
	} else {
		return MODFN_NEXT( ItemInitialSpawnDelay, ( MODFN_NC, ent ) );
	}
}

/*
================
(ModFN) AdjustItemRespawnTime
================
*/
static float MOD_PREFIX(AdjustItemRespawnTime)( MODFN_CTV, float time, const gentity_t *ent ) {
	// powerup respawn time is effectively 120 - 240s
	if ( ent->item->giType == IT_POWERUP ) {
		return 120 + random() * 120;
	} else {
		return MODFN_NEXT( AdjustItemRespawnTime, ( MODFN_NC, time, ent ) );
	}
}

/*
================
(ModFN) CheckReplaceItem
================
*/
static gitem_t *MOD_PREFIX(CheckReplaceItem)( MODFN_CTV, gitem_t *item ) {
	item = MODFN_NEXT( CheckReplaceItem, ( MODFN_NC, item ) );

	if ( item->giType == IT_ARMOR && item->quantity == 100 ) {
		return BG_FindItemWithClassname( "item_flight" );
	}
	if ( item->giType == IT_POWERUP && item->giTag == PW_REGEN ) {
		return BG_FindItemWithClassname( "item_enviro" );
	}
	if ( item->giType == IT_WEAPON ) {
		switch ( item->giTag ) {
			case WP_QUANTUM_BURST:
				return BG_FindItemWithClassname( "holdable_shield" );
			case WP_DREADNOUGHT:
				return BG_FindItemWithClassname( "holdable_detpack" );
		}
	}
	if ( item->giType == IT_AMMO ) {
		switch ( item->giTag ) {
			case WP_IMOD:
				return BG_FindItemForAmmo( WP_COMPRESSION_RIFLE );
			case WP_SCAVENGER_RIFLE:
			case WP_STASIS:
				return BG_FindItemForAmmo( WP_GRENADE_LAUNCHER );
			case WP_TETRION_DISRUPTOR:
			case WP_DREADNOUGHT:
				return BG_FindItemForAmmo( WP_QUANTUM_BURST );
		}
	}

	return item;
}

/*
================
(ModFN) CheckItemSpawnDisabled
================
*/
static qboolean MOD_PREFIX(CheckItemSpawnDisabled)( MODFN_CTV, gitem_t *item ) {
	if ( modcfg.mods_enabled.disintegration && item->giType != IT_TEAM ) {
		return qtrue;
	}

	switch ( item->giType ) {
		case IT_WEAPON:
		case IT_ARMOR:
		case IT_HEALTH:
			return qtrue;
		case IT_AMMO:
			switch ( item->giTag ) {
				case WP_COMPRESSION_RIFLE:
				case WP_GRENADE_LAUNCHER:
				case WP_QUANTUM_BURST:
					goto next;
			}
			return qtrue;
		case IT_POWERUP:
			switch ( item->giTag ) {
				case PW_QUAD:
				case PW_BATTLESUIT:
				case PW_INVIS:
				case PW_FLIGHT:
				case PW_SEEKER:
					goto next;
			}
			return qtrue;
		case IT_HOLDABLE:
			switch ( item->giTag ) {
				case HI_TRANSPORTER:
				case HI_DETPACK:
				case HI_SHIELD:
					goto next;
			}
			return qtrue;
	}

	next:
	return MODFN_NEXT( CheckItemSpawnDisabled, ( MODFN_NC, item ) );
}

/*
================
ModRazorItems_Init
================
*/
void ModRazorItems_Init( void ) {
	if ( !MOD_STATE ) {
		MOD_STATE = G_Alloc( sizeof( *MOD_STATE ) );

		MODFN_REGISTER( ItemInitialSpawnDelay, ++modePriorityLevel );
		MODFN_REGISTER( AdjustItemRespawnTime, ++modePriorityLevel );
		MODFN_REGISTER( CheckReplaceItem, ++modePriorityLevel );
		MODFN_REGISTER( CheckItemSpawnDisabled, ++modePriorityLevel );
	}
}
