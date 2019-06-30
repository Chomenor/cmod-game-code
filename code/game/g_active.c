// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"

extern void SP_misc_ammo_station( gentity_t *ent );
extern void ammo_station_finish_spawning ( gentity_t *self );
extern int	borgQueenClientNum;
extern qboolean BG_BorgTransporting( playerState_t *ps );

/*
==============
TryUse

Try and use an entity in the world, directly ahead of us
==============
*/

#define USE_DISTANCE	64.0f
void TryUse( gentity_t *ent )
{
	gentity_t	*target, *newent;
	trace_t		trace;
	vec3_t		src, dest, vf;
	int			i;

	VectorCopy( ent->r.currentOrigin, src );
	src[2] += ent->client->ps.viewheight;

	AngleVectors( ent->client->ps.viewangles, vf, NULL, NULL );

	//extend to find end of use trace
	VectorMA( src, -6, vf, src );//in case we're inside something?
	VectorMA( src, 134, vf, dest );//128+6

	//Trace ahead to find a valid target
	trap_Trace( &trace, src, vec3_origin, vec3_origin, dest, ent->s.number, MASK_OPAQUE|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE );
	
	if ( trace.fraction == 1.0f || trace.entityNum < 0 )
	{
		//FIXME: Play a failure sound
		return;
	}

	target = &g_entities[trace.entityNum];

	//Check for a use command
	if ( target->client )
	{//if a bot, make it follow me, if a teammate, give stuff
		if ( target->client->sess.sessionTeam == ent->client->sess.sessionTeam )
		{//on my team
			gitem_t	*regen = BG_FindItemForPowerup( PW_REGEN );
			gitem_t	*invis = BG_FindItemForPowerup( PW_INVIS );

			switch( ent->client->sess.sessionClass )
			{
			case PC_MEDIC://medic gives regen
				if ( target->client->ps.powerups[PW_REGEN] < level.time )
				{//only give if have some left and they don't already have it
					//FIXME: play sound
					target->client->ps.powerups[regen->giTag] = level.time - ( level.time % 1000 );
					target->client->ps.powerups[regen->giTag] += 60 * 1000;
					G_AddEvent( target, EV_ITEM_PICKUP, (regen-bg_itemlist) );
					return;
				}
				break;
			case PC_TECH://tech gives invisibility
				if ( ent->count )
				{
					//FIXME: play sound
					target->client->ps.powerups[invis->giTag] = level.time - ( level.time % 1000 );
					target->client->ps.powerups[invis->giTag] += 30 * 1000;
					ent->count--;
					G_AddEvent( target, EV_ITEM_PICKUP, (invis-bg_itemlist) );
					return;
				}
				break;
			}
			switch( target->client->sess.sessionClass )
			{
			case PC_TECH://using tech gives you full ammo
				G_Sound(ent, G_SoundIndex("sound/player/suitenergy.wav") );
				for ( i = 0 ; i < MAX_WEAPONS ; i++ ) 
				{
					ent->client->ps.ammo[i] = Max_Ammo[i];
				}
				return;
				break;
			}
		}
	}
	else if ( target->use && Q_stricmp("func_usable", target->classname) == 0 ) 
	{//usable brush
		if ( target->team && atoi( target->team ) != 0 )
		{//usable has a team
			if ( atoi( target->team ) != ent->client->sess.sessionTeam )
			{//not on my team
				if ( ent->client->sess.sessionClass != PC_TECH )
				{//only a tech can use enemy usables
					//FIXME: Play a failure sound
					return;
				}
			}
		}
		//FIXME: play sound?
		target->use( target, ent, ent );
		return;
	}
	else if ( target->use && Q_stricmp("misc_ammo_station", target->classname) == 0 ) 
	{//ammo station
		if ( ent->client->sess.sessionTeam )
		{
			if ( target->team )
			{
				if ( atoi( target->team ) != ent->client->sess.sessionTeam )
				{
					//FIXME: play sound?
					return;
				}
			}
		}
		target->use( target, ent, ent );
		return;
	}
	else if ( target->s.number == ENTITYNUM_WORLD || (target->s.pos.trType == TR_STATIONARY && !(trace.surfaceFlags & SURF_NOIMPACT) && !target->takedamage) )
	{
		switch( ent->client->sess.sessionClass )
		{
		case PC_TECH://tech can place an ammo station
			if ( ent->count )
			{
				VectorCopy( trace.endpos, src );
				VectorMA( src, -8, vf, src );
				//place an ammo station here
				newent = G_Spawn();
				if ( newent )
				{
					ent->count--;
					VectorCopy( src, newent->s.origin );
					vectoangles( trace.plane.normal, newent->s.angles );
					newent->classname = "misc_ammo_station";
					//newent->r.ownerNum = ent->s.number;
					newent->parent = ent;
					if ( ent->client->sess.sessionTeam == TEAM_RED )
					{
						newent->team = "1";
					}
					else if ( ent->client->sess.sessionTeam == TEAM_BLUE )
					{
						newent->team = "2";
					}
					SP_misc_ammo_station( newent );
					newent->splashDamage = 100;
					newent->splashRadius = 200;
					//make it "spawn" in
					//Make noise
					G_Sound( newent, G_SoundIndex( "sound/items/respawn1.wav" ) );
					newent->s.eFlags |= EF_ITEMPLACEHOLDER;
					newent->use = 0;
					newent->think = ammo_station_finish_spawning;
					newent->nextthink = level.time + 1000;
				}
			}
			break;
		}
		//FIXME: play sound
		return;
	}
	//FIXME: Play a failure sound
}

