/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

/* created by Pointer*/

/* currently is made for the grenade fountain*/

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>

#include "structure.h"
#include "laser.h"
#include "projectile.h"

CStructure::CStructure(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_STRUCTURE)
{
	m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_LifeSpan = 100;
	m_Owner = Owner;
	// m_Force = Force;
	// m_Damage = Damage;
	// m_SoundImpact = SoundImpact;
	// m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	// m_Explosive = Explosive;
	// m_inTele = false;

	// m_Bounces = 0; // for bouncy grenades

	GameWorld()->InsertEntity(this);
}

void CStructure::Reset()
{
	// GameServer()->m_World.DestroyEntity(this);
	m_LifeSpan = 100;
}

void CStructure::Tick()
{
	// float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	// float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	// vec2 PrevPos = GetPos(Pt);
	// vec2 CurPos = GetPos(Ct);
	// int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	// CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	// CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	m_LifeSpan--;

	// new CLaser(GameWorld(), m_Pos, {0,1}, GameServer()->Tuning()->m_LaserReach, 1, 0);
	if (m_LifeSpan < 0) {
		m_LifeSpan = (float)Server()->TickSpeed() / 1000.0 * (float)g_Config.m_SvGrenadeFountainDelay;

		if (g_Config.m_SvGrenadeFountain) {
			CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
													-1,
													m_Pos,
													m_Direction,
													(int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeLifetime),
													1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);
			CNetObj_Projectile p;
			pProj->FillInfo(&p);

			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(1);
			for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
				Msg.AddInt(((int *)&p)[i]);
			Server()->SendMsg(&Msg, 0, -1);

			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
		}
	}
	
}

void CStructure::TickPaused()
{
	++m_StartTick;
}

void CStructure::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CStructure::Snap(int SnappingClient)
{
	// float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	// if(NetworkClipped(SnappingClient, GetPos(Ct)))
	// 	return;

	// CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	// if(pProj)
	// 	FillInfo(pProj);
}
