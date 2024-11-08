/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include "player.h"


MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }

CPlayer::CPlayer(CGameContext *pGameServer, int ClientID, int Team)
{
	m_pGameServer = pGameServer;
	m_RespawnTick = Server()->Tick();
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	m_pCharacter = 0;
	m_ClientID = ClientID;
	m_Team = GameServer()->m_pController->ClampTeam(Team);
	m_SpectatorID = SPEC_FREEVIEW;
	m_LastActionTick = Server()->Tick();
	m_TeamChangeTick = Server()->Tick();

	m_ChatTicks = 0;
	m_OldChatMsgCount = 0;
	mem_zero(&m_Stats, sizeof(m_Stats));
	for(int i = 0; i < NUM_WEAPONS-1; i++)
		m_KeepWeapon[i] = false;
	m_KeepAward = false;
	m_Spree = 0;
	m_GotAward = false;
	//
	m_SetEmoteStop = Server()->Tick();
	m_SetEmoteType = EMOTE_NORMAL;
	m_LastPMReceivedFrom = -1;
	m_FreezeOnSpawn = false;

	m_Lives = g_Config.m_SvLMSLives;
	m_isBot = 0;
	m_botDirection = 1;
	m_ticksSinceX = 0;
	m_botAggro = -1;

	m_WantsPause = false;
}

CPlayer::~CPlayer()
{
	delete m_pCharacter;
	m_pCharacter = 0;
}

