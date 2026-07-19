/*
    HidNSek
    Copyright (C) Benjamín Gajardo (also known as +KZ)

    This program is shared under the PLUSKAIZO LICENSE, you should have received
    a copy of the license along with the program.
*/

#include "hidnsek.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <cstdio>
#include <game/server/localization.h>

int CGameControllerHidNSek::m_SpecialMode = 0;

CGameControllerHidNSek::CGameControllerHidNSek(CGameContext *pGameServer)
: IGameController(pGameServer)
{
    m_pGameType = "HidNSek";
    m_GameFlags = GAMEFLAG_SURVIVAL;
	m_Instagib = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_HidNSekPlayers[i].m_Ball = Server()->SnapNewID();
		m_HidNSekPlayers[i].m_IsSeeker = false;
		m_HidNSekPlayers[i].m_Infected = false;
		m_HidNSekPlayers[i].m_FrozenSpecial = false;
		m_HidNSekPlayers[i].m_SentSpecialModeBroadcast = false;
	}

	if(Config()->m_SvHidNSekSpecialModes == -1)
	{
		m_SpecialMode++;
		m_SpecialMode %= (MAX_SPECIAL_MODES);
	}
	else
	{
		m_SpecialMode = Config()->m_SvHidNSekSpecialModes;
	}

	m_AlreadySetSeekers = false;
}

CGameControllerHidNSek::~CGameControllerHidNSek()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		Server()->SnapFreeID(m_HidNSekPlayers[i].m_Ball);
	}
}

const char* CGameControllerHidNSek::GetGameHelpText()
{
	return "Gametype: Hide & Seek, Seekers will try to kill the Hiders, Hiders need to run!";
}

