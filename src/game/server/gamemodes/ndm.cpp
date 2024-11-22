/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "ndm.h"
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <iostream>
#include <game/server/gamecontext.h>

CGameControllerNDM::CGameControllerNDM(class CGameContext *pGameServer, int TypeFlags)
: IGameController(pGameServer)
{
	m_Flags = TypeFlags | GAMETYPE_NOPICKUPS;
	m_pGameType = "nDM+";
#if defined(CONF_FAMILY_WINDOWS)
	
#else
	srand (time(NULL));
#endif
	m_CurrentWeapon = WEAPON_GUN_SUPER;
	m_NextWeapon = WEAPON_SHOTGUN;
	m_NextSwitchTick = Server()->Tick();
}

void CGameControllerNDM::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->GiveWeapon(m_CurrentWeapon, 10);
	pChr->SetWeapon(m_CurrentWeapon);
	if (m_CurrentWeapon == WEAPON_NINJA)
		pChr->GiveNinja();
	if (m_CurrentWeapon == WEAPON_GUN_SUPER)
		pChr->SetAmmo(m_CurrentWeapon, 10);
	else
		pChr->SetAmmo(m_CurrentWeapon, 9999);
	
	pChr->SetHealth(10);
	pChr->IncreaseArmor(5);
}

void CGameControllerNDM::Tick()
{
	IGameController::Tick();

	if(GameServer()->m_World.m_ResetRequested || GameServer()->m_World.m_Paused)
		return;
		
	if (Server()->Tick() == m_NextSwitchTick - Server()->TickSpeed() || Server()->Tick() == m_NextSwitchTick - Server()->TickSpeed()*2
		|| Server()->Tick() == m_NextSwitchTick - Server()->TickSpeed()*3 || Server()->Tick() == m_NextSwitchTick - Server()->TickSpeed()*4) {
		char aBuf[1024] = "No message (this should not appear)";
		if (m_NextWeapon == WEAPON_HAMMER)
			str_format(aBuf, sizeof(aBuf), "Next weapon in %d\nHammer", (m_NextSwitchTick - Server()->Tick())/Server()->TickSpeed());
		else if (m_NextWeapon == WEAPON_GUN)
			str_format(aBuf, sizeof(aBuf), "Next weapon in %d\nPistol", (m_NextSwitchTick - Server()->Tick())/Server()->TickSpeed());
		else if (m_NextWeapon == WEAPON_SHOTGUN)
			str_format(aBuf, sizeof(aBuf), "Next weapon in %d\nShotgun", (m_NextSwitchTick - Server()->Tick())/Server()->TickSpeed());
		else if (m_NextWeapon == WEAPON_GRENADE)
			str_format(aBuf, sizeof(aBuf), "Next weapon in %d\nGrenade Launcher", (m_NextSwitchTick - Server()->Tick())/Server()->TickSpeed());
		else if (m_NextWeapon == WEAPON_RIFLE)
			str_format(aBuf, sizeof(aBuf), "Next weapon in %d\nLaser Rifle", (m_NextSwitchTick - Server()->Tick())/Server()->TickSpeed());
		else if (m_NextWeapon == WEAPON_NINJA)
			str_format(aBuf, sizeof(aBuf), "Next weapon in %d\nNinja", (m_NextSwitchTick - Server()->Tick())/Server()->TickSpeed());
		else if (m_NextWeapon == WEAPON_PLASMAGUN)
			str_format(aBuf, sizeof(aBuf), "Next weapon in %d\nPlasma Gun", (m_NextSwitchTick - Server()->Tick())/Server()->TickSpeed());
		else if (m_NextWeapon == WEAPON_HAMMER_SUPER)
			str_format(aBuf, sizeof(aBuf), "Next weapon in %d\nCharge Hammer", (m_NextSwitchTick - Server()->Tick())/Server()->TickSpeed());
		else if (m_NextWeapon == WEAPON_GUN_SUPER)
			str_format(aBuf, sizeof(aBuf), "Next weapon in %d\nSuper Pistol", (m_NextSwitchTick - Server()->Tick())/Server()->TickSpeed());
			
		// str_format(aBuf, sizeof(aBuf), "Next weapon in %d\n %d", (m_NextSwitchTick - Server()->Tick())/Server()->TickSpeed(), m_NextWeapon);
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_IsReady)
				GameServer()->SendBroadcast(aBuf, i);
		}
	} else if (Server()->Tick() > m_NextSwitchTick) {
		const int time_between_changes = g_Config.m_SvNDMTime; // in seconds
		m_NextSwitchTick = Server()->Tick()
							+ Server()->TickSpeed() * time_between_changes;

		int old_weapon = m_CurrentWeapon;
		m_CurrentWeapon = m_NextWeapon;
		m_NextWeapon = rand() % (NUM_WEAPONS-1 + NUM_WEAPONS_EXTRA);
		if (m_NextWeapon >= m_CurrentWeapon)
			m_NextWeapon = m_NextWeapon+1;
		else
			m_NextWeapon = m_NextWeapon;
		int new_weapon = m_CurrentWeapon;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_IsReady) {
				GameServer()->SendBroadcast("", i);
				if (GameServer()->IsClientPlayer(i) && GameServer()->m_apPlayers[i]->GetCharacter()) {
					CCharacter * pChr = GameServer()->m_apPlayers[i]->GetCharacter();
					pChr->GiveWeapon(m_CurrentWeapon, 10);
					pChr->SetWeapon(m_CurrentWeapon);
					if (m_CurrentWeapon == WEAPON_NINJA)
						pChr->GiveNinja();
					if (m_CurrentWeapon == WEAPON_GUN_SUPER)
						pChr->SetAmmo(m_CurrentWeapon, 10);
					else
						pChr->SetAmmo(m_CurrentWeapon, 9999);

					if (!((m_CurrentWeapon == WEAPON_RIFLE && old_weapon == WEAPON_PLASMAGUN) || (old_weapon == WEAPON_RIFLE && m_CurrentWeapon == WEAPON_PLASMAGUN) ||
							(m_CurrentWeapon == WEAPON_HAMMER && old_weapon == WEAPON_HAMMER_SUPER) || (old_weapon == WEAPON_HAMMER && m_CurrentWeapon == WEAPON_HAMMER_SUPER) ||
							(m_CurrentWeapon == WEAPON_GUN && old_weapon == WEAPON_GUN_SUPER) || (old_weapon == WEAPON_GUN && m_CurrentWeapon == WEAPON_GUN_SUPER)))
						pChr->TakeWeapon(old_weapon);
				}
			}
		}
	}
}
