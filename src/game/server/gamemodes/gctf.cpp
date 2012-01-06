/* gCTF - CTF with grenade - 1Shot1Kill
 * Made by Teetime
 */
#include "gctf.h"
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

CGameControllerGCTF::CGameControllerGCTF(class CGameContext *pGameServer, int TypeFlags)
: CGameControllerCTF(pGameServer, TypeFlags)
{
	m_Flags = TypeFlags;
	m_pGameType = "gCTF";
}

void CGameControllerGCTF::OnCharacterSpawn(CCharacter* pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GRENADE, -1);
}

void CGameControllerGCTF::Tick()
{
	CGameControllerCTF::Tick();
}
