#include "hidnsek.h"
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <cstdio>

CGameControllerHidNSek::CGameControllerHidNSek(CGameContext *pGameServer)
: IGameController(pGameServer)
{
    m_pGameType = "HidNSek";
    m_GameFlags = GAMEFLAG_SURVIVAL;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_HidNSekPlayers[i].SetID(i);
		m_HidNSekPlayers[i].m_Ball = Server()->SnapNewID();
		m_HidNSekPlayers[i].m_IsSeeker = false;
	}
}

CGameControllerHidNSek::~CGameControllerHidNSek()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		Server()->SnapFreeID(m_HidNSekPlayers[i].m_Ball);
	}
}

void CGameControllerHidNSek::Tick()
{
	if(!Config()->m_SvTimelimit)
		Config()->m_SvTimelimit = 1;

	if(!HasEnoughPlayers())
		SetGameState(IGS_WARMUP_GAME, TIMER_INFINITE);

	IGameController::Tick();

	if(!GameServer()->m_World.m_Paused && m_DoResetSeekers)
	{
		ResetSeekers();
		m_DoResetSeekers = false;
		m_ToldSeekers = false;
	}

	if(!IsGamePaused() && HasEnoughPlayers())
	{
		if(Seekers() < Config()->m_SvHidNSekSeekers && Seekers() <= GetRealPlayerNum()/2)
		{
			for(auto *pPlayer : GameServer()->m_apPlayers)
			{
				if(!pPlayer)
					continue;

				if(m_HidNSekPlayers[pPlayer->GetCID()].m_WasSeeker)
					continue;

				if(rand() % 10)
				{
					m_HidNSekPlayers[pPlayer->GetCID()].SetSeeker(true);
					SendSkinChangeHNS(pPlayer->GetCID(), -1, 65408);
					if(CCharacter *pChr = pPlayer->GetCharacter())
					{
						pChr->RemoveWeapon(WEAPON_GUN);
						pChr->RemoveWeapon(WEAPON_HAMMER);
						pChr->GiveWeapon(Config()->m_SvHidNSekSeekerWeapon, -1);
						pChr->SetWeapon(Config()->m_SvHidNSekSeekerWeapon);
					}
					pPlayer->m_RespawnDisabled = false;
					m_HidNSekPlayers[pPlayer->GetCID()].m_FrozenTick = Server()->Tick() + (Server()->TickSpeed() * Config()->m_SvHidNSekFreezeStart - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit);
				}

				if(Seekers() >= Config()->m_SvHidNSekSeekers || Seekers() > GetRealPlayerNum()/2)
					break;
			}
		}
		else if(!m_ToldSeekers)
		{
			for(auto *pPlayer : GameServer()->m_apPlayers)
			{
				if(!pPlayer)
					continue;

				if(m_HidNSekPlayers[pPlayer->GetCID()].m_IsSeeker)
				{
					GameServer()->SendChat(pPlayer->GetCID(), CHAT_TEAM, pPlayer->GetCID(), "You are a Seeker now! Kill the Hiders!");
				}
				else
				{
					GameServer()->SendChat(pPlayer->GetCID(), CHAT_TEAM, pPlayer->GetCID(), "You are a Hider now! Run away from the Seekers!");
				}
			}
			m_ToldSeekers = true;
		}
	}
}

void CGameControllerHidNSek::CHidNSekPlayer::Reset()
{
    SetSeeker(false);
}

void CGameControllerHidNSek::CHidNSekPlayer::SetSeeker(bool set)
{
	m_WasSeeker = false;
	if(set)
	{
		m_IsSeeker = true;
	}
	else
	{
		if(m_IsSeeker)
			m_WasSeeker = true;
		m_IsSeeker = false;
	}
}

