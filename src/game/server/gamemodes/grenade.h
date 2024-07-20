/* gCTF - CTF only with grenade - 1Shot1Kill
 * Made by Teetime
 */
#ifndef GAME_SERVER_GAMEMODES_GCTF_H
#define GAME_SERVER_GAMEMODES_GCTF_H
#include <game/server/gamecontroller.h>
#include "ctf.h"
#include "dm.h"
#include "tdm.h"
#include "htf.h"


class CGameControllerGCTF : public CGameControllerCTF
{
public:
	CGameControllerGCTF(class CGameContext *pGameServer, int);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual void Tick();
};

class CGameControllerGDM : public CGameControllerDM
{
public:
	CGameControllerGDM(class CGameContext *pGameServer, int);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual void Tick();
};

class CGameControllerGTDM : public CGameControllerTDM
{
public:
	CGameControllerGTDM(class CGameContext *pGameServer, int);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual void Tick();
};

class CGameControllerGHTF : public CGameControllerHTF
{
public:
	CGameControllerGHTF(class CGameContext *pGameServer, int);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual void Tick();
};

#endif
