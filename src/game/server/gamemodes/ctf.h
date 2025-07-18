/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_CTF_H
#define GAME_SERVER_GAMEMODES_CTF_H
#include <game/server/gamecontroller.h>
#include <game/server/entity.h>

class CGameControllerCTF : public IGameController
{
public:
	// class CFlag *m_apFlags[2];
	vec2 m_flagstands_0[10];
	int m_flagstand_temp_i_0;
	vec2 m_flagstands_1[10];
	int m_flagstand_temp_i_1;

	CGameControllerCTF(class CGameContext *pGameServer, int);
	virtual void DoWincheck();
	virtual bool CanBeMovedOnBalance(int ClientID);
	virtual void Snap(int SnappingClient);
	virtual void Tick();

	virtual bool OnEntity(int Index, vec2 Pos);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
};

#endif

