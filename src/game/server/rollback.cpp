#include <base/math.h>

#include <engine/shared/config.h>

#include <game/gamecore.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

#include "rollback.h"
#include <game/server/entities/character.h>

CGameWorld *CRollback::GameWorld()
{
	return &m_pGameServer->m_World;
}

CGameContext *CRollback::GameServer()
{
	return m_pGameServer;
}

IServer *CRollback::Server()
{
	return m_pGameServer->Server();
}

CCollision *CRollback::Collision()
{
	return m_pGameServer->Collision();
}

CTuningParams *CRollback::Tuning()
{
	return m_pGameServer->Tuning();
}

void CRollback::Init(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
}

inline int CRollback::NormalizeTick(int Tick)
{
	if(Tick < 0)
		Tick = Server()->Tick();
	else if(Tick + ROLLBACK_POSITION_HISTORY < Server()->Tick())
		Tick = Server()->Tick() - ROLLBACK_POSITION_HISTORY;

	return Tick % ROLLBACK_POSITION_HISTORY;
}

CCharacter *CRollback::IntersectCharacterOnTick(vec2 Pos0, vec2 Pos1, float Radius, vec2 &NewPos, const CCharacter *pNotThis, const CCharacter *pThisOnly, const CCharacter *pOwnerChar, int Tick)
{
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CCharacter *pClosest = nullptr;
	int LocalTick = NormalizeTick(Tick);

	CCharacter *pChar = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER);
	for(; pChar; pChar = (CCharacter *)pChar->TypeNext())
	{
		if(pChar == pNotThis)
			continue;

		if(pThisOnly && pChar != pThisOnly)
			continue;

		vec2 Charpos;
		if(pChar != pOwnerChar && pChar->m_Positions[LocalTick].m_Valid) // treat owner/invalid position as normal
			Charpos = pChar->m_Positions[LocalTick].m_Position;
		else
			Charpos = pChar->m_Pos;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, Charpos);
		
		float Len = distance(Charpos, IntersectPos);
		if(Len < pChar->m_ProximityRadius + Radius)
		{
			Len = distance(Pos0, IntersectPos);
			if(Len < ClosestLen)
			{
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = pChar;
			}
		}
		
	}

	return pClosest;
}

int CRollback::FindCharactersOnTick(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Tick)
{
	int LocalTick = NormalizeTick(Tick);

	int Num = 0;
	for(CCharacter *pChr = (CCharacter *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pChr; pChr = (CCharacter *)pChr->TypeNext())
	{
		if(pChr->m_Positions[LocalTick].m_Valid ? (distance(pChr->m_Positions[LocalTick].m_Position, Pos) < Radius + pChr->m_ProximityRadius) : (distance(pChr->m_Pos, Pos) < Radius + pChr->m_ProximityRadius))
		{
			if(ppEnts)
				ppEnts[Num] = pChr;
			Num++;
			if(Num == Max)
				break;
		}
	}

	return Num;
}

void CRollback::CreateExplosionOnTick(vec2 Pos, int Owner, int Weapon, bool NoDamage, int Tick)
{
	int LocalTick = NormalizeTick(Tick);

	// create the event
	CNetEvent_Explosion *pEvent = (CNetEvent_Explosion *)GameServer()->m_Events.Create(NETEVENTTYPE_EXPLOSION, sizeof(CNetEvent_Explosion));
	if(pEvent)
	{
		pEvent->m_X = (int)Pos.x;
		pEvent->m_Y = (int)Pos.y;
	}

	if (!NoDamage)
	{
		// deal damage
		CCharacter *apEnts[MAX_CLIENTS];
		float Radius = 135.0f;
		float InnerRadius = 48.0f;
		int Num = GameWorld()->FindEntities(Pos, Radius, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
		for(int i = 0; i < Num; i++)
		{
			vec2 Diff;
			if(((CCharacter *)apEnts[i])->m_Positions[LocalTick].m_Valid && ((CCharacter *)apEnts[i])->GetPlayer() && ((CCharacter *)apEnts[i])->GetPlayer()->GetCID() != Owner) // treat owner/invalid position as normal
				Diff = apEnts[i]->m_Positions[LocalTick].m_Position - Pos;
			else
				Diff = apEnts[i]->m_Pos - Pos;
			vec2 ForceDir(0,1);
			float l = length(Diff);
			if(l)
				ForceDir = normalize(Diff);
			l = 1-clamp((l-InnerRadius)/(Radius-InnerRadius), 0.0f, 1.0f);
			float Dmg = 6 * l;
			if((int)Dmg) {
				vec2 force = ForceDir*l*Tuning()->m_ExplosionStrength*2;
				if (Owner < 0 && !g_Config.m_SvGrenadeFountainHarm) // if the source is a grenade fountain, maybe do not do harm
					Dmg = 0;
				apEnts[i]->TakeDamage(force, (int)Dmg, Owner, Weapon);
			}
		}
	}
}
