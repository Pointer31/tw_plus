/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/shared/config.h>

#include <game/mapitems.h>

#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>

#include "htf.h"


CGameControllerHTF::CGameControllerHTF(class CGameContext *pGameServer, int TypeFlags)
: IGameController(pGameServer)
{
	m_Flags = TypeFlags;
	if (TypeFlags&IGameController::GAMETYPE_ISTEAM)
		m_pGameType = (IsInstagib()) ? "iTHTF" : "THTF";
	else
		m_pGameType = (IsInstagib()) ? "iHTF" : "HTF";

	m_apFlags[0] = 0;
	m_apFlags[1] = 0;
	m_Flags = TypeFlags;
	m_flagstand_temp_i_0 = 0;
	m_flagstand_temp_i_1 = 0;
	m_GameFlags = GAMEFLAG_FLAGS;
	if (TypeFlags&IGameController::GAMETYPE_ISTEAM)
		m_GameFlags |= GAMEFLAG_TEAMS;
}

// returns 0 if no anticamper is happening, 1 if warning, 2 if flag is reset
int CGameControllerHTF::Anticamper(CFlag* F)
{
	if (g_Config.m_SvHTFAnticamper) {
		if (GameServer()->m_World.m_Paused || F->m_pCarryingCharacter->Frozen())
		{
			F->m_pCarryingCharacter->m_CampTick = -1;
			return 0;
		}

		int AnticamperTime = g_Config.m_SvAnticamperTime;
		int AnticamperRange = g_Config.m_SvAnticamperRange;

		if (F->m_pCarryingCharacter->m_CampTick == -1)
		{
			F->m_pCarryingCharacter->m_CampPos = F->m_pCarryingCharacter->m_Pos;
			F->m_pCarryingCharacter->m_CampTick = Server()->Tick() + Server()->TickSpeed() * AnticamperTime;
			F->m_pCarryingCharacter->m_SentCampMsg = false;
		}

		// Check if the player is moving
		if ((F->m_pCarryingCharacter->m_CampPos.x - F->m_pCarryingCharacter->m_Pos.x >= (float)AnticamperRange || F->m_pCarryingCharacter->m_CampPos.x - F->m_pCarryingCharacter->m_Pos.x <= -(float)AnticamperRange) ||
			(F->m_pCarryingCharacter->m_CampPos.y - F->m_pCarryingCharacter->m_Pos.y >= (float)AnticamperRange || F->m_pCarryingCharacter->m_CampPos.y - F->m_pCarryingCharacter->m_Pos.y <= -(float)AnticamperRange))
		{
			F->m_pCarryingCharacter->m_CampTick = -1;
		}

		
		if ((F->m_pCarryingCharacter->m_CampTick <= Server()->Tick()) && (F->m_pCarryingCharacter->m_CampTick > 0))
		{ // Reset Flag
			F->m_pCarryingCharacter->m_CampTick = -1;
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "");
			GameServer()->SendBroadcast(aBuf, F->m_pCarryingCharacter->GetPlayer()->GetCID());
			F->Reset();
			return 2;
		} else if (F->m_pCarryingCharacter->m_CampTick <= Server()->Tick() + Server()->TickSpeed() * AnticamperTime / 2 && F->m_pCarryingCharacter->m_CampTick != -1) 
		{ // Send Warning
			if (!F->m_pCarryingCharacter->m_SentCampMsg) {
				char aBuf[128];
				int score = F->m_pCarryingCharacter->GetPlayer()->m_Score;
				if (IsTeamplay())
					score = m_aTeamscore[F->m_pCarryingCharacter->GetPlayer()->GetTeam()];
				str_format(aBuf, sizeof(aBuf), "                   %d/%ds\nANTICAMPER: move or lose flag", score, g_Config.m_SvScorelimit);
				GameServer()->SendBroadcast(aBuf, F->m_pCarryingCharacter->GetPlayer()->GetCID());
				F->m_pCarryingCharacter->m_SentCampMsg = true;
			}
			return 1;
		}
	}
	return 0;
}