/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	float	count;
	vec3_t	angles;

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if ( count == 0 ) {
		return;		// didn't take any damage
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// play an apropriate pain sound
	if ( (level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) ) {
		player->pain_debounce_time = level.time + 700;
		G_AddEvent( player, EV_PAIN, player->health );
		client->ps.damageEvent++;
	}
	
	client->ps.damageCount = client->damage_blood;
	if (client->ps.damageCount > 255)
	{
		client->ps.damageCount = 255;
	}

	client->ps.damageShieldCount = client->damage_armor;
	if (client->ps.damageShieldCount > 255)
	{
		client->ps.damageShieldCount = 255;
	}

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
	qboolean	envirosuit;
	int			waterlevel;

	if ( ent->client->noclip ) {
		ent->client->airOutTime = level.time + 12000;	// don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	envirosuit = ent->client->ps.powerups[PW_BATTLESUIT] > level.time;

	//
	// check for drowning
	//
	if ( waterlevel == 3 && !(ent->watertype&CONTENTS_LADDER)) {
		// envirosuit give air, techs can't drown
		if ( envirosuit || ent->client->sess.sessionClass == PC_TECH ) {
			ent->client->airOutTime = level.time + 10000;
		}

		// if out of air, start drowning
		if ( ent->client->airOutTime < level.time) {
			// drown!
			ent->client->airOutTime += 1000;
			if ( ent->health > 0 ) {
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// play a gurp sound instead of a normal pain sound
				if (ent->health <= ent->damage) {
					G_Sound(ent, G_SoundIndex("*drown.wav"));
				} else if (rand()&1) {
					G_Sound(ent, G_SoundIndex("sound/player/gurp1.wav"));
				} else {
					G_Sound(ent, G_SoundIndex("sound/player/gurp2.wav"));
				}

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage (ent, NULL, NULL, NULL, NULL, 
					ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	} else {
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel && 
		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		if (ent->health > 0
			&& ent->pain_debounce_time < level.time	) {

			if ( envirosuit ) {
				G_AddEvent( ent, EV_POWERUP_BATTLESUIT, 0 );
			} else {
				if (ent->watertype & CONTENTS_LAVA) {
					G_Damage (ent, NULL, NULL, NULL, NULL, 
						30*waterlevel, 0, MOD_LAVA);
				}

				if (ent->watertype & CONTENTS_SLIME) {
					G_Damage (ent, NULL, NULL, NULL, NULL, 
						10*waterlevel, 0, MOD_SLIME);
				}
			}
		}
	}
}



/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) 
{	// 3/28/00 kef -- this is dumb.
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) )
		ent->s.loopSound = level.snd_fry;
	else
		ent->s.loopSound = 0;
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pm->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static vec3_t	range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->ps.eFlags&EF_ELIMINATED) ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}
		} else {
			if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset( &trace, 0, sizeof(trace) );

		if ( hit->touch ) {
			hit->touch (hit, ent, &trace);
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pm;
	gclient_t	*client;

	client = ent->client;

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = 400;	// faster than normal

		// set up for pmove
		memset (&pm, 0, sizeof(pm));
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		pm.trace = trap_Trace;
		pm.pointcontents = trap_PointContents;

		// perform a pmove
		Pmove (&pm);

		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// attack button cycles through spectators
	if ( ( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK ) ) {
		Cmd_FollowCycle_f( ent, 1 );
	}
	else if ( ( client->buttons & BUTTON_ALT_ATTACK ) && ! ( client->oldbuttons & BUTTON_ALT_ATTACK ) )
	{
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
	}
}



/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client )
{
	if ( ! g_inactivity.integer )
	{
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	}
	else if (	client->pers.cmd.forwardmove || 
				client->pers.cmd.rightmove || 
				client->pers.cmd.upmove ||
				(client->pers.cmd.buttons & BUTTON_ATTACK) ||
				(client->pers.cmd.buttons & BUTTON_ALT_ATTACK) )
	{
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	}
	else if ( !client->pers.localClient )
	{
		if ( level.time > client->inactivityTime )
		{
			trap_DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning )
		{
			client->inactivityWarning = qtrue;
			trap_SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t *client;

	client = ent->client;
	client->timeResidual += msec;

	while ( client->timeResidual >= 1000 ) {
		client->timeResidual -= 1000;

		// regenerate
		if ( client->ps.powerups[PW_REGEN] ) {
			if ( client->sess.sessionClass != PC_NOCLASS && client->sess.sessionClass != PC_BORG && client->sess.sessionClass != PC_ACTIONHERO )
			{
				if ( ent->health < client->ps.stats[STAT_MAX_HEALTH] ) 
				{
					ent->health++;
					G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
				}
			}
			else if ( ent->health < client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health += 15;
				if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.1 ) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.1;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			} else if ( ent->health < client->ps.stats[STAT_MAX_HEALTH] * 2) {
				ent->health += 5;
				if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 2 ) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 2;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			}
		} else {
			// count down health when over max
			if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] ) {
				ent->health--;
			}
		}

		// THIS USED TO count down armor when over max, more slowly now (every other second)
//		if ( client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH] && ((int)(level.time/1000))&0x01) {

		// NOW IT ONCE AGAIN counts down armor when over max, once per second
		if ( client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH]) {
			client->ps.stats[STAT_ARMOR]--;
		}

		// if we've got the seeker powerup, see if we can shoot it at someone
		if ( client->ps.powerups[PW_SEEKER] )
		{
			vec3_t	seekerPos;

			if (SeekerAcquiresTarget(ent, seekerPos)) // set the client's enemy to a valid target
			{
				FireSeeker( ent, ent->enemy, seekerPos );
				G_AddEvent( ent, EV_POWERUP_SEEKER_FIRE, 0 ); // draw the thingy
			}
		}

		if ( !client->ps.stats[STAT_HOLDABLE_ITEM] )
		{//holding nothing...
			if ( client->ps.stats[STAT_USEABLE_PLACED] > 0 )
			{//we're in some kind of countdown
				//so count down
				client->ps.stats[STAT_USEABLE_PLACED]--;
			}
		}

	}
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if (g_gametype.integer != GT_SINGLE_PLAYER)
	{
		if ( client->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( client->oldbuttons ^ client->buttons ) ) {
			client->readyToExit ^= 1;
		}
	}
}

/*
====================
Cmd_Ready_f
====================

  This function is called from the ui_sp_postgame.c as a result of clicking on the
  "next" button in non GT_TOURNAMENT games.  This replaces the old system of waiting
  for the user to click an ATTACK or USE button to signal ready 
  (see ClientIntermissionThink() above)

  when all clients have signaled ready, the game continues to the next match.
*/
void Cmd_Ready_f (gentity_t *ent)
{
	gclient_t *client;
	client = ent->client;

	client->readyToExit ^= 1;
}



typedef struct detHit_s
{
	gentity_t	*detpack;
	gentity_t	*player;
	int			time;
} detHit_t;

#define MAX_DETHITS		32		// never define this to be 0

detHit_t	detHits[MAX_DETHITS];
qboolean	bDetInit = qfalse;

