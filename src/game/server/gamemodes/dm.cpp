/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "dm.h"


CGameControllerDM::CGameControllerDM(class CGameContext *pGameServer, int TypeFlags)
: IGameController(pGameServer)
{
	m_Flags = TypeFlags;
	m_pGameType = (IsInstagib()) ? "iDM+" : "DM+";
}

void CGameControllerDM::Tick()
{
	IGameController::Tick();
}
