/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PICKUP_H
#define GAME_SERVER_ENTITIES_PICKUP_H

#include <game/server/entity.h>

const int PickupPhysSize = 14;

class CPickup : public CEntity
{
public:
	CPickup(CGameWorld *pGameWorld, int Type, int SubType = 0, bool remove_on_pickup = false);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	int Type() {return m_Type;}
	int SubType() {return m_Subtype;}
	int GetSpawnTick() {return m_SpawnTick;}

private:
	int m_Type;
	int m_Subtype;
	int m_SpawnTick;
	int m_ID2;
	bool m_Remove_on_pickup;
};

#endif