void CGameControllerHTF::Tick()
{
	IGameController::Tick();

	if(GameServer()->m_World.m_ResetRequested || GameServer()->m_World.m_Paused)
		return;

	for(int fi = 0; fi < 2; fi++)
	{
		CFlag *F = m_apFlags[fi];

		if(!F)
			continue;

		// flag hits death-tile or left the game layer (or no-flag zone), reset it
		if(GameServer()->Collision()->GetCollisionAt(F->m_Pos.x, F->m_Pos.y)&CCollision::COLFLAG_DEATH || F->GameLayerClipped(F->m_Pos)
			|| GameServer()->Collision()->GetCollisionAtNew(F->m_Pos.x, F->m_Pos.y) == TILE_NOFLAG)
		{
			GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "flag_return");
			GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
			if (F->m_pCarryingCharacter)
				GameServer()->SendBroadcast(" ", F->m_pCarryingCharacter->GetPlayer()->GetCID());
			F->Reset();
			continue;
		}

		//
		if(F->m_pCarryingCharacter)
		{
			// update flag position
			F->m_Pos = F->m_pCarryingCharacter->m_Pos;

			CPlayer *player = F->m_pCarryingCharacter->GetPlayer();

			player->m_ScoreTick++;

			int anticamp = Anticamper(F);

			if (player->m_ScoreTick >= Server()->TickSpeed()) {
				player->m_ScoreTick = 0;
				player->m_Score++;
				m_aTeamscore[player->GetTeam()]++;

				int score = player->m_Score;
				if (IsTeamplay())
					score = m_aTeamscore[player->GetTeam()];

				char aBuf[128];
				if (anticamp == 0)
					str_format(aBuf, sizeof(aBuf), "%d/%ds", score, g_Config.m_SvScorelimit);
				else if (anticamp == 1)
					str_format(aBuf, sizeof(aBuf), "                   %d/%ds\nANTICAMPER: move or lose flag", score, g_Config.m_SvScorelimit);
				else if (anticamp == 2)
					str_format(aBuf, sizeof(aBuf), "");
				GameServer()->SendBroadcast(aBuf, player->GetCID());
			}
		}
		else
		{
			// teleports
			if (GameServer()->Collision()->GetCollisionAt(F->m_Pos.x, F->m_Pos.y)&CCollision::COLFLAG_TELEONE)
			{
				if (!F->m_inTele)
				{
					F->m_inTele = true;
					int x = GameServer()->Collision()->getTeleX(0);
					int y = GameServer()->Collision()->getTeleY(0);
					int tx = GameServer()->Collision()->getTeleX(1);
					int ty = GameServer()->Collision()->getTeleY(1);
					vec2 start = {(float)x, (float)y};
					vec2 end = {(float)tx, (float)ty};
					//vec2 oneTile = {ms_PhysSize * 1.5, 0};
					F->m_Pos = F->m_Pos - start * 32 + end * 32;
				}
				//std::cout << "TELE 1" << std::endl;
				// do some teleporting
			}
			else if (GameServer()->Collision()->GetCollisionAt(F->m_Pos.x, F->m_Pos.y)&CCollision::COLFLAG_TELETWO)
			{
				if (!F->m_inTele)
				{
					F->m_inTele = true;
					int x = GameServer()->Collision()->getTeleX(1);
					int y = GameServer()->Collision()->getTeleY(1);
					int tx = GameServer()->Collision()->getTeleX(0);
					int ty = GameServer()->Collision()->getTeleY(0);
					vec2 start = {(float)x, (float)y};
					vec2 end = {(float)tx, (float)ty};
					//vec2 oneTile = {ms_PhysSize * 1.5, 0};
					F->m_Pos = F->m_Pos - start * 32 + end * 32;
				}
				//std::cout << "TELE 2" << std::endl;
			}
			else
			{
				// the flag is not in a teleport
				F->m_inTele = false;
			}

			CCharacter *apCloseCCharacters[MAX_CLIENTS];
			int Num = GameServer()->m_World.FindEntities(F->m_Pos, CFlag::ms_PhysSize, (CEntity**)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			for(int i = 0; i < Num; i++)
			{
				if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(F->m_Pos, apCloseCCharacters[i]->m_Pos, NULL, NULL))
					continue;

				// a person cannot take two flags at the same time
				if (m_apFlags[1-fi] != NULL && m_apFlags[1-fi]->m_pCarryingCharacter == apCloseCCharacters[i])
					return;

				// take the flag
				if(F->m_AtStand)
				{
					// m_aTeamscore[fi^1]++;
					F->m_GrabTick = Server()->Tick();
				}

				F->m_AtStand = 0;
				F->m_pCarryingCharacter = apCloseCCharacters[i];
				F->m_pCarryingCharacter->m_CampTick = -1;
				F->m_pCarryingCharacter->GetPlayer()->m_Score += 1;
				if(g_Config.m_SvLoltextShow)
					GameServer()->CreateLolText(F->m_pCarryingCharacter, "+1");

				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "flag_grab player='%d:%s'",
					F->m_pCarryingCharacter->GetPlayer()->GetCID(),
					Server()->ClientName(F->m_pCarryingCharacter->GetPlayer()->GetCID()));
				GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

				for(int c = 0; c < MAX_CLIENTS; c++)
				{
					CPlayer *pPlayer = GameServer()->m_apPlayers[c];
					if(!pPlayer)
						continue;

					if(pPlayer->GetTeam() == TEAM_SPECTATORS && pPlayer->m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[pPlayer->m_SpectatorID] && GameServer()->m_apPlayers[pPlayer->m_SpectatorID]->GetTeam() == fi)
						GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, c);
					else if(pPlayer != F->m_pCarryingCharacter->GetPlayer())
						GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, c);
					else
						GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_PL, c);
				}

				int score = F->m_pCarryingCharacter->GetPlayer()->m_Score;
				if (IsTeamplay())
					score = m_aTeamscore[F->m_pCarryingCharacter->GetPlayer()->GetTeam()];
				str_format(aBuf, sizeof(aBuf), "%d/%ds", score, g_Config.m_SvScorelimit);
				GameServer()->SendBroadcast(aBuf, F->m_pCarryingCharacter->GetPlayer()->GetCID());

				// demo record entry
				GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, -2);
				break;
			}

			if(!F->m_pCarryingCharacter && !F->m_AtStand)
			{
				if(Server()->Tick() > F->m_DropTick + Server()->TickSpeed()*30)
				{
					GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
					F->Reset();
				}
				else
				{
					F->m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
					GameServer()->Collision()->MoveBox(&F->m_Pos, &F->m_Vel, vec2(F->ms_PhysSize, F->ms_PhysSize), 0.5f);
				}
			}
		}
	}
}

