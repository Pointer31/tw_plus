/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include "tdm.h"

CGameControllerTDM::CGameControllerTDM(class CGameContext *pGameServer) : IGameController(pGameServer)
{
	switch (m_Instagib)
	{
	case 1: m_pGameType = "iTDM+"; break;
	case 2: m_pGameType = "gTDM+"; break;
	case 3: m_pGameType = "nTDM+"; break;
	default: m_pGameType = "TDM+"; break;
	}
	m_GameFlags = GAMEFLAG_TEAMS;
}

const char* CGameControllerTDM::GetGameHelpText()
{
	return "Gametype: Team Death Match. Kill the other team for points!";
}

// event
int CGameControllerTDM::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, Weapon);


	if(pKiller && Weapon != WEAPON_GAME)
	{
		// do team scoring
		if(pKiller == pVictim->GetPlayer() || pKiller->GetTeam() == pVictim->GetPlayer()->GetTeam())
			m_aTeamscore[pKiller->GetTeam()&1]--; // klant arschel
		else
			m_aTeamscore[pKiller->GetTeam()&1]++; // good shit
	}

	pVictim->GetPlayer()->m_RespawnTick = maximum(pVictim->GetPlayer()->m_RespawnTick, Server()->Tick()+Server()->TickSpeed()*Config()->m_SvRespawnDelayTDM);

	return 0;
}