extern qboolean FinishSpawningDetpack( gentity_t *ent, int itemIndex );
//-----------------------------------------------------------------------------DECOY TEMP
extern qboolean FinishSpawningDecoy( gentity_t *ent, int itemIndex );
//-----------------------------------------------------------------------------DECOY TEMP
void DetonateDetpack(gentity_t *ent);

#define DETPACK_DAMAGE			750
#define DETPACK_RADIUS			500

void detpack_shot( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
{
	int i = 0;
	gentity_t *ent = NULL;

	//so we can't be blown up by things we're blowing up
	self->takedamage = 0;

	G_TempEntity(self->s.origin, EV_GRENADE_EXPLODE);
	G_RadiusDamage( self->s.origin, self->parent?self->parent:self, DETPACK_DAMAGE*0.125, DETPACK_RADIUS*0.25, 
		self, DAMAGE_ALL_TEAMS, MOD_DETPACK );
	// we're blowing up cuz we've been shot, so make sure we remove ourselves
	//from our parent's inventory (so to speak)
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (((ent = &g_entities[i])!=NULL) && ent->inuse && (self->parent == ent))
		{
			ent->client->ps.stats[STAT_USEABLE_PLACED] = 0;
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
			break;
		}
	}
	G_FreeEntity(self);
}

qboolean PlaceDetpack(gentity_t *ent)
{
	gentity_t	*detpack = NULL;
	static gitem_t *detpackItem = NULL;
	float		detDistance = 80;
	trace_t		tr;
	vec3_t		fwd, right, up, end, mins = {-16,-16,-16}, maxs = {16,16,16};

	if (detpackItem == NULL)
	{
		detpackItem = BG_FindItemForHoldable(HI_DETPACK);
	}

	// make sure our detHit info is init'd
	if (!bDetInit)
	{
		memset(detHits, 0, MAX_DETHITS*sizeof(detHit_t));
		bDetInit = 1;
	}

	// can we place this in front of us?
	AngleVectors (ent->client->ps.viewangles, fwd, right, up);
	fwd[2] = 0;
	VectorMA(ent->client->ps.origin, detDistance, fwd, end);
	trap_Trace (&tr, ent->client->ps.origin, mins, maxs, end, ent->s.number, MASK_SHOT );
	if (tr.fraction > 0.9)
	{
		// got enough room so place the detpack
		detpack = G_Spawn();
		G_SpawnItem(detpack, detpackItem);
		detpack->physicsBounce = 0.0f;//detpacks are *not* bouncy
		VectorMA(ent->client->ps.origin, detDistance + mins[0], fwd, detpack->s.origin);
		if ( !FinishSpawningDetpack(detpack, detpackItem - bg_itemlist) )
		{
			return qfalse;
		}
		VectorNegate(fwd, fwd);
		vectoangles(fwd, detpack->s.angles);
		detpack->think = DetonateDetpack;
		detpack->nextthink = level.time + 120000;	// if not used after 2 minutes it blows up anyway
		detpack->parent = ent;
		return qtrue;
	}
	else
	{
		// no room
		return qfalse;
	}
}

qboolean PlayerHitByDet(gentity_t *det, gentity_t *player)
{
	int i = 0;

	if (!bDetInit)
	{
		// detHit stuff not initialized. who knows what's going on?
		return qfalse;
	}
	for (i = 0; i < MAX_DETHITS; i++)
	{
		if ( (detHits[i].detpack == det) && (detHits[i].player == player) )
		{
			return qtrue;
		}
	}
	return qfalse;
}

void AddPlayerToDetHits(gentity_t *det, gentity_t *player)
{
	int			i = 0;
	detHit_t	*lastHit = NULL, *curHit = NULL;

	for (i = 0; i < MAX_DETHITS; i++)
	{
		if (0 == detHits[i].time)
		{
			// empty slot. add our player here.
			detHits[i].detpack = det;
			detHits[i].player = player;
			detHits[i].time = level.time;
		}
		lastHit = &detHits[i];
	}
	// getting here means we've filled our list of detHits, so begin recycling them, starting with the oldest hit.
	curHit = &detHits[0];
	while (lastHit->time < curHit->time)
	{
		lastHit = curHit;
		curHit++;
		// just a safety check here
		if (curHit == &detHits[0])
		{
			break;
		}
	}
	curHit->detpack = det;
	curHit->player = player;
	curHit->time = level.time;
}

void ClearThisDetpacksHits(gentity_t *det)
{
	int			i = 0;

	for (i = 0; i < MAX_DETHITS; i++)
	{
		if (detHits[i].detpack == det)
		{
			detHits[i].player = NULL;
			detHits[i].detpack = NULL;
			detHits[i].time = 0;
		}
	}
}

void DetpackBlammoThink(gentity_t *ent)
{
	int i = 0, lifetime = 3000;	// how long (in msec) the shockwave lasts
	int			knockback = 400;
	float		curBlastRadius = 50.0*ent->count, radius = 0;
	vec3_t		vTemp;
	trace_t		tr;
	gentity_t	*pl = NULL;

	if (ent->count++ > (lifetime*0.01))
	{
		ClearThisDetpacksHits(ent);
		G_FreeEntity(ent);
		return;
	}

	// get a list of players within the blast radius at this time.
	// only hit the ones we can see from the center of the explosion.
	for (i=0; i<MAX_CLIENTS; i++)
	{
		if ( g_entities[i].client && g_entities[i].takedamage) 
		{
			pl = &g_entities[i];
			VectorSubtract(pl->s.pos.trBase, ent->s.origin, vTemp);
			radius = VectorNormalize(vTemp);
			if ( (radius <= curBlastRadius) && !PlayerHitByDet(ent, pl) )
			{
				// within the proper radius. do we have LOS to the player?
				trap_Trace (&tr, ent->s.origin, NULL, NULL, pl->s.pos.trBase, ent->s.number, MASK_SHOT );
				if (tr.entityNum == i)
				{
					// oh yeah. you're gettin' hit.
					AddPlayerToDetHits(ent, pl);
					VectorMA(pl->client->ps.velocity, knockback, vTemp, pl->client->ps.velocity);
					// make sure the player goes up some
					if (pl->client->ps.velocity[2] < 100)
					{
						pl->client->ps.velocity[2] = 100;
					}
					if ( !pl->client->ps.pm_time )
					{
						int		t;

						t = knockback * 2;
						if ( t < 50 ) {
							t = 50;
						}
						if ( t > 200 ) {
							t = 200;
						}
						pl->client->ps.pm_time = t;
						pl->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
					}
				}
			}
			// this was just for testing. looks pretty neat, though.
/*
			else
			{
				VectorMA(ent->s.origin, curBlastRadius, vTemp, vTemp);
				G_TempEntity(vTemp, EV_FX_STEAM);
			}
*/
		} 
	}

	ent->nextthink = level.time + FRAMETIME;
}

