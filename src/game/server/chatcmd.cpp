/* File is created by Teetime for the TW+ mod
 */

#include "gamecontext.h"
#include <engine/shared/config.h>
#include <stdio.h>

bool CGameContext::ChatCommand(int ClientID, CPlayer* pPlayer, const char* pMessage)
{
	if(pMessage[0] == '/' || pMessage[0] == '!')
		pMessage++;
	else
		return false;

	// AuthLevel > 0  => Moderator/Admin
	int AuthLevel = Server()->IsAuthed(ClientID);

	if(!str_comp(pMessage, "info"))
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "TW+ Mod v.%s created by Teetime.", MOD_VERSION);
		SendChatTarget(ClientID, aBuf);

		SendChatTarget(ClientID, "For a list of available commands type \"/cmdlist\"");

		str_format(aBuf, sizeof(aBuf), "Gametype: %s", GameType());
		SendChatTarget(ClientID, aBuf);

		if(m_pController->IsIFreeze())
				SendChatTarget(ClientID, "iFreeze is originally created by Tom94. Big thanks to him");
		return true;
	}
	else if(!str_comp(pMessage, "credits"))
	{
		SendChatTarget(ClientID, "Credits goes to the whole Teeworlds-community and especially");
		SendChatTarget(ClientID, "to BotoX, Tom and Greyfox. This mod has some of their ideas included.");
		return true;
	}
	else if(!str_comp(pMessage, "cmdlist"))
	{
		SendChatTarget(ClientID, "----- Commands -----");
		SendChatTarget(ClientID, "\"/info\" Information about the mod");
		SendChatTarget(ClientID, "\"/credits\" See some credits");
		SendChatTarget(ClientID, "\"/stats\" Show player stats");

		if(g_Config.m_SvStopGoFeature)
		{
			SendChatTarget(ClientID, "\"/stop\" Pause the game");
			SendChatTarget(ClientID, "\"/go\" Start the game");
			SendChatTarget(ClientID, "\"/restart\" Start a new round");
		}

		if(g_Config.m_SvXonxFeature)
		{
			SendChatTarget(ClientID, "\"/reset\" Reset the Spectator Slots");
			SendChatTarget(ClientID, "\"/1on1\" - \"6on6\" Starts a war");
		}

		if(g_Config.m_SvPrivateMessage || AuthLevel)
		{
			SendChatTarget(ClientID, "\"/sayto <Name/ID> <Msg>\" Send a private message to a player");
		}

		if(CanExec(ClientID, "set_team"))
		{
			SendChatTarget(ClientID, "\"/spec <client id>\" Set player to spectators");
			SendChatTarget(ClientID, "\"/red <client id>\" Set player to red team");
			SendChatTarget(ClientID, "\"/blue <client id>\" Set player to blue team");
		}
		return true;
	}
	else if(!str_comp(pMessage, "stop"))
	{
		if(pPlayer->GetTeam() != TEAM_SPECTATORS)
		{
			if(g_Config.m_SvStopGoFeature)
			{
				m_World.m_Paused = true;
				SendChat(-1, CHAT_ALL, "Game paused.");
			}
			else
			{
				SendChatTarget(ClientID, "This feature is not available at the moment.");
				return true;
			}
		}
	}
	else if(!str_comp(pMessage, "go"))
		{
			if(pPlayer->GetTeam() != TEAM_SPECTATORS)
			{
				if(g_Config.m_SvStopGoFeature)
				{
					m_pController->m_FakeWarmup = Server()->TickSpeed() * g_Config.m_SvGoTime;
				}
				else
				{
					SendChatTarget(ClientID, "This feature is not available at the moment.");
					return true;
				}
			}
		}
	else if(!str_comp(pMessage, "restart"))
	{
		if(pPlayer->GetTeam() != TEAM_SPECTATORS)
		{
			if(g_Config.m_SvStopGoFeature)
			{
				if(m_pController->IsWarmup())
					m_pController->DoWarmup(0);
				else
					m_pController->DoWarmup(g_Config.m_SvGoTime);
			}
			else
			{
				SendChatTarget(ClientID, "This feature is not available at the moment.");
				return true;
			}
		}
	}
	else if(!str_comp(pMessage, "1on1") || !str_comp(pMessage, "2on2") || !str_comp(pMessage, "3on3") ||
			!str_comp(pMessage, "4on4") || !str_comp(pMessage, "5on5") || !str_comp(pMessage, "6on6"))
	{
		if(pPlayer->GetTeam() != TEAM_SPECTATORS)
		{
			if(!g_Config.m_SvXonxFeature)
			{
				SendChatTarget(ClientID, "This feature is not available at the moment.");
				return true;
			}
			else
			{
				int Mode = (int)pMessage[0] - (int)'0';
				g_Config.m_SvSpectatorSlots = MAX_CLIENTS - 2*Mode;
				m_pController->DoWarmup(g_Config.m_SvWarTime);
				char aBuf[128];

				str_format(aBuf, sizeof(aBuf), "Upcoming %don%d! Please stay on spectator", Mode, Mode);
				SendBroadcast(aBuf, -1);

				str_format(aBuf, sizeof(aBuf), "The %don%d will start in %d seconds!", Mode, Mode, g_Config.m_SvWarTime);
				SendChat(-1, CHAT_ALL, aBuf);
			}
		}
	}
	else if(!str_comp(pMessage, "reset"))
	{
		if(pPlayer->GetTeam() != TEAM_SPECTATORS)
		{
			if(!g_Config.m_SvXonxFeature)
			{
				SendChatTarget(ClientID, "This feature is not available at the moment.");
				return true;
			}
			else
			{
				g_Config.m_SvSpectatorSlots = 0;
				SendChat(-1, CHAT_ALL, "Resetted spectator slots");
			}
		}
	}
	else if(!str_comp_num(pMessage, "sayto", 5))
	{
		if(!g_Config.m_SvPrivateMessage && !AuthLevel)
			SendChatTarget(ClientID, "This feature is not available at the moment.");
		else
		{
			int ReceiverID;
			char aMsg[256];
			char aName[MAX_NAME_LENGTH];

			if((sscanf(pMessage, "sayto %d %255s", &ReceiverID, aMsg) == 2) && IsValidCID(ReceiverID))
			{
				//PM by ClientID
				if(ReceiverID == ClientID)
				{
					SendChatTarget(ClientID, "You can't send yourself a private message");
				}
				else
				{
					char aBuf[512];
					str_format(aBuf, sizeof(aBuf), "You received a private message from %s (ID: %d)", Server()->ClientName(ClientID), ClientID);
					SendChatTarget(ReceiverID, aBuf);

					str_format(aBuf, sizeof(aBuf), "%s: %s", Server()->ClientName(ClientID), aMsg);
					SendChatTarget(ReceiverID, aBuf);
					SendChatTarget(ClientID, "PM successfully sent");
				}
			}
			// TODO: Solution for player with spaces in their name?
			else if(sscanf(pMessage, "sayto %15s %255s", aName, aMsg) == 2)
			{
				//PM by name
				int ID = -1;
				for(int i = 0; i < MAX_CLIENTS; i++)
					if(!str_comp_nocase(aName, Server()->ClientName(i)))
					{
						ID = i;
						break;
					}

				if(ID == ClientID)
				{
					SendChatTarget(ClientID, "You can't send yourself a private message");
				}
				else
				{
					if(IsValidCID(ID))
					{
						char aBuf[512];
						str_format(aBuf, sizeof(aBuf), "You received a private message from %s (ID: %d)", Server()->ClientName(ClientID), ClientID);
						SendChatTarget(ID, aBuf);

						str_format(aBuf, sizeof(aBuf), "%s: %s", Server()->ClientName(ClientID), aMsg);
						SendChatTarget(ID, aBuf);
						SendChatTarget(ClientID, "PM successfully sent");
					}
					else
					{
						SendChatTarget(ClientID, "A player with this name doesn't exist");
					}
				}
			}
			else
			{
				SendChatTarget(ClientID, "Invalid ID or name. Player doesn't exist");
			}
		}
		return true;
	}
	else if(!str_comp_num(pMessage, "emote", 5))
	{
		char aType[16];
		int Time = -1, Args;
		if(!pPlayer->GetCharacter())
			return true;

		if((Args = sscanf(pMessage, "emote %15s %d", aType, &Time)) >= 1)
		{
			if(Args < 2 || Time < 0)
				Time = 2;

			int Tick = Server()->Tick() + Time*Server()->TickSpeed();

			if(!str_comp_nocase(aType, "surprise"))
				pPlayer->GetCharacter()->SetEmote(EMOTE_SURPRISE, Tick);
			else if(!str_comp_nocase(aType, "blink"))
				pPlayer->GetCharacter()->SetEmote(EMOTE_BLINK, Tick);
			else if(!str_comp_nocase(aType, "happy"))
				pPlayer->GetCharacter()->SetEmote(EMOTE_HAPPY, Tick);
			else if(!str_comp_nocase(aType, "pain"))
				pPlayer->GetCharacter()->SetEmote(EMOTE_PAIN, Tick);
			else if(!str_comp_nocase(aType, "angry"))
				pPlayer->GetCharacter()->SetEmote(EMOTE_ANGRY, Tick);
			else
				SendChatTarget(ClientID, "Unkown emote. Type \"/emote\"");
		}
		else
		{
			SendChatTarget(ClientID, "Usage: /emote <type> <sec>. Use as type: \"surprise\", \"blink\", \"happy\", \"pain\" or \"angry\".");
			SendChatTarget(ClientID, "Example: \"/emote pain 10\" for showing 10 seconds emote pain.");
		}

		return true;
	}
	else if(!str_comp_num(pMessage, "stats", 5))
	{
		CPlayer* pP = pPlayer;
		//TODO: Possibility to show stats of other players?

		char aaBuf[4][128];
		str_format(aaBuf[0], sizeof(aaBuf[0]), "Total Shots: %d", pP->m_Stats.m_TotalShots);
		str_format(aaBuf[1], sizeof(aaBuf[1]), "Kills: %d", pP->m_Stats.m_Kills);
		str_format(aaBuf[2], sizeof(aaBuf[2]), "Deaths: %d", pP->m_Stats.m_Deaths);
		str_format(aaBuf[3], sizeof(aaBuf[3]), "Ratio: %.2f", (pP->m_Stats.m_Deaths > 0) ? ((float)pP->m_Stats.m_Kills / (float)pP->m_Stats.m_Deaths) : 0);

		SendChatTarget(ClientID, "--- Statistics ---");
		for(int i = 0; i < 4; i++)
			SendChatTarget(ClientID, aaBuf[i]);

		return true;
	}
	else if(AuthLevel)
	{
		if(CanExec(ClientID, "set_team"))
		{
			int ID;
			if(!str_comp_num(pMessage, "spec", 4))
			{
				if(sscanf(pMessage, "spec %d", &ID) == 1)
				{
					if(!IsValidCID(ID))
						SendChatTarget(ID, "Invalid ID");
					else
						m_apPlayers[ID]->SetTeam(TEAM_SPECTATORS);
				}
			}
			else if(!str_comp_num(pMessage, "red", 3))
			{
				if(sscanf(pMessage, "red %d", &ID) == 1)
				{
					if(!IsValidCID(ID))
						SendChatTarget(ID, "Invalid ID");
					else
						m_apPlayers[ID]->SetTeam(TEAM_RED);
				}
			}
			else if(!str_comp_num(pMessage, "blue", 4))
			{
				if(sscanf(pMessage, "blue %d", &ID) == 1)
				{
					if(!IsValidCID(ID))
						SendChatTarget(ID, "Invalid ID");
					else
						m_apPlayers[ID]->SetTeam(TEAM_BLUE);
				}
			}
		}
	}

	return false;
}

bool CGameContext::CanExec(int ClientID, const char* pCommand)
{
	return (Server()->IsAuthed(ClientID) == 2 || (Server()->IsAuthed(ClientID) == 1 && Console()->GetCommandInfo(pCommand, CFGFLAG_SERVER, false)->GetAccessLevel() == IConsole::ACCESS_LEVEL_MOD));
}
