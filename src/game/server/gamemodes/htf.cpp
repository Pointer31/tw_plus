/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <game/mapitems.h>

#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include "htf.h"

CGameControllerHTF::CGameControllerHTF(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// game
	m_apFlags[0] = 0;
	m_apFlags[1] = 0;
	switch (m_Instagib)
	{
	case 1: m_pGameType = "iHTF+"; break;
	case 2: m_pGameType = "gHTF+"; break;
	case 3: m_pGameType = "nHTF+"; break;
	default: m_pGameType = "HTF+"; break;
	}
	m_GameFlags = GAMEFLAG_FLAGS;
	m_UseTimeDisplay = Config()->m_SvHTFTimeDisplay;
	if (m_UseTimeDisplay)
		m_GameFlags |= GAMEFLAG_RACE;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aHTFPlayers[i].PointTicks = Server()->TickSpeed();
	}
}

const char* CGameControllerHTF::GetGameHelpText()
{
	return "Gametype: Hold The Flag. Hold the flag to continually get points!";
}

void CGameControllerHTF::OnPlayerConnect(class CPlayer *pPlayer)
{
	SetGameState(IGS_GAME_RUNNING);
	IGameController::OnPlayerConnect(pPlayer);
	m_aHTFPlayers[pPlayer->GetCID()].PointTicks = Server()->TickSpeed();
}

// balancing
bool CGameControllerHTF::CanBeMovedOnBalance(int ClientID) const
{
	CCharacter* Character = GameServer()->m_apPlayers[ClientID]->GetCharacter();
	if(Character)
	{
		for(int fi = 0; fi < 2; fi++)
		{
			CFlag *F = m_apFlags[fi];
			if(F && F->GetCarrier() == Character)
				return false;
		}
	}
	return true;
}

// event
int CGameControllerHTF::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int WeaponID)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, WeaponID);
	int HadFlag = 0;

	// drop flags
	for(int i = 0; i < 2; i++)
	{
		CFlag *F = m_apFlags[i];
		if(F && pKiller && pKiller->GetCharacter() && F->GetCarrier() == pKiller->GetCharacter())
			HadFlag |= 2;
		if(F && F->GetCarrier() == pVictim)
		{
			GameServer()->SendGameMsg(GAMEMSG_CTF_DROP, -1);
			F->Drop();

			if(pKiller && pKiller->GetTeam() != pVictim->GetPlayer()->GetTeam())
				pKiller->m_Score++;

			HadFlag |= 1;
		}
	}

	return HadFlag;
}

void CGameControllerHTF::OnFlagReturn(CFlag *pFlag)
{
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "flag_return");
	GameServer()->SendGameMsg(GAMEMSG_CTF_RETURN, -1);
}

bool CGameControllerHTF::OnEntity(int Index, vec2 Pos)
{
	if(IGameController::OnEntity(Index, Pos))
		return true;

	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED) Team = TEAM_RED;
	if(Index == ENTITY_FLAGSTAND_BLUE) Team = TEAM_BLUE;
	if(Team == -1 || m_apFlags[Team])
		return false;

	CFlag *F = new CFlag(&GameServer()->m_World, Team, Pos);
	m_apFlags[Team] = F;
	return true;
}

// game
bool CGameControllerHTF::DoWincheckMatch()
{
	{
		// gather some stats
		int Topscore = 0;
		int TopscoreCount = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i])
			{
				if(GameServer()->m_apPlayers[i]->m_Score > Topscore)
				{
					Topscore = GameServer()->m_apPlayers[i]->m_Score;
					TopscoreCount = 1;
				}
				else if(GameServer()->m_apPlayers[i]->m_Score == Topscore)
					TopscoreCount++;
			}
		}

		// check score win condition
		if((m_GameInfo.m_ScoreLimit > 0 && Topscore >= m_GameInfo.m_ScoreLimit) ||
			(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60))
		{
			if(TopscoreCount == 1)
			{
				EndMatch();
				return true;
			}
			else
				m_SuddenDeath = 1;
		}
	}
	return false;
	// // check score win condition
	// if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)) ||
	// 	(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60))
	// {
	// 	if(m_SuddenDeath)
	// 	{
	// 		if(m_aTeamscore[TEAM_RED]/100 != m_aTeamscore[TEAM_BLUE]/100)
	// 		{
	// 			EndMatch();
	// 			return true;
	// 		}
	// 	}
	// 	else
	// 	{
	// 		if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
	// 		{
	// 			EndMatch();
	// 			return true;
	// 		}
	// 		else
	// 			m_SuddenDeath = 1;
	// 	}
	// }
	// return false;
}

