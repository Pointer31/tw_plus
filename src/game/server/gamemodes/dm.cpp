/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "dm.h"


CGameControllerDM::CGameControllerDM(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = m_Instagib ? (m_Instagib == 2 ? "gDM+" : "iDM+") : "DM+";
}

const char* CGameControllerDM::GetGameHelpText()
{
	return "Gametype: Death Match. Kill others to get points!";
}