void CPlayer::Tick()
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	Server()->SetClientScore(m_ClientID, m_Score);

	// do latency stuff
	{
		IServer::CClientInfo Info;
		if(Server()->GetClientInfo(m_ClientID, &Info))
		{
			m_Latency.m_Accum += Info.m_Latency;
			m_Latency.m_AccumMax = max(m_Latency.m_AccumMax, Info.m_Latency);
			m_Latency.m_AccumMin = min(m_Latency.m_AccumMin, Info.m_Latency);
		}
		// each second
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum/Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}
	}

	// Make player receive message if they are not using the ddnet client while the server supports > 16 players
	// each 5 seconds
	if(Server()->Tick()%(Server()->TickSpeed()*5) == 0)
	{
		IServer::CClientInfo Info;
		int infoExists = Server()->GetClientInfo(m_ClientID, &Info);
		int ddnetversion = Info.m_DDNetVersion;
		if (!((infoExists && ddnetversion > VERSION_DDNET_WHISPER) || g_Config.m_SvMaxClients <= MAX_CLIENTS_VANILLA))
			m_pGameServer->SendChatTarget(m_ClientID, "If you are not using the ddnet client please download it from https://ddnet.org/ This server only properly supports ddnet clients.");
	}

	if(m_ChatTicks)
		m_ChatTicks--;

	if(!GameServer()->m_World.m_Paused)
	{
		if((!m_pCharacter && m_Team == TEAM_SPECTATORS && m_SpectatorID == SPEC_FREEVIEW) || m_Lives <= 0)
			m_ViewPos -= vec2(clamp(m_ViewPos.x-m_LatestActivity.m_TargetX, -500.0f, 500.0f), clamp(m_ViewPos.y-m_LatestActivity.m_TargetY, -400.0f, 400.0f));

		if(!m_pCharacter && m_DieTick+Server()->TickSpeed()*3 <= Server()->Tick())
			m_Spawning = true;

		if(m_pCharacter && m_Lives > 0)
		{
			if(m_pCharacter->IsAlive())
			{
				m_ViewPos = m_pCharacter->m_Pos;
			}
			else
			{
				delete m_pCharacter;
				m_pCharacter = 0;
			}
		}
		else if(m_Spawning && m_RespawnTick <= Server()->Tick())
			TryRespawn();

		if (m_isBot >= 2) { 
			// >=2 means the AI will shoot and maybe move, instead of being a dummy
			// if there are no clients connected, the bots will idle
			if (GameServer()->m_ClientCount <= 0) {
				CNetObj_PlayerInput input;
				input.m_Direction = 0;
				input.m_TargetX = 0; // look randomly
				input.m_TargetY = 0;
				input.m_Jump = false;
				input.m_Fire = false;
				input.m_Hook = false;
				input.m_PlayerFlags = PLAYERFLAG_PLAYING;
				input.m_WantedWeapon = WEAPON_GUN+1;
				input.m_NextWeapon = WEAPON_GUN+1;
				input.m_PrevWeapon = WEAPON_GUN+1;
				OnPredictedInput(&input);
				OnDirectInput(&input);
			} else {
				CNetObj_PlayerInput input;
				input.m_Direction = 0;
				input.m_TargetX = (rand() % 128) - 64; // look randomly
				input.m_TargetY = (rand() % 128) - 64;
				input.m_Jump = false;
				input.m_Fire = true;	
				if (!GameServer()->m_pController->IsInstagib()) // make non-automatic weapons work, but still have ammo reload work in instagib
					input.m_Fire = GameServer()->Server()->Tick() % (2) == 1;
				input.m_Hook = false;
				input.m_PlayerFlags = PLAYERFLAG_PLAYING;
				input.m_WantedWeapon = WEAPON_GUN+1;
				if (GameServer()->m_pController->IsInstagib())
					input.m_WantedWeapon = WEAPON_RIFLE+1;
				if (GameServer()->m_pController->IsGrenade())
					input.m_WantedWeapon = WEAPON_GRENADE+1;
				input.m_NextWeapon = WEAPON_GUN+1;
				input.m_PrevWeapon = WEAPON_GUN+1;
				if (m_isBot >= 3) { // move, and occasionally jump
					if (rand() % (SERVER_TICK_SPEED*2) == 1)
						m_botDirection = -m_botDirection;
					input.m_Direction = m_botDirection;
					if (rand() % (SERVER_TICK_SPEED*2) == 1)
						input.m_Jump = true;
				}
				if (m_isBot >= 4 && m_pCharacter) {
					if (GameServer()->Server()->Tick() % (SERVER_TICK_SPEED) == 1) {
						// get a new aggro
						float smallestDistance = 900.0;
						m_botAggro = -1;
						
						for (int i = 0; i < MAX_CLIENTS; i++) {
							if (i != m_ClientID && GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter()) {
								vec2 pos = GameServer()->m_apPlayers[i]->GetCharacter()->m_Pos;
								float d = sqrt((pos.x - m_pCharacter->m_Pos.x)*(pos.x - m_pCharacter->m_Pos.x) + (pos.y - m_pCharacter->m_Pos.y)*(pos.y - m_pCharacter->m_Pos.y));
								if (d < smallestDistance && !(GameServer()->m_pController->IsTeamplay() && GameServer()->m_apPlayers[i]->GetTeam() == GetTeam())) {
									smallestDistance = d;
									m_botAggro = i;
								}
							}
						}
					}
					if (m_botAggro == -1)
						input.m_Fire = false; // do not shoot when not aggroed
					else {
						// aim
						if (GameServer()->m_apPlayers[m_botAggro] && GameServer()->m_apPlayers[m_botAggro]->GetCharacter()) {
							vec2 pos = GameServer()->m_apPlayers[m_botAggro]->GetCharacter()->m_Pos;
							float d = sqrt((pos.x - m_pCharacter->m_Pos.x)*(pos.x - m_pCharacter->m_Pos.x) + (pos.y - m_pCharacter->m_Pos.y)*(pos.y - m_pCharacter->m_Pos.y));
							input.m_TargetX = pos.x - m_pCharacter->m_Pos.x; // aim
							input.m_TargetY = pos.y - m_pCharacter->m_Pos.y;
							if (GameServer()->m_pController->IsGrenade()) // grenade curve correction, somewhat
								input.m_TargetY = input.m_TargetY + (-abs(input.m_TargetX)*0.3);
							if (m_isBot == 4)// aim worse
							{
								input.m_TargetX = (float)input.m_TargetX + (d * 0.3 * ((float)(rand() % 64) / 64.0 - 0.5));
								input.m_TargetY = (float)input.m_TargetY + (d * 0.3 *  ((float)(rand() % 64) / 64.0 - 0.5));
							}
						}
					}
				}
				OnPredictedInput(&input);
				OnDirectInput(&input);
				if (m_pCharacter) {
					m_pCharacter->SetAmmo(WEAPON_GUN, 10);
				}
			}
		}
	}
	else
	{
		++m_RespawnTick;
		++m_DieTick;
		++m_ScoreStartTick;
		++m_LastActionTick;
		++m_TeamChangeTick;
 	}
}

void CPlayer::PostTick()
{
	// update latency value
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				m_aActLatency[i] = GameServer()->m_apPlayers[i]->m_Latency.m_Min;
		}
	}

	// update view pos for spectators
	if(m_Team == TEAM_SPECTATORS && m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[m_SpectatorID])
		m_ViewPos = GameServer()->m_apPlayers[m_SpectatorID]->m_ViewPos;
}