void DetonateDetpack(gentity_t *ent)
{
	// find all detpacks. the one whose parent is ent...blow up
	gentity_t	*detpack = NULL;
	char		*classname = BG_FindClassnameForHoldable(HI_DETPACK);

	if (!classname)
	{
		return;
	}
	while ((detpack = G_Find (detpack, FOFS(classname), classname)) != NULL)
	{
		if (detpack->parent == ent)
		{
			// found it. BLAMMO!
			// play explosion sound to all clients
			gentity_t	*te = NULL;

			te = G_TempEntity( detpack->s.pos.trBase, EV_GLOBAL_SOUND );
			te->s.eventParm = G_SoundIndex( "sound/weapons/explosions/detpakexplode.wav" );//cgs.media.detpackExplodeSound
			te->r.svFlags |= SVF_BROADCAST;

			//so we can't be blown up by things we're blowing up
			detpack->takedamage = 0;

			G_AddEvent(detpack, EV_DETPACK, 0);
			G_RadiusDamage( detpack->s.origin, detpack->parent, DETPACK_DAMAGE, DETPACK_RADIUS, 
				detpack, DAMAGE_HALF_NOTLOS|DAMAGE_ALL_TEAMS, MOD_DETPACK );
			// just turn the model invisible and let the entity think for a bit to deliver a shockwave
			//G_FreeEntity(detpack);
			detpack->classname = NULL;
			detpack->s.modelindex = 0;
			detpack->think = DetpackBlammoThink;
			detpack->count = 1;
			detpack->nextthink = level.time + FRAMETIME;
			return;
		}
		else if (detpack == ent)	// if detpack == ent, we're blowing up this detpack cuz it's been sitting too long
		{
			detpack_shot(detpack, NULL, NULL, 0, 0);
			return;
		}
	}
	// hmm. couldn't find it.
	detpack = NULL;
}





#define SHIELD_HEALTH				250
#define SHIELD_HEALTH_DEC			10		// 25 seconds	
#define MAX_SHIELD_HEIGHT			254
#define MAX_SHIELD_HALFWIDTH		255
#define SHIELD_HALFTHICKNESS		4
#define SHIELD_PLACEDIST			64


static qhandle_t	shieldAttachSound=0;
static qhandle_t	shieldActivateSound=0;
static qhandle_t	shieldDamageSound=0;


void ShieldRemove(gentity_t *self)
{
	self->think = G_FreeEntity;
	self->nextthink = level.time + 100;

	// Play raising sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldActivateSound);

	return;
}


// Count down the health of the shield.
void ShieldThink(gentity_t *self)
{
	self->s.eFlags &= ~(EF_ITEMPLACEHOLDER | EF_NODRAW);
	self->health -= SHIELD_HEALTH_DEC;
	self->nextthink = level.time + 1000;
	if (self->health <= 0)
	{
		ShieldRemove(self);
	}
	return;
}


// The shield was damaged to below zero health.
void ShieldDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	// Play damaging sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldDamageSound);

	ShieldRemove(self);
}


// The shield had damage done to it.  Make it flicker.
void ShieldPain(gentity_t *self, gentity_t *attacker, int damage)
{
	// Set the itemplaceholder flag to indicate the the shield drawing that the shield pain should be drawn.
	self->s.eFlags |= EF_ITEMPLACEHOLDER;
	self->think = ShieldThink;
	self->nextthink = level.time + 400;

	// Play damaging sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldDamageSound);

	return;
}


// Try to turn the shield back on after a delay.
void ShieldGoSolid(gentity_t *self)
{
	trace_t		tr;

	// see if we're valid
	self->health--;
	if (self->health <= 0)
	{
		ShieldRemove(self);
		return;
	}
	
	trap_Trace (&tr, self->r.currentOrigin, self->r.mins, self->r.maxs, self->r.currentOrigin, self->s.number, CONTENTS_BODY );
	if(tr.startsolid)
	{	// gah, we can't activate yet
		self->nextthink = level.time + 200;
		self->think = ShieldGoSolid;
		trap_LinkEntity(self);
	}
	else
	{ // get hard... huh-huh...
		self->r.contents = CONTENTS_SOLID;
		self->s.eFlags &= ~(EF_NODRAW | EF_ITEMPLACEHOLDER);
		self->nextthink = level.time + 1000;
		self->think = ShieldThink;
		self->takedamage = qtrue;
		trap_LinkEntity(self);

		// Play raising sound...
		G_AddEvent(self, EV_GENERAL_SOUND, shieldActivateSound);
	}

	return;
}


// Turn the shield off to allow a friend to pass through.
void ShieldGoNotSolid(gentity_t *self)
{
	// make the shield non-solid very briefly
	self->r.contents = CONTENTS_NONE;
	self->s.eFlags |= EF_NODRAW;
	// nextthink needs to have a large enough interval to avoid excess accumulation of Activate messages
	self->nextthink = level.time + 200;
	self->think = ShieldGoSolid;
	self->takedamage = qfalse;
	trap_LinkEntity(self);

	// Play raising sound...
	G_AddEvent(self, EV_GENERAL_SOUND, shieldActivateSound);
}


// Somebody (a player) has touched the shield.  See if it is a "friend".
void ShieldTouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if (g_gametype.integer >= GT_TEAM)
	{ // let teammates through
		// compare the parent's team to the "other's" team
		if (( self->parent->client) && (other->client))
		{
			if ( self->parent->client->sess.sessionTeam == other->client->sess.sessionTeam || other->client->sess.sessionClass == PC_TECH )
			{
				ShieldGoNotSolid(self);
			}
		}
	}
	else
	{//let the person who dropped the shield through
		if (self->parent->s.number == other->s.number)
		{
			ShieldGoNotSolid(self);
		}
	}
}


