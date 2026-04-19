/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <generated/server_data.h>
#include <game/server/gamecontroller.h>
#include <game/server/gamecontext.h>

#include <algorithm>

#include "character.h"
#include "laser_better.h"

CLaserBetter::CLaserBetter(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, Pos)
{
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;

	m_Pierces = false;
	m_IgnoreSolids = false;
	m_MaxBounces = GameServer()->Tuning()->m_LaserBounceNum;
	m_TimeToBounce = GameServer()->Tuning()->m_LaserBounceDelay;
	m_RotateAngle = 0.0f;
	m_MaxLength = StartEnergy;
	m_Damage = g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Damage;
	GameWorld()->InsertEntity(this);
}


bool CLaserBetter::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit = GameWorld()->IntersectCharacter(m_Pos, To, 0.f, At, pOwnerChar);
	if(!pHit)
		return false;

	m_From = From;
	m_Pos = At;
	m_Energy = -1;
	pHit->TakeDamage(vec2(0.f, 0.f), normalize(To-From), m_Damage, m_Owner, WEAPON_LASER);
	return !m_Pierces;
}

void CLaserBetter::DoBounce()
{
	m_EvalTick = Server()->Tick();

	if(m_Energy < 0)
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	vec2 To = m_Pos + m_Dir * std::min(m_Energy, (float)m_MaxLength);

	if(!m_IgnoreSolids && GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
	{
		if(!HitCharacter(m_Pos, To))
		{
			// intersected
			m_From = m_Pos;
			m_Pos = To;

			vec2 TempPos = m_Pos;
			vec2 TempDir = m_Dir * 4.0f;

			GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
			m_Pos = TempPos;
			m_Dir = normalize(TempDir);

			m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost;
			m_Bounces++;

			if (GameServer()->m_pController->IsInstagib() && Config()->m_SvLaserJumps && distance(m_From, m_Pos) < 96 && m_Bounces <= 1)
			{
				GameServer()->CreateExplosion(To, m_Owner, 4, 0);
				m_Energy = -1;
			}

			if(m_Bounces > m_MaxBounces)
				m_Energy = -1;

			GameServer()->CreateSound(m_Pos, SOUND_LASER_BOUNCE);
		}
	}
	else
	{
		if(!HitCharacter(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;
			m_Dir = direction(angle(m_Dir) + m_RotateAngle);
			// m_Energy = -1;

			m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost + 0.01;
			m_Bounces++;
		}
	}
}

void CLaserBetter::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CLaserBetter::Tick()
{
	if((Server()->Tick() - m_EvalTick) > (Server()->TickSpeed()*m_TimeToBounce)/1000.0f)
		DoBounce();
}

void CLaserBetter::TickPaused()
{
	++m_EvalTick;
}

void CLaserBetter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient) && NetworkClipped(SnappingClient, m_From))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = round_to_int(m_Pos.x);
	pObj->m_Y = round_to_int(m_Pos.y);
	pObj->m_FromX = round_to_int(m_From.x);
	pObj->m_FromY = round_to_int(m_From.y);
	pObj->m_StartTick = m_EvalTick;
}
