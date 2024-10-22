/*
 * ifreeze.cpp
 *
 *  Created on: 25.12.2011
 *      Author: Teetime
 *      iFreeze ported to 0.6 by Teetime, originally created by Tom (Tom94).
 *      Visit http://www.teeworlds.com/forum/viewtopic.php?pid=46951#p46951 for more information.
 */

#include "tdm.h"
#include "ifreeze.h"
#include <engine/shared/config.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>

CGameControllerIFreeze::CGameControllerIFreeze(class CGameContext *pGameServer, int TypeFlags) : CGameControllerTDM(pGameServer, TypeFlags)
{
	m_Flags = TypeFlags;
	// m_pGameType = "iFreeze+";
	m_pGameType = (IsGrenade()) ? "gFreeze+" : "iFreeze+";
}

int CGameControllerIFreeze::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, Weapon);

	if(Weapon != WEAPON_GAME && Weapon != WEAPON_WORLD && Weapon != WEAPON_SELF)
	{
		CPlayer* pPV = pVictim->GetPlayer();
		if(pKiller != pVictim->GetPlayer())
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "You froze %s", Server()->ClientName(pPV->GetCID()));
			GameServer()->SendBroadcast(aBuf, pKiller->GetCID());
			str_format(aBuf, sizeof(aBuf), "%s froze you", Server()->ClientName(pKiller->GetCID()));
			GameServer()->SendBroadcast(aBuf, pPV->GetCID());
		}
		pVictim->Freeze(g_Config.m_SvIFreezeAutomeltTime);
	}
	return 0;
}

void CGameControllerIFreeze::DoFreezeScoring()
{
	int Red = 0, Blue = 0, RedFr = 0, BlueFr = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_BLUE)
			{
				Blue++;
				if(GameServer()->GetPlayerChar(i) && GameServer()->GetPlayerChar(i)->Frozen())
					BlueFr++;
			}
			else if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_RED)
			{
				Red++;
				if(GameServer()->GetPlayerChar(i) && GameServer()->GetPlayerChar(i)->Frozen())
					RedFr++;
			}
		}
	}

	bool BlueScored = RedFr >= Red && Red && RedFr;
	if(BlueScored || (BlueFr >= Blue && Blue && BlueFr))
	{
		m_aTeamscore[(BlueScored) ? TEAM_BLUE : TEAM_RED]++;
		ResetFrozenPlayer();
		GameServer()->SendBroadcast((BlueScored) ? "Blue team scores" : "Red team scores", -1);
		GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
	}
}

void CGameControllerIFreeze::ResetFrozenPlayer()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && GameServer()->GetPlayerChar(i))
			GameServer()->GetPlayerChar(i)->KillChar();
}

void CGameControllerIFreeze::DoWincheck()
{
	CGameControllerTDM::DoWincheck();
	DoFreezeScoring();
}

void CGameControllerIFreeze::Tick()
{
	CGameControllerTDM::Tick();
}
