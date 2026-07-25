/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_STRUCTURE_H
#define GAME_SERVER_ENTITIES_STRUCTURE_H

#include <game/server/entity.h>

class CStructure : public CEntity
{
public:
	CStructure(CGameWorld *pGameWorld, int Type, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	vec2 FigureOutAngle(vec2 Pos);
	int Type() { return m_Type; }
	int GetSpawnTick() { return m_SpawnTick; }

private:
	int m_Type;
	int m_SpawnTick;
	vec2 m_Angle;
	int m_TicksBeforeNextSpawn;
};

#endif