void CGameControllerHidNSek::Tick()
{
	if(!Config()->m_SvTimelimit)
		Config()->m_SvTimelimit = 1;

	IGameController::Tick();

	if(!GameServer()->m_World.m_Paused && m_DoResetSeekers)
	{
		ResetSeekers();
		m_DoResetSeekers = false;
		m_ToldSeekers = false;
		m_AlreadySetSeekers = false;
	}

	if(!IsGamePaused() && HasEnoughPlayers())
	{
		if(!m_AlreadySetSeekers && Seekers() < Config()->m_SvHidNSekSeekers && Seekers() < GetRealPlayerNum()/2)
		{
			bool Found = false;
			while(!Found)
			{
				for(auto *pPlayer : GameServer()->m_apPlayers)
				{
					if(!pPlayer)
						continue;

					if(pPlayer->GetTeam() == TEAM_SPECTATORS)
						continue;

					if(m_HidNSekPlayers[pPlayer->GetCID()].m_IsSeeker)
						continue;

					if(m_HidNSekPlayers[pPlayer->GetCID()].m_WasSeeker)
						continue;

					SetPlayerSeeker(pPlayer->GetCID(), true);
					if (CCharacter *pChr = pPlayer->GetCharacter())
					{
						pChr->RemoveWeapon(WEAPON_GUN);
						pChr->RemoveWeapon(WEAPON_HAMMER);
						pChr->GiveWeapon(Config()->m_SvHidNSekSeekerWeapon, -1);
						pChr->SetWeapon(Config()->m_SvHidNSekSeekerWeapon);
					}
					pPlayer->m_RespawnDisabled = false;
					if(Server()->Tick() - Server()->TickSpeed() * 5 < m_GameStartTick)
						m_HidNSekPlayers[pPlayer->GetCID()].m_FrozenTick = Server()->Tick() + (Server()->TickSpeed() * Config()->m_SvHidNSekFreezeStart - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit);

					Found = true;

					if(Seekers() >= Config()->m_SvHidNSekSeekers || Seekers() >= GetRealPlayerNum()/2)
						break;
				}
				if(!Found)
				{
					for(auto *pPlayer : GameServer()->m_apPlayers)
					{
						if(!pPlayer)
							continue;
						
						m_HidNSekPlayers[pPlayer->GetCID()].m_WasSeeker = false;
					}
				}
			}

			if(Seekers() >= Config()->m_SvHidNSekSeekers && Seekers() >= GetRealPlayerNum()/2)
				m_AlreadySetSeekers = true;
		}
		else if(!m_ToldSeekers)
		{
			for(auto *pPlayer : GameServer()->m_apPlayers)
			{
				if(!pPlayer)
					continue;

				if(pPlayer->GetTeam() == TEAM_SPECTATORS)
					continue;

				if(m_HidNSekPlayers[pPlayer->GetCID()].m_IsSeeker)
				{
					SendChatMsg(pPlayer->GetCID(), pPlayer->GetCID(), CHAT_WHISPER, "You are a Seeker now! Kill the Hiders!");
				}
				else
				{
					SendChatMsg(pPlayer->GetCID(), pPlayer->GetCID(), CHAT_WHISPER, "You are a Hider now! Run away from the Seekers!");
				}
			}
			m_ToldSeekers = true;
		}

		//hint sound each 15 seconds
		if(
			Config()->m_SvHidNSekHintSound &&
			(m_GameStartTick - Server()->Tick()) % (Server()->TickSpeed() * 15) == 0
		)
		{
			for(int i = 0; i < MAX_CLIENTS; ++i)
			{
				if(
					GameServer()->m_apPlayers[i] &&
					GameServer()->m_apPlayers[i]->GetCharacter() &&
					GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS &&
					!m_HidNSekPlayers[i].m_IsSeeker
				)
				{
					GameServer()->CreateSound(GameServer()->m_apPlayers[i]->GetCharacter()->GetPos(), SOUND_PLAYER_PAIN_LONG);
				}
			}
		}
	}
	else if(!IsGamePaused() && !HasEnoughPlayers())
	{
		// not enough players, so always allow respawn
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				GameServer()->m_apPlayers[i]->m_RespawnDisabled = false;
		}
	}

	if(Server()->Tick() % Server()->TickSpeed() == 0)
	{
		//keep ninja active
		int ninjas = 0;

		if(Config()->m_SvHidNSekHiderWeapon == WEAPON_NINJA)
		{
			ninjas |= 1;
		}

		if(Config()->m_SvHidNSekSeekerWeapon == WEAPON_NINJA)
		{
			ninjas |= 1 << 1;
		}

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			UpdatePlayerSkin(i);

			if(!GameServer()->m_apPlayers[i])
				continue;


			if(
				((m_HidNSekPlayers[i].m_IsSeeker ? (1 << 1) : 1) & ninjas) &&
				GameServer()->m_apPlayers[i]->GetCharacter() &&
				GameServer()->m_apPlayers[i]->GetCharacter()->GetActiveWeapon() == WEAPON_NINJA
			)
			{
				GameServer()->m_apPlayers[i]->GetCharacter()->UpdateNinjaActivationTick();
			}
		}
	}
}

void CGameControllerHidNSek::SetPlayerSeeker(int ClientID, bool set, bool infected)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS)
		return;

	if(set)
	{
		m_HidNSekPlayers[ClientID].m_IsSeeker = true;
	}
	else
	{
		if(m_HidNSekPlayers[ClientID].m_IsSeeker && !m_HidNSekPlayers[ClientID].m_Infected)
			m_HidNSekPlayers[ClientID].m_WasSeeker = true;
		m_HidNSekPlayers[ClientID].m_IsSeeker = false;
	}

	if(infected)
		m_HidNSekPlayers[ClientID].m_Infected = infected;

	UpdatePlayerSkin(ClientID);
}

void CGameControllerHidNSek::SendSkinChangeHNS(int ClientID, int TargetID, int ColorBody)
{
	if(!GameServer()->m_apPlayers[ClientID])
		return;
	CNetMsg_Sv_SkinChange Msg;
	Msg.m_ClientID = ClientID;
	for(int p = 0; p < NUM_SKINPARTS; p++)
	{
		Msg.m_apSkinPartNames[p] = GameServer()->m_apPlayers[ClientID]->m_TeeInfos.m_aaSkinPartNames[p];
		Msg.m_aUseCustomColors[p] = GameServer()->m_apPlayers[ClientID]->m_TeeInfos.m_aUseCustomColors[p];
		Msg.m_aSkinPartColors[p] = ColorBody ? (p == 5 ? 0 : ColorBody) : GameServer()->m_apPlayers[ClientID]->m_TeeInfos.m_aSkinPartColors[p];
	}
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_NORECORD, TargetID);
}

