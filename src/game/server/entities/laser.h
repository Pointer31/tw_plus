/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_LASER_H
#define GAME_SERVER_ENTITIES_LASER_H

#include <game/server/entity.h>

class CLaser : public CEntity
{
public:
	CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Clockwise);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	float m_Energy;

protected:
	bool HitCharacter(vec2 From, vec2 To);
	void DoBounce();

	int m_FireAckedTick = -1; //rollback (ddnet-insta specific)

private:
	vec2 m_From;
	vec2 m_Dir;
	int m_Bounces;
	int m_EvalTick;
	int m_Owner;
	int m_Clockwise;
};

#endif
