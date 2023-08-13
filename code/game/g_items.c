// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

/*

  Items are any object that a player can touch to gain some effect.

  Pickup will return the number of seconds until they should respawn.

  all items should pop when dropped in lava or slime

  Respawnable items don't actually go away when picked up, they are
  just made invisible and untouchable.  This allows them to ride
  movers and respawn apropriately.
*/


#define	RESPAWN_ARMOR		20
#define	RESPAWN_HEALTH		30
#define	RESPAWN_AMMO		40
#define	RESPAWN_HOLDABLE	60
#define	RESPAWN_MEGAHEALTH	120
#define	RESPAWN_POWERUP		120


/*QUAKED item_botroam (.5 .3 .7) (-16 -16 -24) (16 16 0)
Bots in MP will go to these spots when there's nothing else to get- helps them patrol.
*/

/*
================
(ModFN) AdaptRespawnNumPlayers

Returns the effective number of players to use for g_adaptRespawn calculation.
================
*/
int ModFNDefault_AdaptRespawnNumPlayers( void ) {
	return level.numPlayingClients;
}

// For more than four players, adjust the respawn times, up to 1/4.
int adjustRespawnTime(float respawnTime, const gentity_t *ent)
{
	int numPlayers;

	// Peform mod adjustments.
	respawnTime = modfn.AdjustItemRespawnTime( respawnTime, ent );

	numPlayers = modfn.AdaptRespawnNumPlayers();
	if (g_adaptRespawn.integer && numPlayers > 4 && ent->item->giType != IT_POWERUP)
	{	// Start scaling the respawn times.
		if (numPlayers > 32)
		{	// 1/4 time minimum.
			respawnTime *= 0.25;
		}
		else if (numPlayers > 12)
		{	// From 12-32, scale from 0.5 to 0.25;
			respawnTime *= 20.0 / (float)(numPlayers + 8);
		}
		else
		{	// From 4-12, scale from 1.0 to 0.5;
			respawnTime *= 8.0 / (float)(numPlayers + 4);
		}
	}

	if (respawnTime < 1.0)
	{	// No matter what, don't go lower than 1 second, or the pickups become very noisy!
		respawnTime = 1.0;
	}

	return (tonextint(respawnTime));
}


//======================================================================

int Pickup_Powerup( gentity_t *ent, gentity_t *other ) {
	int			quantity;
	int			i;
	gclient_t	*client;

	if ( !other->client->ps.powerups[ent->item->giTag] ) {
		// round timing to seconds to make multiple powerup timers
		// count in sync
		other->client->ps.powerups[ent->item->giTag] =
			level.time - ( level.time % 1000 );

		// kef -- log the fact that we picked up this powerup
		G_LogWeaponPowerup(other->s.number, ent->item->giTag);
	}

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	other->client->ps.powerups[ent->item->giTag] += quantity * 1000;

	// give any nearby players a "denied" anti-reward
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		vec3_t		delta;
		float		len;
		vec3_t		forward;
		trace_t		tr;

		client = &level.clients[i];
		if ( client == other->client ) {
			continue;
		}
		if ( client->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
			continue;
		}

	// if same team in team game, no sound
	// cannot use OnSameTeam as it expects to g_entities, not clients
	if ( g_gametype.integer >= GT_TEAM && other->client->sess.sessionTeam == client->sess.sessionTeam ) {
		continue;
	}

		// if too far away, no sound
		VectorSubtract( ent->s.pos.trBase, client->ps.origin, delta );
		len = VectorNormalize( delta );
		if ( len > 192 ) {
			continue;
		}

		// if not facing, no sound
		AngleVectors( client->ps.viewangles, forward, NULL, NULL );
		if ( DotProduct( delta, forward ) < 0.4 ) {
			continue;
		}

		// if not line of sight, no sound
		trap_Trace( &tr, client->ps.origin, NULL, NULL, ent->s.pos.trBase, ENTITYNUM_NONE, CONTENTS_SOLID );
		if ( tr.fraction != 1.0 ) {
			continue;
		}

		// anti-reward
		client->ps.persistant[PERS_REWARD_COUNT]++;
		client->ps.persistant[PERS_REWARD] = REWARD_DENIED;
	}

	return adjustRespawnTime(RESPAWN_POWERUP, ent);
}

