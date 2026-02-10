/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_LASER_BETTER_H
#define GAME_SERVER_ENTITIES_LASER_BETTER_H

#include <game/server/entity.h>

class CLaserBetter : public CEntity
{
public:
	CLaserBetter(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);\
	void DoBounce();

	bool m_Pierces; // pierce through tees?
	bool m_IgnoreSolids; // ignore solid tiles?
	int m_MaxBounces;
	int m_TimeToBounce; // in ms
	int m_MaxLength; // max length before forcing a bounce
	float m_RotateAngle; // after a forced bounce mid-air, how to rotate if any?
	int m_Damage;

protected:
	bool HitCharacter(vec2 From, vec2 To);

private:
	vec2 m_From;
	vec2 m_Dir;
	float m_Energy;
	int m_Bounces;
	int m_EvalTick;
	int m_Owner;
};

#endif