// After a short delay, create the shield by expanding in all directions.
void CreateShield(gentity_t *ent)
{
	trace_t		tr;
	vec3_t		mins, maxs, end, posTraceEnd, negTraceEnd, start;
	int			height, posWidth, negWidth, halfWidth = 0;
	qboolean	xaxis;
	int			paramData = 0;
	static int	shieldID;

	// trace upward to find height of shield
	VectorCopy(ent->r.currentOrigin, end);
	end[2] += MAX_SHIELD_HEIGHT;
	trap_Trace (&tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT );
	height = (int)(MAX_SHIELD_HEIGHT * tr.fraction);

	// use angles to find the proper axis along which to align the shield
	VectorSet(mins, -SHIELD_HALFTHICKNESS, -SHIELD_HALFTHICKNESS, 0);
	VectorSet(maxs, SHIELD_HALFTHICKNESS, SHIELD_HALFTHICKNESS, height);
	VectorCopy(ent->r.currentOrigin, posTraceEnd);
	VectorCopy(ent->r.currentOrigin, negTraceEnd);

	if ((int)(ent->s.angles[YAW]) == 0) // shield runs along y-axis
	{
		posTraceEnd[1]+=MAX_SHIELD_HALFWIDTH;
		negTraceEnd[1]-=MAX_SHIELD_HALFWIDTH;
		xaxis = qfalse;
	}
	else  // shield runs along x-axis
	{
		posTraceEnd[0]+=MAX_SHIELD_HALFWIDTH;
		negTraceEnd[0]-=MAX_SHIELD_HALFWIDTH;
		xaxis = qtrue;
	}

	// trace horizontally to find extend of shield
	// positive trace
	VectorCopy(ent->r.currentOrigin, start);
	start[2] += (height>>1);
	trap_Trace (&tr, start, 0, 0, posTraceEnd, ent->s.number, MASK_SHOT );
	posWidth = MAX_SHIELD_HALFWIDTH * tr.fraction;
	// negative trace
	trap_Trace (&tr, start, 0, 0, negTraceEnd, ent->s.number, MASK_SHOT );
	negWidth = MAX_SHIELD_HALFWIDTH * tr.fraction;

	// kef -- monkey with dimensions and place origin in center
	halfWidth = (posWidth + negWidth)>>1;
	if (xaxis)
	{
		ent->r.currentOrigin[0] = ent->r.currentOrigin[0] - negWidth + halfWidth;
	}
	else
	{
		ent->r.currentOrigin[1] = ent->r.currentOrigin[1] - negWidth + halfWidth;
	}
	ent->r.currentOrigin[2] += (height>>1);

	// set entity's mins and maxs to new values, make it solid, and link it
	if (xaxis)
	{
		VectorSet(ent->r.mins, -halfWidth, -SHIELD_HALFTHICKNESS, -(height>>1));
		VectorSet(ent->r.maxs, halfWidth, SHIELD_HALFTHICKNESS, height>>1);
	}
	else
	{
		VectorSet(ent->r.mins, -SHIELD_HALFTHICKNESS, -halfWidth, -(height>>1));
		VectorSet(ent->r.maxs, SHIELD_HALFTHICKNESS, halfWidth, height);
	}
	ent->clipmask = MASK_SHOT;

	// Information for shield rendering.

//	xaxis - 1 bit
//	height - 0-254 8 bits
//	posWidth - 0-255 8 bits
//  negWidth - 0 - 255 8 bits

	paramData = (xaxis << 24) | (height << 16) | (posWidth << 8) | (negWidth);
	ent->s.time2 = paramData;

	if ( g_pModSpecialties.integer != 0 )
	{
		if ( ent->s.otherEntityNum2 == TEAM_RED )
		{
			ent->team = "1";
		}
		else if ( ent->s.otherEntityNum2 == TEAM_BLUE )
		{
			ent->team = "2";
		}
		ent->health = ceil(SHIELD_HEALTH*4*g_dmgmult.value);
	}
	else
	{
		ent->health = ceil(SHIELD_HEALTH*g_dmgmult.value);
	}

	ent->s.time = ent->health;//???
	ent->pain = ShieldPain;
	ent->die = ShieldDie;
	ent->touch = ShieldTouch;

	ent->r.svFlags |= SVF_SHIELD_BBOX;	

	// see if we're valid
	trap_Trace (&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ent->s.number, CONTENTS_BODY ); 

	if (tr.startsolid)
	{	// Something in the way!
		// make the shield non-solid very briefly
		ent->r.contents = CONTENTS_NONE;
		ent->s.eFlags |= EF_NODRAW;
		// nextthink needs to have a large enough interval to avoid excess accumulation of Activate messages
		ent->nextthink = level.time + 200;
		ent->think = ShieldGoSolid;
		ent->takedamage = qfalse;
		trap_LinkEntity(ent);
	}
	else
	{	// Get solid.
		ent->r.contents = CONTENTS_PLAYERCLIP|CONTENTS_SHOTCLIP;//CONTENTS_SOLID;

		ent->nextthink = level.time + 1000;
		ent->think = ShieldThink;

		ent->takedamage = qtrue;
		trap_LinkEntity(ent);

		// Play raising sound...
		G_AddEvent(ent, EV_GENERAL_SOUND, shieldActivateSound);
	}

	return;
}

