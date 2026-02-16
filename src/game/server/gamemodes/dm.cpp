/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "dm.h"


CGameControllerDM::CGameControllerDM(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	switch (m_Instagib)
	{
	case 1: m_pGameType = "iDM+"; break;
	case 2: m_pGameType = "gDM+"; break;
	case 3: m_pGameType = "nDM+"; break;
	default: m_pGameType = "DM+"; break;
	}
}

const char* CGameControllerDM::GetGameHelpText()
{
	return "Gametype: Death Match. Kill others to get points!";
}