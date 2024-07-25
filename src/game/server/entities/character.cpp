/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>

#include "character.h"
#include "laser.h"
#include "projectile.h"

// this include was for testing only
#include <iostream>
// testing only

// input count
struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

CInputCount CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while (i != Cur)
	{
		i = (i + 1) & INPUT_STATE_MASK;
		if (i & 1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}

MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld)
	: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
	m_Health = 0;
	m_Armor = 0;
	m_FreezeTicks = 0;
	m_DeepFreeze = false;
	m_inTele = false;
	m_slowDeathTick = 0;
	m_healthArmorZoneTick = 0;
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	m_EmoteStop = -1;
	m_LastAction = -1;

	if (GameServer()->m_pController->IsInstagib())
	{
		if (GameServer()->m_pController->m_Flags & IGameController::GAMETYPE_GCTF)
		{
			m_ActiveWeapon = WEAPON_GRENADE;
			m_LastWeapon = WEAPON_GRENADE;
		}
		else
		{
			m_ActiveWeapon = WEAPON_RIFLE;
			m_LastWeapon = WEAPON_RIFLE;
		}
	}
	else
	{
		m_ActiveWeapon = WEAPON_GUN;
		m_LastWeapon = WEAPON_HAMMER;
	}

	m_LastNoAmmoSound = -1;
	m_QueuedWeapon = -1;

	m_pPlayer = pPlayer;
	m_Pos = Pos;

	for (int i = 0; i < NUM_WEAPONS - 1; i++)
		if (m_pPlayer->m_KeepWeapon[i] == true)
			GiveWeapon(i, -1);
	if (m_pPlayer->m_KeepAward)
		m_pPlayer->m_GotAward = true;

	if (g_Config.m_SvSpawnprotection)
		m_SpawnProtectTick = Server()->Tick() + Server()->TickSpeed() * g_Config.m_SvSpawnprotection;

	if (m_pPlayer->m_FreezeOnSpawn)
	{
		Freeze(g_Config.m_SvIFreezeAutomeltTime);
		m_pPlayer->m_FreezeOnSpawn = false;
	}

	m_Core.Reset();
	m_Core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
	m_Core.m_Pos = m_Pos;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;
	m_slowDeathTick = 0;
	m_healthArmorZoneTick = 0;

	GameServer()->m_pController->OnCharacterSpawn(this);

	return true;
}

void CCharacter::Destroy()
{
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
}

void CCharacter::SetWeapon(int W)
{
	if (W == m_ActiveWeapon)
		return;

	m_LastWeapon = m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = W;
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);

	if (m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPONS)
		m_ActiveWeapon = 0;
}

bool CCharacter::IsGrounded()
{
	if (GameServer()->Collision()->CheckPoint(m_Pos.x + m_ProximityRadius / 2, m_Pos.y + m_ProximityRadius / 2 + 5))
		return true;
	if (GameServer()->Collision()->CheckPoint(m_Pos.x - m_ProximityRadius / 2, m_Pos.y + m_ProximityRadius / 2 + 5))
		return true;
	return false;
}