bool CGameControllerHTF::OnEntity(int Index, vec2 Pos)
{
	if(IGameController::OnEntity(Index, Pos))
		return true;

	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED) Team = TEAM_RED;
	if(Index == ENTITY_FLAGSTAND_BLUE) Team = TEAM_BLUE;

	if(!(Team == -1 || m_apFlags[Team])) {
		CFlag *F = new CFlag(&GameServer()->m_World, Team);
		//F->m_StandPos = Pos;
		F->m_Pos = Pos;
		m_apFlags[Team] = F;
		GameServer()->m_World.InsertEntity(F);
	}

	if (Team == TEAM_RED && m_flagstand_temp_i_0 < 10) {
		//m_flagstands_0[m_flagstand_temp_i_0] = Pos;
		m_apFlags[Team]->m_StandPositions[m_flagstand_temp_i_0] = Pos;
		m_flagstand_temp_i_0++;
		m_apFlags[Team]->m_no_stands = m_flagstand_temp_i_0;
	}
	if (Team == TEAM_BLUE && m_flagstand_temp_i_1 < 10) {
		//m_flagstands_1[m_flagstand_temp_i_1] = Pos;
		m_apFlags[Team]->m_StandPositions[m_flagstand_temp_i_1] = Pos;
		m_flagstand_temp_i_1++;
		m_apFlags[Team]->m_no_stands = m_flagstand_temp_i_1;
	}

	if (Team == -1) {
		return false;
	}

	return true;
}

int CGameControllerHTF::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponID)
{
	IGameController::OnCharacterDeath(pVictim, pKiller, WeaponID);
	int HadFlag = 0;

	// drop flags
	for(int i = 0; i < 2; i++)
	{
		CFlag *F = m_apFlags[i];
		if(F && pKiller && pKiller->GetCharacter() && F->m_pCarryingCharacter == pKiller->GetCharacter())
			HadFlag |= 2;
		if(F && F->m_pCarryingCharacter == pVictim)
		{
			GameServer()->SendBroadcast(" ", F->m_pCarryingCharacter->GetPlayer()->GetCID());
			GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
			F->m_DropTick = Server()->Tick();
			F->m_pCarryingCharacter = 0;
			F->m_Vel = vec2(0,0);
			pVictim->GetPlayer()->m_Stats.m_LostFlags++;

			if(pKiller && pKiller->GetTeam() != pVictim->GetPlayer()->GetTeam())
			{
				if(g_Config.m_SvLoltextShow)
					GameServer()->CreateLolText(pKiller->GetCharacter(), "+1");
				pKiller->m_Score++;
			}

			HadFlag |= 1;
		}
	}

	return HadFlag;
}

void CGameControllerHTF::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameData *pGameDataObj = (CNetObj_GameData *)Server()->SnapNewItem(NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData));
	if(!pGameDataObj)
		return;

	pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
	pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];

	if(m_apFlags[TEAM_RED])
	{
		if(m_apFlags[TEAM_RED]->m_AtStand)
			pGameDataObj->m_FlagCarrierRed = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_RED]->m_pCarryingCharacter && m_apFlags[TEAM_RED]->m_pCarryingCharacter->GetPlayer())
			pGameDataObj->m_FlagCarrierRed = m_apFlags[TEAM_RED]->m_pCarryingCharacter->GetPlayer()->GetCID();
		else
			pGameDataObj->m_FlagCarrierRed = FLAG_TAKEN;
	}
	else
		pGameDataObj->m_FlagCarrierRed = FLAG_MISSING;
	if(m_apFlags[TEAM_BLUE])
	{
		if(m_apFlags[TEAM_BLUE]->m_AtStand)
			pGameDataObj->m_FlagCarrierBlue = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_BLUE]->m_pCarryingCharacter && m_apFlags[TEAM_BLUE]->m_pCarryingCharacter->GetPlayer())
			pGameDataObj->m_FlagCarrierBlue = m_apFlags[TEAM_BLUE]->m_pCarryingCharacter->GetPlayer()->GetCID();
		else
			pGameDataObj->m_FlagCarrierBlue = FLAG_TAKEN;
	}
	else
		pGameDataObj->m_FlagCarrierBlue = FLAG_MISSING;
}