//======================================================================

int Pickup_Holdable( gentity_t *ent, gentity_t *other )
{
	int nItem = ent->item - bg_itemlist;

	other->client->ps.stats[STAT_HOLDABLE_ITEM] = nItem;

	// if we just picked up the detpack, indicate that it has not been placed yet
	if (HI_DETPACK == bg_itemlist[nItem].giTag)
	{
		other->client->ps.stats[STAT_USEABLE_PLACED] = 0;
	}
	// kef -- log the fact that we picked up this item
	G_LogWeaponItem(other->s.number, bg_itemlist[nItem].giTag);

	return adjustRespawnTime(RESPAWN_HOLDABLE, ent);
}


//======================================================================

/*
================
(ModFN) AddAmmoForItem

Adds ammo when player picks up either a weapon or ammo item.
================
*/
void ModFNDefault_AddAmmoForItem( int clientNum, gentity_t *item, int weapon, int count ) {
	gclient_t *client = &level.clients[clientNum];
	client->ps.ammo[weapon] += count;
	if ( client->ps.ammo[weapon] > Max_Ammo[weapon] ) {
		client->ps.ammo[weapon] = Max_Ammo[weapon];
	}
}

int Pickup_Ammo (gentity_t *ent, gentity_t *other)
{
	int		quantity;

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	modfn.AddAmmoForItem( other - g_entities, ent, ent->item->giTag, quantity );

	return adjustRespawnTime(RESPAWN_AMMO, ent);
}

//======================================================================


int Pickup_Weapon (gentity_t *ent, gentity_t *other) {
	int		quantity;

	if ( ent->count < 0 ) {
		quantity = 0; // None for you, sir!
	} else {
		if ( ent->count ) {
			quantity = ent->count;
		} else {
			quantity = ent->item->quantity;
		}

		// dropped items and teamplay weapons always have full ammo
		if ( ! (ent->flags & FL_DROPPED_ITEM) && g_gametype.integer != GT_TEAM ) {
			// respawning rules

			// New method:  If the player has less than half the minimum, give them the minimum, else add 1/2 the min.

			// drop the quantity if the already have over the minimum
			if ( other->client->ps.ammo[ ent->item->giTag ] < quantity*0.5 ) {
				quantity = quantity - other->client->ps.ammo[ ent->item->giTag ];
			} else {
				quantity = quantity*0.5;		// only add half the value.
			}

			// Old method:  If the player has less than the minimum, give them the minimum, else just add 1.
/*
			// drop the quantity if the already have over the minimum
			if ( other->client->ps.ammo[ ent->item->giTag ] < quantity ) {
				quantity = quantity - other->client->ps.ammo[ ent->item->giTag ];
			} else {
				quantity = 1;		// only add a single shot
			}
			*/
		}
	}

	// add the weapon
	other->client->ps.stats[STAT_WEAPONS] |= ( 1 << ent->item->giTag );

	modfn.AddAmmoForItem( other - g_entities, ent, ent->item->giTag, quantity );

	G_LogWeaponPickup(other->s.number, ent->item->giTag);

	// team deathmatch has slow weapon respawns
	if ( g_gametype.integer == GT_TEAM )
	{
		return adjustRespawnTime(g_teamWeaponRespawn.integer, ent);
	}

	return adjustRespawnTime(g_weaponRespawn.integer, ent);
}


//======================================================================

int Pickup_Health (gentity_t *ent, gentity_t *other) {
	int			max;
	int			quantity;

	// small and mega healths will go over the max
	if ( ent->item->quantity != 5 && ent->item->quantity != 100  ) {
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	} else {
		max = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	}

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	other->health += quantity;

	if (other->health > max ) {
		other->health = max;
	}
	other->client->ps.stats[STAT_HEALTH] = other->health;

	if ( ent->item->giTag == 100 ) {		// mega health respawns slow
		return RESPAWN_MEGAHEALTH;			// It also does not adapt like other health pickups.
	}

	return adjustRespawnTime(RESPAWN_HEALTH, ent);
}

