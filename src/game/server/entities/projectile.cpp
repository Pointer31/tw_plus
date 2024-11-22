/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "projectile.h"

CProjectile::CProjectile(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir, int Span,
		int Damage, bool Explosive, float Force, int SoundImpact, int Weapon)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Type = Type;
	m_Pos = Pos;
	m_Direction = Dir;
	m_LifeSpan = Span;
	m_Owner = Owner;
	m_Force = Force;
	m_Damage = Damage;
	m_SoundImpact = SoundImpact;
	m_Weapon = Weapon;
	m_StartTick = Server()->Tick();
	m_Explosive = Explosive;
	m_inTele = false;

	m_Bounces = 0; // for bouncy grenades

	GameWorld()->InsertEntity(this);
}

void CProjectile::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

vec2 CProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = GameServer()->Tuning()->m_GrenadeCurvature;
			Speed = GameServer()->Tuning()->m_GrenadeSpeed;
			break;

		case WEAPON_SHOTGUN:
			Curvature = GameServer()->Tuning()->m_ShotgunCurvature;
			Speed = GameServer()->Tuning()->m_ShotgunSpeed;
			break;

		case WEAPON_GUN:
			Curvature = GameServer()->Tuning()->m_GunCurvature;
			Speed = GameServer()->Tuning()->m_GunSpeed;
			break;
	}

	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}


void CProjectile::Tick()
{
	float Pt = (Server()->Tick()-m_StartTick-1)/(float)Server()->TickSpeed();
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();
	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	int Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &CurPos, 0);
	CCharacter *OwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *TargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, CurPos, 6.0f, CurPos, OwnerChar);

	m_LifeSpan--;

	if(TargetChr || Collide || m_LifeSpan < 0 || GameLayerClipped(CurPos))
	{
		if (Collide && m_Weapon == WEAPON_GRENADE && g_Config.m_SvBouncyGrenade) {

			m_Pos = PrevPos;
			m_Direction = m_Direction * -1;
			m_StartTick = Server()->Tick();
			m_LifeSpan = m_LifeSpan;

			GameServer()->CreateSound(CurPos, SOUND_GRENADE_FIRE);
			m_Force = 1;
			// CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
			// 	m_Owner,
			// 	PrevPos,
			// 	m_Direction * -1,
			// 	m_LifeSpan,//(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
			// 	1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);
			if (m_LifeSpan < 0) {
				if(m_Explosive)
					GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false);

				GameServer()->m_World.DestroyEntity(this);
			}
		} else {
			if(m_LifeSpan >= 0 || m_Weapon == WEAPON_GRENADE)
				GameServer()->CreateSound(CurPos, m_SoundImpact);

			if(m_Explosive) 
				GameServer()->CreateExplosion(CurPos, m_Owner, m_Weapon, false);

			else if(TargetChr)
				TargetChr->TakeDamage(m_Direction * max(0.001f, m_Force), m_Damage, m_Owner, m_Weapon);

			GameServer()->m_World.DestroyEntity(this);
		}
	} else if (g_Config.m_SvProjectileTeleport) {
		if (GameServer()->Collision()->GetCollisionAt(CurPos.x, CurPos.y)&CCollision::COLFLAG_TELEONE) {
			if (!m_inTele) {
				m_inTele = true;
				int x = GameServer()->Collision()->getTeleX(0);
				int y = GameServer()->Collision()->getTeleY(0);
				int tx = GameServer()->Collision()->getTeleX(1);
				int ty = GameServer()->Collision()->getTeleY(1);
				vec2 start = {(float)x, (float)y};
				vec2 end = {(float)tx, (float)ty};
				m_Pos = m_Pos - start * 32 + end * 32;
			}
		} else if (GameServer()->Collision()->GetCollisionAt(CurPos.x, CurPos.y)&CCollision::COLFLAG_TELETWO) {
			if (!m_inTele) {
				m_inTele = true;
				int x = GameServer()->Collision()->getTeleX(1);
				int y = GameServer()->Collision()->getTeleY(1);
				int tx = GameServer()->Collision()->getTeleX(0);
				int ty = GameServer()->Collision()->getTeleY(0);
				vec2 start = {(float)x, (float)y};
				vec2 end = {(float)tx, (float)ty};
				m_Pos = m_Pos - start * 32 + end * 32;
			}
		} else if (GameServer()->Collision()->GetCollisionAt(CurPos.x, CurPos.y)&CCollision::COLFLAG_TELETHREE) {
			if (!m_inTele) {
				m_inTele = true;
				int x = GameServer()->Collision()->getTeleX(2);
				int y = GameServer()->Collision()->getTeleY(2);
				int tx = GameServer()->Collision()->getTeleX(3);
				int ty = GameServer()->Collision()->getTeleY(3);
				vec2 start = {(float)x, (float)y};
				vec2 end = {(float)tx, (float)ty};
				m_Pos = m_Pos - start * 32 + end * 32;
			}
		} else if (GameServer()->Collision()->GetCollisionAt(CurPos.x, CurPos.y)&CCollision::COLFLAG_TELEFOUR) {
			if (!m_inTele) {
				m_inTele = true;
				int x = GameServer()->Collision()->getTeleX(3);
				int y = GameServer()->Collision()->getTeleY(3);
				int tx = GameServer()->Collision()->getTeleX(2);
				int ty = GameServer()->Collision()->getTeleY(2);
				vec2 start = {(float)x, (float)y};
				vec2 end = {(float)tx, (float)ty};
				m_Pos = m_Pos - start * 32 + end * 32;
			}
		} else {
			m_inTele = false;
		}
	}
}

void CProjectile::TickPaused()
{
	++m_StartTick;
}

void CProjectile::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_Pos.x;
	pProj->m_Y = (int)m_Pos.y;
	pProj->m_VelX = (int)(m_Direction.x*100.0f);
	pProj->m_VelY = (int)(m_Direction.y*100.0f);
	pProj->m_StartTick = m_StartTick;
	pProj->m_Type = m_Type;
}

void CProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick()-m_StartTick)/(float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);
}
