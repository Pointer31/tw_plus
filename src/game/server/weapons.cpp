#include "weapons.h"
#include "weapons_list.h"
#include <engine/shared/config.h>

#include <generated/server_data.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/entities/character.h>
#include <game/server/entities/projectile.h>
#include <game/server/entities/laser.h>
#include <game/server/entities/laser_better.h>
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
    char m_Name[32];
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
    str_copy(WeaponInfos[WEAPON_GUN].m_Name, "Pistol", sizeof(WeaponInfos[WEAPON_GUN].m_Name));
    str_copy(WeaponInfos[WEAPON_HAMMER].m_Name, "Hammer", sizeof(WeaponInfos[WEAPON_GUN].m_Name));
    str_copy(WeaponInfos[WEAPON_SHOTGUN].m_Name, "Shotgun", sizeof(WeaponInfos[WEAPON_GUN].m_Name));
    str_copy(WeaponInfos[WEAPON_GRENADE].m_Name, "Grenade Launcher", sizeof(WeaponInfos[WEAPON_GUN].m_Name));
    str_copy(WeaponInfos[WEAPON_LASER].m_Name, "Laser Rifle", sizeof(WeaponInfos[WEAPON_GUN].m_Name));
    str_copy(WeaponInfos[WEAPON_NINJA].m_Name, "Ninja", sizeof(WeaponInfos[WEAPON_GUN].m_Name));

    WeaponInfos[WEAPON_STARGUN].m_LooksLike = WEAPON_GUN;
    WeaponInfos[WEAPON_STARGUN].m_PredictsLike = WEAPON_GUN;
    WeaponInfos[WEAPON_STARGUN].m_FireDelay = 200;
    str_copy(WeaponInfos[WEAPON_STARGUN].m_Name, "Stargun", sizeof(WeaponInfos[WEAPON_GUN].m_Name));

    WeaponInfos[WEAPON_LASER_REPEATER].m_LooksLike = WEAPON_SHOTGUN;
    WeaponInfos[WEAPON_LASER_REPEATER].m_PredictsLike = -1;
    WeaponInfos[WEAPON_LASER_REPEATER].m_FireDelay = 300;
    str_copy(WeaponInfos[WEAPON_LASER_REPEATER].m_Name, "Laser Repeater", sizeof(WeaponInfos[WEAPON_GUN].m_Name));

    WeaponInfos[WEAPON_PLASMAGUN].m_LooksLike = WEAPON_LASER;
    WeaponInfos[WEAPON_PLASMAGUN].m_PredictsLike = -1;
    WeaponInfos[WEAPON_PLASMAGUN].m_FireDelay = 400;
    str_copy(WeaponInfos[WEAPON_PLASMAGUN].m_Name, "Plasmagun", sizeof(WeaponInfos[WEAPON_GUN].m_Name));

    WeaponInfos[WEAPON_SPIRAL].m_LooksLike = WEAPON_SHOTGUN;
    WeaponInfos[WEAPON_SPIRAL].m_PredictsLike = WEAPON_LASER;
    WeaponInfos[WEAPON_SPIRAL].m_FireDelay = 1000;
    str_copy(WeaponInfos[WEAPON_SPIRAL].m_Name, "Spiral", sizeof(WeaponInfos[WEAPON_GUN].m_Name));

    WeaponInfos[WEAPON_CHARGE_HAMMER].m_LooksLike = WEAPON_HAMMER;
    WeaponInfos[WEAPON_CHARGE_HAMMER].m_PredictsLike = WEAPON_HAMMER;
    WeaponInfos[WEAPON_CHARGE_HAMMER].m_FireDelay = 200;
    str_copy(WeaponInfos[WEAPON_CHARGE_HAMMER].m_Name, "Charge Hammer", sizeof(WeaponInfos[WEAPON_GUN].m_Name));
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
                3, 
                true, 
                0, 
                SOUND_GRENADE_EXPLODE, 
                WEAPON_GUN);

            pChar->GameServer()->CreateSound(pChar->GetPos(), SOUND_GUN_FIRE);
        } break;

        case WEAPON_LASER_REPEATER:
        {
            CLaserBetter* laser = new CLaserBetter(pChar->GameWorld(), pChar->GetPos(), Direction, pChar->GameServer()->Tuning()->m_LaserReach, pChar->GetPlayer()->GetCID());
            laser->m_MaxLength = 128;
            laser->m_MaxBounces = 30;
            laser->m_TimeToBounce = 80;
            laser->m_Damage = 5;
            laser->m_Pierces = true;
            laser->DoBounce();

			pChar->GameServer()->CreateSound(pChar->GetPos(), SOUND_LASER_FIRE);
        } break;

        case WEAPON_PLASMAGUN:
        {
            for (int i = -1; i < 2; i += 2)
            {
                const float ExtraAngle = 0.1;
                vec2 Direction2 = direction(angle(Direction) - i*2*ExtraAngle);
                CLaserBetter* laser = new CLaserBetter(pChar->GameWorld(), pChar->GetPos(), Direction2, 600, pChar->GetPlayer()->GetCID());
                laser->m_MaxLength = 100;
                laser->m_MaxBounces = 10;
                laser->m_TimeToBounce = 100;
                laser->m_IgnoreSolids = true;
                laser->m_RotateAngle = i*ExtraAngle;
                laser->m_Damage = 5;
                laser->DoBounce();
            }

			pChar->GameServer()->CreateSound(pChar->GetPos(), SOUND_LASER_FIRE);
        } break;

        case WEAPON_SPIRAL:
        {
            for (int i = -5; i < 6; i += 2)
            {
                const float ExtraAngle = 0.05*i;
                vec2 Direction2 = direction(angle(Direction) + ExtraAngle);
                CLaserBetter* laser = new CLaserBetter(pChar->GameWorld(), pChar->GetPos(), Direction2, 600, pChar->GetPlayer()->GetCID());
                laser->m_MaxLength = 100;
                laser->m_MaxBounces = 10;
                laser->m_TimeToBounce = 100;
                laser->m_RotateAngle = ExtraAngle;
                laser->m_Damage = 4;
                laser->DoBounce();
            }
			pChar->GameServer()->CreateSound(pChar->GetPos(), SOUND_HAMMER_FIRE);
        } break;

        case WEAPON_CHARGE_HAMMER:
		{
            int Ticks = pChar->Server()->Tick() - pChar->GetAttackTick();
            const int WIDTH_TILE = 32;

			pChar->GameServer()->CreateSound(pChar->GetPos() + Direction*WIDTH_TILE*4, SOUND_GRENADE_EXPLODE);
			pChar->GameServer()->CreateExplosion(pChar->GetPos() + Direction*WIDTH_TILE*4, pChar->GetPlayer()->GetCID(), WEAPON_HAMMER, 5);
			if (Ticks > 30)
				pChar->GameServer()->CreateExplosion(pChar->GetPos() + Direction*WIDTH_TILE*7, pChar->GetPlayer()->GetCID(), WEAPON_HAMMER, 5);
			if (Ticks > 45)
				pChar->GameServer()->CreateExplosion(pChar->GetPos() + Direction*WIDTH_TILE*10, pChar->GetPlayer()->GetCID(), WEAPON_HAMMER, 4);
			if (Ticks > 60)
				pChar->GameServer()->CreateExplosion(pChar->GetPos() + Direction*WIDTH_TILE*13, pChar->GetPlayer()->GetCID(), WEAPON_HAMMER, 3);
            
            // pChar->GameServer()->CreateExplosion(Pos, pChar->GetPlayer()->GetCID(), WEAPON_HAMMER, 4);

			pChar->GameServer()->CreateSound(pChar->GetPos(), SOUND_GRENADE_EXPLODE);
			pChar->GameServer()->CreateSound(pChar->GetPos(), SOUND_HAMMER_FIRE);
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

const char* CWeapons::GetName(int WeaponId)
{
    return WeaponInfos[WeaponId].m_Name;
}