//======================================================================

int Pickup_Armor( gentity_t *ent, gentity_t *other ) {
	other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;
	if ( other->client->ps.stats[STAT_ARMOR] > other->client->ps.stats[STAT_MAX_HEALTH] * 2 ) {
		other->client->ps.stats[STAT_ARMOR] = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	}

	return adjustRespawnTime(RESPAWN_ARMOR, ent);
}

//======================================================================

/*
================
(ModFN) CheckItemSuppressed

Temporarily suppresses item from spawning, but doesn't permanently remove it.
If suppressed, returns number of milliseconds until next check, otherwise returns 0.
================
*/
int ModFNDefault_CheckItemSuppressed( gentity_t *item ) {
	return 0;
}

/*
===============
RespawnItem
===============
*/
void RespawnItem( gentity_t *ent ) {
	// check if item respawn suppressed
	int suppressed = modfn.CheckItemSuppressed( ent );
	if ( suppressed > 0 ) {
		ent->nextthink = level.time + suppressed;
		return;
	}

	// randomly select from teamed entities
	if (ent->team) {
		gentity_t	*master;
		int	count;
		int choice;

		if ( !ent->teammaster ) {
			G_Error( "RespawnItem: bad teammaster");
		}
		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->teamchain, count++)
			;

		choice = rand() % count;

		for (count = 0, ent = master; count < choice; ent = ent->teamchain, count++)
			;
	}

	ent->r.contents = CONTENTS_TRIGGER;
	ent->s.eFlags &= ~(EF_NODRAW | EF_ITEMPLACEHOLDER);
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity (ent);

	if ( ent->item->giType == IT_POWERUP ) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		te->s.eventParm = G_SoundIndex( "sound/items/poweruprespawn.wav" );//cgs.media.poweruprespawn
		te->r.svFlags |= SVF_BROADCAST;
	}

	// play the normal respawn sound only to nearby clients
	G_AddEvent( ent, EV_ITEM_RESPAWN, 0 );

	ent->nextthink = 0;
}

/*
================
(ModFN) CanItemBeGrabbed

Check if player is allowed to pick up item.
================
*/
qboolean ModFNDefault_CanItemBeGrabbed( gentity_t *item, int clientNum ) {
	// the same pickup rules are used for client side and server side
	return BG_CanItemBeGrabbed( &item->s, &level.clients[clientNum].ps );
}

