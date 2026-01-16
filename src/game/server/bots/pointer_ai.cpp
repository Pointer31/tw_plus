#include "pointer_ai.h"
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/entities/flag.h>
#include <game/server/player.h>
#include <game/server/entities/character.h>
#include <engine/shared/config.h>

CPointerBotAI::CPointerBotAI(CGameContext *pContext, CPlayer *pPlayer, int Difficulty) :
CBotAI(pContext, pPlayer)
{
    m_isBot = clamp(Difficulty, 1, 6);
    str_format(m_Clan, sizeof(m_Clan), "bot%i", m_isBot);

    str_copy(pPlayer->m_TeeInfos.m_aaSkinPartNames[0], rand() % 2 == 0 ? "kitty" : "dog", MAX_SKIN_LENGTH);
    for (int i = 0; i < NUM_SKINPARTS; i++)
    {   
        pPlayer->m_TeeInfos.m_aUseCustomColors[i] = true;
        pPlayer->m_TeeInfos.m_aSkinPartColors[i] = 917601 + (rand() % 150);
    }
}

void CPointerBotAI::HandleInput(CNetObj_PlayerInput &Input)
{
    CFlag *pEnemyFlag = nullptr; //+KZ
	CFlag *pTeamFlag = nullptr; //+KZ

	//+KZ
	{ 
		CFlag *p = (CFlag *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_FLAG);
		for(; p; p = (CFlag *)p->TypeNext())
		{
			
			if(p->GetTeam() == GetPlayer()->GetTeam())
			{
				pTeamFlag = p;
				continue;
			}
			
			pEnemyFlag = p;
		}
	}

    if (m_isBot >= 2)
    {
        // >=2 means the AI will shoot and maybe move, instead of being a dummy
        // if there are no clients connected, the bots will idle
        if (GameServer()->m_pController->GetRealPlayerNum() <= 0)
        {
            Input.m_Direction = 0;
            Input.m_TargetX = 0; // look randomly
            Input.m_TargetY = 0;
            Input.m_Jump = false;
            Input.m_Fire = false;
            Input.m_Hook = false;
            //Input.m_PlayerFlags = 0;
            Input.m_WantedWeapon = WEAPON_GUN + 1;
            Input.m_NextWeapon = WEAPON_GUN + 1;
            Input.m_PrevWeapon = WEAPON_GUN + 1;
        }
        else
        {
            Input.m_Direction = 0;
            Input.m_TargetX = (rand() % 128) - 64; // look randomly
            Input.m_TargetY = (rand() % 128) - 64;
            Input.m_Jump = false;
            Input.m_Fire = true;
            if (!GameServer()->m_pController->IsInstagib()) // make non-automatic weapons work, but still have ammo reload work in instagib
                Input.m_Fire = GameServer()->Server()->Tick() % (2) == 1;
            Input.m_Hook = false;
            //Input.m_PlayerFlags = PLAYERFLAG_PLAYING;
            Input.m_WantedWeapon = WEAPON_GUN + 1;
            if (GameServer()->m_pController->IsInstagibLaser())
                Input.m_WantedWeapon = WEAPON_LASER + 1;
            if (GameServer()->m_pController->IsInstagibGrenade())
                Input.m_WantedWeapon = WEAPON_GRENADE + 1;
            Input.m_NextWeapon = WEAPON_GUN + 1;
            Input.m_PrevWeapon = WEAPON_GUN + 1;

            CCharacter* pOwnChar = GetPlayer()->GetCharacter();

            if (m_isBot >= 3 && pOwnChar)
            { // move, and occasionally jump
                if (rand() % (SERVER_TICK_SPEED * 2) == 1)
                    m_botDirection = -m_botDirection;
                Input.m_Direction = m_botDirection;
                if (rand() % (SERVER_TICK_SPEED * 2) == 1)
                    Input.m_Jump = true;
                if (str_comp_nocase(GameServer()->m_pController->GetGameType(), "CTF+") == 0 || str_comp_nocase(GameServer()->m_pController->GetGameType(), "iCTF+") == 0 || str_comp_nocase(GameServer()->m_pController->GetGameType(), "gCTF+") == 0)
                {
                    int team = GetPlayer()->GetTeam();
                    int teamEnemy = 1 - GetPlayer()->GetTeam();
                    if (pTeamFlag && pEnemyFlag)
                    {
                        if (pEnemyFlag->GetCarrier() == pOwnChar)
                        {
                            if (pTeamFlag->GetPos().x > pOwnChar->GetPos().x)
                                m_botDirection = 1;
                            else
                                m_botDirection = -1;
                        }
                        else
                        {
                            if (pEnemyFlag->GetPos().x > pOwnChar->GetPos().x)
                                m_botDirection = 1;
                            else
                                m_botDirection = -1;
                        }
                    }
                }
            }
            if (m_isBot >= 4 && pOwnChar)
            {
                if (GetPlayer()->GetCharacter() && m_botAggro >= 0 && GameServer()->m_apPlayers[m_botAggro] && GameServer()->m_apPlayers[m_botAggro]->GetCharacter())
                {
                    vec2 posAggro = GameServer()->m_apPlayers[m_botAggro]->GetCharacter()->GetPos();
                    int distance = sqrt((posAggro.x - pOwnChar->GetPos().x) * (posAggro.x - pOwnChar->GetPos().x) 
                                    + (posAggro.y - pOwnChar->GetPos().y) * (posAggro.y - pOwnChar->GetPos().y));
                    if (GetPlayer()->GetCharacter()->GetWeaponAmmo(WEAPON_LASER))
                        Input.m_WantedWeapon = WEAPON_LASER + 1;
                    else if (GetPlayer()->GetCharacter()->GetWeaponGot(WEAPON_HAMMER) && distance < 2*32)
                        Input.m_WantedWeapon = WEAPON_HAMMER + 1;
                    else if (GetPlayer()->GetCharacter()->GetWeaponAmmo(WEAPON_GRENADE))
                        Input.m_WantedWeapon = WEAPON_GRENADE + 1;
                    else if (GetPlayer()->GetCharacter()->GetWeaponAmmo(WEAPON_SHOTGUN) && distance < 16*32)
                        Input.m_WantedWeapon = WEAPON_SHOTGUN + 1;
                    else
                        Input.m_WantedWeapon = WEAPON_GUN + 1;

                    if (m_isBot >= 5 && GetPlayer()->GetCharacter()->GetWeaponGot(WEAPON_GUN))
                        GetPlayer()->GetCharacter()->GiveWeapon(WEAPON_GUN, 10);
                }
                if (GameServer()->Server()->Tick() % (SERVER_TICK_SPEED) == 1)
                {
                    // get a new aggro
                    float smallestDistance = 850.0;
                    if (m_isBot == 4)
                        smallestDistance = 750.0;
                    m_botAggro = -1;

                    for (int i = 0; i < MAX_CLIENTS; i++)
                    {
                        if (i != GetPlayer()->GetCID() && GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
                        {
                            vec2 pos = GameServer()->m_apPlayers[i]->GetCharacter()->GetPos();
                            float d = sqrt((pos.x - pOwnChar->GetPos().x) * (pos.x - pOwnChar->GetPos().x) + (1.35) * (pos.y - pOwnChar->GetPos().y) * (pos.y - pOwnChar->GetPos().y));
                            // vertical distance is multiplied by a factor, since screens are larger horizontally
                            if (d < smallestDistance && !(GameServer()->m_pController->IsTeamplay() && GameServer()->m_apPlayers[i]->GetTeam() == GetPlayer()->GetTeam()))
                            {
                                smallestDistance = d;
                                m_botAggro = i;
                            }
                        }
                    }
                }
                if (m_botAggro == -1)
                {
                    m_ticksSinceFire = 0; // reset
                    Input.m_Fire = false; // do not shoot by default
                }
                else
                {
                    m_ticksSinceFire++;
                    if (GameServer()->m_pController->IsInstagibGrenade())
                    {
                        if (m_isBot >= 5 || m_ticksSinceFire > 50)
                        {
                            m_ticksSinceFire = 0; // reset
                            Input.m_Fire = true;  // fire
                        }
                        else
                            Input.m_Fire = false; // do not shoot by default
                    }
                    else if (GameServer()->m_pController->IsInstagib())
                    {
                        if (m_isBot >= 6 || (m_isBot == 5 && m_ticksSinceFire > 5 + /*Config()->m_SvLaserReloadTime*/ 800 / Server()->TickSpeed()) || m_ticksSinceFire > 20 + /*Config()->m_SvLaserReloadTime*/ 800 / Server()->TickSpeed())
                        {
                            m_ticksSinceFire = 0; // reset
                            Input.m_Fire = true;  // fire
                        }
                        else
                            Input.m_Fire = false; // do not shoot by default
                    }
                    else
                    {
                        if (m_ticksSinceFire > 10)
                        {                         // 10 = 0.2s
                            m_ticksSinceFire = 0; // reset
                            Input.m_Fire = true;  // fire
                        }
                        else
                            Input.m_Fire = false; // do not shoot by default
                        if (m_isBot >= 5)
                            Input.m_Fire = GameServer()->Server()->Tick() % (2) == 1;
                    }

                    // aim
                    if (GameServer()->m_apPlayers[m_botAggro] && GameServer()->m_apPlayers[m_botAggro]->GetCharacter())
                    {
                        vec2 pos = GameServer()->m_apPlayers[m_botAggro]->GetCharacter()->GetPos();
                        float d = sqrt((pos.x - pOwnChar->GetPos().x) * (pos.x - pOwnChar->GetPos().x) + (pos.y - pOwnChar->GetPos().y) * (pos.y - pOwnChar->GetPos().y));
                        Input.m_TargetX = pos.x - pOwnChar->GetPos().x; // aim
                        Input.m_TargetY = pos.y - pOwnChar->GetPos().y;
                        if (GetPlayer()->GetCharacter()->GetActiveWeapon() == WEAPON_GRENADE) // grenade curve correction, somewhat
                            Input.m_TargetY = Input.m_TargetY + (-abs(Input.m_TargetX) * 0.3);
                        if (m_isBot <= 5) // aim worse
                        {
                            Input.m_TargetX = (float)Input.m_TargetX + (d * 0.3 * ((float)(rand() % 64) / 64.0 - 0.5));
                            Input.m_TargetY = (float)Input.m_TargetY + (d * 0.3 * ((float)(rand() % 64) / 64.0 - 0.5));
                        }
                    }
                }
            }
        }
    }
}

void CPointerBotAI::GetSkin(STeeInfos &TeeInfos)
{
    CBotAI::GetSkin(TeeInfos);
}

const char * CPointerBotAI::GetClan()
{
    return m_Clan;
}