void CGameControllerHidNSek::UpdatePlayerSkin(int ClientID)
{
	CPlayer * pPlayer = GameServer()->m_apPlayers[ClientID];
	if(!pPlayer)
		return;

	if(m_HidNSekPlayers[pPlayer->GetCID()].m_IsSeeker)
	{
		if(m_HidNSekPlayers[pPlayer->GetCID()].m_Infected)
			SendSkinChangeHNS(pPlayer->GetCID(), -1, 0xFF08);
		else
			SendSkinChangeHNS(pPlayer->GetCID(), -1, 65408);
	}
	else
		SendSkinChangeHNS(pPlayer->GetCID(), -1, 0);
}

bool CGameControllerHidNSek::IsPlayerFrozen(int ClientID)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;
    return m_HidNSekPlayers[ClientID].m_FrozenSpecial || (m_HidNSekPlayers[ClientID].m_FrozenTick != -1 &&
		m_HidNSekPlayers[ClientID].m_FrozenTick > Server()->Tick() - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit);
}

void CGameControllerHidNSek::OnCharacterSpawn(CCharacter *pChr)
{
	IGameController::OnCharacterSpawn(pChr);

	// give start equipment
	pChr->IncreaseArmor(10);
	// prevent respawn
	if(m_HidNSekPlayers[pChr->GetPlayer()->GetCID()].m_IsSeeker)
		pChr->GetPlayer()->m_RespawnDisabled = false;
	else
		pChr->GetPlayer()->m_RespawnDisabled = GetStartRespawnState();

	for(int i = WEAPON_GUN; i <= WEAPON_LASER; i++)
		pChr->RemoveWeapon(i);
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->SetWeapon(WEAPON_HAMMER);

	if(m_HidNSekPlayers[pChr->GetPlayer()->GetCID()].m_IsSeeker)
	{
		pChr->RemoveWeapon(WEAPON_HAMMER);
		if(Config()->m_SvHidNSekSeekerWeapon == WEAPON_NINJA)
		{
			pChr->GiveNinja();
		}
		else
		{
			pChr->GiveWeapon(Config()->m_SvHidNSekSeekerWeapon, -1);
			pChr->SetWeapon(Config()->m_SvHidNSekSeekerWeapon);
		}
	}
	else
	{
		pChr->RemoveWeapon(WEAPON_HAMMER);
		//-1 is no weapon
		if(Config()->m_SvHidNSekHiderWeapon >= 0)
		{
			if(Config()->m_SvHidNSekHiderWeapon == WEAPON_NINJA)
			{
				pChr->GiveNinja();
			}
			else
			{
				pChr->GiveWeapon(Config()->m_SvHidNSekHiderWeapon, -1);
				pChr->SetWeapon(Config()->m_SvHidNSekHiderWeapon);
			}
		}
	}

	if(!m_HidNSekPlayers[pChr->GetPlayer()->GetCID()].m_SentSpecialModeBroadcast)
	{
		switch (m_SpecialMode)
		{
		case SPECIAL_MODE_INFECTION:
			GameServer()->SendBroadcast("Mode: Infection", pChr->GetPlayer()->GetCID());
			break;
		case SPECIAL_MODE_FREEZE:
			GameServer()->SendBroadcast("Mode: Freeze", pChr->GetPlayer()->GetCID());
			break;
		case SPECIAL_MODE_NONE:
			GameServer()->SendBroadcast("Mode: Normal", pChr->GetPlayer()->GetCID());
			break;
		case SPECIAL_MODE_KAIZOINSTA:
			GameServer()->SendBroadcast("Mode: Classic", pChr->GetPlayer()->GetCID());
			break;
		}

		m_HidNSekPlayers[pChr->GetPlayer()->GetCID()].m_SentSpecialModeBroadcast = true;
	}
}