qboolean PlaceShield(gentity_t *playerent)
{
	static const gitem_t *shieldItem = NULL;
	gentity_t	*shield = NULL;
	trace_t		tr;
	vec3_t		fwd, pos, dest, mins = {-16,-16, 0}, maxs = {16,16,16};

	if (shieldAttachSound==0)
	{
		shieldAttachSound = G_SoundIndex("sound/weapons/detpacklatch.wav");
		shieldActivateSound = G_SoundIndex("sound/movers/forceup.wav");
		shieldDamageSound = G_SoundIndex("sound/ambience/spark5.wav");
		shieldItem = BG_FindItemForHoldable(HI_SHIELD);
	}

	// can we place this in front of us?
	AngleVectors (playerent->client->ps.viewangles, fwd, NULL, NULL);
	fwd[2] = 0;
	VectorMA(playerent->client->ps.origin, SHIELD_PLACEDIST, fwd, dest);
	trap_Trace (&tr, playerent->client->ps.origin, mins, maxs, dest, playerent->s.number, MASK_SHOT );
	if (tr.fraction > 0.9)
	{//room in front
		VectorCopy(tr.endpos, pos);
		// drop to floor
		VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
		trap_Trace( &tr, pos, mins, maxs, dest, playerent->s.number, MASK_SOLID );
		if ( !tr.startsolid && !tr.allsolid )
		{
			// got enough room so place the portable shield
			shield = G_Spawn();

			// Figure out what direction the shield is facing.
			if (fabs(fwd[0]) > fabs(fwd[1]))
			{	// shield is north/south, facing east.
				shield->s.angles[YAW] = 0;
			}
			else
			{	// shield is along the east/west axis, facing north
				shield->s.angles[YAW] = 90;
			}
			shield->think = CreateShield;
			shield->nextthink = level.time + 500;	// power up after .5 seconds
			shield->parent = playerent;

			// Set team number.
			shield->s.otherEntityNum2 = playerent->client->sess.sessionTeam;

			shield->s.eType = ET_USEABLE;
			shield->s.modelindex =  HI_SHIELD;	// this'll be used in CG_Useable() for rendering.
			shield->classname = shieldItem->classname;

			shield->r.contents = CONTENTS_TRIGGER;

			shield->touch = 0;
			// using an item causes it to respawn
			shield->use = 0; //Use_Item;

			// allow to ride movers
			shield->s.groundEntityNum = tr.entityNum;

			G_SetOrigin( shield, tr.endpos );

			shield->s.eFlags &= ~EF_NODRAW;
			shield->r.svFlags &= ~SVF_NOCLIENT;

			trap_LinkEntity (shield);

			// Play placing sound...
			G_AddEvent(shield, EV_GENERAL_SOUND, shieldAttachSound);

			return qtrue;
		}
	}
	// no room
	return qfalse;
}



//-------------------------------------------------------------- DECOY ACTIVITIES
void DecoyThink(gentity_t *ent)
{
	ent->s.apos =(ent->parent)->s.apos;					// Update Current Rotation
	ent->nextthink = level.time + irandom(2000, 6000);	// Next think between 2 & 8 seconds

	(ent->count) --;									// Count Down
	if (ent->count<0)			G_FreeEntity(ent);		// Time To Erase The Ent
}

qboolean PlaceDecoy(gentity_t *ent)
{
	gentity_t	*decoy = NULL;
	static gitem_t *decoyItem = NULL;
	float		detDistance = 80;	// Distance the object will be placed from player
	trace_t		tr;
	vec3_t		fwd, right, up, end, mins = {-16,-16,-24}, maxs = {16,16,16};
	
	if (decoyItem == NULL)
	{
		decoyItem = BG_FindItemForHoldable(HI_DECOY);
	}

	// can we place this in front of us?
	AngleVectors (ent->client->ps.viewangles, fwd, right, up);
	fwd[2] = 0;
	VectorMA(ent->client->ps.origin, detDistance, fwd, end);
	trap_Trace (&tr, ent->client->ps.origin, mins, maxs, end, ent->s.number, MASK_SHOT );
	if ( !tr.allsolid && !tr.startsolid && tr.fraction > 0.9 )
	{
		//--------------------------- SPAWN AND PLACE DECOY ON GROUND
		decoy = G_Spawn();
		G_SpawnItem(decoy, decoyItem);				// Generate it as an item, temporarly
		decoy->physicsBounce = 0.0f;//decoys are *not* bouncy
		VectorMA(ent->client->ps.origin, detDistance + mins[0], fwd, decoy->s.origin);
		decoy->r.mins[2] = mins[2];//keep it off the floor
		VectorNegate(fwd, fwd);					// ???  What does this do??
		vectoangles(fwd, decoy->s.angles);
		if ( !FinishSpawningDecoy(decoy, decoyItem - bg_itemlist) )
		{
			return qfalse;		// Drop to ground and trap to clients
		}
		decoy->s.clientNum = ent->client->ps.clientNum;

		//--------------------------- SPECIALIZED DECOY SETUP
		decoy->think = DecoyThink;
		decoy->count = 12;						// about 1 minute before dissapear
		decoy->nextthink = level.time + 4000;	// think after 4 seconds
		decoy->parent = ent;

		(decoy->s).eType = (ent->s).eType;		// set to type PLAYER
		(decoy->s).eFlags= (ent->s).eFlags;
		(decoy->s).eFlags |= EF_ITEMPLACEHOLDER;// set the HOLOGRAM FLAG to ON

		decoy->s.weapon = ent->s.weapon;		// get Player's Wepon Type
//		decoy->s.constantLight = 10 + (10 << 8) + (10 << 16) + (9 << 24);

		//decoy->s.pos.trBase[2] += 24;			// shift up to adjust origin of body
		decoy->s.apos = ent->s.apos;			// copy angle of player to decoy

		decoy->s.legsAnim = LEGS_IDLE;			// Just standing
		decoy->s.torsoAnim = TORSO_STAND;

		//--------------------------- WEAPON ADJUST
		if (decoy->s.weapon==WP_PHASER || decoy->s.weapon==WP_DREADNOUGHT)
			decoy->s.weapon = WP_COMPRESSION_RIFLE;

		//   The Phaser and Dreadnought (Arc Welder) weapons are rendered on the
		//   client side differently, and cannot be used by the decoy

		return qtrue;						// SUCCESS
	}
	else
	{
		return qfalse;						// FAILURE: no room
	}
}
//-------------------------------------------------------------- DECOY ACTIVITIES





void G_Rematerialize( gentity_t *ent )
{
	if ( ent->s.number == borgQueenClientNum )
	{
		ent->client->teleportTime = level.time + 60000;//get it back in 60 seconds
		ent->client->ps.stats[STAT_USEABLE_PLACED] = 60;
	}
	else
	{
		ent->client->teleportTime = level.time + 15000;//get it back in 15 seconds
		ent->client->ps.stats[STAT_USEABLE_PLACED] = 15;
	}
	ent->flags &= ~FL_NOTARGET;
	ent->takedamage = qtrue;
	ent->r.contents = MASK_PLAYERSOLID;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->client->ps.eFlags &= ~EF_NODRAW;
	TeleportPlayer( ent, ent->client->ps.origin, ent->client->ps.viewangles, TP_BORG );
	//ent->client->ps.stats[STAT_USEABLE_PLACED] = 0;
	//take it away
	ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
}

void G_GiveHoldable( gclient_t *client, holdable_t item )
{
	gitem_t	*holdable = BG_FindItemForHoldable( item );

	client->ps.stats[STAT_HOLDABLE_ITEM] = holdable - bg_itemlist;//teleport spots should be on other side of map
	RegisterItem( holdable );
}