void CGameControllerHidNSek::CHidNSekPlayer::SetID(int id)
{
	if(m_SelfID < 0)
		m_SelfID = id;
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

void CGameControllerHidNSek::OnCharacterSpawn(CCharacter *pChr)
{
	IGameController::OnCharacterSpawn(pChr);

	// give start equipment
	pChr->IncreaseArmor(10);
	// prevent respawn
	pChr->GetPlayer()->m_RespawnDisabled = GetStartRespawnState();

	pChr->RemoveWeapon(WEAPON_GUN);
	pChr->SetWeapon(WEAPON_HAMMER);

	if(m_HidNSekPlayers[pChr->GetPlayer()->GetCID()].m_IsSeeker)
	{
		pChr->RemoveWeapon(WEAPON_HAMMER);
		pChr->GiveWeapon(Config()->m_SvHidNSekSeekerWeapon, -1);
		pChr->SetWeapon(Config()->m_SvHidNSekSeekerWeapon);
	}
}

bool CGameControllerHidNSek::OnCharacterSnap(CCharacter *pChar, int SnappingClient)
{
	if(SnappingClient < 0 || SnappingClient >= MAX_CLIENTS)
		return false;

	CPlayer * pPlayer = GameServer()->m_apPlayers[SnappingClient];
	if(!pPlayer)
		return false;

	if(m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_IsSeeker)
		return false;

	CCharacter *pOther = GameServer()->GetPlayerChar(SnappingClient);
	if(!pOther)
		return true;

	if(m_HidNSekPlayers[SnappingClient].m_IsSeeker && GameServer()->Collision()->FastIntersectLine(pChar->GetPos(), pOther->GetPos(), nullptr, nullptr))
		return true;

	if(!m_HidNSekPlayers[pChar->GetPlayer()->GetCID()].m_IsSeeker)
	{
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
	}

    return false;
}

bool CGameControllerHidNSek::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
	if(From < 0 || From >= MAX_CLIENTS)
		return false;

	if(m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_IsSeeker)
	{
		Character.GetCore().m_Vel += Force;
		if(m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_FrozenTick == -1 ||
			m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_FrozenTick <= Server()->Tick() - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit)
		{
			m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_FrozenTick = Server()->Tick();
		}
		return true;
	}

	if(m_HidNSekPlayers[From].m_IsSeeker == m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_IsSeeker)
	{
		Character.GetCore().m_Vel += Force;
		return true;
	}

	if(m_HidNSekPlayers[From].m_IsSeeker && !m_HidNSekPlayers[Character.GetPlayer()->GetCID()].m_IsSeeker)
	{
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
		Character.Die(From, Weapon);
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
	if(ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;

	if(!m_HidNSekPlayers[ClientID].m_IsSeeker)
		return false;

    return true;
}

bool CGameControllerHidNSek::CanFireWeapon(CCharacter &Char)
{
	if(m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick != -1 &&
		m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick > Server()->Tick() - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit)
	{
		return false;
	}	
    return true;
}

// game
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
			if(GameServer()->m_apPlayers[i] && !m_HidNSekPlayers[i].m_IsSeeker && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				GameServer()->m_apPlayers[i]->m_Score++;
		}
		m_GameStartTick = Server()->Tick(); // hack to not end match
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
			if(GameServer()->m_apPlayers[i] && !m_HidNSekPlayers[i].m_IsSeeker && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS &&
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
				if(GameServer()->m_apPlayers[i] && m_HidNSekPlayers[i].m_IsSeeker && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
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
		m_HidNSekPlayers[i].Reset();
		SendSkinChangeHNS(i, -1, 0);
		if(!GameServer()->m_apPlayers[i])
			m_HidNSekPlayers[i].m_WasSeeker = false;
		m_HidNSekPlayers[i].m_FrozenTick = -1;
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
	if(m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick != -1 &&
		m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick > Server()->Tick() - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit)
	{
		pInput->m_Direction = 0;
		pInput->m_Hook = 0;
		pInput->m_Jump = 0;
	}
}

void CGameControllerHidNSek::HandleCharacterSnap(CCharacter &Char, CNetObj_Character *pCharObj, int SnappingClient)
{
	if(m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick != -1 &&
		m_HidNSekPlayers[Char.GetPlayer()->GetCID()].m_FrozenTick > Server()->Tick() - Server()->TickSpeed() * Config()->m_SvHidNSekFreezeHit)
	{
		pCharObj->m_Weapon = WEAPON_NINJA;
	}
}
