/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_CUSTOMPICKUP_H
#define GAME_SERVER_ENTITIES_CUSTOMPICKUP_H

#include <game/server/entity.h>

const int PickupCustomPhysSize = 14;

class CCustomPickup : public CEntity
{
public:
	CCustomPickup(CGameWorld *pGameWorld, int Type, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	int Type() { return m_Type; }
	int GetSpawnTick() { return m_SpawnTick; }

private:
	int m_Type;
	int m_SpawnTick;
	int m_ID2;
};

#endif