/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int		i;
	int		event;
	gclient_t *client;
	int		damage;

	client = ent->client;

	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

		switch ( event ) {
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
			if ( ent->s.eType != ET_PLAYER ) {
				break;		// not in the player model
			}
			if ( g_dmflags.integer & DF_NO_FALLING ) {
				break;
			}
			if ( event == EV_FALL_FAR ) {
				damage = 10;
			} else {
				damage = 5;
			}
			ent->pain_debounce_time = level.time + 200;	// no normal pain sound
			G_Damage (ent, NULL, NULL, NULL, NULL, damage, DAMAGE_ARMOR_PIERCING, MOD_FALLING);
			break;

		case EV_FIRE_WEAPON:
			FireWeapon( ent, qfalse );
			break;

		case EV_ALT_FIRE:
			FireWeapon( ent, qtrue );
			break;

		case EV_FIRE_EMPTY_PHASER:
			FireWeapon( ent, qfalse );
			break;

		case EV_USE_ITEM1:		// transporter
			//------------------------------------------------------- DROP FLAGS
			if      ( ent->client->ps.powerups[PW_REDFLAG] ) {
				Team_ReturnFlag(TEAM_RED);
			}
			else if ( ent->client->ps.powerups[PW_BLUEFLAG] ) {
				Team_ReturnFlag(TEAM_BLUE);
			}
			ent->client->ps.powerups[PW_BLUEFLAG] = 0;
			ent->client->ps.powerups[PW_REDFLAG]  = 0;
			//------------------------------------------------------- DROP FLAGS
			if ( ent->client->sess.sessionClass == PC_BORG )
			{
				if ( !ent->client->ps.stats[STAT_USEABLE_PLACED] )
				{//go into free-roaming mode
					gentity_t	*tent;
					//FIXME: limit this to 10 seconds
					ent->flags |= FL_NOTARGET;
					ent->takedamage = qfalse;
					ent->r.contents = CONTENTS_PLAYERCLIP;
					ent->s.eFlags |= EF_NODRAW;
					ent->client->ps.eFlags |= EF_NODRAW;
					ent->client->ps.stats[STAT_USEABLE_PLACED] = 2;
					ent->client->teleportTime = level.time + 10000;
					tent = G_TempEntity( ent->client->ps.origin, EV_BORG_TELEPORT );
					tent->s.clientNum = ent->s.clientNum;
				}
				else
				{//come out of free-roaming mode, teleport to current position
					G_Rematerialize( ent );
				}
			}
			else// get rid of transporter and go to random spawnpoint
			{
				vec3_t	origin, angles;

				ent->client->ps.stats[STAT_USEABLE_PLACED] = 0;
				SelectSpawnPoint( ent->client->ps.origin, origin, angles );
				TeleportPlayer( ent, origin, angles, TP_NORMAL );
			}
			break;

		case EV_USE_ITEM2:		// medkit
			// New set of rules.  You get either 100 health, or an extra 25, whichever is higher.
			// Give 1/4 health.
			ent->health += ent->client->ps.stats[STAT_MAX_HEALTH]*0.25;

			if (ent->health < ent->client->ps.stats[STAT_MAX_HEALTH])
			{	// If that doesn't bring us up to 100, make it go up to 100.
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
			}
			else if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH]*2)
			{	// Otherwise, 25 is all you get.  Just make sure we don't go above 200.
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH]*2;
			}
			break;

		case EV_USE_ITEM3:		// detpack
			// if we haven't placed it yet, place it
			if (0 == ent->client->ps.stats[STAT_USEABLE_PLACED])
			{
				if ( PlaceDetpack(ent) )
				{
					ent->client->ps.stats[STAT_USEABLE_PLACED] = 1;
					trap_SendServerCommand( ent-g_entities, "cp \"CHARGE PLACED\"" );
				}
				else
				{//couldn't place it
					ent->client->ps.stats[STAT_HOLDABLE_ITEM] = (BG_FindItemForHoldable( HI_DETPACK ) - bg_itemlist);
					trap_SendServerCommand( ent-g_entities, "cp \"NO ROOM TO PLACE CHARGE\"" );
				}
			}
			else
			{
				//turn off invincibility since you are going to do massive damage
				//ent->client->ps.powerups[PW_GHOST] = 0;//NOTE: this means he can respawn and get a detpack 10 seconds later
				// ok, we placed it earlier. blow it up.
				ent->client->ps.stats[STAT_USEABLE_PLACED] = 0;
				DetonateDetpack(ent);
			}
			break;

		case EV_USE_ITEM4:		// portable shield
			if ( !PlaceShield(ent) )	// fixme if we fail, perhaps just spawn it as a pickup
			{//couldn't place it
				ent->client->ps.stats[STAT_HOLDABLE_ITEM] = (BG_FindItemForHoldable( HI_SHIELD ) - bg_itemlist);
				trap_SendServerCommand( ent-g_entities, "cp \"NO ROOM TO PLACE FORCE FIELD\"" );
			}
			else
			{
				trap_SendServerCommand( ent-g_entities, "cp \"FORCE FIELD PLACED\"" );
			}
			break;

		case EV_USE_ITEM5:		// decoy
			if ( !PlaceDecoy(ent) )
			{//couldn't place it
				ent->client->ps.stats[STAT_HOLDABLE_ITEM] = (BG_FindItemForHoldable( HI_DECOY ) - bg_itemlist);
				trap_SendServerCommand( ent-g_entities, "cp \"NO ROOM TO PLACE DECOY\"" );
			}
			else
			{
				trap_SendServerCommand( ent-g_entities, "cp \"DECOY PLACED\"" );
			}
			break;

		default:
			break;
		}
	}

}

void BotTestSolid(vec3_t origin);

/*
==============
SendPendingPredictableEvents
==============
*/
void SendPendingPredictableEvents( playerState_t *ps ) {
	gentity_t *t;
	int event, seq;
	int extEvent;

	// if there are still events pending
	if ( ps->entityEventSequence < ps->eventSequence ) {
		// create a temporary entity for this event which is sent to everyone
		// except the client generated the event
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_TempEntity( ps->origin, event );
		BG_PlayerStateToEntityState( ps, &t->s, qtrue );
		t->s.eType = ET_EVENTS + event;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *ent ) {
	gclient_t	*client;
	pmove_t		pm;
	vec3_t		oldOrigin;
	int			oldEventSequence;
	int			msec;
	usercmd_t	*ucmd;

	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}
	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		G_Printf("serverTime <<<<<\n" );
	}
	if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