bool CGameControllerHidNSek::OnCharacterSnap(CCharacter *pChar, int SnappingClient)
{
	//always snap seekers
	//but in classic do it like in Kaizo-Insta
	if(
		m_SpecialMode != SPECIAL_MODE_KAIZOINSTA &&
		m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_IsSeeker
	)
	{
		if((m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_FrozenTick <= Server()->Tick() - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit && 
		m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_FrozenTick + Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHitProtection > Server()->Tick() - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit))
		{
			CNetObj_Pickup *pPickup = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_Ball, sizeof(CNetObj_Pickup)));

			if(pPickup)
			{
				vec2 postemp;
				postemp.x = pChar->GetPos().x + 32*sin((float)Server()->Tick() / 25.0);
				postemp.y = pChar->GetPos().y + 32*cos((float)Server()->Tick() / 25.0);

				pPickup->m_Type = PICKUP_ARMOR;
				pPickup->m_X = round_to_int(postemp.x);
				pPickup->m_Y = round_to_int(postemp.y);
			}
		}

		return false;
	}

	if(SnappingClient < 0 || SnappingClient >= MAX_CLIENTS)
		return false;

	//always snap frozen hiders
	if(!(m_SpecialMode == SPECIAL_MODE_FREEZE && m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_FrozenSpecial))
	{
		CPlayer * pPlayer = GameServer()->m_apPlayers[SnappingClient];
		if(!pPlayer)
			return false;

		if(
			(
				Config()->m_SvHidNSekShowHidersSpec ||
				m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_IsSeeker //always snap seekers to spectators, even in kaizo-insta mode
			) &&
			pPlayer->GetTeam() == TEAM_SPECTATORS
		)
			return false;

		CCharacter *pOther = GameServer()->GetPlayerChar(SnappingClient);
		if(!pOther)
		{
			return true;
		}

		if(GameServer()->Collision()->FastIntersectLine(pChar->GetPos(), pOther->GetPos(), nullptr, nullptr))
		{
			return true;
		}
		else if(
			m_SpecialMode == SPECIAL_MODE_KAIZOINSTA &&
			m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_IsSeeker
		)
		{
			if(IsPlayerFrozen(pChar->GetPlayer()->GetCID()))
			{
				CNetObj_Pickup *pPickup = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_Ball, sizeof(CNetObj_Pickup)));

				if(pPickup)
				{
					vec2 postemp;
					postemp.x = pChar->GetPos().x + 32*sin((float)Server()->Tick() / 25.0);
					postemp.y = pChar->GetPos().y + 32*cos((float)Server()->Tick() / 25.0);

					pPickup->m_Type = PICKUP_ARMOR;
					pPickup->m_X = round_to_int(postemp.x);
					pPickup->m_Y = round_to_int(postemp.y);
				}
			}
		}
	}

	//snap ball
	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_Ball, sizeof(CNetObj_Projectile)));
	if(pProj)
	{
		vec2 postemp;
		postemp.x = pChar->GetPos().x + 32*sin((float)Server()->Tick() / 25.0);
		postemp.y = pChar->GetPos().y + 32*cos((float)Server()->Tick() / 25.0);

		pProj->m_Type = WEAPON_HAMMER;
		pProj->m_StartTick = Server()->Tick();
		pProj->m_VelX = 0;
		pProj->m_VelY = 0;
		pProj->m_X = round_to_int(postemp.x);
		pProj->m_Y = round_to_int(postemp.y);
	}

    return false;
}

