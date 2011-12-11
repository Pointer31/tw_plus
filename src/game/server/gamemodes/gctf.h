/* gCTF - CTF only with grenade - 1Shot1Kill
 * Made by Teetime
 */
#ifndef GAME_SERVER_GAMEMODES_GCTF_H
#define GAME_SERVER_GAMEMODES_GCTF_H
#include <game/server/gamecontroller.h>
#include "ctf.h"

class CGameControllerGCTF : public CGameControllerCTF
{
public:
	CGameControllerGCTF(class CGameContext *pGameServer, int);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual void Tick();
};

#endif
