/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "race.h"

CGameControllerRACE::CGameControllerRACE(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "race(silly)+";
	m_GameFlags = GAMEFLAG_RACE;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aRacers[i].start = 0;
		m_aRacers[i].finish = 0;
	}
	if(true)
		GameServer()->Collision()->ClearTelePositions();
}

const char* CGameControllerRACE::GetGameHelpText()
{
	return "WIP racing gametype? Race from start to finish. Ranks are not saved.";
}

int CGameControllerRACE::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int WeaponID)
{
	return 0;
}

void CGameControllerRACE::Tick()
{
	IGameController::Tick();

	const int TILE_START = 33;
	const int TILE_FINISH = 34;
	const int TILE_FREEZE = 9;
	const int TILE_UNFREEZE = 11;
	const int TILE_DEEPFREEZE = 12;
	const int TILE_DEEPUNFREEZE = 13;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter *pChar = GameServer()->GetPlayerChar(i);
		if (pChar)
		{
			vec2 Pos = pChar->GetPos();
			if (GameServer()->Collision()->GetCollisionAtId(Pos.x, Pos.y) == TILE_START)
			{
				m_aRacers[i].start = Server()->Tick();
			}
			else if (GameServer()->Collision()->GetCollisionAtId(Pos.x, Pos.y) == TILE_FINISH && m_aRacers[i].start > 0)
			{
				m_aRacers[i].finish = Server()->Tick();
				int Ticks = m_aRacers[i].finish - m_aRacers[i].start;
				int Time = Ticks * 1000 / Server()->TickSpeed();
				m_aRacers[i].start = 0;

				if (pChar->GetPlayer()->m_Score <= 0 || Time < pChar->GetPlayer()->m_Score)
				{
					pChar->GetPlayer()->m_Score = Time;
				}
				CNetMsg_Sv_RaceFinish Msg;
				Msg.m_ClientID = i;
				Msg.m_Time = Time;
				Msg.m_Diff = 0;
				Msg.m_RecordPersonal = 0;
				Msg.m_RecordServer = 0;
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);

				if (Config()->m_SvRaceFinishInChat)
				{
					char aBuf[128];
					str_format(aBuf, sizeof(aBuf), "★ '%s' finished in: %.2fs", Server()->ClientName(i), (float)Time/1000);
					GameServer()->SendChat(-1, CHAT_ALL, -1, aBuf);
				}
			}
			else if (GameServer()->Collision()->GetCollisionAtId(Pos.x, Pos.y) == TILE_FREEZE && !pChar->IsDeepFrozen())
			{
				pChar->Freeze(3);
			}
			else if (GameServer()->Collision()->GetCollisionAtId(Pos.x, Pos.y) == TILE_UNFREEZE && !pChar->IsDeepFrozen())
			{
				pChar->Freeze(0);
			}
			else if (GameServer()->Collision()->GetCollisionAtId(Pos.x, Pos.y) == TILE_DEEPFREEZE)
			{
				pChar->Freeze(-1);
			}
			else if (GameServer()->Collision()->GetCollisionAtId(Pos.x, Pos.y) == TILE_DEEPUNFREEZE)
			{
				pChar->Freeze(0);
			}

		}
		else
		{
			m_aRacers[i].start = 0;
			m_aRacers[i].finish = 0;
		}
	}
}

void CGameControllerRACE::OnPlayerConnect(class CPlayer *pPlayer)
{
	SetGameState(IGS_GAME_RUNNING);
	IGameController::OnPlayerConnect(pPlayer);
	pPlayer->m_Score = -1;
}

bool CGameControllerRACE::IsFriendlyFire(int ClientID1, int ClientID2) const
{
	if(ClientID1 == ClientID2)
		return false;

	return !Config()->m_SvRaceFriendlyFire;
}

void CGameControllerRACE::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	if (m_aRacers[SnappingClient].start != 0)
	{
		CNetObj_PlayerInfoRace *pPlayerInfoRace = static_cast<CNetObj_PlayerInfoRace *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFORACE, SnappingClient, sizeof(CNetObj_PlayerInfoRace)));
		if(!pPlayerInfoRace)
			return;

		pPlayerInfoRace->m_RaceStartTick = m_aRacers[SnappingClient].start;
	}

	CNetObj_GameDataRace *pGameDataRace = static_cast<CNetObj_GameDataRace *>(Server()->SnapNewItem(NETOBJTYPE_GAMEDATARACE, 0, sizeof(CNetObj_GameDataRace)));
	if(!pGameDataRace)
		return;

	pGameDataRace->m_BestTime = -1;
	pGameDataRace->m_Precision = 2;
	pGameDataRace->m_RaceFlags = 0;
}