bool CGameControllerHidNSek::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
	if(From < 0 || From >= MAX_CLIENTS)
		return false;

	if(Weapon == WEAPON_GRENADE && Dmg < 3) //fix for grenade
	{
		Character.GetCore().m_Vel += Force;
		return true;
	}

	if(Character.GetPlayer()->GetCID() == From) //no self damage
	{
		Character.GetCore().m_Vel += Force;
		return true;
	}

	if(m_SpecialMode == SPECIAL_MODE_FREEZE && !m_HidNSekPlayers[From].m_IsSeeker && !m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_IsSeeker)
	{
		Character.GetCore().m_Vel += Force;
		m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_FrozenSpecial = false;
		return true;
	}

	if(m_HidNSekPlayers[From].m_IsSeeker == m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_IsSeeker) //do nothing for same team
	{
		Character.GetCore().m_Vel += Force;
		return true;
	}

	if(m_SpecialMode == SPECIAL_MODE_KAIZOINSTA && m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_IsSeeker)
	{
		if(!IsPlayerFrozen(Character.GetPlayer()->GetCID()))
			return false; //do damage
	}

	if(m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_IsSeeker)
	{
		Character.GetCore().m_Vel += Force;
		if(m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_FrozenTick == -1 ||
			m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_FrozenTick <= Server()->Tick() - Server()->TickSpeed() * (Config()->m_SvHidNSekFreezeHit + Config()->m_SvHidNSekFreezeHitProtection))
		{
			m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_FrozenTick = Server()->Tick();
		}
		return true;
	}

	if(m_HidNSekPlayers[From].m_IsSeeker && !m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_IsSeeker)
	{
		Character.GetCore().m_Vel += Force;
		// do damage Hit sound
		if(From >= 0 && From != Character.GetPlayer()->GetCID() && GameServer()->m_apPlayers[From])
		{
			int64 Mask = CmaskOne(From);
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(GameServer()->m_apPlayers[i] && (GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS ||  GameServer()->m_apPlayers[i]->m_DeadSpecMode) &&
					GameServer()->m_apPlayers[i]->GetSpectatorID() == From)
					Mask |= CmaskOne(i);
			}
			GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
		}
		if(m_SpecialMode == SPECIAL_MODE_INFECTION)
		{
			SetPlayerSeeker(Character.GetPlayer()->GetCID(), true, true);
			Character.RemoveWeapon(WEAPON_HAMMER);
			Character.GiveWeapon(Config()->m_SvHidNSekSeekerWeapon, -1);
			Character.SetWeapon(Config()->m_SvHidNSekSeekerWeapon);
		}
		else if(m_SpecialMode == SPECIAL_MODE_FREEZE)
		{
			m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_FrozenSpecial = true;
		}
		else
		{
			Character.Die(From, Weapon);
		}
		return true;
	}

    return false;
}

bool CGameControllerHidNSek::CanChangeSkin(int ClientID)
{
	if(m_HidNSekPlayers[ClientID].m_IsSeeker)
		return false;
    return true;
}

bool CGameControllerHidNSek::CanSpecID(int ClientID)
{
	if(ClientID == -1)
		return true;

	if(ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	if(!m_HidNSekPlayers[ClientID].m_IsSeeker)
		return false;

    return true;
}

bool CGameControllerHidNSek::CanFireWeapon(CCharacter &Char)
{
	if(m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenSpecial || (m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick != -1 &&
		m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick > Server()->Tick() - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit))
	{
		return false;
	}	
    return true;
}

bool CGameControllerHidNSek::DoWincheckMatch()
{
	// gather some stats
	int Topscore = 0;
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (GameServer()->m_apPlayers[i])
		{
			if (GameServer()->m_apPlayers[i]->m_Score > Topscore)
			{
				Topscore = GameServer()->m_apPlayers[i]->m_Score;
			}
		}
	}

	// check score win condition
	if (m_GameInfo.m_ScoreLimit > 0 && Topscore >= m_GameInfo.m_ScoreLimit)
	{
		EndMatch();
		return true;
	}

	return false;
}

void CGameControllerHidNSek::DoWincheckRound()
{
	if(!Seekers())
		return;

	if(Server()->Tick() - Server()->TickSpeed() * 5 < m_GameStartTick)
		return;

	// check for time based win
	if(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && !m_HidNSekPlayers[i].m_IsSeeker && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS &&
				(!GameServer()->m_apPlayers[i]->m_RespawnDisabled ||
				(GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())))
				GameServer()->m_apPlayers[i]->m_Score++;
		}
		EndRound();
		m_DoResetSeekers = true;
		GameServer()->SendChat(-1, CHAT_ALL, -1, "Hiders won this round!");
	}
	else
	{
		// check for survival win
		int AlivePlayerCount = 0;
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && !m_HidNSekPlayers[i].m_IsSeeker && !m_HidNSekPlayers[i].m_FrozenSpecial && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS &&
				(!GameServer()->m_apPlayers[i]->m_RespawnDisabled ||
				(GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())))
			{
				++AlivePlayerCount;
			}
		}

		if(AlivePlayerCount == 0)		// no hiders
		{
			AlivePlayerCount = 0;
			for(int i = 0; i < MAX_CLIENTS; ++i)
			{
				if(GameServer()->m_apPlayers[i] && !m_HidNSekPlayers[i].m_Infected && m_HidNSekPlayers[i].m_IsSeeker &&
					GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				{
					GameServer()->m_apPlayers[i]->m_Score++;
				}
			}
			EndRound();
			m_DoResetSeekers = true;
			GameServer()->SendChat(-1, CHAT_ALL, -1, "Seekers won this round!");
		}
	}
}

