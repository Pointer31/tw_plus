/* gCTF - CTF with grenade - 1Shot1Kill
 * Made by Teetime
 */
#include "grenade.h"
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

// gCTF

CGameControllerGCTF::CGameControllerGCTF(class CGameContext *pGameServer, int TypeFlags)
: CGameControllerCTF(pGameServer, TypeFlags)
{
	m_Flags = TypeFlags;
	m_pGameType = "gCTF+";
}

void CGameControllerGCTF::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GRENADE, g_Config.m_SvGrenadeAmmo);
}

void CGameControllerGCTF::Tick()
{
	CGameControllerCTF::Tick();
}


// gDM

CGameControllerGDM::CGameControllerGDM(class CGameContext *pGameServer, int TypeFlags)
: CGameControllerDM(pGameServer, TypeFlags)
{
	m_Flags = TypeFlags;
	m_pGameType = "gDM+";
}

void CGameControllerGDM::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	if (g_Config.m_SvInstagibFiniteAmmo) {
		pChr->GiveWeapon(WEAPON_GRENADE, 10);
	} else {
		pChr->GiveWeapon(WEAPON_GRENADE, g_Config.m_SvGrenadeAmmo);
	}
}

void CGameControllerGDM::Tick()
{
	CGameControllerDM::Tick();
}


// gTDM

CGameControllerGTDM::CGameControllerGTDM(class CGameContext *pGameServer, int TypeFlags)
: CGameControllerTDM(pGameServer, TypeFlags)
{
	m_Flags = TypeFlags;
	m_pGameType = "gTDM+";
}

void CGameControllerGTDM::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GRENADE, g_Config.m_SvGrenadeAmmo);
}

void CGameControllerGTDM::Tick()
{
	CGameControllerTDM::Tick();
}

// gHTF

CGameControllerGHTF::CGameControllerGHTF(class CGameContext *pGameServer, int TypeFlags)
: CGameControllerHTF(pGameServer, TypeFlags)
{
	m_Flags = TypeFlags;
	m_pGameType = "gHTF";
	if (TypeFlags&IGameController::GAMETYPE_ISTEAM)
		m_pGameType = "gTHTF";
	else
		m_pGameType = "gHTF";
}

void CGameControllerGHTF::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GRENADE, g_Config.m_SvGrenadeAmmo);
}

void CGameControllerGHTF::Tick()
{
	CGameControllerHTF::Tick();
}


// gLMS

CGameControllerGLMS::CGameControllerGLMS(class CGameContext *pGameServer, int TypeFlags)
: CGameControllerLMS(pGameServer, TypeFlags)
{
	m_Flags = TypeFlags;
	if (TypeFlags&IGameController::GAMETYPE_ISTEAM)
		m_pGameType = "gLTS+";
	else
		m_pGameType = "gLMS+";
}

void CGameControllerGLMS::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GRENADE, g_Config.m_SvGrenadeAmmo);
}

void CGameControllerGLMS::Tick()
{
	CGameControllerLMS::Tick();
}