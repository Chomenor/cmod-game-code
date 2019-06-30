// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_public.h -- definitions shared by both the server game and client game modules

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame
#define	GAME_VERSION		"baseef-1"

#define	DEFAULT_GRAVITY		800
#define	GIB_HEALTH			-40
#define	ARMOR_PROTECTION	1.0//0.66
#define PIERCED_ARMOR_PROTECTION		0.50 // trek: shields only stop 50% of armor-piercing dmg
// #define SUPER_PIERCED_ARMOR_PROTECTION	0.25 // trek: shields only stop 25% of super-armor-piercing dmg

#define	MAX_ITEMS			256

#define	RANK_TIED_FLAG		0x4000

#define	ITEM_RADIUS			15		// item sizes are needed for client side pickup detection

#define	LIGHTNING_RANGE		768

#define	SCORE_NOT_PRESENT	-9999	// for the CS_SCORES[12] when only one player is present

#define	VOTE_TIME			30000	// 30 seconds before vote times out

#define	MINS_Z				-24
#define	DEFAULT_VIEWHEIGHT	26
#define CROUCH_VIEWHEIGHT	12
#define	DEAD_VIEWHEIGHT		-16

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define	CS_MUSIC			2
#define	CS_MESSAGE			3		// from the map worldspawn's message field
#define	CS_MOTD				4		// g_motd string for server message of the day
#define	CS_WARMUP			5		// server time when the match will be restarted
#define	CS_SCORES1			6
#define	CS_SCORES2			7
#define CS_VOTE_TIME		8
#define CS_VOTE_STRING		9
#define	CS_VOTE_YES			10
#define	CS_VOTE_NO			11
#define	CS_GAME_VERSION		12
#define	CS_LEVEL_START_TIME	13		// so the timer only shows the current level
#define	CS_INTERMISSION		14		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS		15		// string indicating flag status in CTF
#define CS_RED_GROUP		16		// used to send down what the group the red team is
#define CS_BLUE_GROUP		17		// used to send down what the group the blue team is
#define	CS_ITEMS			27		// string of 0's and 1's that tell which items are present

#define	CS_MODELS			32
#define	CS_SOUNDS			(CS_MODELS+MAX_MODELS)
#define	CS_PLAYERS			(CS_SOUNDS+MAX_SOUNDS)
#define CS_LOCATIONS		(CS_PLAYERS+MAX_CLIENTS)

#define CS_MAX				(CS_LOCATIONS+MAX_LOCATIONS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef enum {
	GT_FFA,				// free for all
	GT_TOURNAMENT,		// one on one tournament
	GT_SINGLE_PLAYER,	// single player tournament

	//-- team games go after this --

	GT_TEAM,			// team deathmatch
	GT_CTF,				// capture the flag

	GT_MAX_GAME_TYPE
} gametype_t;

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION	// no movement or status bar
} pmtype_t;