void CGameControllerHidNSek::OnPlayerConnect(CPlayer *pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);

	for(auto *pEachPlayer : GameServer()->m_apPlayers)
	{
		if(!pEachPlayer)
			continue;

		if(pEachPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;
	}

	m_HidNSekPlayers[pPlayer->GetCID()].m_SentSpecialModeBroadcast = false;
}

int CGameControllerHidNSek::Seekers()
{
	int a = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (GameServer()->m_apPlayers[i] && m_HidNSekPlayers[i].m_IsSeeker && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS &&
			(!GameServer()->m_apPlayers[i]->m_RespawnDisabled ||
			 (GameServer()->m_apPlayers[i]->GetCharacter() && GameServer()->m_apPlayers[i]->GetCharacter()->IsAlive())))
		{
			a++;
		}
	}
	return a;
}

void CGameControllerHidNSek::ResetSeekers()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		SetPlayerSeeker(i, false);
		if(!GameServer()->m_apPlayers[i])
			m_HidNSekPlayers[i].m_WasSeeker = false;
		m_HidNSekPlayers[i].m_FrozenTick = -1;
		m_HidNSekPlayers[i].m_FrozenSpecial = false;
		m_HidNSekPlayers[i].m_Infected = false;
		UpdatePlayerSkin(i);
	}
}

int CGameControllerHidNSek::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon)
{
	if(!pKiller || Weapon == WEAPON_GAME)
		return 0;

	// update spectator modes for dead players in survival
	if(m_GameFlags&GAMEFLAG_SURVIVAL)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_DeadSpecMode)
				GameServer()->m_apPlayers[i]->UpdateDeadSpecMode();
	}

	return 0;
}

void CGameControllerHidNSek::HandleCharacterInput(class CCharacter &Char, CNetObj_PlayerInput *pInput, bool Predicted)
{
	if(m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenSpecial || (m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick != -1 &&
		m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick > Server()->Tick() - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit))
	{
		pInput->m_Direction = 0;
		pInput->m_Hook = 0;
		pInput->m_Jump = 0;
	}
}

void CGameControllerHidNSek::HandleCharacterSnap(CCharacter &Char, CNetObj_Character *pCharObj, int SnappingClient)
{
	if(m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_IsSeeker)
		pCharObj->m_Emote = EMOTE_ANGRY;

	if(IsPlayerFrozen(Char.GetPlayer()->GetCID()))
	{
		pCharObj->m_Weapon = WEAPON_NINJA;
		pCharObj->m_AmmoCount = m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick + Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit;
		if(m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenSpecial)
			pCharObj->m_Emote = EMOTE_SURPRISE;
		else
			pCharObj->m_Emote = EMOTE_PAIN;
		pCharObj->m_Jumped = 3;
		pCharObj->m_Direction = 0;
	}
}

void CGameControllerHidNSek::HandleDDNetCharacterSnap(CCharacter &Char, CNetObj_DDNetCharacter *pCharObj, int SnappingClient)
{
	if(IsPlayerFrozen(Char.GetPlayer()->GetCID()))
	{
		pCharObj->m_FreezeEnd = -1;
	}
}

void CGameControllerHidNSek::SendChatMsg(int From, int To, int Mode, const char *pText)
{
	//not intended for actual players chatting, but for server messages

	CNetMsg_Sv_Chat Msg;
	Msg.m_Mode = Mode;
	Msg.m_ClientID = From;
	Msg.m_pMessage = Localize(pText);
	Msg.m_TargetID = To;

	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, To);
}