void CPlayer::Snap(int SnappingClient)
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, m_ClientID, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;

	if(m_pCharacter && m_pCharacter->Frozen() && GameServer()->m_pController->IsIFreeze() && g_Config.m_SvIFreezeFrozenTag)
	{
		char aBuf[MAX_NAME_LENGTH];
		str_format(aBuf, sizeof(aBuf), "[F] %s", Server()->ClientName(m_ClientID));
		StrToInts(&pClientInfo->m_Name0, 4, aBuf);
	}
	else
		StrToInts(&pClientInfo->m_Name0, 4, Server()->ClientName(m_ClientID));

	StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(m_ClientID));
	pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);
	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
	pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
	pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, m_ClientID, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = SnappingClient == -1 ? m_Latency.m_Min : GameServer()->m_apPlayers[SnappingClient]->m_aActLatency[m_ClientID];
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = m_ClientID;
	if (GameServer()->m_pController->IsLMS())
		pPlayerInfo->m_Score = m_Lives;
	else
		pPlayerInfo->m_Score = m_Score;
	pPlayerInfo->m_Team = m_Team;

	if(m_ClientID == SnappingClient)
		pPlayerInfo->m_Local = 1;

	if((m_ClientID == SnappingClient && m_Team == TEAM_SPECTATORS) || (GameServer()->m_pController->IsLMS() && m_Lives <= 0))
	{
		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, m_ClientID, sizeof(CNetObj_SpectatorInfo)));
		if(!pSpectatorInfo)
			return;

		pSpectatorInfo->m_SpectatorID = m_SpectatorID;
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}

	if (m_isBot) {
		StrToInts(&pClientInfo->m_Name0, 4, "bot");
		StrToInts(&pClientInfo->m_Clan0, 3, "bot");
		if (m_isBot == 4)
			StrToInts(&pClientInfo->m_Clan0, 3, "bot4");
		else if (m_isBot == 5)
			StrToInts(&pClientInfo->m_Clan0, 3, "bot5");
		StrToInts(&pClientInfo->m_Skin0, 6, g_Config.m_SvBotSkin);
		pPlayerInfo->m_Latency = 0;
	}

	// WARNING, this is very hardcoded; for ddnet client support
	CNetObj_DDNetPlayer *pDDNetPlayer = (CNetObj_DDNetPlayer *)Server()->SnapNewItem(32765, GetCID(), 8);
	if(!pDDNetPlayer)
		return;

	pDDNetPlayer->m_Flags = 0;
	if (Server()->Tick() > m_LastActionTick + Server()->TickSpeed()*(max(g_Config.m_SvInactiveKickTime, 60)))
		pDDNetPlayer->m_Flags |= EXPLAYERFLAG_AFK;
	if (m_Lives <= 0)
		pDDNetPlayer->m_Flags |= EXPLAYERFLAG_SPEC;

	if (g_Config.m_SvDDExposeAuthed && Server()->IsAuthed(GetCID()))
		pDDNetPlayer->m_AuthLevel = AUTHED_MOD;
	else
		pDDNetPlayer->m_AuthLevel = AUTHED_NO;
}

void CPlayer::OnDisconnect(const char *pReason)
{
	KillCharacter();

	if(Server()->ClientIngame(m_ClientID))
	{
		char aBuf[512];
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(m_ClientID), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	if(m_pCharacter)
		m_pCharacter->OnPredictedInput(NewInput);
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	if(NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		// skip the input if chat is active
		if(m_PlayerFlags&PLAYERFLAG_CHATTING)
			return;

		// reset input
		if(m_pCharacter)
			m_pCharacter->ResetInput();

		m_PlayerFlags = NewInput->m_PlayerFlags;
 		return;
	}

	m_PlayerFlags = NewInput->m_PlayerFlags;

	if(m_pCharacter && m_pCharacter->Frozen())
		return;

	if(m_pCharacter)
		m_pCharacter->OnDirectInput(NewInput);

	if(!m_pCharacter && m_Team != TEAM_SPECTATORS && (NewInput->m_Fire&1))
		m_Spawning = true;

	// check for activity
	if(NewInput->m_Direction || m_LatestActivity.m_TargetX != NewInput->m_TargetX ||
		m_LatestActivity.m_TargetY != NewInput->m_TargetY || NewInput->m_Jump ||
		NewInput->m_Fire&1 || NewInput->m_Hook)
	{
		m_LatestActivity.m_TargetX = NewInput->m_TargetX;
		m_LatestActivity.m_TargetY = NewInput->m_TargetY;
		m_LastActionTick = Server()->Tick();
	}
}

CCharacter *CPlayer::GetCharacter()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return 0;
}

void CPlayer::KillCharacter(int Weapon)
{
	if(m_pCharacter)
	{
		m_pCharacter->Die(m_ClientID, Weapon);
		delete m_pCharacter;
		m_pCharacter = 0;
	}
}

void CPlayer::Respawn()
{
	if(m_Team != TEAM_SPECTATORS)
		m_Spawning = true;
}

void CPlayer::SetTeam(int Team, bool DoChatMsg)
{
	// clamp the team
	Team = GameServer()->m_pController->ClampTeam(Team);
	if(m_Team == Team)
		return;

	char aBuf[512];
	if(DoChatMsg)
	{
		str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(m_ClientID), GameServer()->m_pController->GetTeamName(Team));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}

	KillCharacter();

	m_Team = Team;
	m_LastActionTick = Server()->Tick();
	m_SpectatorID = SPEC_FREEVIEW;
	// we got to wait 0.5 secs before respawning
	m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_SpectatorID == m_ClientID)
				GameServer()->m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
		}
	}
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos;

	if(!GameServer()->m_pController->CanSpawn(m_Team, &SpawnPos) || m_Lives <= 0)
		return;

	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(&GameServer()->m_World);
	m_pCharacter->Spawn(this, SpawnPos);
	GameServer()->CreatePlayerSpawn(SpawnPos);
}
