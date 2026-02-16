#ifndef GAME_SERVER_WEAPONS_H
#define GAME_SERVER_WEAPONS_H

#include <engine/shared/config.h>
#include <generated/server_data.h>
#include <game/server/entities/character.h>

class CWeapons
{
private:
    /* data */
public:
    // weapons(/* args */);
    // ~weapons();

    static void Init();
    static void FireWeapon(int WeaponId, CCharacter *pChar);
    static int GetFireDelay(int WeaponId);
    static int GetAmmoRegen(int WeaponId);
    static int LooksLike(int WeaponId);
    static const char* GetName(int WeaponId);
};

#endif