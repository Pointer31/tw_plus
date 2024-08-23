/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "ndm.h"
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

CGameControllerNDM::CGameControllerNDM(class CGameContext *pGameServer, int TypeFlags)
: IGameController(pGameServer)
{
	m_Flags = TypeFlags | GAMETYPE_NOPICKUPS;
	m_pGameType = "nDM+";
	m_CurrentWeapon = WEAPON_GUN;
	m_LastSwitchTick = Server()->Tick();
}

void CGameControllerNDM::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->GiveWeapon(m_CurrentWeapon, 10);
	pChr->SetWeapon(m_CurrentWeapon);
	if (m_CurrentWeapon == WEAPON_NINJA)
		pChr->GiveNinja();
	if (m_CurrentWeapon == WEAPON_GUN || m_CurrentWeapon == WEAPON_HAMMER || m_CurrentWeapon == WEAPON_NINJA)
		pChr->SetAmmo(m_CurrentWeapon, 9999);
	else
		pChr->SetAmmo(m_CurrentWeapon, 15);
	
	pChr->SetHealth(10);
	pChr->IncreaseArmor(5);
}

void CGameControllerNDM::Tick()
{
	if (Server()->Tick() > m_LastSwitchTick) {
		const int time_between_changes = g_Config.m_SvNDMTime; // in seconds
		m_LastSwitchTick = Server()->Tick()
							+ (float)Server()->TickSpeed() * (float)time_between_changes;

		int old_weapon = m_CurrentWeapon;
		int new_weapon = rand() % (NUM_WEAPONS-1 + NUM_WEAPONS_EXTRA);
		if (new_weapon >= m_CurrentWeapon)
			m_CurrentWeapon = new_weapon+1;
		else
			m_CurrentWeapon = new_weapon;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_IsReady) {
				GameServer()->SendBroadcast("New weapon has been chosen", i);
				if (GameServer()->IsClientPlayer(i) && GameServer()->m_apPlayers[i]->GetCharacter()) {
					CCharacter * pChr = GameServer()->m_apPlayers[i]->GetCharacter();
					pChr->GiveWeapon(m_CurrentWeapon, 10);
					pChr->SetWeapon(m_CurrentWeapon);
					if (m_CurrentWeapon == WEAPON_NINJA)
						pChr->GiveNinja();
					if (m_CurrentWeapon == WEAPON_GUN || m_CurrentWeapon == WEAPON_HAMMER || m_CurrentWeapon == WEAPON_NINJA)
						pChr->SetAmmo(m_CurrentWeapon, 9999);
					else
						pChr->SetAmmo(m_CurrentWeapon, 15);

					if (!((m_CurrentWeapon == WEAPON_RIFLE && old_weapon == WEAPON_PLASMAGUN) || (old_weapon == WEAPON_RIFLE && m_CurrentWeapon == WEAPON_PLASMAGUN) ||
							(m_CurrentWeapon == WEAPON_HAMMER && old_weapon == WEAPON_HAMMER_SUPER) || (old_weapon == WEAPON_HAMMER && m_CurrentWeapon == WEAPON_HAMMER_SUPER) ||
							(m_CurrentWeapon == WEAPON_GUN && old_weapon == WEAPON_GUN_SUPER) || (old_weapon == WEAPON_GUN && m_CurrentWeapon == WEAPON_GUN_SUPER)))
						pChr->TakeWeapon(old_weapon);
				}
			}
		}
	}
	IGameController::Tick();
}
