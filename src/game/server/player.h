/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed
#include "entities/character.h"
#include "gamecontext.h"

// player object
class CPlayer
{
	MACRO_ALLOC_POOL_ID()

public:
	CPlayer(CGameContext *pGameServer, int ClientID, int Team);
	~CPlayer();

	void Init(int CID);

	void TryRespawn();
	void Respawn();
	void SetTeam(int Team, bool DoChatMsg=true);
	int GetTeam() const { return m_Team; };
	int GetCID() const { return m_ClientID; };

	void Tick();
	void PostTick();
	void Snap(int SnappingClient);

	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);
	void OnDisconnect(const char *pReason);

	void KillCharacter(int Weapon = WEAPON_GAME);
	CCharacter *GetCharacter();

	//---------------------------------------------------------
	// this is used for snapping so we know how we can clip the view for the player
	vec2 m_ViewPos;

	// states if the client is chatting, accessing a menu etc.
	int m_PlayerFlags;

	// used for snapping to just update latency if the scoreboard is active
	int m_aActLatency[MAX_CLIENTS];

	// used for spectator mode
	int m_SpectatorID;

	bool m_IsReady;


	//
	int m_Vote;
	int m_VotePos;
	//
	int m_LastVoteCall;
	int m_LastVoteTry;
	int m_LastChat;
	int m_LastSetTeam;
	int m_LastSetSpectatorMode;
	int m_LastChangeInfo;
	int m_LastEmote;
	int m_LastKill;

	// TODO: clean this up
	struct
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	int m_RespawnTick;
	int m_DieTick;
	int m_Score;
	int m_ScoreTick; // can be used to have partial scores
	int m_ScoreStartTick;
	bool m_ForceBalanced;
	int m_LastActionTick;
	int m_TeamChangeTick;
	struct
	{
		int m_TargetX;
		int m_TargetY;
	} m_LatestActivity;

	// network latency calculations
	struct
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	} m_Latency;

	int m_ChatTicks;
	char m_aOldChatMsg[256];
	int m_OldChatMsgCount;
	int m_LastPMReceivedFrom;

	int m_SetEmoteType;
	int m_SetEmoteStop;

	struct Stats
	{
		int m_Shots[NUM_WEAPONS];
		int m_TotalShots;
		int m_Kills;
		int m_Deaths;
		int m_Hits;
		int m_Captures;
		int m_LostFlags;
		double m_FastestCapture;
	} m_Stats;
	bool m_KeepWeapon[NUM_WEAPONS-1];
	bool m_KeepAward;
	int m_Spree;
	bool m_GotAward;

	bool m_FreezeOnSpawn;

	int m_Lives; // for LMS
	int m_isBot; // for detecting if this is a bot and what kind
	int m_botDirection; // last moved direction
	int m_ticksSinceFire; // ticks since last fire
	int m_botAggro; // whether it is aggroed and on whom
	bool m_WantsPause;
private:
	CCharacter *m_pCharacter;
	CGameContext *m_pGameServer;

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const;

	//
	bool m_Spawning;
	int m_ClientID;
	int m_Team;

	void HandleInputKZ(CNetObj_PlayerInput &Input);
	int m_TryingDirectionSmart = 0;
	bool m_TryingOppositeSmart = false;
	bool m_StopUntilTouchGround = false;
	int m_DontDoSmartTargetChase = 0;
	bool m_DoGrenadeJump = false;

	CGameWorld *GameWorld() { return m_pCharacter->GameWorld(); }
	//CTuningParams *Tuning() { return GameWorld()->Tuning(); }
	//CTuningParams *TuningList() { return GameWorld()->TuningList(); }
	//CTuningParams *GetTuning(int i) { return GameWorld()->GetTuning(i); }
	//class CConfig *Config() { return m_pCharacter->GameWorld()->Config(); }
	//class CGameContext *GameServer() { return m_pCharacter->GameServer(); }
	//class IServer *Server() { return m_pCharacter->Server(); }
	CCollision *Collision();
};

#endif