void CCharacter::HandleNinja()
{
	if (m_ActiveWeapon != WEAPON_NINJA)
		return;

	if ((Server()->Tick() - m_Ninja.m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000) && !(m_FreezeTicks || m_DeepFreeze))
	{
		// time's up, return
		m_aWeapons[WEAPON_NINJA].m_Got = false;
		m_ActiveWeapon = m_LastWeapon;

		SetWeapon(m_ActiveWeapon);
		return;
	}

	// force ninja Weapon
	SetWeapon(WEAPON_NINJA);

	m_Ninja.m_CurrentMoveTime--;

	if (m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * m_Ninja.m_OldVelAmount;
	}

	if (m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		GameServer()->Collision()->MoveBox(&m_Core.m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		m_Core.m_Vel = vec2(0.f, 0.f);

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[MAX_CLIENTS];
			vec2 Dir = m_Pos - OldPos;
			float Radius = m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity **)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				if (aEnts[i] == this)
					continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(aEnts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if (m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

				aEnts[i]->TakeDamage(vec2(0, -10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, m_pPlayer->GetCID(), WEAPON_NINJA);
			}
		}

		return;
	}

	return;
}

void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if (m_ReloadTimer != 0 || m_QueuedWeapon == -1 || m_aWeapons[WEAPON_NINJA].m_Got)
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_ActiveWeapon;
	if (m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;

	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if (Next < 128) // make sure we only try sane stuff
	{
		while (Next) // Next Weapon selection
		{
			WantedWeapon = (WantedWeapon + 1) % NUM_WEAPONS;
			if (m_aWeapons[WantedWeapon].m_Got)
				Next--;
		}
	}

	if (Prev < 128) // make sure we only try sane stuff
	{
		while (Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon - 1) < 0 ? NUM_WEAPONS - 1 : WantedWeapon - 1;
			if (m_aWeapons[WantedWeapon].m_Got)
				Prev--;
		}
	}

	// Direct Weapon selection
	if (m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon - 1;

	// check for insane values
	if (WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != m_ActiveWeapon && m_aWeapons[WantedWeapon].m_Got)
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
}

void CCharacter::FireWeapon()
{
	if (m_ReloadTimer != 0)
		return;

	DoWeaponSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if (m_ActiveWeapon == WEAPON_GRENADE || m_ActiveWeapon == WEAPON_SHOTGUN || m_ActiveWeapon == WEAPON_RIFLE || (m_ActiveWeapon == WEAPON_GUN && g_Config.m_SvPistolAuto == 1))
		FullAuto = true;

	// check if we gonna fire
	bool WillFire = false;
	if (CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if (FullAuto && (m_LatestInput.m_Fire & 1) && m_aWeapons[m_ActiveWeapon].m_Ammo)
		WillFire = true;

	if (!WillFire)
		return;

	// check for ammo
	if (!m_aWeapons[m_ActiveWeapon].m_Ammo)
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		if (m_LastNoAmmoSound + Server()->TickSpeed() <= Server()->Tick())
		{
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
			m_LastNoAmmoSound = Server()->Tick();
		}
		return;
	}

	vec2 ProjStartPos = m_Pos + Direction * m_ProximityRadius * 0.75f;

	switch (m_ActiveWeapon)
	{
	case WEAPON_HAMMER:
	{
		// reset objects Hit
		m_NumObjectsHit = 0;
		GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE);

		CCharacter *apEnts[MAX_CLIENTS];
		int Hits = 0;
		int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius * 0.5f, (CEntity **)apEnts,
													 MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

		for (int i = 0; i < Num; ++i)
		{
			CCharacter *pTarget = apEnts[i];

			if ((pTarget == this) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
				continue;

			// set his velocity to fast upward (for now)
			if (length(pTarget->m_Pos - ProjStartPos) > 0.0f)
				GameServer()->CreateHammerHit(pTarget->m_Pos - normalize(pTarget->m_Pos - ProjStartPos) * m_ProximityRadius * 0.5f);
			else
				GameServer()->CreateHammerHit(ProjStartPos);

			vec2 Dir;
			if (length(pTarget->m_Pos - m_Pos) > 0.0f)
				Dir = normalize(pTarget->m_Pos - m_Pos);
			else
				Dir = vec2(0.f, -1.f);

			pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
								m_pPlayer->GetCID(), m_ActiveWeapon);
			Hits++;
		}

		// if we Hit anything, we have to wait for the reload
		if (Hits)
			m_ReloadTimer = Server()->TickSpeed() / 3;
	}
	break;

	case WEAPON_GUN:
	{
		CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GUN,
											 m_pPlayer->GetCID(),
											 ProjStartPos,
											 Direction,
											 (int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GunLifetime),
											 1, 0, 0, -1, WEAPON_GUN);

		// pack the Projectile and send it to the client Directly
		CNetObj_Projectile p;
		pProj->FillInfo(&p);

		CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
		Msg.AddInt(1);
		for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
			Msg.AddInt(((int *)&p)[i]);

		Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

		GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
	}
	break;

	case WEAPON_SHOTGUN:
	{
		int ShotSpread = 2;

		CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
		Msg.AddInt(ShotSpread * 2 + 1);

		for (int i = -ShotSpread; i <= ShotSpread; ++i)
		{
			float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
			float a = GetAngle(Direction);
			a += Spreading[i + 2];
			float v = 1 - (absolute(i) / (float)ShotSpread);
			float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
			CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
												 m_pPlayer->GetCID(),
												 ProjStartPos,
												 vec2(cosf(a), sinf(a)) * Speed,
												 (int)(Server()->TickSpeed() * GameServer()->Tuning()->m_ShotgunLifetime),
												 1, g_Config.m_SvExplosiveShotgun, 0, -1, WEAPON_SHOTGUN);

			// pack the Projectile and send it to the client Directly
			CNetObj_Projectile p;
			pProj->FillInfo(&p);

			for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
				Msg.AddInt(((int *)&p)[i]);
		}

		Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

		GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
	}
	break;

	case WEAPON_GRENADE:
	{
		CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
											 m_pPlayer->GetCID(),
											 ProjStartPos,
											 Direction,
											 (int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeLifetime),
											 1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

		// pack the Projectile and send it to the client Directly
		CNetObj_Projectile p;
		pProj->FillInfo(&p);

		CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
		Msg.AddInt(1);
		for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
			Msg.AddInt(((int *)&p)[i]);
		Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
	}
	break;

	case WEAPON_RIFLE:
	{
		if (g_Config.m_SvPlasmaGun)
		{
			vec2 dir1 = Direction;
			vec2 dir2 = Direction;

			double deg = 50;
			double theta = deg / 180.0 * M_PI;
			double c = cos(theta);
			double s = sin(theta);
			double tx = Direction.x * c - Direction.y * s;
			double ty = Direction.x * s + Direction.y * c;
			dir1.x = tx;
			dir1.y = ty;

			deg = -50;
			theta = deg / 180.0 * M_PI;
			c = cos(theta);
			s = sin(theta);
			tx = Direction.x * c - Direction.y * s;
			ty = Direction.x * s + Direction.y * c;
			dir2.x = tx;
			dir2.y = ty;

			new CLaser(GameWorld(), m_Pos, dir1, g_Config.m_SvPlasmaGunReach, m_pPlayer->GetCID(), -1);
			new CLaser(GameWorld(), m_Pos, dir2, g_Config.m_SvPlasmaGunReach, m_pPlayer->GetCID(), 1);
		}
		else
		{
			if (!m_pPlayer->m_GotAward || (g_Config.m_SvKillingspreeAwardLasers == 1))
			{
				new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID(), 0);
			}
			else
				for (float i = -0.5f * g_Config.m_SvKillingspreeAwardLasers; i < 0.5f * g_Config.m_SvKillingspreeAwardLasers; i++)
				{
					float a = i * 0.01f * (float)g_Config.m_SvKillingspreeAwardLasersSplit + GetAngle(Direction);
					new CLaser(GameWorld(), m_Pos, normalize(vec2(cosf(a), sinf(a))), GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID(), 0);
				}
		}
		GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
	}
	break;

	case WEAPON_NINJA:
	{
		// reset Hit objects
		m_NumObjectsHit = 0;

		m_Ninja.m_ActivationDir = Direction;
		m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
		m_Ninja.m_OldVelAmount = g_Config.m_SvNinjaConstantSpeed; // length(m_Core.m_Vel);

		GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE);

		if (g_Config.m_SvNinjaAllWeapons)
		{
			new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID(), 0);
			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);

			CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
												 m_pPlayer->GetCID(),
												 ProjStartPos,
												 Direction,
												 (int)(Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeLifetime),
												 1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);

			int ShotSpread = 2;

			for (int i = -ShotSpread; i <= ShotSpread; ++i)
			{
				float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
				float a = GetAngle(Direction);
				a += Spreading[i + 2];
				float v = 1 - (absolute(i) / (float)ShotSpread);
				float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
													 m_pPlayer->GetCID(),
													 ProjStartPos,
													 vec2(cosf(a), sinf(a)) * Speed,
													 (int)(Server()->TickSpeed() * GameServer()->Tuning()->m_ShotgunLifetime),
													 1, 0, 0, -1, WEAPON_SHOTGUN);
			}

			GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
		}
	}
	break;
	}

	m_pPlayer->m_Stats.m_Shots[m_ActiveWeapon]++;
	m_pPlayer->m_Stats.m_TotalShots++;

	m_AttackTick = Server()->Tick();

	if (m_aWeapons[m_ActiveWeapon].m_Ammo > 0) // -1 == unlimited
		m_aWeapons[m_ActiveWeapon].m_Ammo--;

	if (!m_ReloadTimer)
	{
		int FireDelay = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay;

		if (m_ActiveWeapon == WEAPON_RIFLE)
		{
			FireDelay = g_Config.m_SvLaserReloadTime;
			if (g_Config.m_SvPlasmaGun)
				FireDelay = g_Config.m_SvPlasmaGunFireDelay;
		}

		if (m_pPlayer->m_GotAward)
			FireDelay = g_Config.m_SvKillingspreeAwardFiredelay;

		m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
	}
}

