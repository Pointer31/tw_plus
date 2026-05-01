/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_RACE_H
#define GAME_SERVER_GAMEMODES_RACE_H
#include <game/server/gamecontroller.h>

class CGameControllerRACE : public IGameController
{
	struct Racer
	{
		int start;
		int finish;
	};
	
	Racer m_aRacers[MAX_CLIENTS];
public:
	CGameControllerRACE(class CGameContext *pGameServer);
	virtual const char *GetGameHelpText();

	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual void Snap(int SnappingClient);
	virtual void Tick();
	virtual void OnPlayerConnect(class CPlayer *pPlayer);
	virtual bool IsFriendlyFire(int ClientID1, int ClientID2) const;

	virtual bool IsUnfreezeHammers() const { return true; }
	virtual bool IsUnfreezeLasers() const { return true; }
};

#endif
