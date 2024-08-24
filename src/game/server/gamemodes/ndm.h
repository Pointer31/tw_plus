/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_NDM_H
#define GAME_SERVER_GAMEMODES_NDM_H
#include <game/server/gamecontroller.h>

class CGameControllerNDM : public IGameController
{
public:
	CGameControllerNDM(class CGameContext *pGameServer, int);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual void Tick();
private:
	int m_CurrentWeapon;
	int m_NextWeapon;
	int m_NextSwitchTick;
};
#endif
