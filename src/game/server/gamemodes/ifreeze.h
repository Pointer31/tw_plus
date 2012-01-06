/*
 * ifreeze.cpp
 *
 *  Created on: 25.12.2011
 *      Author: Teetime
 *      iFreeze ported to 0.6 by Teetime, originally created by Tom (Tom94).
 *      Visit http://www.teeworlds.com/forum/viewtopic.php?pid=46951#p46951 for more informations.
 */

#ifndef GAME_SERVER_GAMEMODES_IFREEZE_H
#define GAME_SERVER_GAMEMODES_IFREEZE_H

#include <game/server/gamecontroller.h>

class CGameControllerIFreeze : public CGameControllerTDM
{
public:
	CGameControllerIFreeze(class CGameContext *pGameServer, int);

	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	void DoFreezeScoring();
	void ResetFrozenPlayer();

	virtual void DoWincheck();
	virtual void Tick();
};


#endif /* IFREEZE_H_ */
