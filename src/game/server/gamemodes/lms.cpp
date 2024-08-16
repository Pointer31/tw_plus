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
	m_pGameType = (IsInstagib()) ? "iLMS+" : "LMS+";
}

void CGameControllerLMS::DoWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup)
	{
		int PlayerCount = 0, AliveCount = 0;
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_IsReady && GameServer()->IsClientPlayer(i)) {
				if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_Lives > 0)
					AliveCount++;

				PlayerCount++;
			}
		}
		if (AliveCount <= 1 && PlayerCount > 1)
			EndRound();
	}
}

void CGameControllerLMS::Tick()
{
	IGameController::Tick();
}

