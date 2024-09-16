/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "pickup.h"
#include "projectile.h"

CPickup::CPickup(CGameWorld *pGameWorld, int Type, int SubType, bool remove_on_pickup)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Type = Type;
	m_Subtype = SubType;
	m_ProximityRadius = PickupPhysSize;
	m_ID2 = Server()->SnapNewID();
	m_Remove_on_pickup = remove_on_pickup;

	Reset();
	if (m_Remove_on_pickup)
		m_SpawnTick = -1;

	GameWorld()->InsertEntity(this);
}

void CPickup::Reset()
{
	if (g_pData->m_aPickups[m_Type].m_Spawndelay > 0)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[m_Type].m_Spawndelay;
	else if (m_Type == POWERUP_WEAPON && (m_Subtype == WEAPON_GUN_SUPER || m_Subtype == WEAPON_HAMMER_SUPER))
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[POWERUP_NINJA].m_Spawndelay/2;
	else
		m_SpawnTick = -1;
}

void CPickup::Tick()
{
	// wait for respawn
	if(m_SpawnTick > 0)
	{
		if (Server()->Tick() % 2 == 0 && g_Config.m_SvPickupParticles) {
			CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_HAMMER,
											 -1,
											 {m_Pos.x - WIDTH_TILE/2 + rand() % WIDTH_TILE, m_Pos.y - WIDTH_TILE/2 + rand() % WIDTH_TILE},
											 {0,1},
											 10,
											 0, 0, 0, -1, WEAPON_HAMMER);
		}
		if(Server()->Tick() > m_SpawnTick)
		{
			// respawn
			m_SpawnTick = -1;

			if(m_Type == POWERUP_WEAPON)
				GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SPAWN);
		}
		else
			return;
	}
	// Check if a player intersected us
	CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, 20.0f, 0);
	if(pChr && pChr->IsAlive())
	{
		// player picked us up, is someone was hooking us, let them go
		int RespawnTime = -1;
		int amount = 1; // amount of heart/shields
		switch (m_Type)
		{
			case POWERUP_HEALTH:
				if (m_Subtype == 1)
					amount = 5;
				if(pChr->IncreaseHealth(amount))
				{
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
					RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
					if (m_Subtype == 1)
						RespawnTime = RespawnTime*2;
				}
				break;

			case POWERUP_ARMOR:
				if (m_Subtype == 1)
					amount = 5;
				if(pChr->IncreaseArmor(amount))
				{
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
					RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;
					if (m_Subtype == 1)
						RespawnTime = RespawnTime*2;
				}
				break;

			case POWERUP_WEAPON:
				if(m_Subtype >= 0 && m_Subtype < NUM_WEAPONS+NUM_WEAPONS_EXTRA)
				{
					if(pChr->GiveWeapon(m_Subtype, 10))
					{
						RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;

						if(m_Subtype == WEAPON_GRENADE || m_Subtype == WEAPON_HAMMER_SUPER || m_Subtype == WEAPON_GUN_SUPER)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE);
						else if(m_Subtype == WEAPON_SHOTGUN || m_Subtype == WEAPON_PLASMAGUN || m_Subtype == WEAPON_RIFLE)
							GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);

						int weapon = m_Subtype;
						if(m_Subtype == WEAPON_PLASMAGUN)
							weapon = WEAPON_RIFLE;
						if(m_Subtype == WEAPON_GUN_SUPER)
							weapon = WEAPON_GUN;
						if(m_Subtype == WEAPON_HAMMER_SUPER)
							weapon = WEAPON_HAMMER;
						if (m_Subtype == WEAPON_GUN_SUPER || m_Subtype == WEAPON_HAMMER_SUPER)
							RespawnTime = g_pData->m_aPickups[POWERUP_NINJA].m_Respawntime/2;
						if(pChr->GetPlayer())
							GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), weapon);
					}
				}
				break;

			case POWERUP_NINJA:
				{
					// activate ninja on target player
					pChr->GiveNinja();
					RespawnTime = g_pData->m_aPickups[m_Type].m_Respawntime;

					// loop through all players, setting their emotes
					CCharacter *pC = static_cast<CCharacter *>(GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER));
					for(; pC; pC = (CCharacter *)pC->TypeNext())
					{
						if (pC != pChr)
							pC->SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
					}

					pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
					break;
				}

			default:
				break;
		};

		if(RespawnTime >= 0)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "pickup player='%d:%s' item=%d/%d",
				pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type, m_Subtype);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
			m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime;

			if (m_Remove_on_pickup) {
				GameWorld()->DestroyEntity(this);
			}
		}
	}
}

void CPickup::TickPaused()
{
	if(m_SpawnTick != -1)
		++m_SpawnTick;
}

void CPickup::Snap(int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;

	if ((m_Type == POWERUP_HEALTH || m_Type == POWERUP_ARMOR) && m_Subtype == 1) {
		CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
		if(!pP)
			return;
		CNetObj_Pickup *pP2 = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID2, sizeof(CNetObj_Pickup)));
		if(!pP2)
			return;
		
		float t = Server()->Tick();
		if (GameServer()->m_World.m_Paused)
			t = 0.0;
		pP->m_X = (int)m_Pos.x + 16*sin(t / 25.0);
		pP->m_Y = (int)m_Pos.y + 16*sin(t / 25.0);
		pP->m_Type = m_Type;
		pP->m_Subtype = 0;
		pP2->m_X = (int)m_Pos.x + 16*cos(t / 25.0);
		pP2->m_Y = (int)m_Pos.y + -16*cos(t / 25.0);
		pP2->m_Type = m_Type;
		pP2->m_Subtype = 0;
	} else {
		CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
		if(!pP)
			return;

		pP->m_X = (int)m_Pos.x;
		pP->m_Y = (int)m_Pos.y;
		pP->m_Type = m_Type;
		pP->m_Subtype = m_Subtype;
		if (pP->m_Subtype == WEAPON_PLASMAGUN) {
			pP->m_Subtype = WEAPON_RIFLE;
			CNetObj_Pickup *pP2 = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID2, sizeof(CNetObj_Pickup)));
			if(!pP2)
				return;

			float t = Server()->Tick();
			if (GameServer()->m_World.m_Paused)
				t = 0.0;
			pP2->m_X = (int)m_Pos.x + 16*cos(t / 10.0);
			pP2->m_Y = (int)m_Pos.y;
			pP2->m_Type = POWERUP_ARMOR;
			pP2->m_Subtype = 0;
		}
		else if (pP->m_Subtype == WEAPON_HAMMER_SUPER)
			pP->m_Subtype = WEAPON_HAMMER;
		else if (pP->m_Subtype == WEAPON_GUN_SUPER)
			pP->m_Subtype = WEAPON_GUN;
	}
}
