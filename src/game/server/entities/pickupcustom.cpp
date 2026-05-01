/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <generated/server_data.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "character.h"
#include "pickupcustom.h"
#include "projectile.h"

CCustomPickup::CCustomPickup(CGameWorld *pGameWorld, int Type, vec2 Pos, int SubType)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, Pos, PickupCustomPhysSize)
{
	m_Type = Type;
	m_SubType = SubType;
	m_ID2 = Server()->SnapNewID();

	Reset();

	GameWorld()->InsertEntity(this);
}

void CCustomPickup::Reset()
{
	if (m_Type == 2)
		m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * g_pData->m_aPickups[PICKUP_NINJA].m_Spawndelay;
	else
		m_SpawnTick = -1;
}

void CCustomPickup::Tick()
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

			if(m_Type == PICKUP_GRENADE || m_Type == PICKUP_SHOTGUN || m_Type == PICKUP_LASER)
				GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SPAWN);
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
		int RespawnTime = g_pData->m_aPickups[PICKUP_HEALTH].m_Respawntime * 2;
		switch (m_Type)
		{
			case 0:
				if(pChr->IncreaseArmor(5))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
				}
				break;

			case 1:
				if(pChr->IncreaseHealth(5))
				{
					Picked = true;
					GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
				}
				break;

			case 2:
				{
					Picked = true;
					RespawnTime = g_pData->m_aPickups[PICKUP_NINJA].m_Respawntime;

					// give one of the subtypes
					if (m_SubType == 0)
						pChr->GiveNinja();
					else if (m_SubType == 1)
					{
						pChr->GivePowerupShields();
						GameServer()->SendBroadcast("You gained shields for 15s!", pChr->GetPlayer()->GetCID());
					}
					else if (m_SubType == 2)
					{
						pChr->GivePowerupStrength();
						GameServer()->SendBroadcast("You gained strength for 15s!", pChr->GetPlayer()->GetCID());
					}

					m_SubType = rand() % 3;

					// loop through all players, setting their emotes
					CCharacter *pC = static_cast<CCharacter *>(GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER));
					for(; pC; pC = (CCharacter *)pC->TypeNext())
					{
						if (pC != pChr)
							pC->SetEmote(EMOTE_SURPRISE, Server()->Tick() + Server()->TickSpeed());
					}

					pChr->SetEmote(EMOTE_ANGRY, Server()->Tick() + 1200 * Server()->TickSpeed() / 1000);
				}
				break;

			default:
				break;
		};

		if(Picked)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "(c)pickup player='%d:%s' item=%d:%d",
				pChr->GetPlayer()->GetCID(), Server()->ClientName(pChr->GetPlayer()->GetCID()), m_Type, m_SubType);
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
			if(RespawnTime >= 0)
				m_SpawnTick = Server()->Tick() + Server()->TickSpeed() * RespawnTime;
		}
	}
}

void CCustomPickup::TickPaused()
{
	if(m_SpawnTick != -1)
		++m_SpawnTick;
}

void CCustomPickup::Snap(int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	if(!pP)
		return;
	
	float t = Server()->Tick();
	if (GameServer()->m_World.m_Paused)
		t = 0.0f;

	switch (m_Type)
	{
		case 0:
		case 1:
		{
			CNetObj_Pickup *pP2 = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID2, sizeof(CNetObj_Pickup)));
			if(!pP2)
				return;

			pP->m_X = (int)m_Pos.x + 16*sin(t / 25.0);
			pP->m_Y = (int)m_Pos.y + 16*sin(t / 25.0);
			pP->m_Type = m_Type==0 ? PICKUP_ARMOR : PICKUP_HEALTH;
			pP2->m_X = (int)m_Pos.x + 16*cos(t / 25.0);
			pP2->m_Y = (int)m_Pos.y + -16*cos(t / 25.0);
			pP2->m_Type = m_Type==0 ? PICKUP_ARMOR : PICKUP_HEALTH;
		}
		break;
		case 2:
		{
			t = t*1.5f;
			pP->m_X = m_Pos.x;
			pP->m_Y = m_Pos.y;
			pP->m_Type = PICKUP_NINJA;
			if (m_SubType == 1)
			{
				CNetObj_Pickup *pP2 = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID2, sizeof(CNetObj_Pickup)));
				if(!pP2)
					return;

				pP2->m_X = (int)m_Pos.x + 32*sin(t / 25.0);
				pP2->m_Y = (int)m_Pos.y + -32*cos(t / 25.0);
				pP2->m_Type = PICKUP_ARMOR;
			} 
			else if (m_SubType == 2)
			{

				CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetID(), sizeof(CNetObj_Projectile)));
				if(!pProj)
					return;

				pProj->m_X = (int)m_Pos.x + 32*sin(t / 25.0);
				pProj->m_Y = (int)m_Pos.y + -32*cos(t / 25.0);
				pProj->m_VelX = 0;
				pProj->m_VelY = 0;
				pProj->m_StartTick = Server()->Tick();
				pProj->m_Type = 4 /*WEAPON_LASER*/;
			}
		}
		break;
	
	default:
		break;
	}
}
