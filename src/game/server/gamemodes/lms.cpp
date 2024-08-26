/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "lms.h"
#include <engine/shared/config.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>

CGameControllerLMS::CGameControllerLMS(class CGameContext *pGameServer, int TypeFlags)
: IGameController(pGameServer)
{
	m_Flags = TypeFlags;
	if (TypeFlags&IGameController::GAMETYPE_ISTEAM)
		m_pGameType = (IsInstagib()) ? "iLTS+" : "LTS+";
	else
		m_pGameType = (IsInstagib()) ? "iLMS+" : "LMS+";

	if (TypeFlags&IGameController::GAMETYPE_ISTEAM)
		m_GameFlags |= GAMEFLAG_TEAMS;
}

void CGameControllerLMS::DoWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup)
	{
		int PlayerCount = 0, PlayerCountBlue = 0;
		int AliveCount = 0, AliveCountBlue = 0;
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_IsReady && GameServer()->IsClientPlayer(i)) {
				if (GameServer()->m_apPlayers[i]->GetTeam() == TEAM_BLUE) {
					PlayerCountBlue++;
					if (GameServer()->m_apPlayers[i]->m_Lives > 0)
						AliveCountBlue++;
				} else {
					PlayerCount++;
					if (GameServer()->m_apPlayers[i]->m_Lives > 0)
						AliveCount++;
				}
			}
		}
		if (IsTeamplay()) {
			if ((AliveCountBlue <= 0 || AliveCount <= 0) && (PlayerCount > 0 && PlayerCountBlue > 0))
				EndRound();
		} else {
			if (AliveCount <= 1 && PlayerCount > 1)
				EndRound();
		}
	}
}

void CGameControllerLMS::Tick()
{
	IGameController::Tick();
}