/*
===============
Touch_Item
===============
*/
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace) {
	int			respawn;

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup

	if ( !modfn.CanItemBeGrabbed( ent, other - g_entities ) ) {
		return;
	}

	// If ghosted, then end the ghost-ness in favor of the pickup.
	modfn.SetClientGhosting( other - g_entities, qfalse );

	G_LogPrintf( "Item: %i %s\n", other->s.number, ent->item->classname );

	// call the item-specific pickup function
	switch( ent->item->giType ) {
	case IT_WEAPON:
		respawn = Pickup_Weapon(ent, other);
		break;
	case IT_AMMO:
		respawn = Pickup_Ammo(ent, other);
		break;
	case IT_ARMOR:
		respawn = Pickup_Armor(ent, other);
		break;
	case IT_HEALTH:
		respawn = Pickup_Health(ent, other);
		break;
	case IT_POWERUP:
		respawn = Pickup_Powerup(ent, other);
		break;
	case IT_TEAM:
		respawn = Pickup_Team(ent, other);
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable(ent, other);
		break;
	default:
		return;
	}

	if ( !respawn ) {
		return;
	}

	// play the normal pickup sound
	if ( other->client->pers.predictItemPickup ) {
		G_AddPredictableEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
	} else {
		G_AddEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
	}

	// powerup pickups are global broadcasts
	if ( ent->item->giType == IT_POWERUP || ent->item->giType == IT_TEAM) {
		gentity_t	*te;

		te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
		te->s.eventParm = ent->s.modelindex;
		// tell us which client fired off this global sound
		te->s.otherEntityNum = other->s.number;
		te->r.svFlags |= SVF_BROADCAST;
	}

	// special seeker pickup sound
	if ( ent->item->giType == IT_POWERUP && ent->item->giTag == PW_SEEKER &&
			modfn.AdjustGeneralConstant( GC_SEEKER_PICKUP_SOUND, 0 ) ) {
		G_ClientGlobalSound( G_SoundIndex( "sound/enemies/borg/alcoveout.wav" ), other - g_entities );
	}

	// fire item targets
	G_UseTargets (ent, other);

	// wait of -1 will not respawn
	if ( ent->wait == -1 ) {
		ent->r.svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = qtrue;
		return;
	}

	// non zero wait overrides respawn time
	if ( ent->wait ) {
		respawn = ent->wait;
	}

	// random can be used to vary the respawn time
	if ( ent->random ) {
		respawn += crandom() * ent->random;
		if ( respawn < 1 ) {
			respawn = 1;
		}
	}

	// dropped items will not respawn
	if ( ent->flags & FL_DROPPED_ITEM ) {
		ent->freeAfterEvent = qtrue;
	}

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	if (ent->item->giType==IT_WEAPON || ent->item->giType==IT_POWERUP)
	{
		ent->s.eFlags |= EF_ITEMPLACEHOLDER;
	}
	else
	{
//	this line used to prevent items that were picked up from being drawn, but we now want to draw the techy grid thing instead
		ent->s.eFlags |= EF_NODRAW;
		ent->r.svFlags |= SVF_NOCLIENT;
	}



	ent->r.contents = 0;

// ***************
	// ZOID
	// A negative respawn times means to never respawn this item (but don't
	// delete it).  This is used by items that are respawned by third party
	// events such as ctf flags
	if ( respawn <= 0 ) {
		ent->nextthink = 0;
		ent->think = 0;
	} else {
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
	}
	trap_LinkEntity( ent );
}


//======================================================================

/*
================
(ModFN) ItemDropExpireTime

Returns number of milliseconds until dropped item expires (0 = never expires).
================
*/
int ModFNDefault_ItemDropExpireTime( const gitem_t *item ) {
	return 30000;
}

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity ) {
	gentity_t	*dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	dropped->s.eFlags |= EF_DEAD;	// Yes, this is totally lame, but we use it bg_misc to check
									// if the item has been droped, and if so, make it pick-up-able
									// cdr

	if (item->giType == IT_TEAM) { // Special case for CTF flags
		gentity_t	*te;

		VectorSet (dropped->r.mins, -23, -23, -15);
		VectorSet (dropped->r.maxs, 23, 23, 31);
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = level.time + 30000;
		Team_CheckDroppedItem( dropped );

		// make the sound call for a dropped flag
		te = G_TempEntity( dropped->s.pos.trBase, EV_TEAM_SOUND );
		te->s.eventParm = DROPPED_FLAG_SOUND;
		if (dropped->item->giTag == PW_REDFLAG)
		{
			te->s.otherEntityNum = TEAM_RED;
		}
		else
		{
			te->s.otherEntityNum = TEAM_BLUE;
		}

		te->r.svFlags |= SVF_BROADCAST;

	} else { // auto-remove after 30 seconds
		int duration = modfn.ItemDropExpireTime( item );
		if ( duration > 0 ) {
			dropped->think = G_FreeEntity;
			dropped->nextthink = level.time + duration;
		} else {
			dropped->think = NULL;
		}
	}

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity (dropped);

	return dropped;
}

/*
================
(ModFN) ItemTossOrigin

Set the client origin to use when tossing items.
================
*/
void ModFNDefault_ItemTossOrigin( int clientNum, vec3_t originOut ) {
	VectorCopy( level.clients[clientNum].ps.origin, originOut );
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t	origin;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;

	modfn.ItemTossOrigin( ent - g_entities, origin );
	return LaunchItem( item, origin, velocity );
}


/*
================
Use_Item

Respawn the item
================
*/
void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	RespawnItem( ent );
}

//======================================================================

