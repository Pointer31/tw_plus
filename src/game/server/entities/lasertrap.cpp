/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include <game/server/player.h>

#include "lasertrap.h"
#include "laser.h"

CLaserTrap::CLaserTrap(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASERTRAP, Pos)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_OwnerTeam = Owner >= 0 ? GameServer()->m_apPlayers[Owner]->GetTeam() : -1; //PLAYER_TEAM_RED
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = Server()->Tick();
	GameWorld()->InsertEntity(this);

	vec2 To = m_Pos + m_Dir * m_Energy;
	GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To);
	m_From = m_Pos;
	m_Pos = To;
}

bool CLaserTrap::HitCharacter(vec2 From, vec2 To)
{
	// vec2 At;
	// CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	// CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, pOwnerChar);
	// if(!pHit)
	// 	return false;

	// m_From = From;
	// m_Pos = At;
	// m_Energy = -1;
	// pHit->TakeDamage(vec2(0.f, 0.f), GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE);
	return true;
}

void CLaserTrap::DoBounce()
{
	CLaser* laser = new CLaser(GameWorld(), m_From, m_Dir, m_Energy, m_Owner);
	laser->m_Energy = -1;
	GameServer()->m_World.DestroyEntity(this);
	return;
}

void CLaserTrap::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLaserTrap::LoseOwner()
{
	if(m_OwnerTeam == TEAM_BLUE)
		m_Owner = -2; // PLAYER_TEAM_BLUE
	else
		m_Owner = -1; // PLAYER_TEAM_RED
}

void CLaserTrap::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay*5)/1000.0f)
		DoBounce();
}

void CLaserTrap::TickPaused()
{
	++m_EvalTick;
}

void CLaserTrap::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = Server()->Tick() - Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay/1000.0f*0.6f;
}