typedef enum {
	WEAPON_READY, 
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_WATERJUMP	256		// pm_time is waterjump
#define	PMF_RESPAWNED		512		// clear after attack and jump buttons come up
#define	PMF_USE_ITEM_HELD	1024
#define PMF_GRAPPLE_PULL	2048	// pull towards grapple location
#define PMF_FOLLOW			4096	// spectate following another player
#define PMF_SCOREBOARD		8192	// spectate as a scoreboard

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

#define	MAXTOUCH	32
typedef struct {
	// state (in / out)
	playerState_t	*ps;

	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qboolean	noFootsteps;		// if the game is setup for no footsteps by the server
	qboolean	pModDisintegration;		// true if the Disintegration playerMod is on

	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];

	int			useEvent;

	vec3_t		mins, maxs;			// bounding box size

	int			watertype;
	int			waterlevel;

	float		xyspeed;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int			(*pointcontents)( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void Pmove (pmove_t *pmove);

//===================================================================================


// player_state->stats[] indexes
//
// maximum of MAX_STATS...currently 16
typedef enum {
	STAT_HEALTH,
	STAT_HOLDABLE_ITEM,
	STAT_WEAPONS,					// 16 bit fields
	STAT_ARMOR,				
	STAT_DEAD_YAW,					// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY,				// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH,				// health / armor limit, changable by handicap
	STAT_USEABLE_PLACED,			// have we placed the detpack yet?
} statIndex_t;

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
typedef enum {
	PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,						// total points damage inflicted so damage beeps can sound on change
	PERS_SHIELDS,					// total shield points damage inflicted so damage beeps can sound on change
	PERS_RANK,				
	PERS_TEAM,				
	PERS_SPAWN_COUNT,				// incremented every respawn
	PERS_REWARD_COUNT,				// incremented for each reward sound
	PERS_REWARD,					// a reward_t
	PERS_ATTACKER,					// clientnum of last damage inflicter
	PERS_KILLED,					// count of the number of times you died
	// these were added for single player awards tracking
	PERS_IMPRESSIVE_COUNT,
	PERS_EXCELLENT_COUNT,
	PERS_CLASS,
	PERS_ACCURACY_SHOTS,
	PERS_ACCURACY_HITS,
	PERS_STREAK_COUNT
} persEnum_t;


// entityState_t->eFlags
// IMPORTANT:  You need to check entityStateFields in msg.c to be certain that enough bits are being sent!

#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#define EF_ITEMPLACEHOLDER	0x00000002		// faded items
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_BOUNCE_HALF		0x00000020		// for missiles
#define EF_MISSILE_STICK	0x00000040		// missiles that stick to the wall.
#define	EF_NODRAW			0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#define EF_ALT_FIRING		0x00000200		// for alt-fires, mostly for lightning guns though
#define	EF_ELIMINATED		0x00000400		// Eliminatied in Elimination mode
#define	EF_SHIELD_BOX_X		0x00000800		// tells the client to use special client-collision info, x-axis aligned
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define EF_VOTED			0x00004000		// already cast a vote
#define	EF_AWARD_IMPRESSIVE	0x00008000		// draw an impressive sprite
#define EF_ANIM_ALLFAST		0x00010000		// automatically cycle through all frames at 10hz
#define EF_AWARD_FIRSTSTRIKE 0x00020000		// First blood on a map.
#define EF_AWARD_ACE		0x00040000		// 5 frags
#define EF_AWARD_EXPERT		0x00080000		// 10 frags
#define EF_AWARD_MASTER		0x00100000		// 15 frags
#define EF_AWARD_CHAMPION	0x00200000		// 20 frags
#define EF_SHIELD_BOX_Y		0x00400000		// tells the client to use special client-collision info, y-axis aligned
#define	EF_CLIENT_NODRAW	0x00800000		// prevents double pickup sounds, only set/removed by client
#define EF_ASSIMILATED		0x01000000		// Assimilated in Assimilation mode
#define EF_ANIM_ONCE		0x02000000		// cycle through all frames just once then stop
#define	EF_MOVER_STOP		0x04000000// will push otherwise


#define EF_AWARD_STREAK_MASK (EF_AWARD_ACE|EF_AWARD_EXPERT|EF_AWARD_MASTER|EF_AWARD_CHAMPION)
#define EF_AWARD_MASK		(EF_AWARD_EXCELLENT|EF_AWARD_IMPRESSIVE|EF_AWARD_FIRSTSTRIKE|EF_AWARD_STREAK_MASK)


typedef enum {
	PW_NONE,

	PW_QUAD,
	PW_BATTLESUIT,
	PW_HASTE,
	PW_INVIS,
	PW_REGEN,
	PW_FLIGHT,
	PW_SEEKER, 

	PW_REDFLAG,
 	PW_BLUEFLAG,
	PW_OUCH,

	PW_DISINTEGRATE,
	PW_GHOST,

	PW_EXPLODE,		
	PW_ARCWELD_DISINT,
	PW_BORG_ADAPT,

	PW_NUM_POWERUPS
} powerup_t;

typedef enum {
	HI_NONE,		 // keep this first enum entry equal to 0 so cgs.useableModels will work properly

	HI_TRANSPORTER,
	HI_MEDKIT,
	HI_DETPACK,
	HI_SHIELD,
	HI_DECOY,		// cdr 

	HI_NUM_HOLDABLE
} holdable_t;

typedef enum {
	WP_NONE,

	WP_PHASER,				// 3/13/00 kef -- used to be WP_LIGHTNING
	WP_COMPRESSION_RIFLE,	// 3/13/00 kef -- added
	WP_IMOD,				// 3/10/00 kef -- used to be WP_RAILGUN
	WP_SCAVENGER_RIFLE,		// 3/13/00 kef -- used to be WP_PLASMAGUN
	WP_STASIS,				// 3/13/00 kef -- used to be WP_ROCKET_LAUNCHER
	WP_GRENADE_LAUNCHER,	// 3/10/00 kef -- used to be...heh...WP_GRENADE_LAUNCHER
	WP_TETRION_DISRUPTOR,	// 3/13/00 kef -- added
	WP_QUANTUM_BURST,		// 3/13/00 kef -- added
	WP_DREADNOUGHT,			// 3/13/00 kef -- added
	WP_VOYAGER_HYPO,		// 10/6/00 mcg -- added for MP patch/gold
	WP_BORG_ASSIMILATOR,	// 10/12/00 jtd -- added for MP patch/gold
	WP_BORG_WEAPON,			// 10/12/00 jtd -- added for MP patch/gold - merges single player projectile and taser
	//WP_TRICORDER,			// 

	WP_NUM_WEAPONS
} weapon_t;


// reward sounds
typedef enum {
	REWARD_BAD,

	REWARD_IMPRESSIVE,		// One shot kill with Compression
	REWARD_EXCELLENT,		// Two frags in a row
	REWARD_DENIED,			// Near powerup, stolen
	REWARD_FIRST_STRIKE,	// First blood in a level
	REWARD_STREAK			// Ace/Expert/Master/Champion
} reward_t;

#define STREAK_ACE			5
#define STREAK_EXPERT		10
#define STREAK_MASTER		15
#define STREAK_CHAMPION		20


// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

typedef enum {
	EV_NONE,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,

	EV_JUMP_PAD,			// boing sound at origin, jump sound on player

	EV_JUMP,
	EV_WATER_TOUCH,	// foot touches
	EV_WATER_LEAVE,	// foot leaves
	EV_WATER_UNDER,	// head touches
	EV_WATER_CLEAR,	// head leaves

	EV_ITEM_PICKUP,			// normal item pickups are predictable
	EV_GLOBAL_ITEM_PICKUP,	// powerup / team sounds are broadcast to everyone

	EV_NOAMMO,
	EV_NOAMMO_ALT,
	EV_CHANGE_WEAPON,
	EV_FIRE_WEAPON,
	EV_ALT_FIRE,
	EV_FIRE_EMPTY_PHASER,

	EV_USE_ITEM0,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,

	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,

	EV_GRENADE_BOUNCE,		// eventParm will be the soundindex
	EV_MISSILE_STICK,		// eventParm will be the soundindex

	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,		// no attenuation
	EV_TEAM_SOUND,

	EV_MISSILE_HIT,
	EV_MISSILE_MISS,

	EV_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY,

	EV_POWERUP_BATTLESUIT,
	EV_POWERUP_REGEN,
	EV_POWERUP_SEEKER_FIRE,

	EV_DEBUG_LINE,
	EV_TAUNT,

	//
	// kef -- begin Trek stuff
	//
	
	// kef -- taken directly from Trek code
	EV_COMPRESSION_RIFLE,
	EV_COMPRESSION_RIFLE_ALT,

	EV_IMOD,
	EV_IMOD_HIT,
	EV_IMOD_ALTFIRE,
	EV_IMOD_ALTFIRE_HIT,

	EV_STASIS,

	EV_GRENADE_EXPLODE,
	EV_GRENADE_SHRAPNEL_EXPLODE,
	EV_GRENADE_SHRAPNEL,

	EV_DREADNOUGHT_MISS,

	EV_TETRION,

	EV_SHIELD_HIT,

	EV_FX_SPARK,
	EV_FX_STEAM,
	EV_FX_BOLT,
	EV_FX_TRANSPORTER_PAD,
	EV_FX_DRIP,

	EV_SCREENFX_TRANSPORTER,

	EV_DISINTEGRATION,
	EV_EXPLODESHELL,
	EV_ARCWELD_DISINT,

	EV_DETPACK,

	EV_DISINTEGRATION2,

	//
	// expansion pack
	//
	EV_OBJECTIVE_COMPLETE,
	EV_USE,
	EV_BORG_ALT_WEAPON,// TASER
	EV_BORG_TELEPORT,
	EV_FX_CHUNKS
} entity_event_t;


// animations
typedef enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,

	LEGS_TURN,

	MAX_ANIMATIONS
} animNumber_t;


typedef struct animation_s {
	int		firstFrame;
	int		numFrames;
	int		loopFrames;			// 0 to numFrames
	int		frameLerp;			// msec between frames
	int		initialLerp;		// msec to get to first frame
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define	ANIM_TOGGLEBIT		128


typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

enum {
	RETURN_FLAG_SOUND,
	SCORED_FLAG_SOUND,
	DROPPED_FLAG_SOUND,
	SCORED_FLAG_NO_VOICE_SOUND,
	MAX_TEAM_SOUNDS
};

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME		1000

// How many players on the overlay
#define TEAM_MAXOVERLAY		8

// means of death
typedef enum {
	MOD_UNKNOWN,

	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,

// Trek weapons
	MOD_PHASER,
	MOD_PHASER_ALT,
	MOD_CRIFLE,
	MOD_CRIFLE_SPLASH,
	MOD_CRIFLE_ALT,
	MOD_CRIFLE_ALT_SPLASH,
	MOD_IMOD,
	MOD_IMOD_ALT,
	MOD_SCAVENGER,
	MOD_SCAVENGER_ALT,
	MOD_SCAVENGER_ALT_SPLASH,
	MOD_STASIS,
	MOD_STASIS_ALT,
	MOD_GRENADE,
	MOD_GRENADE_ALT,
	MOD_GRENADE_SPLASH,
	MOD_GRENADE_ALT_SPLASH,
	MOD_TETRION,
	MOD_TETRION_ALT,
	MOD_DREADNOUGHT,
	MOD_DREADNOUGHT_ALT,
	MOD_QUANTUM,
	MOD_QUANTUM_SPLASH,
	MOD_QUANTUM_ALT,
	MOD_QUANTUM_ALT_SPLASH,

	MOD_DETPACK,
	MOD_SEEKER,

//expansion pack
	MOD_KNOCKOUT,
	MOD_ASSIMILATE,
	MOD_BORG,
	MOD_BORG_ALT,

	MOD_RESPAWN,
	MOD_EXPLOSION,

	MOD_MAX
} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON,				// EFX: rotate + upscale + minlight
	IT_AMMO,				// EFX: rotate
	IT_ARMOR,				// EFX: rotate + minlight
	IT_HEALTH,				// EFX: static external sphere + rotating internal
	IT_POWERUP,				// instant on, timer based
							// EFX: rotate + external ring that rotates
	IT_HOLDABLE,			// single use, holdable item
							// EFX: rotate + bob
	IT_TEAM
} itemType_t;

typedef struct gitem_s {
	char		*classname;	// spawning name
	char		*pickup_sound;
	char		*world_model;
	char		*view_model;

	char		*icon;
	char		*pickup_name;	// for printing on pickup

	int			quantity;		// for ammo how much, or duration of powerup
	itemType_t  giType;			// IT_* flags

	int			giTag;

	char		*precaches;		// string of all models and images this item will use
	char		*sounds;		// string of all sounds this item will use
} gitem_t;

// included in both the game dll and the client
extern	gitem_t	bg_itemlist[];
extern	int		bg_numItems;

gitem_t *BG_FindItemWithClassname(const char *name);
char *BG_FindClassnameForHoldable(holdable_t pw);
gitem_t	*BG_FindItem( const char *pickupName );
gitem_t	*BG_FindItemForWeapon( weapon_t weapon );
gitem_t	*BG_FindItemForAmmo( weapon_t weapon );
gitem_t	*BG_FindItemForPowerup( powerup_t pw );
gitem_t	*BG_FindItemForHoldable( holdable_t pw );
#define	ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean	BG_CanItemBeGrabbed( const entityState_t *ent, const playerState_t *ps );


// g_dmflags->integer flags
#define	DF_NO_FALLING			8
#define DF_FIXED_FOV			16
#define	DF_NO_FOOTSTEPS			32

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE|CONTENTS_SHOTCLIP)


//
// entityState_t->eType
//

//	VERY IMPORTANT, if you change this enum, you MUST also change the enum in be_aas_entity.c!  THANKS ID!!!!!  WTG! GG!

typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_ALT_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_USEABLE,

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;



void	BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void	BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void	BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );

void	BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap );

qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime );


#define ARENAS_PER_TIER		4
#define MAX_ARENAS			1024
#define	MAX_ARENAS_TEXT		8192

#define MAX_BOTS			1024
#define MAX_BOTS_TEXT		8192

//make this match Max_Ammo in g_items please;
#define PHASER_AMMO_MAX		50

extern int Max_Ammo[];