/*
================
(ModFN) ItemInitialSpawnDelay

Returns number of milliseconds to delay spawning item at start of level,
or 0 for no delay.
================
*/
int ModFNDefault_ItemInitialSpawnDelay( gentity_t *ent ) {
	// powerups don't spawn in for a while
	if ( ent->item->giType == IT_POWERUP ) {
		float respawn = 45 + crandom() * 15;
		return respawn * 1000;
	}

	return 0;
}

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		dest;
	int			suppressed;

	VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
	VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );

	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;		// store item number in modelindex
	ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	// useing an item causes it to respawn
	ent->use = Use_Item;


	if ( ent->item->giType == IT_TEAM )
	{
		VectorSet( ent->r.mins, -23, -23, 0 );
		VectorSet( ent->r.maxs, 23, 23, 47 );
	}

	if ( ent->spawnflags & 1 ) {
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {
		// drop to floor
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
		if ( tr.startsolid ) {
			G_DedPrintf ("FinishSpawningItem: removing %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEntity( ent );
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin( ent, tr.endpos );
	}

	// team slaves and targeted items aren't present at start
	if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname ) {
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	// check if item spawn suppressed
	suppressed = modfn.ItemInitialSpawnDelay( ent );
	if ( suppressed <= 0 ) {
		suppressed = modfn.CheckItemSuppressed( ent );
	}
	if ( suppressed > 0 ) {
		ent->nextthink = level.time + suppressed;
		ent->think = RespawnItem;
		return;
	}

	trap_LinkEntity (ent);
}


/*
================
FinishSpawningDetpack

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
qboolean FinishSpawningDetpack( gentity_t *ent, int itemIndex )
{
	trace_t		tr;
	vec3_t		dest;

	VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );
	VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );

	ent->s.eType = ET_USEABLE;
	ent->s.modelindex = bg_itemlist[itemIndex].giTag;	// this'll be used in CG_Useable()
	ent->s.modelindex2 = itemIndex;	// store item number in modelindex

	ent->classname = bg_itemlist[itemIndex].classname;
	ent->r.contents = CONTENTS_CORPSE;//CONTENTS_TRIGGER;
	ent->takedamage = 1;
	ent->health = 5;
	ent->touch = 0;
	ent->die = modfn.DetpackShot;

	// useing an item causes it to respawn
	ent->use = Use_Item;

	// drop to floor
	VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
	trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
	if ( tr.startsolid )
	{
		G_FreeEntity( ent );
		return qfalse;
	}

	// allow to ride movers
	ent->physicsObject = qtrue;
	ent->s.groundEntityNum = tr.entityNum;

	G_SetOrigin( ent, tr.endpos );

	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;

	trap_LinkEntity (ent);

	ent->noise_index = G_SoundIndex("sound/weapons/detpacklatch.wav");
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );
	return qtrue;
}


/*
================
FinishSpawningDecoy

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
qboolean FinishSpawningDecoy( gentity_t *ent, int itemIndex )
{
	trace_t		tr;
	vec3_t		dest;

	// OLD RADIUS SETTINGS
//	VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
//	VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );
//	ent->s.eType = ET_USEABLE;
//	ent->s.modelindex = bg_itemlist[itemIndex].giTag;	// this'll be used in CG_Useable()

	ent->classname = bg_itemlist[itemIndex].classname;
//	ent->r.contents = CONTENTS_CORPSE;
	ent->touch = 0;				// null touch function pointer
	// useing an item causes it to respawn
	ent->use = Use_Item;

	// drop to floor
	VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
	trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );

	if ( tr.startsolid )
	{	// If stuck in a solid, give up and go home
		G_FreeEntity( ent );
		return qfalse;
	}

	G_SetOrigin( ent, tr.endpos );

	// allow to ride movers
	ent->physicsObject = qtrue;
	ent->s.pos.trType = TR_GRAVITY;//have to do this because it thinks it's an ET_PLAYER
	ent->s.groundEntityNum = tr.entityNum;

	// Turn off the NODRAW and NOCLIENT flags
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;

	trap_LinkEntity (ent);
	return qtrue;
}




qboolean	itemRegistered[MAX_ITEMS];

/*
==================
G_CheckTeamItems
==================
*/
void G_CheckTeamItems( void ) {

	// Set up team stuff
	Team_InitGame();

	if ( g_gametype.integer == GT_CTF ) {
		gitem_t	*item;

		// make sure we actually have two flags...
		item = BG_FindItem( "Red Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( "^1WARNING: No team_CTF_redflag in map" );
		}
		item = BG_FindItem( "Blue Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( "^1WARNING: No team_CTF_blueflag in map" );
		}
	}
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem( gitem_t *item ) {
	if ( !item ) {
		G_Error( "RegisterItem: NULL" );
	}
	itemRegistered[ item - bg_itemlist ] = qtrue;
}

/*
================
(ModFN) AddRegisteredItems

Can be used to add additional registered items, which are added to a configstring for
client caching purposes.
================
*/
void ModFNDefault_AddRegisteredItems( void ) {
	// players always start with the base weapon
	RegisterItem( BG_FindItemForWeapon( WP_PHASER ) );
	RegisterItem( BG_FindItemForWeapon( WP_COMPRESSION_RIFLE ) );	//this is for the podium at the end, make sure we have the model
}

/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems( void ) {
	char	string[MAX_ITEMS+1];
	int		i;
	int		count;

	// add any extra items needed by mods
	modfn.AddRegisteredItems();

	count = 0;
	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( itemRegistered[i] ) {
			count++;
			string[i] = '1';
		} else {
			string[i] = '0';
		}
	}
	string[ bg_numItems ] = 0;

	G_Printf( "%i items registered\n", count );
	trap_SetConfigstring(CS_ITEMS, string);
}


/*
================
(ModFN) CheckReplaceItem

Allows replacing an item with a different item during initial spawn.
================
*/
gitem_t *ModFNDefault_CheckReplaceItem( gitem_t *item ) {
	return item;
}

/*
================
(ModFN) CheckItemSpawnDisabled

Allows permanently removing an item during initial spawn.
Returns qtrue to disable item, qfalse to spawn item normally.
================
*/
qboolean ModFNDefault_CheckItemSpawnDisabled( gitem_t *item ) {
	return qfalse;
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem (gentity_t *ent, gitem_t *item) {
	// This should now only be called from G_CallSpawn during map initialization.
	EF_WARN_ASSERT( level.spawning );

	item = modfn.CheckReplaceItem( item );
	ent->classname = item->classname;

	if ( modfn.CheckItemSpawnDisabled( item ) ) {
		G_FreeEntity( ent );
		return;
	}

	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnFloat( "wait", "0", &ent->wait );

	RegisterItem( item );

	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	ent->physicsBounce = 0.50;		// items are bouncy

	if ( item->giType == IT_POWERUP ) {
		G_SoundIndex( "sound/items/poweruprespawn.wav" );//cgs.media.poweruprespawn
	}
}


/*
================
G_BounceItem

================
*/
void G_BounceItem( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	// cut the velocity to keep from bouncing forever
	VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );

	// check for stop
	if ( trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40 ) {
		trace->endpos[2] += 1.0;	// make sure it is off ground
		SnapVector( trace->endpos );
		G_SetOrigin( ent, trace->endpos );
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_RunItem

================
*/
void G_RunItem( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			contents;
	int			mask;

	// if groundentity has been set to -1, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == -1 ) {
		if ( ent->s.pos.trType != TR_GRAVITY ) {
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if ( ent->s.pos.trType == TR_STATIONARY ) {
		// check think function
		G_RunThink( ent );
		return;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// trace a line from the previous position to the current position
	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_PLAYERSOLID & ~CONTENTS_BODY;//MASK_SOLID;
	}
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
		ent->r.ownerNum, mask );

	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	trap_LinkEntity( ent );	// FIXME: avoid this for stationary?

	// check think function
	G_RunThink( ent );

	if ( tr.fraction == 1 ) {
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents( ent->r.currentOrigin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		if (ent->item && ent->item->giType == IT_TEAM) {
			Team_FreeEntity(ent);
		} else {
			G_FreeEntity( ent );
		}
		return;
	}

	G_BounceItem( ent, &tr );
}
