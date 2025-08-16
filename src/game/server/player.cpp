/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include "player.h"
#include <game/server/entities/flag.h>
#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/gamemodes/ctf.h>


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
	m_ticksSinceFire = 0;
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

		if (m_isBot >= 2 && !(m_isBot == 7)) { 
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
				if (m_isBot >= 3 && m_pCharacter) { // move, and occasionally jump
					if (rand() % (SERVER_TICK_SPEED*2) == 1)
						m_botDirection = -m_botDirection;
					input.m_Direction = m_botDirection;
					if (rand() % (SERVER_TICK_SPEED*2) == 1)
						input.m_Jump = true;
					if (str_comp_nocase(GameServer()->m_pController->m_pGameType, "CTF+") == 0
					|| str_comp_nocase(GameServer()->m_pController->m_pGameType, "iCTF+") == 0
					|| str_comp_nocase(GameServer()->m_pController->m_pGameType, "gCTF+") == 0) {
						int team = m_Team;
						int teamEnemy = 1-m_Team;
						if (GameServer()->m_pController->m_apFlags[team]
						&& GameServer()->m_pController->m_apFlags[teamEnemy]) {
							if (GameServer()->m_pController->m_apFlags[teamEnemy]->m_pCarryingCharacter == m_pCharacter) {
								if (GameServer()->m_pController->m_apFlags[team]->m_Pos.x > m_pCharacter->m_Pos.x)
									m_botDirection = 1;
								else
									m_botDirection = -1;
							} else {
								if (GameServer()->m_pController->m_apFlags[teamEnemy]->m_Pos.x > m_pCharacter->m_Pos.x)
									m_botDirection = 1;
								else
									m_botDirection = -1;
							}
						}
					}
				}
				if (m_isBot >= 4 && m_pCharacter) {
					if (GameServer()->Server()->Tick() % (SERVER_TICK_SPEED) == 1) {
						// get a new aggro
						float smallestDistance = 850.0;
						if (m_isBot == 4)
							smallestDistance = 750.0;
						m_botAggro = -1;
						
						for (int i = 0; i < MAX_CLIENTS; i++) {
							if (i != m_ClientID && GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter()) {
								vec2 pos = GameServer()->m_apPlayers[i]->GetCharacter()->m_Pos;
								float d = sqrt((pos.x - m_pCharacter->m_Pos.x)*(pos.x - m_pCharacter->m_Pos.x) + (1.35)*(pos.y - m_pCharacter->m_Pos.y)*(pos.y - m_pCharacter->m_Pos.y));
								// vertical distance is multiplied by a factor, since screens are larger horizontally
								if (d < smallestDistance && !(GameServer()->m_pController->IsTeamplay() && GameServer()->m_apPlayers[i]->GetTeam() == GetTeam())) {
									smallestDistance = d;
									m_botAggro = i;
								}
							}
						}
					}
					if (m_botAggro == -1) {
						m_ticksSinceFire = 0; // reset
						input.m_Fire = false; // do not shoot by default
					} else {
						m_ticksSinceFire++;
						if (GameServer()->m_pController->IsGrenade()) {
							if (m_isBot >= 5 || m_ticksSinceFire > 50) {
								m_ticksSinceFire = 0; // reset
								input.m_Fire = true; // fire
							} else 
								input.m_Fire = false; // do not shoot by default
						}
						else if (GameServer()->m_pController->IsInstagib()) {
							if (m_isBot >= 6 || (m_isBot == 5 && m_ticksSinceFire > 5 + g_Config.m_SvLaserReloadTime / Server()->TickSpeed()) || m_ticksSinceFire > 20+g_Config.m_SvLaserReloadTime / Server()->TickSpeed()) {
								m_ticksSinceFire = 0; // reset
								input.m_Fire = true; // fire
							} else 
								input.m_Fire = false; // do not shoot by default
						}
						else {
							if (m_ticksSinceFire > 10) { // 10 = 0.2s
								m_ticksSinceFire = 0; // reset
								input.m_Fire = true; // fire
							} else 
								input.m_Fire = false; // do not shoot by default
							if (m_isBot >= 5)
								input.m_Fire = GameServer()->Server()->Tick() % (2) == 1;
						}

						// aim
						if (GameServer()->m_apPlayers[m_botAggro] && GameServer()->m_apPlayers[m_botAggro]->GetCharacter()) {
							vec2 pos = GameServer()->m_apPlayers[m_botAggro]->GetCharacter()->m_Pos;
							float d = sqrt((pos.x - m_pCharacter->m_Pos.x)*(pos.x - m_pCharacter->m_Pos.x) + (pos.y - m_pCharacter->m_Pos.y)*(pos.y - m_pCharacter->m_Pos.y));
							input.m_TargetX = pos.x - m_pCharacter->m_Pos.x; // aim
							input.m_TargetY = pos.y - m_pCharacter->m_Pos.y;
							if (GameServer()->m_pController->IsGrenade()) // grenade curve correction, somewhat
								input.m_TargetY = input.m_TargetY + (-abs(input.m_TargetX)*0.3);
							if (m_isBot <= 5) // aim worse
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
		else if (m_isBot == 7) //+KZ Aimbot
		{
			CNetObj_PlayerInput input;
			input = m_pCharacter->GetLatestInput();
			input.m_Jump = false;
			
			
			HandleInputKZ(input);
			OnPredictedInput(&input);
			OnDirectInput(&input);
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
		switch (m_isBot)
		{
		case 4: StrToInts(&pClientInfo->m_Clan0, 3, "bot4"); break;
		case 5: StrToInts(&pClientInfo->m_Clan0, 3, "bot5"); break;
		case 6: StrToInts(&pClientInfo->m_Clan0, 3, "bot6"); break;
		default: break;
		}
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

void CPlayer::HandleInputKZ(CNetObj_PlayerInput &Input)
{
	//Spaghetti yummy
	
	CCharacter *pClosestChar = nullptr;
	CPickup *pClosestPickup = nullptr;
	CFlag *pEnemyFlag = nullptr;
	CFlag *pTeamFlag = nullptr;
	bool targetisup = false;
	bool dontjump = false;
	bool butjumpifwall = false;
	bool jumpifgoingtofall = false;
	
	vec2 TargetPos = vec2(0,0);
	bool TargetPosSet = false;
	bool DoSmartTargetChase = false;
	
	Input.m_Fire = false;
	
	//if(str_find_nocase(GameServer()->m_pController->m_pGameType, "CTF"))
	{
		CFlag *p = (CFlag *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_FLAG);
		for(; p; p = (CFlag *)p->TypeNext())
		{
			
			if(p->m_Team == GetTeam())
			{
				pTeamFlag = p;
				continue;
			}
			
			pEnemyFlag = p;
		}
	}
	
	{
		float ClosestRange = 100000.0f;
		CPickup *pClosest = 0;
		
		CPickup *p = (CPickup *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PICKUP);
		for(; p; p = (CPickup *)p->TypeNext())
		{
			if(Collision()->FastIntersectLine(m_pCharacter->m_Pos,p->m_Pos,nullptr,nullptr))
				continue;
			
			if((p->Type() == POWERUP_HEALTH && GetCharacter()->GetHealth() >= 10) || (p->Type() == POWERUP_ARMOR && GetCharacter()->GetArmor() >= 10) || (p->SubType() == WEAPON_SHOTGUN && m_pCharacter->m_aWeapons[WEAPON_SHOTGUN].m_Ammo) || (p->SubType() == WEAPON_RIFLE && m_pCharacter->m_aWeapons[WEAPON_RIFLE].m_Ammo) || (p->SubType() == WEAPON_GRENADE && m_pCharacter->m_aWeapons[WEAPON_GRENADE].m_Ammo)  || (p->Type() == POWERUP_NINJA && m_pCharacter->m_aWeapons[WEAPON_NINJA].m_Got))
				continue;
			
			if(p->GetSpawnTick() > 0)
			{
				//printf("%d",((CPickup*)p)->GetSpawnTick());
				continue;
			}
			
			
			float Len = distance(m_pCharacter->m_Pos, p->m_Pos);
			if(Len < p->m_ProximityRadius + 100000.0f)
			{
				if(Len < ClosestRange)
				{
					ClosestRange = Len;
					pClosest = p;
				}
			}
		}
		
		pClosestPickup = pClosest;
	}
	
	{
		float ClosestRange = 100000.0f;
		CCharacter *pClosest = 0;
		
		CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER);
		for(; p; p = (CCharacter *)p->TypeNext())
		{
			if(p == GetCharacter())
				continue;
			
			if(GameServer()->m_pController->IsTeamplay() && p->GetPlayer()->GetTeam() == GetTeam())
				continue;
			
			if(str_find_nocase(GameServer()->m_pController->m_pGameType, "freeze") && p->IsDeepFrozen())
				continue;

			//if(str_find_nocase(GameServer()->m_pController->m_pGameType, "catch64") && p->GetPlayer()->m_CatchColor == m_CatchColor)
			//	continue;
			
			
			float Len = distance(m_pCharacter->m_Pos, p->m_Pos);
			if(Len < p->m_ProximityRadius + 100000.0f)
			{
				if(Len < ClosestRange)
				{
					ClosestRange = Len;
					pClosest = p;
				}
			}
		}
		
		pClosestChar = pClosest;
	}
	
	if(pEnemyFlag)
	{
		if(pTeamFlag && pEnemyFlag->m_pCarryingCharacter == GetCharacter())
		{
			//Input.m_Direction = pTeamFlag->m_Pos->x > m_pCharacter->m_Pos.x ? 1 : -1;
			TargetPos = pTeamFlag->m_Pos;
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
		else if(!pEnemyFlag->m_pCarryingCharacter)
		{
			//Input.m_Direction = pEnemyFlag->m_Pos->x > m_pCharacter->m_Pos.x ? 1 : -1;
			TargetPos = pEnemyFlag->m_Pos;
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
	}
	
	if(pClosestPickup && !(pTeamFlag && pEnemyFlag && (pEnemyFlag->m_pCarryingCharacter == GetCharacter() || !pEnemyFlag->m_pCarryingCharacter)))
	{
		//Input.m_Direction = pClosestPickup->m_Pos->x > m_pCharacter->m_Pos.x ? 1 : -1;
		TargetPos = pClosestPickup->m_Pos;
		TargetPosSet = true;
		
		/*if((pClosestPickup->m_Pos->y + pClosestPickup->m_ProximityRadius * 2.f) < m_pCharacter->m_Pos.y && !(Collision()->GetCollisionAt(m_pCharacter->m_Pos.x , m_pCharacter->m_Pos.y - m_ProximityRadius / 3.f) == TILE_DEATH))
		{
			targetisup = true;
		}*/
	}
	
	if(pClosestChar)
	{
		Input.m_TargetX = pClosestChar->m_Pos.x - m_pCharacter->m_Pos.x; // aim
		Input.m_TargetY = pClosestChar->m_Pos.y - m_pCharacter->m_Pos.y;
		
		if(!pClosestPickup && !(pTeamFlag && pEnemyFlag && (pEnemyFlag->m_pCarryingCharacter == GetCharacter() || !pEnemyFlag->m_pCarryingCharacter)))
		{
			TargetPos = pClosestChar->m_Pos;
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
		
		if(m_pCharacter->m_aWeapons[WEAPON_NINJA].m_Got)
		{
		}
		else if(m_pCharacter->m_aWeapons[WEAPON_RIFLE].m_Got && m_pCharacter->m_aWeapons[WEAPON_RIFLE].m_Ammo && distance(m_pCharacter->m_Pos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_LaserReach)
		{
			GetCharacter()->SetWeapon(WEAPON_RIFLE);
		}
		else if(m_pCharacter->m_aWeapons[WEAPON_SHOTGUN].m_Got && m_pCharacter->m_aWeapons[WEAPON_SHOTGUN].m_Ammo && distance(m_pCharacter->m_Pos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_ShotgunLifetime * 3000.0f)
		{
			GetCharacter()->SetWeapon(WEAPON_SHOTGUN);
		}
		else if(m_pCharacter->m_aWeapons[WEAPON_HAMMER].m_Got && distance(m_pCharacter->m_Pos, pClosestChar->m_Pos) < 40.0f)
		{
			GetCharacter()->SetWeapon(WEAPON_HAMMER);
		}
		else if(m_pCharacter->m_aWeapons[WEAPON_GUN].m_Got)
		{
			GetCharacter()->SetWeapon(WEAPON_GUN);
		}
		
		if((m_pCharacter->m_ActiveWeapon == WEAPON_RIFLE ? (!Collision()->FastIntersectLine(m_pCharacter->m_Pos,pClosestChar->m_Pos,nullptr,nullptr) && distance(m_pCharacter->m_Pos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_LaserReach) : !Collision()->FastIntersectLine(m_pCharacter->m_Pos,pClosestChar->m_Pos,nullptr,nullptr)) || m_pCharacter->m_aWeapons[WEAPON_NINJA].m_Got)
		{
			if(!GetCharacter()->GetLatestInput().m_Fire)
				Input.m_Fire = true;
			else
				Input.m_Fire = false;
			
			if(Server()->Tick() % Server()->TickSpeed() == 0)
			{
				Input.m_Hook = false;
			}
			else if(distance(m_pCharacter->m_Pos, pClosestChar->m_Pos) < GameServer()->Tuning()->m_HookLength)
			{
				Input.m_Hook = true;
			}
		}
		
		if(Collision()->FastIntersectLine(m_pCharacter->m_Pos,pClosestChar->m_Pos,nullptr,nullptr) && m_pCharacter->m_ActiveWeapon == WEAPON_RIFLE)
		{
			bool fire = false;
			
			float halfy = (pClosestChar->m_Pos.y - m_pCharacter->m_Pos.y) / 2;
			float halfx = (pClosestChar->m_Pos.x - m_pCharacter->m_Pos.x) / 2;
			
			vec2 BouncePos_Right,BouncePos_Left,BouncePos_Up,BouncePos_Down;
			bool Bounced_Right = false,Bounced_Left = false,Bounced_Up = false,Bounced_Down = false;
			
			vec2 At, tempvar;
			
			//try y first
			if(Collision()->FastIntersectLine(vec2(m_pCharacter->m_Pos.x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pCharacter->m_Pos.y),vec2(m_pCharacter->m_Pos.x,halfy + m_pCharacter->m_Pos.y),nullptr,&tempvar))
			{
				Collision()->UnIntersectLineKZ(vec2(m_pCharacter->m_Pos.x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pCharacter->m_Pos.y),vec2(m_pCharacter->m_Pos.x,halfy + m_pCharacter->m_Pos.y),&BouncePos_Right,nullptr);
				if(!Collision()->IntersectLine(m_pCharacter->m_Pos,BouncePos_Right,nullptr,nullptr) && tempvar.x > BouncePos_Right.x)
					Bounced_Right = true;
			}
			if(Collision()->FastIntersectLine(vec2(m_pCharacter->m_Pos.x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pCharacter->m_Pos.y),vec2(m_pCharacter->m_Pos.x,halfy + m_pCharacter->m_Pos.y),nullptr,&tempvar))
			{
				Collision()->UnIntersectLineKZ(vec2(m_pCharacter->m_Pos.x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pCharacter->m_Pos.y),vec2(m_pCharacter->m_Pos.x,halfy + m_pCharacter->m_Pos.y),&BouncePos_Left,nullptr);
				if(!Collision()->IntersectLine(m_pCharacter->m_Pos,BouncePos_Left,nullptr,nullptr) && tempvar.x < BouncePos_Left.x)
					Bounced_Left = true;
			}
			//try x now
			if(Collision()->FastIntersectLine(vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y),nullptr,&tempvar))
			{
				Collision()->UnIntersectLineKZ(vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y),&BouncePos_Down,nullptr);
				if(!Collision()->IntersectLine(m_pCharacter->m_Pos,BouncePos_Down,nullptr,nullptr) && tempvar.y > BouncePos_Down.y)
					Bounced_Down = true;
			}
			if(Collision()->FastIntersectLine(vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y),nullptr,&BouncePos_Up))
			{
				Collision()->UnIntersectLineKZ(vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y),&BouncePos_Up,nullptr);
				if(!Collision()->IntersectLine(m_pCharacter->m_Pos,BouncePos_Up,nullptr,nullptr) && tempvar.y < BouncePos_Up.y)
					Bounced_Up = true;
			}
			
			//try y
			if(Bounced_Right && (BouncePos_Right.y > (halfy + m_pCharacter->m_Pos.y) - 5.f && BouncePos_Right.y < (halfy + m_pCharacter->m_Pos.y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Right,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Right, vec2(m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y + (BouncePos_Right.y - m_pCharacter->m_Pos.y)*2), 0.f, At, GetCharacter()))
			{
				Input.m_TargetX = BouncePos_Right.x - m_pCharacter->m_Pos.x; // aim
				Input.m_TargetY = BouncePos_Right.y - m_pCharacter->m_Pos.y;
				fire = true;
			}
			else if(Bounced_Left && (BouncePos_Left.y > (halfy + m_pCharacter->m_Pos.y) - 5.f && BouncePos_Left.y < (halfy + m_pCharacter->m_Pos.y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Left,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Left, vec2(m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y + (BouncePos_Left.y - m_pCharacter->m_Pos.y)*2), 0.f, At, GetCharacter()))
			{
				Input.m_TargetX = BouncePos_Left.x - m_pCharacter->m_Pos.x; // aim
				Input.m_TargetY = BouncePos_Left.y - m_pCharacter->m_Pos.y;
				fire = true;
			}
				
			//try x
			if(Bounced_Up && (BouncePos_Up.x > (halfx + m_pCharacter->m_Pos.x) - 5.f && BouncePos_Up.x < (halfx + m_pCharacter->m_Pos.x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Up,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Up, vec2(m_pCharacter->m_Pos.x + (BouncePos_Up.x - m_pCharacter->m_Pos.x)*2,m_pCharacter->m_Pos.y), 0.f, At, GetCharacter()))
			{
				Input.m_TargetX = BouncePos_Up.x - m_pCharacter->m_Pos.x; // aim
				Input.m_TargetY = BouncePos_Up.y - m_pCharacter->m_Pos.y;
				fire = true;
			}
			else if(Bounced_Down && (BouncePos_Down.x > (halfx + m_pCharacter->m_Pos.x) - 5.f && BouncePos_Down.x < (halfx + m_pCharacter->m_Pos.x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Down,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Down, vec2(m_pCharacter->m_Pos.x + (BouncePos_Down.x - m_pCharacter->m_Pos.x)*2,m_pCharacter->m_Pos.y), 0.f, At, GetCharacter()))
			{
				Input.m_TargetX = BouncePos_Down.x - m_pCharacter->m_Pos.x; // aim
				Input.m_TargetY = BouncePos_Down.y - m_pCharacter->m_Pos.y;
				fire = true;
			}
					
			if(!fire)
			{
				//try again but moving point
				{
					float tempfloat =0.0f;
					
					tempfloat = halfy; //save halfy
					
					//move point
					halfx = halfx - m_pCharacter->m_Pos.x > 0 ? halfx + (tempfloat/2 + tempfloat/halfx) : halfx - (tempfloat/2 + tempfloat/halfx);
					if(halfy - m_pCharacter->m_Pos.y > 0)
					{
						halfy += (halfx/2 + halfx/halfy);
						if(halfy - m_pCharacter->m_Pos.y < 0)
							halfy += (halfy - m_pCharacter->m_Pos.y)*2;
					}
					else if(halfy - m_pCharacter->m_Pos.y < 0)
					{
						halfy -= (halfx/2 + halfx/halfy);
						if(halfy - m_pCharacter->m_Pos.y > 0)
							halfy -= (halfy - m_pCharacter->m_Pos.y)*2;
					}
					
					if(halfx - m_pCharacter->m_Pos.x > 0)
					{
						halfx += (tempfloat/2 + tempfloat/halfx);
						if(halfx - m_pCharacter->m_Pos.x < 0)
						{
							halfx += (halfx - m_pCharacter->m_Pos.x)*2;
						}
					}
					else if(halfx - m_pCharacter->m_Pos.x < 0)
					{
						halfx -= (tempfloat/2 + tempfloat/halfx);
						if(halfx - m_pCharacter->m_Pos.x > 0)
						{
							halfx -= (halfx - m_pCharacter->m_Pos.x)*2;
						}
					}
				}
				
				//vec2 BouncePos_Right,BouncePos_Left,BouncePos_Up,BouncePos_Down;
				Bounced_Right = false,Bounced_Left = false,Bounced_Up = false,Bounced_Down = false;
				
				//vec2 At, tempvar;
				
				//try y first
				if(Collision()->FastIntersectLine(vec2(m_pCharacter->m_Pos.x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pCharacter->m_Pos.y),vec2(m_pCharacter->m_Pos.x,halfy + m_pCharacter->m_Pos.y),nullptr,&tempvar))
				{
					Collision()->UnIntersectLineKZ(vec2(m_pCharacter->m_Pos.x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pCharacter->m_Pos.y),vec2(m_pCharacter->m_Pos.x,halfy + m_pCharacter->m_Pos.y),&BouncePos_Right,nullptr);
					if(!Collision()->IntersectLine(m_pCharacter->m_Pos,BouncePos_Right,nullptr,nullptr) && tempvar.x > BouncePos_Right.x)
						Bounced_Right = true;
				}
				if(Collision()->FastIntersectLine(vec2(m_pCharacter->m_Pos.x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pCharacter->m_Pos.y),vec2(m_pCharacter->m_Pos.x,halfy + m_pCharacter->m_Pos.y),nullptr,&tempvar))
				{
					Collision()->UnIntersectLineKZ(vec2(m_pCharacter->m_Pos.x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pCharacter->m_Pos.y),vec2(m_pCharacter->m_Pos.x,halfy + m_pCharacter->m_Pos.y),&BouncePos_Left,nullptr);
					if(!Collision()->IntersectLine(m_pCharacter->m_Pos,BouncePos_Left,nullptr,nullptr) && tempvar.x < BouncePos_Left.x)
						Bounced_Left = true;
				}
				//try x now
				if(Collision()->FastIntersectLine(vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y),nullptr,&tempvar))
				{
					Collision()->UnIntersectLineKZ(vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y),&BouncePos_Down,nullptr);
					if(!Collision()->IntersectLine(m_pCharacter->m_Pos,BouncePos_Down,nullptr,nullptr) && tempvar.y > BouncePos_Down.y)
						Bounced_Down = true;
				}
				if(Collision()->FastIntersectLine(vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y),nullptr,&BouncePos_Up))
				{
					Collision()->UnIntersectLineKZ(vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y),&BouncePos_Up,nullptr);
					if(!Collision()->IntersectLine(m_pCharacter->m_Pos,BouncePos_Up,nullptr,nullptr) && tempvar.y < BouncePos_Up.y)
						Bounced_Up = true;
				}
				
				//try y
				if(Bounced_Right && (BouncePos_Right.y > (halfy + m_pCharacter->m_Pos.y) - 5.f && BouncePos_Right.y < (halfy + m_pCharacter->m_Pos.y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Right,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Right, vec2(m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y + (BouncePos_Right.y - m_pCharacter->m_Pos.y)*2), 0.f, At, GetCharacter()))
				{
					Input.m_TargetX = BouncePos_Right.x - m_pCharacter->m_Pos.x; // aim
					Input.m_TargetY = BouncePos_Right.y - m_pCharacter->m_Pos.y;
					fire = true;
				}
				else if(Bounced_Left && (BouncePos_Left.y > (halfy + m_pCharacter->m_Pos.y) - 5.f && BouncePos_Left.y < (halfy + m_pCharacter->m_Pos.y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Left,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Left, vec2(m_pCharacter->m_Pos.x,m_pCharacter->m_Pos.y + (BouncePos_Left.y - m_pCharacter->m_Pos.y)*2), 0.f, At, GetCharacter()))
				{
					Input.m_TargetX = BouncePos_Left.x - m_pCharacter->m_Pos.x; // aim
					Input.m_TargetY = BouncePos_Left.y - m_pCharacter->m_Pos.y;
					fire = true;
				}
					
				//try x
				if(Bounced_Up && (BouncePos_Up.x > (halfx + m_pCharacter->m_Pos.x) - 5.f && BouncePos_Up.x < (halfx + m_pCharacter->m_Pos.x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Up,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Up, vec2(m_pCharacter->m_Pos.x + (BouncePos_Up.x - m_pCharacter->m_Pos.x)*2,m_pCharacter->m_Pos.y), 0.f, At, GetCharacter()))
				{
					Input.m_TargetX = BouncePos_Up.x - m_pCharacter->m_Pos.x; // aim
					Input.m_TargetY = BouncePos_Up.y - m_pCharacter->m_Pos.y;
					fire = true;
				}
				else if(Bounced_Down && (BouncePos_Down.x > (halfx + m_pCharacter->m_Pos.x) - 5.f && BouncePos_Down.x < (halfx + m_pCharacter->m_Pos.x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Down,pClosestChar->m_Pos,nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Down, vec2(m_pCharacter->m_Pos.x + (BouncePos_Down.x - m_pCharacter->m_Pos.x)*2,m_pCharacter->m_Pos.y), 0.f, At, GetCharacter()))
				{
					Input.m_TargetX = BouncePos_Down.x - m_pCharacter->m_Pos.x; // aim
					Input.m_TargetY = BouncePos_Down.y - m_pCharacter->m_Pos.y;
					fire = true;
				}
			}
				
			
			if(fire)
			{
				Input.m_Fire = true;
			}
				
			
			
		}
	}
	
	
	if(TargetPosSet && !m_StopUntilTouchGround)
	{
		if(DoSmartTargetChase && !m_DontDoSmartTargetChase)
		{
			if(m_TryingDirectionSmart)
			{
				Input.m_Direction = m_TryingDirectionSmart;
				if((TargetPos.y + 56.0f) < m_pCharacter->m_Pos.y && !(Collision()->GetCollisionAt(m_pCharacter->m_Pos.x , m_pCharacter->m_Pos.y -  GetCharacter()->m_ProximityRadius / 3.f) == TILE_DEATH))
				{
					targetisup = true;
				}
				else
				{
					dontjump = true;
					butjumpifwall = true;
				}
				if(!(Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(0.f,1000.f),nullptr,nullptr)))
				{
					//danger no floor, remove directionsmart
					m_TryingOppositeSmart = m_TryingDirectionSmart = 0;
				}
			}
			else if(((TargetPos.x - m_pCharacter->m_Pos.x < 0 ? (TargetPos.y - m_pCharacter->m_Pos.y < 0 ? TargetPos.x - m_pCharacter->m_Pos.x > TargetPos.y - m_pCharacter->m_Pos.y : (TargetPos.x - m_pCharacter->m_Pos.x)*-1 < TargetPos.y - m_pCharacter->m_Pos.y) : (TargetPos.y - m_pCharacter->m_Pos.y < 0 ? TargetPos.x - m_pCharacter->m_Pos.x < (TargetPos.y - m_pCharacter->m_Pos.y)*-1 : TargetPos.x - m_pCharacter->m_Pos.x < TargetPos.y - m_pCharacter->m_Pos.y)) && TargetPos.x > m_pCharacter->m_Pos.x - 500.0f && TargetPos.x < m_pCharacter->m_Pos.x + 500.0f))
			{
				//is up/down
				
				bool left = false,right = false;
				
				if(TargetPos.y < m_pCharacter->m_Pos.y)
				{
					left = Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-400.f,-400.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(0,-350.f),nullptr,nullptr);
					right = Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(400.f,-400.f),nullptr,nullptr);
					
					if(left && right && !m_TryingDirectionSmart && !Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(0,-350.f),nullptr,nullptr))//middle
					{
						Input.m_Direction = TargetPos.x > m_pCharacter->m_Pos.x ? 1 : -1;
						targetisup = true;
					}
					else if(!m_TryingDirectionSmart && !left)//left
					{
						Input.m_Direction = -1;
						targetisup = true;
					}
					else if(!m_TryingDirectionSmart && !right)//right
					{
						Input.m_Direction = 1;
						targetisup = true;
					}
					else
					{
						//bool leftside = false,rightside = false;
						//vec2 leftcol,rightcol;
						
						
						if(!m_TryingDirectionSmart)
						{
							m_TryingDirectionSmart = TargetPos.x > m_pCharacter->m_Pos.x ? 1 : -1;
							/*leftside = Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-250.f,0),nullptr,&leftcol);
							rightside = Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(250.f,0),nullptr,&rightcol);
							if(!leftside && !rightside)
							{
								m_TryingDirectionSmart = TargetPos.x > m_pCharacter->m_Pos.x ? 1 : -1;
							}
							else
							{
								if(!leftside)
								{
									m_TryingDirectionSmart = -1;
								}
								else if(!rightside)
								{
									m_TryingDirectionSmart = 1;
								}
								else
								{
									float d1,d2;
									d1 = distance(m_pCharacter->m_Pos,leftcol);
									d2 = distance(m_pCharacter->m_Pos,rightcol);
									
									if(d1 > d2)
									{
										m_TryingDirectionSmart = 1;
									}
									else
									{
										m_TryingDirectionSmart = -1;
									}
								}
							}*/
						}
						else
						{
							Input.m_Direction = m_TryingDirectionSmart;
						}
					}
				}
				else
				{
					left = Collision()->FastIntersectLine(m_pCharacter->m_Pos + vec2(-100.f,0),m_pCharacter->m_Pos + vec2(-100.f,-100.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(m_pCharacter->m_Pos + vec2(0,0),m_pCharacter->m_Pos + vec2(0,-100.f),nullptr,nullptr);
					right = Collision()->FastIntersectLine(m_pCharacter->m_Pos + vec2(100.f,0),m_pCharacter->m_Pos + vec2(100.f,-100.f),nullptr,nullptr);
					
					if(left && right && !m_TryingDirectionSmart && !Collision()->FastIntersectLine(m_pCharacter->m_Pos + vec2(0,0),m_pCharacter->m_Pos + vec2(0,-100.f),nullptr,nullptr))//middle
					{
						m_TryingDirectionSmart = TargetPos.x > m_pCharacter->m_Pos.x ? 1 : -1;
					}
					else if(!left && !m_TryingDirectionSmart)
					{
						m_TryingDirectionSmart = -1;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!right && !m_TryingDirectionSmart)
					{
						m_TryingDirectionSmart = 1;
						dontjump = true;
						butjumpifwall = true;
					}
					else
					{
						bool leftside = false,rightside = false;
						vec2 leftcol,rightcol;
						
						
						if(!m_TryingDirectionSmart)
						{
							if(!(leftside = Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-1000.f,0),nullptr,&leftcol)))
							{
								m_TryingDirectionSmart = -1;
							}
							else if(!(rightside = Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(1000.f,0),nullptr,&rightcol)))
							{
								m_TryingDirectionSmart = 1;
							}
							else
							{
								float d1,d2;
								d1 = distance(m_pCharacter->m_Pos,leftcol);
								d2 = distance(m_pCharacter->m_Pos,rightcol);
								
								if(d1 > d2)
								{
									m_TryingDirectionSmart = 1;
								}
								else
								{
									m_TryingDirectionSmart = -1;
								}
							}
						}
						else
						{
							Input.m_Direction = m_TryingDirectionSmart;
						}
					}
				}
			}
			else
			{
				//is still away
				
				//bool up = false,middle = false,down = false;
				
				if(TargetPos.x > m_pCharacter->m_Pos.x)
				{
					//up = Collision()->IntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(100.f,-100.f),nullptr,nullptr) || Collision()->IntersectLine(m_pCharacter->m_Pos + vec2(0,-100.f),m_pCharacter->m_Pos + vec2(50.f,-150.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(150.f,0),nullptr,nullptr);
					//down = Collision()->IntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(100.f,100.f),nullptr,nullptr)  || Collision()->IntersectLine(m_pCharacter->m_Pos + vec2(0,100.f),m_pCharacter->m_Pos + vec2(50.f,150.f),nullptr,nullptr);
					
					if(!Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(100.f,0),nullptr,nullptr))//middle
					{
						Input.m_Direction = 1;
						dontjump = true;
						jumpifgoingtofall = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(50.f,50.f),nullptr,nullptr))  || !(Collision()->IntersectLine(m_pCharacter->m_Pos + vec2(0,100.f),m_pCharacter->m_Pos + vec2(50.f,150.f),nullptr,nullptr)))//down
					{
						Input.m_Direction = 1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(50.f,-50.f),nullptr,nullptr)) || !(Collision()->IntersectLine(m_pCharacter->m_Pos + vec2(0,-100.f),m_pCharacter->m_Pos + vec2(50.f,-150.f),nullptr,nullptr)))//up
					{
						Input.m_Direction = 1;
						targetisup = true;
					}
					else
					{
						if(!m_TryingDirectionSmart)
						{
							bool upside = false, downside = false;
							downside = Collision()->FastIntersectLine(m_pCharacter->m_Pos + vec2(GetCharacter()->m_ProximityRadius/2,0),m_pCharacter->m_Pos + vec2(0,300.f),nullptr,nullptr);
							upside = Collision()->FastIntersectLine(m_pCharacter->m_Pos + vec2(GetCharacter()->m_ProximityRadius/2,0),m_pCharacter->m_Pos + vec2(0,-150.f),nullptr,nullptr);
							if(!upside)
							{
								targetisup = true;
							}
							else if(downside)
							{
								m_TryingDirectionSmart = -1;
								targetisup = true;
							}
							
						}
					}
				}
				else
				{
					//up = Collision()->IntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-100.f,-100.f),nullptr,nullptr) || Collision()->IntersectLine(m_pCharacter->m_Pos + vec2(0,-100.f),m_pCharacter->m_Pos + vec2(-50.f,-150.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-150.f,0),nullptr,nullptr);
					//down = Collision()->IntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-100.f,100.f),nullptr,nullptr)  || Collision()->IntersectLine(m_pCharacter->m_Pos + vec2(0,100.f),m_pCharacter->m_Pos + vec2(-50.f,150.f),nullptr,nullptr);
					
					if(!Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-100.f,0),nullptr,nullptr))//middle
					{
						Input.m_Direction = -1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-50.f,100.f),nullptr,nullptr))  || !(Collision()->FastIntersectLine(m_pCharacter->m_Pos + vec2(0,100.f),m_pCharacter->m_Pos + vec2(-50.f,150.f),nullptr,nullptr)))//down
					{
						Input.m_Direction = -1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-50.f,-100.f),nullptr,nullptr)) || !(Collision()->FastIntersectLine(m_pCharacter->m_Pos + vec2(0,-100.f),m_pCharacter->m_Pos + vec2(-50.f,-150.f),nullptr,nullptr)))//up
					{
						Input.m_Direction = -1;
						targetisup = true;
					}
					else
					{
						if(!m_TryingDirectionSmart)
						{
							bool upside = false, downside = false;
							downside = Collision()->FastIntersectLine(m_pCharacter->m_Pos + vec2(GetCharacter()->m_ProximityRadius/-2,0),m_pCharacter->m_Pos + vec2(0,300.f),nullptr,nullptr);
							upside = Collision()->FastIntersectLine(m_pCharacter->m_Pos + vec2(GetCharacter()->m_ProximityRadius/-2,0),m_pCharacter->m_Pos + vec2(0,-150.f),nullptr,nullptr);
							if(!upside)
							{
								targetisup = true;
							}
							else if(upside && downside)
							{
								m_TryingDirectionSmart = 1;
								targetisup = true;
							}
							
						}
					}
				}
			}
			
		}
		else
		{
			Input.m_Direction = TargetPos.x > m_pCharacter->m_Pos.x ? 1 : -1;
			
			if((TargetPos.y + 56.0f) < m_pCharacter->m_Pos.y && !(Collision()->GetCollisionAt(m_pCharacter->m_Pos.x , m_pCharacter->m_Pos.y - GetCharacter()->m_ProximityRadius / 3.f) == TILE_DEATH))
			{
				targetisup = true;
			}
			else
			{
				dontjump = true;
				butjumpifwall = true;
			}
			if(m_DontDoSmartTargetChase > 0)
				m_DontDoSmartTargetChase--;
		}

		if(m_DoGrenadeJump && ((GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE)) || (false && GetCharacter()->GetWeaponGot(WEAPON_RIFLE) && GetCharacter()->GetWeaponAmmo(WEAPON_RIFLE))))
		{
			if(GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE))
				GetCharacter()->SetActiveWeapon(WEAPON_GRENADE);
			else if(false && GetCharacter()->GetWeaponGot(WEAPON_RIFLE) && GetCharacter()->GetWeaponAmmo(WEAPON_RIFLE))
				GetCharacter()->SetActiveWeapon(WEAPON_RIFLE);
			Input.m_TargetX = 0;
			Input.m_TargetY = 1;
			Input.m_Fire = 1;
		}

		m_DoGrenadeJump = false;

		if(((GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE)) || (false && GetCharacter()->GetWeaponGot(WEAPON_RIFLE) && GetCharacter()->GetWeaponAmmo(WEAPON_RIFLE))) && ((GameServer()->m_pController->m_pGameType[0] == 'i' || GameServer()->m_pController->m_pGameType[0] == 'g') || GetCharacter()->GetHealth() >= 10 || (GetCharacter()->GetHealth() >= 5 && GetCharacter()->GetArmor() >= 5)))
		{
			if(m_pCharacter->GetCore()->m_Jumped > 0 && TargetPos.x > m_pCharacter->m_Pos.x - 300.f && TargetPos.x < m_pCharacter->m_Pos.x + 300.f && TargetPos.y < m_pCharacter->m_Pos.y - 150.f && GetCharacter()->IsGrounded() && !Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(0.f,-300.f),nullptr,nullptr))
			{
				m_DoGrenadeJump = true;
				Input.m_Jump = true;
				if(GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE))
					GetCharacter()->SetActiveWeapon(WEAPON_GRENADE);
				else if(false && GetCharacter()->GetWeaponGot(WEAPON_RIFLE) && GetCharacter()->GetWeaponAmmo(WEAPON_RIFLE))
					GetCharacter()->SetActiveWeapon(WEAPON_RIFLE);
			}
			else if(TargetPos.x < m_pCharacter->m_Pos.x - 700.f && GetCharacter()->IsGrounded() && !Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-300.f,0.f),nullptr,nullptr))
			{
				Input.m_TargetX = 1;
				Input.m_TargetY = 1;
				Input.m_Fire = 1;
				if(GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE))
					GetCharacter()->SetActiveWeapon(WEAPON_GRENADE);
				else if(false && GetCharacter()->GetWeaponGot(WEAPON_RIFLE) && GetCharacter()->GetWeaponAmmo(WEAPON_RIFLE))
					GetCharacter()->SetActiveWeapon(WEAPON_RIFLE);
			}
			else if(TargetPos.x > m_pCharacter->m_Pos.x + 700.f && GetCharacter()->IsGrounded() && !Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(300.f,0.f),nullptr,nullptr))
			{
				Input.m_TargetX = -1;
				Input.m_TargetY = 1;
				Input.m_Fire = 1;
				if(GetCharacter()->GetWeaponGot(WEAPON_GRENADE) && GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE))
					GetCharacter()->SetActiveWeapon(WEAPON_GRENADE);
				else if(false && GetCharacter()->GetWeaponGot(WEAPON_RIFLE) && GetCharacter()->GetWeaponAmmo(WEAPON_RIFLE))
					GetCharacter()->SetActiveWeapon(WEAPON_RIFLE);
			}
		}
	}
	
	if(m_TryingDirectionSmart && !Collision()->FastIntersectLine(m_pCharacter->m_Pos,TargetPos,nullptr,nullptr))
	{
		m_TryingOppositeSmart = m_TryingDirectionSmart = 0;
		m_StopUntilTouchGround = true;
	}
	
	if(m_StopUntilTouchGround && GetCharacter()->IsGrounded())
	{
		m_StopUntilTouchGround = false;
		m_DontDoSmartTargetChase = Server()->TickSpeed() * 2;
	}
	if(m_pCharacter->GetCore()->m_Colliding && GetCharacter()->IsGrounded())
	{
		if(!m_TryingOppositeSmart)
		{
			m_TryingDirectionSmart *= -1;
			m_TryingOppositeSmart = true;
		}
		else
		{
			m_TryingDirectionSmart = 0;
			m_TryingOppositeSmart = false;
		}
	}

	if(!m_pCharacter->GetCore()->m_Jumped && !Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(0.f,1000.f),nullptr,nullptr))
	{
		vec2 right,left;

		Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(1000.f,1000.f),&right,nullptr);
		Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(-1000.f,1000.f),&left,nullptr);

		float rightlength = right.x - m_pCharacter->m_Pos.x;
		float leftlength = m_pCharacter->m_Pos.x - left.x;

		if(rightlength == leftlength)
		{
			Input.m_Direction = 0;
		}
		else if(rightlength > leftlength)
		{
			Input.m_Direction = -1;
		}
		else
		{
			Input.m_Direction = 1;
		}
	}
	
	
	//HELP
	if(!m_DoGrenadeJump && ((jumpifgoingtofall ? !(Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(0.f,1000.f),nullptr,nullptr)) : false) || (butjumpifwall ? m_pCharacter->GetCore()->m_Colliding : false) || (!dontjump && ((Collision()->GetCollisionAt(m_pCharacter->m_Pos.x , m_pCharacter->m_Pos.y + GetCharacter()->m_ProximityRadius / 3.f) == TILE_DEATH) || !Collision()->FastIntersectLine(m_pCharacter->m_Pos,m_pCharacter->m_Pos + vec2(0.f,1000.f),nullptr,nullptr) || m_pCharacter->GetCore()->m_Colliding || (((Collision()->CheckPoint(m_pCharacter->m_Pos.x + GetCharacter()->m_ProximityRadius / 2, m_pCharacter->m_Pos.y + GetCharacter()->m_ProximityRadius / 2 + 5)) && !(Collision()->CheckPoint(m_pCharacter->m_Pos.x - GetCharacter()->m_ProximityRadius / 2, m_pCharacter->m_Pos.y + GetCharacter()->m_ProximityRadius / 2 + 5)))) || ((!(Collision()->CheckPoint(m_pCharacter->m_Pos.x + GetCharacter()->m_ProximityRadius / 2, m_pCharacter->m_Pos.y + GetCharacter()->m_ProximityRadius / 2 + 5)) && (Collision()->CheckPoint(m_pCharacter->m_Pos.x - GetCharacter()->m_ProximityRadius / 2, m_pCharacter->m_Pos.y + GetCharacter()->m_ProximityRadius / 2 + 5)))) || targetisup))))
	{
		if(GetCharacter()->IsGrounded() || (m_pCharacter->GetCore()->m_Jumped > 0 && m_pCharacter->GetCore()->m_Vel.y > 0))
			Input.m_Jump = true;
		else
			Input.m_Jump = false;
	}
}

CCollision* CPlayer::Collision()
{ return GameServer()->Collision(); }