void CCharacter::HandleWeapons()
{
	// ninja
	HandleNinja();

	// check reload timer
	if (m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();

	// ammo regen
	int AmmoRegenTime = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Ammoregentime;
	bool gCTF = GameServer()->m_pController->m_Flags & IGameController::GAMETYPE_GCTF;
	if (gCTF && m_aWeapons[m_ActiveWeapon].m_Ammo > -1)
		AmmoRegenTime = g_Config.m_SvGrenadeAmmoRegen;

	if (AmmoRegenTime)
	{
		// If equipped and not active, regen ammo?
		if (m_ReloadTimer <= 0)
		{
			if (m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart < 0)
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = Server()->Tick();

			if ((Server()->Tick() - m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_aWeapons[m_ActiveWeapon].m_Ammo + 1, (gCTF) ? g_Config.m_SvGrenadeAmmo : 10);
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
		}
	}

	return;
}

bool CCharacter::GiveWeapon(int Weapon, int Ammo)
{
	if (m_aWeapons[Weapon].m_Ammo < g_pData->m_Weapons.m_aId[Weapon].m_Maxammo || !m_aWeapons[Weapon].m_Got)
	{
		m_aWeapons[Weapon].m_Got = true;
		m_aWeapons[Weapon].m_Ammo = min(g_pData->m_Weapons.m_aId[Weapon].m_Maxammo, Ammo);
		return true;
	}
	return false;
}

void CCharacter::GiveNinja()
{
	m_Ninja.m_ActivationTick = Server()->Tick();
	m_aWeapons[WEAPON_NINJA].m_Got = true;
	m_aWeapons[WEAPON_NINJA].m_Ammo = -1;
	if (m_ActiveWeapon != WEAPON_NINJA)
		m_LastWeapon = m_ActiveWeapon;
	m_ActiveWeapon = WEAPON_NINJA;

	GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA);

	// add health and shield bonusses, if enabled
	m_Health = m_Health + g_Config.m_SvNinjaHeartBonus;
	if (m_Health > 10)
		m_Health = 10;
	m_Armor = m_Armor + g_Config.m_SvNinjaArmorBonus;
	if (m_Armor > 10)
		m_Armor = 10;
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

void CCharacter::SetEmoteFix(int Emote, int Tick)
{
	m_pPlayer->m_SetEmoteType = Emote;
	m_pPlayer->m_SetEmoteStop = Tick;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	if (m_FreezeTicks || m_DeepFreeze)
		return;

	// check for changes
	if (mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;

	// it is not allowed to aim in the center
	if (m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	if (m_FreezeTicks || m_DeepFreeze)
		return;

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

	// it is not allowed to aim in the center
	if (m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	if (m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if ((m_Input.m_Fire & 1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::Tick()
{
	if (m_pPlayer->m_ForceBalanced)
	{
		char Buf[128];
		str_format(Buf, sizeof(Buf), "You were moved to %s due to team balancing", GameServer()->m_pController->GetTeamName(m_pPlayer->GetTeam()));
		GameServer()->SendBroadcast(Buf, m_pPlayer->GetCID());

		m_pPlayer->m_ForceBalanced = false;
	}

	if (m_FreezeTicks)
	{
		// unfreeze player/automelt
		m_FreezeTicks--;
		if (m_FreezeTicks <= 0)
			Melt(-1);

		// Melting
		if (GameServer()->m_pController->IsIFreeze())
		{
			bool FoundMelter = false;
			CCharacter *apCloseChars[MAX_CLIENTS];
			int Num = GameServer()->m_World.FindEntities(m_Pos, g_Config.m_SvIFreezeMeltRange, (CEntity **)apCloseChars, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			for (int i = 0; i < Num; i++)
			{
				if (!apCloseChars[i]->IsAlive() || apCloseChars[i] == this || apCloseChars[i]->Frozen())
					continue;

				if (apCloseChars[i]->GetPlayer()->GetTeam() == m_pPlayer->GetTeam())
				{
					m_MeltTicks++;
					FoundMelter = true;
					// Send "thawed" on half of melttime
					if (m_MeltTicks == (int)(Server()->TickSpeed() * g_Config.m_SvIFreezeMeltTime * 0.0005f))
						GameServer()->SendBroadcast("You are being thawed", m_pPlayer->GetCID());
					else if (m_MeltTicks >= Server()->TickSpeed() * g_Config.m_SvIFreezeMeltTime * 0.001f)
						Melt(apCloseChars[i]->GetPlayer()->GetCID());
					break;
				}
			}
			// set counter to 0 if melter went away or no melter found
			if (!FoundMelter)
				m_MeltTicks = 0;
		}
	}

	if (g_Config.m_SvSpawnprotection)
	{
		if (m_SpawnProtectTick > Server()->Tick())
		{
			// All 100ms
			if (Server()->Tick() % 5 == 0)
			{
				char aBuf[64];
				str_format(aBuf, sizeof(aBuf), "Spawnprotection for %.1f sec.", (float)(m_SpawnProtectTick - Server()->Tick()) / Server()->TickSpeed());
				GameServer()->SendBroadcast(aBuf, m_pPlayer->GetCID());
			}
		}
		else if (m_SpawnProtectTick == Server()->Tick())
			GameServer()->SendBroadcast("", m_pPlayer->GetCID());
	}

	if ((g_Config.m_SvAnticamper == 1) || (g_Config.m_SvAnticamper == 2 && GameServer()->m_pController->IsInstagib()))
		Anticamper();

	m_Core.m_Input = m_Input;
	m_Core.Tick(true);

	// handle death-tiles and leaving gamelayer
	if (GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_DEATH ||
		GameLayerClipped(m_Pos))
	{
		if (!g_Config.m_SvHookkill) {
			Die(m_pPlayer->GetCID(), WEAPON_WORLD);
		} else {
			int From = m_pPlayer->GetCID();
			CCharacterCore pCharCore = *GetCore();
			if (pCharCore.m_LastHooked > 0) {
				From = pCharCore.m_LastHookedBy;
				pCharCore.m_LastHooked = 0;
				// set attacker's face to happy (taunt!)
				if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
				{
					CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
					if (pChr)
					{
						pChr->m_EmoteType = EMOTE_HAPPY;
						pChr->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();
					}
				}
				// do damage Hit sound
				if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
				{
					int Mask = CmaskOne(From);
					for (int i = 0; i < MAX_CLIENTS; i++)
					{
						if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
							Mask |= CmaskOne(i);
					}
					GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
				}
			}
			Die(From, WEAPON_NINJA);
		}
	}

	// teleports
	if (GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELEONE ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELEONE ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELEONE ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELEONE)
	{
		if (!m_inTele) {
			m_inTele = true;
			int x = GameServer()->Collision()->getTeleX(0);
			int y = GameServer()->Collision()->getTeleY(0);
			int tx = GameServer()->Collision()->getTeleX(1);
			int ty = GameServer()->Collision()->getTeleY(1);
			vec2 start = {x, y};
			vec2 end = {tx, ty};
			m_Pos = m_Pos - start * 32 + end * 32;
			m_Core.m_Pos = m_Pos;
		}
	}
	else if (GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELETWO ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELETWO ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELETWO ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELETWO)
	{
		if (!m_inTele) {
			m_inTele = true;
			int x = GameServer()->Collision()->getTeleX(1);
			int y = GameServer()->Collision()->getTeleY(1);
			int tx = GameServer()->Collision()->getTeleX(0);
			int ty = GameServer()->Collision()->getTeleY(0);
			vec2 start = {x, y};
			vec2 end = {tx, ty};
			m_Pos = m_Pos - start * 32 + end * 32;
			m_Core.m_Pos = m_Pos;
		}
	}
	else if (GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELETHREE ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELETHREE ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELETHREE ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELETHREE)
	{
		if (!m_inTele) {
			m_inTele = true;
			int x = GameServer()->Collision()->getTeleX(2);
			int y = GameServer()->Collision()->getTeleY(2);
			int tx = GameServer()->Collision()->getTeleX(3);
			int ty = GameServer()->Collision()->getTeleY(3);
			vec2 start = {x, y};
			vec2 end = {tx, ty};
			m_Pos = m_Pos - start * 32 + end * 32;
			m_Core.m_Pos = m_Pos;
		}
	}
	else if (GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELEFOUR ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELEFOUR ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELEFOUR ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_TELEFOUR)
	{
		if (!m_inTele) {
			m_inTele = true;
			int x = GameServer()->Collision()->getTeleX(3);
			int y = GameServer()->Collision()->getTeleY(3);
			int tx = GameServer()->Collision()->getTeleX(2);
			int ty = GameServer()->Collision()->getTeleY(2);
			vec2 start = {x, y};
			vec2 end = {tx, ty};
			m_Pos = m_Pos - start * 32 + end * 32;
			m_Core.m_Pos = m_Pos;
		}
	}
	else
	{
		// you are not not in a teleport
		m_inTele = false;
	}

	// Slow death zone
	if (GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_SLOWDEATH ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_SLOWDEATH ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) & CCollision::COLFLAG_SLOWDEATH ||
			 GameServer()->Collision()->GetCollisionAt(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) & CCollision::COLFLAG_SLOWDEATH)
	{
		m_slowDeathTick--;
		if (m_slowDeathTick < 0) {
			if (StrLeftComp(GameServer()->GameType(), "DM+")
			|| StrLeftComp(GameServer()->GameType(), "TDM+")
			|| StrLeftComp(GameServer()->GameType(), "CTF+")
			|| StrLeftComp(GameServer()->GameType(), "HTF")) {
				TakeDamage({0,0}, 1, -1, WEAPON_NINJA);
			} else {
				m_Health = m_Health - 1;
				if (m_Health < 1)
					Die(m_pPlayer->GetCID(), WEAPON_NINJA);
					
				GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);
				m_EmoteType = EMOTE_PAIN;
				m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;
			}
			m_slowDeathTick = g_Config.m_SvSlowDeathTicks;
		}
	}

	// healthzone and armorzone
	if (GameServer()->Collision()->GetCollisionAtNew(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) == TILE_HEALTHZONE ||
			 GameServer()->Collision()->GetCollisionAtNew(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) == TILE_HEALTHZONE ||
			 GameServer()->Collision()->GetCollisionAtNew(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) == TILE_HEALTHZONE ||
			 GameServer()->Collision()->GetCollisionAtNew(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) == TILE_HEALTHZONE)
	{
		m_healthArmorZoneTick--;
		if (m_healthArmorZoneTick < 0) {
			if (m_Health < 10) {
				m_Health = m_Health + 1;
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
			}
			m_EmoteType = EMOTE_HAPPY;
			m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;
			m_healthArmorZoneTick = g_Config.m_SvHealthArmorZoneTicks;
		}
	}
	if (!GameServer()->m_pController->IsInstagib() && (
			GameServer()->Collision()->GetCollisionAtNew(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) == TILE_ARMORZONE ||
			 GameServer()->Collision()->GetCollisionAtNew(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) == TILE_ARMORZONE ||
			 GameServer()->Collision()->GetCollisionAtNew(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f) == TILE_ARMORZONE ||
			 GameServer()->Collision()->GetCollisionAtNew(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f) == TILE_ARMORZONE))
	{
		m_healthArmorZoneTick--;
		if (m_healthArmorZoneTick < 0) {
			if (m_Armor < 10) {
				m_Armor = m_Armor + 1;
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
			}
			m_EmoteType = EMOTE_HAPPY;
			m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;
			m_healthArmorZoneTick = g_Config.m_SvHealthArmorZoneTicks;
		}
	}

	// handle Weapons
	HandleWeapons();

	// Previnput
	m_PrevInput = m_Input;
	return;
}

void CCharacter::TickDefered()
{
	// advance the dummy
	{
		CWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, GameServer()->Collision());
		m_ReckoningCore.Tick(false);
		m_ReckoningCore.Move();
		m_ReckoningCore.Quantize();
	}

	// lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));

	m_Core.Move();
	bool StuckAfterMove = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Core.Quantize();
	bool StuckAfterQuant = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Pos = m_Core.m_Pos;

	if (!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		} StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
				   StuckBefore,
				   StuckAfterMove,
				   StuckAfterQuant,
				   StartPos.x, StartPos.y,
				   StartVel.x, StartVel.y,
				   StartPosX.u, StartPosY.u,
				   StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	int Events = m_Core.m_TriggeredEvents;
	int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());

	if (Events & COREEVENT_GROUND_JUMP)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, Mask);

	if (Events & COREEVENT_HOOK_ATTACH_PLAYER)
		GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, CmaskAll());
	if (Events & COREEVENT_HOOK_ATTACH_GROUND)
		GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, Mask);
	if (Events & COREEVENT_HOOK_HIT_NOHOOK)
		GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, Mask);

	if (m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if (m_ReckoningTick + Server()->TickSpeed() * 3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
		}
	}
}

void CCharacter::TickPaused()
{
	++m_AttackTick;
	++m_DamageTakenTick;
	++m_Ninja.m_ActivationTick;
	++m_ReckoningTick;
	if (m_LastAction != -1)
		++m_LastAction;
	if (m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart > -1)
		++m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart;
	if (m_EmoteStop > -1)
		++m_EmoteStop;
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if (m_Health >= 10)
		return false;
	m_Health = clamp(m_Health + Amount, 0, 10);
	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	if (m_Armor >= 10)
		return false;
	m_Armor = clamp(m_Armor + Amount, 0, 10);
	return true;
}

void CCharacter::Die(int Killer, int Weapon)
{
	// killer == -1 should mean the world, and because of the code, it will be set to the player's own id, so the server doesn't crash.
	if (Killer < 0)
		Killer = m_pPlayer->GetCID();


	// we got to wait 0.5 secs before respawning
	m_pPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() / 2;
	int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], Weapon);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d",
			   Killer, Server()->ClientName(Killer),
			   m_pPlayer->GetCID(), Server()->ClientName(m_pPlayer->GetCID()), Weapon, ModeSpecial);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	if (GameServer()->m_pController->IsInstagib())
	{
		if (GameServer()->GetPlayerChar(Killer) && Killer != m_pPlayer->GetCID())
			GameServer()->GetPlayerChar(Killer)->AddSpree();
		EndSpree(Killer);
	}
	m_pPlayer->m_Stats.m_Deaths++;
	if (GameServer()->m_apPlayers[Killer] && Killer != m_pPlayer->GetCID())
		GameServer()->m_apPlayers[Killer]->m_Stats.m_Kills++;

	// send the kill message
	CNetMsg_Sv_KillMsg Msg;
	Msg.m_Killer = Killer;
	Msg.m_Victim = m_pPlayer->GetCID();
	Msg.m_Weapon = Weapon;
	Msg.m_ModeSpecial = ModeSpecial;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);

	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE);

	if (GameServer()->m_pController->IsIFreeze() && Weapon != WEAPON_GAME && Weapon != WEAPON_WORLD)
		return;

	// Unfreeze if frozen
	m_FreezeTicks = 0;
	// this is for auto respawn after 3 secs
	m_pPlayer->m_DieTick = Server()->Tick();

	m_Alive = false;
	GameServer()->m_World.RemoveEntity(this);
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	if (g_Config.m_SvNinjaInvincible && m_ActiveWeapon == WEAPON_NINJA)
		return false;

	m_Core.m_Vel += Force;

	int From2 = From;
	if (From < 0)
		From2 = m_pPlayer->GetCID();

	if (GameServer()->m_pController->IsFriendlyFire(m_pPlayer->GetCID(), From2) && !g_Config.m_SvTeamdamage)
		return false;

	if (m_SpawnProtectTick > Server()->Tick() || (GameServer()->GetPlayerChar(From) && GameServer()->GetPlayerChar(From)->Spawnprotected()))
		return false;

	/* TW+ */
	if (GameServer()->m_pController->IsInstagib())
	{
		if ((GameServer()->m_pController->m_Flags & IGameController::GAMETYPE_GCTF) && g_Config.m_SvGrenadeMinDamage > Dmg)
			return false;

		if ((From == m_pPlayer->GetCID()) || (Weapon == WEAPON_GAME))
			return false;

		if (GameServer()->m_pController->IsIFreeze() && Frozen())
			return false;

		if (Dmg > 0) {
			m_Health = 0;
			m_Armor = 0;
		}
	}
	/* TW+ */

	// m_pPlayer only inflicts half damage on self
	if (From == m_pPlayer->GetCID())
		Dmg = max(1, Dmg / 2);

	m_DamageTaken++;

	if (!GameServer()->m_pController->IsInstagib())
	{
		// create healthmod indicator
		if (Server()->Tick() < m_DamageTakenTick + 25)
		{
			// make sure that the damage indicators doesn't group together
			GameServer()->CreateDamageInd(m_Pos, m_DamageTaken * 0.25f, Dmg);
		}
		else
		{
			m_DamageTaken = 0;
			GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
		}
	}

	if (Dmg)
	{
		if (m_Armor)
		{
			if (Dmg > 1)
			{
				m_Health--;
				Dmg--;
			}

			if (Dmg > m_Armor)
			{
				Dmg -= m_Armor;
				m_Armor = 0;
			}
			else
			{
				m_Armor -= Dmg;
				Dmg = 0;
			}
		}

		m_Health -= Dmg;
	}

	m_DamageTakenTick = Server()->Tick();

	// do damage Hit sound
	if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
	{
		int Mask = CmaskOne(From);
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
				Mask |= CmaskOne(i);
		}
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
	}

	m_pPlayer->m_Stats.m_Hits++;

	// check for death
	if (m_Health <= 0)
	{
		Die(From, Weapon);

		// set attacker's face to happy (taunt!)
		if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
			{
				pChr->m_EmoteType = EMOTE_HAPPY;
				pChr->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();
			}
		}

		return false;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	if (g_Config.m_SvSpawnprotection && m_SpawnProtectTick >= Server()->Tick() && m_pPlayer->GetCID() != SnappingClient)
	{
		if (15 - ((m_SpawnProtectTick - Server()->Tick()) % (15)) < 5)
			return;
	}

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, m_pPlayer->GetCID(), sizeof(CNetObj_Character)));
	if (!pCharacter)
		return;

	// write down the m_Core
	if (!m_ReckoningTick || GameServer()->m_World.m_Paused)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter);
	}

	// set emote
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}
	if (m_pPlayer->m_SetEmoteStop < Server()->Tick())
	{
		m_pPlayer->m_SetEmoteType = EMOTE_NORMAL;
		m_pPlayer->m_SetEmoteStop = -1;
	}

	pCharacter->m_Emote = (m_EmoteType == EMOTE_NORMAL) ? m_pPlayer->m_SetEmoteType : m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = m_ActiveWeapon;
	pCharacter->m_AttackTick = m_AttackTick;

	pCharacter->m_Direction = m_Input.m_Direction;

	if (m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = (m_FreezeTicks) ? (m_FreezeTicks / Server()->TickSpeed()) / 10 : m_Health;
		pCharacter->m_Armor = (m_FreezeTicks) ? (m_FreezeTicks / Server()->TickSpeed()) % 10 + 1 : m_Armor;
		if (m_aWeapons[m_ActiveWeapon].m_Ammo > 0)
			pCharacter->m_AmmoCount = m_aWeapons[m_ActiveWeapon].m_Ammo;
	}

	if (pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if (250 - ((Server()->Tick() - m_LastAction) % (250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;

	// WARNING, this is very hardcoded; for ddnet client support
	// if (SnappingClient == m_pPlayer->GetCID()) {
		CNetObj_DDNetCharacter *pDDNetCharacter = (CNetObj_DDNetCharacter *)Server()->SnapNewItem(32764, m_pPlayer->GetCID(), 40);
		if(!pDDNetCharacter)
			return;

		pDDNetCharacter->m_Flags = 0;
		if (m_aWeapons[0].m_Got)
			pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_HAMMER;
		if (m_aWeapons[1].m_Got)
			pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GUN;
		if (m_aWeapons[2].m_Got)
			pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_SHOTGUN;
		if (m_aWeapons[3].m_Got)
			pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GRENADE;
		if (m_aWeapons[4].m_Got)
			pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_LASER;
		if (m_aWeapons[5].m_Got)
			pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_NINJA;
		if (m_FreezeTicks > 0)
			pDDNetCharacter->m_Flags |= CHARACTERFLAG_IN_FREEZE | CHARACTERFLAG_MOVEMENTS_DISABLED;
		pDDNetCharacter->m_FreezeEnd = 0;
		pDDNetCharacter->m_Jumps = 2;
		pDDNetCharacter->m_TeleCheckpoint = -1;
		pDDNetCharacter->m_StrongWeakId = 0;
		
		pDDNetCharacter->m_JumpedTotal = m_Core.m_Jumped;
		pDDNetCharacter->m_NinjaActivationTick = -1;
		if (m_ActiveWeapon == WEAPON_NINJA)
			pDDNetCharacter->m_NinjaActivationTick = m_Ninja.m_ActivationTick;
		pDDNetCharacter->m_FreezeStart = -1;
		pDDNetCharacter->m_TargetX = m_LatestInput.m_TargetX;
		pDDNetCharacter->m_TargetY = m_LatestInput.m_TargetY;
		// default values
		// pDDNetCharacter->m_Flags = 0;
		// pDDNetCharacter->m_FreezeEnd = 0;
		// pDDNetCharacter->m_Jumps = 2;
		// pDDNetCharacter->m_TeleCheckpoint = -1;
		// pDDNetCharacter->m_StrongWeakId = 0;
		// pDDNetCharacter->m_JumpedTotal = -1;
		// pDDNetCharacter->m_NinjaActivationTick = -1;
		// pDDNetCharacter->m_FreezeStart = -1;
		// pDDNetCharacter->m_TargetX = 0;
		// pDDNetCharacter->m_TargetY = 0;
	// }
}

int CCharacter::Anticamper()
{
	if (GameServer()->m_World.m_Paused || Frozen())
	{
		m_CampTick = -1;
		m_SentCampMsg = false;
		return 0;
	}

	int AnticamperTime = g_Config.m_SvAnticamperTime;
	int AnticamperRange = g_Config.m_SvAnticamperRange;

	if (m_CampTick == -1)
	{
		m_CampPos = m_Pos;
		m_CampTick = Server()->Tick() + Server()->TickSpeed() * AnticamperTime;
	}

	// Check if the player is moving
	if ((m_CampPos.x - m_Pos.x >= (float)AnticamperRange || m_CampPos.x - m_Pos.x <= -(float)AnticamperRange) ||
		(m_CampPos.y - m_Pos.y >= (float)AnticamperRange || m_CampPos.y - m_Pos.y <= -(float)AnticamperRange))
	{
		m_CampTick = -1;
	}

	// Send warning to the player
	if (m_CampTick <= Server()->Tick() + Server()->TickSpeed() * AnticamperTime / 2 && m_CampTick != -1 && !m_SentCampMsg)
	{
		GameServer()->SendBroadcast("ANTICAMPER: Move or die", m_pPlayer->GetCID());
		m_SentCampMsg = true;
	}

	// Kill him
	if ((m_CampTick <= Server()->Tick()) && (m_CampTick > 0))
	{
		bool IsIFreeze = GameServer()->m_pController->IsIFreeze();
		if (g_Config.m_SvAnticamperFreeze || IsIFreeze)
		{
			Freeze((IsIFreeze) ? g_Config.m_SvIFreezeAutomeltTime : g_Config.m_SvAnticamperFreeze);
			GameServer()->SendBroadcast("You have been frozen due camping", m_pPlayer->GetCID());
		}
		else
			Die(m_pPlayer->GetCID(), WEAPON_GAME);

		m_CampTick = -1;
		m_SentCampMsg = false;
		return 1;
	}
	return 0;
}

void CCharacter::Freeze(int Secs)
{
	if (Secs < 0)
		m_DeepFreeze = true;
	else
		m_FreezeTicks = Server()->TickSpeed() * Secs;
	m_LastWeapon = m_ActiveWeapon;
	m_ActiveWeapon = WEAPON_NINJA;
	ResetInput();
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
}

int CCharacter::Frozen()
{
	return (m_DeepFreeze) ? -1 : m_FreezeTicks;
}

void CCharacter::AddSpree()
{
	m_pPlayer->m_Spree++;
	const int NumMsg = 5;
	char aBuf[128];

	if (m_pPlayer->m_Spree % g_Config.m_SvKillingspreeKills == 0)
	{
		static const char aaSpreeMsg[NumMsg][32] = {"is on a killing spree", "is on a rampage", "is dominating", "is unstoppable", "is godlike"};
		int No = m_pPlayer->m_Spree / NumMsg - 1;

		str_format(aBuf, sizeof(aBuf), "%s %s with %d kills!", Server()->ClientName(m_pPlayer->GetCID()), aaSpreeMsg[(No > NumMsg - 1) ? NumMsg - 1 : No], m_pPlayer->m_Spree);
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}

	if ((m_pPlayer->m_Spree >= g_Config.m_SvKillingspreeKills) && g_Config.m_SvKillingspreeAward &&
		(GameServer()->m_pController->IsIFreeze() || !g_Config.m_SvKillingspreeIFreeze) && !m_pPlayer->m_GotAward)
	{
		m_pPlayer->m_GotAward = true;
		str_format(aBuf, sizeof(aBuf), "%s got the killingspree award", Server()->ClientName(m_pPlayer->GetCID()));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}
}

void CCharacter::EndSpree(int Killer)
{
	if (m_pPlayer->m_Spree >= g_Config.m_SvKillingspreeKills)
	{
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
		GameServer()->CreateExplosion(m_Pos, m_pPlayer->GetCID(), WEAPON_RIFLE, true);

		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s %d-kills killing spree was ended by %s", Server()->ClientName(m_pPlayer->GetCID()), m_pPlayer->m_Spree, Server()->ClientName(Killer));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}
	m_pPlayer->m_GotAward = false;
	m_pPlayer->m_Spree = 0;
}

void CCharacter::KillChar()
{
	// we got to wait 0.5 secs before respawning
	m_pPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() / 2;

	// this is for auto respawn after 3 secs
	m_pPlayer->m_DieTick = Server()->Tick();
	m_Alive = false;
	GameServer()->m_World.RemoveEntity(this);
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());
}

void CCharacter::Melt(int MelterID)
{
	// Unfreeze
	m_FreezeTicks = 0;
	m_DeepFreeze = false;
	m_ActiveWeapon = m_LastWeapon;
	m_MeltTicks = 0;

	if (GameServer()->IsValidCID(MelterID))
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "You melted %s", Server()->ClientName(m_pPlayer->GetCID()));
		GameServer()->SendBroadcast(aBuf, MelterID);
		str_format(aBuf, sizeof(aBuf), "%s melted you", Server()->ClientName(MelterID));
		GameServer()->SendBroadcast(aBuf, m_pPlayer->GetCID());

		GameServer()->m_apPlayers[MelterID]->m_Score++;
		if (g_Config.m_SvLoltextShow)
			GameServer()->CreateLolText(GameServer()->GetPlayerChar(MelterID), "+1");
	}

	if (GameServer()->m_pController->IsIFreeze())
	{
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
		if (g_Config.m_SvIFreezeMeltRespawn)
			KillChar();
	}
}

bool CCharacter::TakeWeapon(int Weapon)
{
	m_aWeapons[Weapon].m_Got = false;
	// if active weapon search another
	if (m_ActiveWeapon == Weapon)
	{
		int k = 0;
		for (int i = 0; i < NUM_WEAPONS - 1; i++)
			if (m_aWeapons[i].m_Got)
			{
				k = i;
				break;
			}
		// if no weapon found give and set default weapon
		if (k == 0)
			GiveWeapon(WEAPON_HAMMER, 0);
		SetWeapon(k);

		return false;
	}
	return true;
}

bool CCharacter::Spawnprotected()
{
	return m_SpawnProtectTick > Server()->Tick();
}

int CCharacter::StrLeftComp(const char *pOrigin, const char *pSub)
{
	const char *pSlide = pOrigin;
	while(*pSlide && *pSub)
	{
		if(*pSlide == *pSub)
		{
			pSlide++;
			pSub++;

			if(*pSub == '\0' && (*pSlide == ' ' || *pSlide == '\0'))
				return pSlide - pOrigin;
		}
		else
			return 0;
	}
	return 0;
}

void CCharacter::SetHealth(int amount) {
	m_Health = amount;
}

void CCharacter::SetShields(int amount) {
	m_Armor = amount;
}