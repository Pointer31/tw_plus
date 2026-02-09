#include "weapons.h"
#include "weapons_list.h"
#include <engine/shared/config.h>

#include <generated/server_data.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/entities/character.h>
#include <game/server/entities/projectile.h>
#include <game/server/entities/laser.h>
#include <game/server/player.h>

#include <stdio.h>

// weapons::weapons(/* args */)
// {
// }

// weapons::~weapons()
// {
// }

struct WeaponInfo
{
    int m_LooksLike;
    int m_PredictsLike;
    int m_FireDelay;
};

static WeaponInfo WeaponInfos[WEAPON_CUSTOM_END];

void CWeapons::Init()
{
    WeaponInfos[WEAPON_GUN].m_LooksLike = WEAPON_GUN;
    WeaponInfos[WEAPON_HAMMER].m_LooksLike = WEAPON_HAMMER;
    WeaponInfos[WEAPON_SHOTGUN].m_LooksLike = WEAPON_SHOTGUN;
    WeaponInfos[WEAPON_GRENADE].m_LooksLike = WEAPON_GRENADE;
    WeaponInfos[WEAPON_LASER].m_LooksLike = WEAPON_LASER;
    WeaponInfos[WEAPON_NINJA].m_LooksLike = WEAPON_NINJA;

    WeaponInfos[WEAPON_STARGUN].m_LooksLike = WEAPON_GUN;
    WeaponInfos[WEAPON_STARGUN].m_PredictsLike = WEAPON_GUN;
    WeaponInfos[WEAPON_STARGUN].m_FireDelay = 200;

    WeaponInfos[WEAPON_PLASMAGUN].m_LooksLike = WEAPON_SHOTGUN;
    WeaponInfos[WEAPON_PLASMAGUN].m_PredictsLike = -1;
    WeaponInfos[WEAPON_PLASMAGUN].m_FireDelay = 200;
}

void CWeapons::FireWeapon(int WeaponId, CCharacter *pChar)
{
    vec2 Direction = normalize(vec2(pChar->GetLatestInput().m_TargetX, pChar->GetLatestInput().m_TargetY));
	vec2 ProjStartPos = pChar->GetPos()+Direction*pChar->GetProximityRadius()*0.75f;

    switch (WeaponId)
    {
        case WEAPON_STARGUN:
        {
            new CProjectile(pChar->GameWorld(), WEAPON_GUN,
                pChar->GetPlayer()->GetCID(),
                ProjStartPos,
                Direction,
                (int)(pChar->Server()->TickSpeed()*pChar->GameServer()->Tuning()->m_GunLifetime),
                5, 
                true, 
                0, 
                SOUND_GRENADE_EXPLODE, 
                WEAPON_GUN);

            pChar->GameServer()->CreateSound(pChar->GetPos(), SOUND_GUN_FIRE);
        } break;

        case WEAPON_PLASMAGUN:
        {
            new CLaser(pChar->GameWorld(), pChar->GetPos(), Direction, pChar->GameServer()->Tuning()->m_LaserReach, pChar->GetPlayer()->GetCID());
			pChar->GameServer()->CreateSound(pChar->GetPos(), SOUND_LASER_FIRE);
        } break;
    
        default:
            break;
    }
}

int CWeapons::LooksLike(int WeaponId)
{
    return WeaponInfos[WeaponId].m_LooksLike;
}

int CWeapons::GetFireDelay(int WeaponId)
{
    return WeaponInfos[WeaponId].m_FireDelay;
}

int CWeapons::GetAmmoRegen(int WeaponId)
{
    return 0;
}

