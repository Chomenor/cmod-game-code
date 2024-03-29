#ifndef	__CG_TEXT_H__
#define	__CG_TEXT_H__

// Ingame Text enum
typedef enum
{
	IGT_NONE,
	IGT_OUTOFAMMO,
	IGT_LOWAMMO,
	// Scoreboard
	IGT_FRAGGEDBY,
	IGT_ASSIMILATEDBY,
	IGT_PLACEDWITH,

	IGT_SPECTATOR,
	IGT_WAITINGTOPLAY,
	IGT_USETEAMMENU,
	IGT_FOLLOWING,
	IGT_LOADING,
	IGT_AWAITINGSNAPSHOT,
	IGT_PURESERVER,
	IGT_CHEATSAREENABLED,
	IGT_TIMELIMIT,
	IGT_FRAGLIMIT,
	IGT_CAPTURELIMIT,
	IGT_READY,
	IGT_SB_SCORE,
	IGT_SB_PING,
	IGT_SB_TIME,
	IGT_SB_NAME,
	IGT_REDTEAM,
	IGT_BLUETEAM,
	IGT_SAY,
	IGT_SAY_TEAM,

	IGT_POINT_LIMIT,
	IGT_TIME_LIMIT,
	IGT_CAPTURE_LIMIT,

	IGT_GAME_FREEFORALL,
	IGT_GAME_SINGLEPLAYER,
	IGT_GAME_TOURNAMENT,
	IGT_GAME_TEAMHOLOMATCH,
	IGT_GAME_CAPTUREFLAG,
	IGT_GAME_UNKNOWN,

	IGT_YOUELIMINATED,
	IGT_YOUASSIMILATED,
	IGT_PLACEWITH,

	IGT_SUICIDES,
	IGT_CRATERED,
	IGT_WASSQUISHED,
	IGT_SANK,
	IGT_MELTED,
	IGT_INTOLAVA,
	IGT_SAWLIGHT,
	IGT_WRONGPLACE,

	IGT_TRIPPEDHERGRENADE,
	IGT_TRIPPEDITSGRENADE,
	IGT_TRIPPEDHISGRENADE,
	IGT_BLEWHERSELFUP,
	IGT_BLEWITSELFUP,
	IGT_BLEWHIMSELFUP,
	IGT_MELTEDHERSELF,
	IGT_MELTEDITSELF,
	IGT_MELTEDHIMSELF,
	IGT_SMALLERGUN,
	IGT_DESTROYEDHERSELF,
	IGT_DESTROYEDITSELF,
	IGT_DESTROYEDHIMSELF,

	IGT_TIEDFOR,

	IGT_REDTEAMLEADS,
	IGT_BLUETEAMLEADS,
	IGT_TO ,
	IGT_TEAMSARETIED,

	IGT_ATE,
	IGT_GRENADE,

	IGT_WITHINBLASTRADIUS,

	IGT_ALMOSTEVADED,
	IGT_PHOTONBURST,
	IGT_DISPATCHEDBY,
	IGT_SCAVENGERWEAPON,

	IGT_NEARLYAVOIDED,
	IGT_RAILEDBY,
	IGT_ELECTROCUTEDBY,
	IGT_DESTROYEDBY,
	IGT_TETRYONPULSE,

	IGT_BLASTEDBY,
	IGT_STASISWEAPON,
	IGT_WASWITHIN,
	IGT_TRANSPORTERBEAM,
	IGT_WASELIMINATEDBY,
	IGT_WASELIMINATED,

	IGT_SNAPSHOT,

	IGT_REPLICATION_MATRIX,
	IGT_HOLOGRAPHIC_PROJECTORS,
	IGT_SIMULATION_DATA_BASE,
	IGT_SAFETY_LOCKS,
	IGT_HOLODECKSIMULATION,
	IGT_USEDTEAMMENU,
	IGT_CONNECTIONINTERRUPTED,
	IGT_VOTE,
	IGT_YES,
	IGT_NO,
	IGT_WAITINGFORPLAYERS,
	IGT_STARTSIN,
	IGT_NONETEXT,

	IGT_EFFICIENCY,
	IGT_SHARPSHOOTER,
	IGT_UNTOUCHABLE,
	IGT_LOGISTICS,
	IGT_TACTICIAN,
	IGT_DEMOLITIONIST,
	IGT_STREAK,
	IGT_ROLE,
	IGT_SECTION31,

	IGT_ACE,
	IGT_EXPERT,
	IGT_MASTER,
	IGT_CHAMPION,

	IGT_MVP,
	IGT_DEFENDER,
	IGT_WARRIOR,
	IGT_CARRIER,
	IGT_INTERCEPTOR,
	IGT_BRAVERY,

	IGT_TIED_FOR,
	IGT_YOUR_RANK,

	IGT_YOUR_TEAM,
	IGT_WON,
	IGT_LOST,
	IGT_TEAMS_TIED,

	IGT_1ST,
	IGT_2ND,
	IGT_3RD,

	IGT_WINNER,
	IGT_CAPTURES,
	IGT_POINTS,
	IGT_OVERALL,

	IGT_CLICK_PLAY_AGAIN,
	IGT_TITLEELIMINATED,
	IGT_WORSTENEMY,
	IGT_FAVORITEWEAPON,
	IGT_CONNECTING,
	IGT_SPECTABBREV,

	IGT_VICTOR,
	IGT_DEFEATED,

	IGT_DROWNING,
	IGT_CORROSION,
	IGT_BOILING,
	IGT_COMPRESSION,
	IGT_TRANSPORTERACCIDENT,
	IGT_IMPACT,
	IGT_SUICIDE,
	IGT_LASERBURNS,
	IGT_MISADVENTURE,
	IGT_PHASERBURNS,
	IGT_ENERGYSCARS,
	IGT_SNIPED,
	IGT_INFINITEMODULATION,
	IGT_GUNNEDDOWN,
	IGT_SCAVENGED,
	IGT_PERMANENTSTASIS,
	IGT_BLASTED,
	IGT_MINED,
	IGT_PERFORATED,
	IGT_DISRUPTED,
	IGT_WELDED,
	IGT_DEGAUSSED,
	IGT_DESTROYED,
	IGT_ANNIHILATED,
	IGT_VAPORIZED,
	IGT_AUTOGUNNED,
	IGT_KNOCKOUT,
	IGT_ASSIMILATED,
	IGT_ZAPPED,
	IGT_UNKNOWN,
	IGT_CASUALTY,
	IGT_METHOD,
	IGT_OBITELIMINATED,
	IGT_CREDIT,
	IGT_PLEASE,

	IGT_11TH,
	IGT_12TH,
	IGT_13TH,
	IGT_NUM_ST,
	IGT_NUM_ND,
	IGT_NUM_RD,
	IGT_NUM_TH,

	IGT_PLAYERS,
	IGT_OBJECTIVES,

	IGT_GAME_ELIMINATION,
	IGT_GAME_ASSIMILATION,
	IGT_GAME_ACTIONHERO,
	IGT_GAME_SPECIALTIES,
	IGT_GAME_DISINTEGRATION,

	IGT_MAX
} ingameTextType_t;


extern char *ingame_text[IGT_MAX];

#endif	//__CG_TEXT_H__