//		G_Printf("serverTime >>>>>\n" );
	} 

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		ClientIntermissionThink( client );
		return;
	}

	// Don't move while under intro sequence.
	if (client->ps.introTime > level.time)
	{	// Don't be visible either.
		ent->s.eFlags |= EF_NODRAW;
		SetClientViewAngle( ent, ent->s.angles );
		ucmd->buttons = 0;
		ucmd->weapon = 0;
//		ucmd->angles[0] = ucmd->angles[1] = ucmd->angles[2] = 0;
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
//		return;
	}

	// spectators don't do much
	if ( client->sess.sessionTeam == TEAM_SPECTATOR || (client->ps.eFlags&EF_ELIMINATED) ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			return;
		}
		SpectatorThink( ent, ucmd );
		return;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) {
		return;
	}

	// clear the rewards if time
	if ( level.time > client->rewardTime ) {
		client->ps.eFlags &= ~EF_AWARD_MASK;
	}

	if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		client->ps.pm_type = PM_DEAD;
	} else {
		client->ps.pm_type = PM_NORMAL;
	}

	client->ps.gravity = g_gravity.value;

	// set speed
	client->ps.speed = g_speed.value;

	if ( client->ps.powerups[PW_HASTE] ) 
	{
		client->ps.speed *= 1.5;
	}
	else if (client->ps.powerups[PW_FLIGHT] || BG_BorgTransporting( &client->ps ) )
	{//flying around
		client->ps.speed *= 1.25;
	}
	if ( client->sess.sessionClass == PC_HEAVY || 
		(client->sess.sessionClass == PC_BORG && !BG_BorgTransporting( &client->ps )) ||
		(client->lasthurt_mod == MOD_ASSIMILATE && ent->pain_debounce_time > level.time) )
	{//heavy weaps guy and borg move slower, as do people being assimilated
		client->ps.speed *= 0.75;
	}

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;
	pm.pModDisintegration = g_pModDisintegration.integer > 0;

	VectorCopy( client->ps.origin, oldOrigin );

	// perform a pmove
	Pmove (&pm);

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}
	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );

	SendPendingPredictableEvents( &ent->client->ps );

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	VectorCopy (pm.mins, ent->r.mins);
	VectorCopy (pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute client events
	ClientEvents( ent, oldEventSequence );

	if ( pm.useEvent )
	{		//TODO: Use
		TryUse( ent );
	}

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	//test for solid areas in the AAS file
	BotTestSolid(ent->r.currentOrigin);

	// touch other objects
	ClientImpacts( ent, &pm );

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		// wait for the attack button to be pressed
		if ( level.time > client->respawnTime ) {
			// forcerespawn is to prevent users from waiting out powerups
			if ( g_forcerespawn.integer > 0 && 
				( level.time - client->respawnTime ) > g_forcerespawn.integer * 1000 ) {
				respawn( ent );
				return;
			}
		
			// pressing attack or use is the normal respawn method
			if ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) {
				respawn( ent );
				return;
			}

			//in assimilation and elimination, always force respawn
			if ( level.time - client->respawnTime > 1300 && //NOTE: when killed, client->respawnTime = level.time + 1700, so this is 3000 ms
				( g_pModAssimilation.integer || g_pModElimination.integer ) )
			{
				respawn( ent );
				return;
			}
		}
		return;
	}

	// perform once-a-second actions
	ClientTimerActions( ent, msec );

	if ( ent->client->teleportTime > 0 && ent->client->teleportTime < level.time )
	{
		if ( ent->client->sess.sessionClass == PC_BORG )
		{
			if ( BG_BorgTransporting( &ent->client->ps ) )
			{//in Borg teleport warp
				G_Rematerialize( ent );
			}
			else
			{//waiting to get another teleport
				G_GiveHoldable( ent->client, HI_TRANSPORTER );
				ent->client->teleportTime = 0;
			}
		}
		else if ( ent->client->sess.sessionClass == PC_DEMO )
		{//after an initial delay, the demo gets the detpack
			G_GiveHoldable( ent->client, HI_DETPACK );//give it to me
			ent->client->ps.stats[STAT_USEABLE_PLACED] = 0;//make sure I'm set up to be able to drop it
			ent->client->teleportTime = 0;//don't ever give it to me again
		}
	}
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t *ent;

	ent = g_entities + clientNum;
	trap_GetUsercmd( clientNum, &ent->client->pers.cmd );

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	if ( !g_synchronousClients.integer ) {
		ClientThink_real( ent );
	}
}


void G_RunClient( gentity_t *ent ) {
	if ( !g_synchronousClients.integer ) {
		return;
	}
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real( ent );
}


/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*cl;

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		int		clientNum;

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if ( clientNum == -1 ) {
			clientNum = level.follow1;
		} else if ( clientNum == -2 ) {
			clientNum = level.follow2;
		}
		if ( clientNum >= 0 ) {
			cl = &level.clients[ clientNum ];
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
				ent->client->ps = cl->ps;
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if ( ent->client->sess.spectatorClient >= 0 ) {
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientBegin( ent->client - level.clients, qfalse );
				}
			}
		}
	}

	if ( ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
	int			i;
	clientPersistant_t	*pers;

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || (ent->client->ps.eFlags&EF_ELIMINATED) ) {
		SpectatorClientEndFrame( ent );
		return;
	}

	pers = &ent->client->pers;

	// turn off any expired powerups
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ent->client->ps.powerups[ i ] < level.time ) {
			ent->client->ps.powerups[ i ] = 0;
		}
	}

	// save network bandwidth
#if 0
	if ( !g_synchronousClients->integer && ent->client->ps.pm_type == PM_NORMAL ) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear( ent->client->ps.viewangles );
	}
#endif

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime ) {
		return;
	}

	// burn from lava, etc
	P_WorldEffects (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - ent->client->lastCmdTime > 1000 ) {
		ent->s.eFlags |= EF_CONNECTION;
	} else {
		ent->s.eFlags &= ~EF_CONNECTION;
	}

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	G_SetClientSound (ent);

	// set the latest infor
	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	SendPendingPredictableEvents( &ent->client->ps );

	/*
	if ( noJoinLimit != 0 && level.time > noJoinLimit )
	{//turn off objectives
		trap_SendServerCommand( ent-g_entities, "-analyze" );
	}
	*/
}