// general
void CGameControllerHTF::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameDataFlag *pGameDataFlag = static_cast<CNetObj_GameDataFlag *>(Server()->SnapNewItem(NETOBJTYPE_GAMEDATAFLAG, 0, sizeof(CNetObj_GameDataFlag)));
	if(!pGameDataFlag)
		return;

	pGameDataFlag->m_FlagDropTickRed = 0;
	if(m_apFlags[TEAM_RED])
	{
		if(m_apFlags[TEAM_RED]->IsAtStand())
			pGameDataFlag->m_FlagCarrierRed = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_RED]->GetCarrier() && m_apFlags[TEAM_RED]->GetCarrier()->GetPlayer())
			pGameDataFlag->m_FlagCarrierRed = m_apFlags[TEAM_RED]->GetCarrier()->GetPlayer()->GetCID();
		else
		{
			pGameDataFlag->m_FlagCarrierRed = FLAG_TAKEN;
			pGameDataFlag->m_FlagDropTickRed = m_apFlags[TEAM_RED]->GetDropTick();
		}
	}
	else
		pGameDataFlag->m_FlagCarrierRed = FLAG_MISSING;
	pGameDataFlag->m_FlagDropTickBlue = 0;
	if(m_apFlags[TEAM_BLUE])
	{
		if(m_apFlags[TEAM_BLUE]->IsAtStand())
			pGameDataFlag->m_FlagCarrierBlue = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_BLUE]->GetCarrier() && m_apFlags[TEAM_BLUE]->GetCarrier()->GetPlayer())
			pGameDataFlag->m_FlagCarrierBlue = m_apFlags[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetCID();
		else
		{
			pGameDataFlag->m_FlagCarrierBlue = FLAG_TAKEN;
			pGameDataFlag->m_FlagDropTickBlue = m_apFlags[TEAM_BLUE]->GetDropTick();
		}
	}
	else
		pGameDataFlag->m_FlagCarrierBlue = FLAG_MISSING;
}

void CGameControllerHTF::Tick()
{
	IGameController::Tick();

	if(GameServer()->m_World.m_ResetRequested || GameServer()->m_World.m_Paused)
		return;

	for(int fi = 0; fi < 2; fi++)
	{
		CFlag *F = m_apFlags[fi];

		if(!F)
			continue;

		//
		if(F->GetCarrier())
		{
			CPlayer *pPlayer = F->GetCarrier()->GetPlayer();
			if (m_aHTFPlayers[pPlayer->GetCID()].PointTicks <= 0)
			{
				m_aHTFPlayers[pPlayer->GetCID()].PointTicks = Server()->TickSpeed();
				pPlayer->m_Score += 1;
			}
			m_aHTFPlayers[pPlayer->GetCID()].PointTicks--;
		}
		else
		{
			CCharacter *apCloseCCharacters[MAX_CLIENTS];
			int Num = GameServer()->m_World.FindEntities(F->GetPos(), CFlag::ms_PhysSize, (CEntity**)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			for(int i = 0; i < Num; i++)
			{
				if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(F->GetPos(), apCloseCCharacters[i]->GetPos(), NULL, NULL))
					continue;

				// player can't take 2 flags
				if (m_apFlags[fi^1] && m_apFlags[fi^1]->GetCarrier() && m_apFlags[fi^1]->GetCarrier() == apCloseCCharacters[i])
					continue;

				{
					// take the flag
					if(F->IsAtStand())
						m_aTeamscore[fi^1]++;

					F->Grab(apCloseCCharacters[i]);

					F->GetCarrier()->GetPlayer()->m_Score += 1;

					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "flag_grab player='%d:%s' team=%d",
						F->GetCarrier()->GetPlayer()->GetCID(),
						Server()->ClientName(F->GetCarrier()->GetPlayer()->GetCID()),
						F->GetCarrier()->GetPlayer()->GetTeam()
					);
					GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
					GameServer()->SendGameMsg(GAMEMSG_CTF_GRAB, fi, -1);
					break;
				}
			}
		}
	}
	// do a win check(grabbing flags could trigger win condition)
	DoWincheckMatch();
}

int CGameControllerHTF::GetPlayerScore(CPlayer *pPlayer, int SnappingClient)
{
	if (m_UseTimeDisplay)
		return (Config()->m_SvScorelimit - pPlayer->m_Score - 1)*1000 + (m_aHTFPlayers[pPlayer->GetCID()].PointTicks/5*5)*1000/Server()->TickSpeed();
	else
		return pPlayer->m_Score;
}

void CGameControllerHTF::StartMatch()
{
	IGameController::StartMatch();

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aHTFPlayers[i].PointTicks = Server()->TickSpeed();
	}
}