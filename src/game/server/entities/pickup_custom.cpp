/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <generated/server_data.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "character.h"
#include "pickup_custom.h"
#include "projectile.h"

CPickupCustom::CPickupCustom(CGameWorld *pGameWorld, int Type, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, Pos, 14)
{
	m_Type = Type;

	Reset();

	GameWorld()->InsertEntity(this);
}

void CPickupCustom::Reset()
{
	if (g_pData->m_aPickups[m_Type].m_Spawndelay > 0)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Spawndelay;
	else
		m_SpawnTick = -1;
}

void CPickupCustom::Tick()
{
	// wait for respawn
	if(m_SpawnTick > 0)
	{
		if (Server()->Tick() % 4 == 0 && Config()->m_SvPickupParticles) {
			CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_HAMMER,
											 -1,
											 {m_Pos.x - 32/2 + rand() % 32, m_Pos.y - 32/2 + rand() % 32},
											 {0,1},
											 10,
											 0, 0, 0, 0, WEAPON_HAMMER);
		}
		if(Server()->Tick() > m_SpawnTick)
		{
			// respawn
			m_SpawnTick = -1;
		}
		else
			return;
	}
	// Check if a player intersected us
	CCharacter *pChr = (CCharacter *)GameWorld()->ClosestEntity(m_Pos, 20.0f, CGameWorld::ENTTYPE_CHARACTER, 0);
	if(pChr && pChr->IsAlive())
	{
		// player picked us up, is someone was hooking us, let them go
		bool Picked = false;
		switch (m_Type)
		{
			case 0:
			{
				Picked = true;
				pChr->GetPlayer()->m_Score++;
				break;
			}

			default:
				break;
		};

		if(Picked)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "pickup (custom) player='%d:%s' item=%d",
				pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
			int RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
			if(RespawnTime >= 0)
				m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime;
		}
	}
}

void CPickupCustom::TickPaused()
{
	if(m_SpawnTick != -1)
		++m_SpawnTick;
}

void CPickupCustom::Snap(int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;

	CNetObj_PickupCustom *pP = static_cast<CNetObj_PickupCustom *>(Server()->SnapNewItem(NETOBJTYPE_PICKUPCUSTOM, GetID(), sizeof(CNetObj_PickupCustom)));
	if(!pP)
		return;

	pP->m_X = round_to_int(m_Pos.x);
	pP->m_Y = round_to_int(m_Pos.y);
	pP->m_ResourceId = GetID() % 3;
	// CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	// if(!pP)
	// 	return;

	// pP->m_X = round_to_int(m_Pos.x);
	// pP->m_Y = round_to_int(m_Pos.y);
	// pP->m_Type = PICKUP_HEALTH;
}
