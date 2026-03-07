/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_HTF_H
#define GAME_SERVER_GAMEMODES_HTF_H
#include <game/server/gamecontroller.h>
#include <game/server/entity.h>
#include <game/server/player.h>

class CGameControllerHTF : public IGameController
{
	// balancing
	virtual bool CanBeMovedOnBalance(int ClientID) const;

	// game
	class CFlag *m_apFlags[2];

	virtual bool DoWincheckMatch();

	bool m_UseTimeDisplay;

	struct HTFPlayer
	{
		int PointTicks;
	};
	
	HTFPlayer m_aHTFPlayers[MAX_CLIENTS];

protected:
	virtual void StartMatch();

public:
	CGameControllerHTF(class CGameContext *pGameServer);
	virtual const char *GetGameHelpText();
	
	// event
	virtual void OnPlayerConnect(class CPlayer *pPlayer);
	virtual int GetPlayerScore(CPlayer *pPlayer, int SnappingClient);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual void OnFlagReturn(class CFlag *pFlag);
	virtual bool OnEntity(int Index, vec2 Pos);

	// general
	virtual void Snap(int SnappingClient);
	virtual void Tick();
};

#endif

