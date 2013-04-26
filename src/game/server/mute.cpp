/*
 * CMute.cpp
 *
 *  Created on: 13.04.2013
 *      Author: Teetime
 */

#include <base/system.h>
#include <game/server/gamecontext.h>
#include "mute.h"

CMute::CMute() :
		m_Mutes(MAX_MUTES)
{
	mem_zero(this, sizeof(CMute));
}

void CMute::Init(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = pGameServer->Server();
	m_pConsole = pGameServer->Console();

	Console()->Register("mute", "ii", CFGFLAG_SERVER, ConMute, this, "Mute a player for x sec");
	Console()->Register("unmuteid", "i", CFGFLAG_SERVER, ConUnmuteID, this, "Unmute a player by its client id");
	Console()->Register("unmuteip", "i", CFGFLAG_SERVER, ConUnmuteIP, this, "Remove a mute by its index");
	Console()->Register("mutes", "", CFGFLAG_SERVER, ConMutes, this, "Show all mutes");
}

int CMute::NumMutes()
{
	if(m_LastPurge != Server()->Tick())
	{
		m_LastPurge = Server()->Tick();
		PurgeMutes();
	}
	return m_Mutes.Num();
}

void CMute::PurgeMutes()
{
	int i = 0;
	while(i < NumMutes())
	{
		const CMuteEntry *pMute = m_Mutes.Get(i);
		if(pMute->m_Expires <= Server()->Tick())
			m_Mutes.Remove(i);
		else
			i++;
	}
}

bool CMute::AddMute(const char* pIP, int Secs)
{
	CMuteEntry *pMute = Muted(pIP);
	if(Secs < 0)
	{
		Unmute(pMute);
		return false;
	}

	if(pMute)
		pMute->m_Expires = Server()->TickSpeed() * Secs + Server()->Tick(); // overwrite mute
	else
	{
		CMuteEntry Mute;
		str_copy(Mute.m_aIP, pIP, sizeof(Mute.m_aIP));
		Mute.m_Expires = Server()->TickSpeed() * Secs + Server()->Tick();

		m_Mutes.Add(Mute);
	}
	return true;
}

void CMute::AddMute(int ClientID, int Secs)
{
	char aAddrStr[NETADDR_MAXSTRSIZE] = { 0 };
	Server()->GetClientAddr(ClientID, aAddrStr, sizeof(aAddrStr));

	if(AddMute(aAddrStr, Secs))
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s has been muted for %d min and %d sec.", Server()->ClientName(ClientID), Secs / 60, Secs % 60);
		GameServer()->SendChatTarget(-1, aBuf);
	}
}

CMute::CMuteEntry *CMute::Muted(const char* pIP)
{
	if(!pIP[0])
		return 0;

	for(int i = 0; i < NumMutes(); i++)
		if(!str_comp_num(pIP, m_Mutes.Get(i)->m_aIP, sizeof(m_Mutes.Get(i)->m_aIP)))
			return const_cast<CMuteEntry *>(m_Mutes.Get(i));
	return 0;
}

CMute::CMuteEntry *CMute::Muted(int ClientID)
{
	char aAddrStr[NETADDR_MAXSTRSIZE] = { 0 };
	Server()->GetClientAddr(ClientID, aAddrStr, sizeof(aAddrStr));
	return Muted(aAddrStr);
}

CMute::CMuteEntry *CMute::GetMute(int Num)
{
	if(Num < 0 || Num >= NumMutes())
		return 0;

	return const_cast<CMuteEntry *>(m_Mutes.Get(Num));
}

void CMute::Unmute(CMuteEntry *pMute)
{
	if(!pMute)
		return;
	pMute->m_Expires = 0;

	char aAddrStr[NETADDR_MAXSTRSIZE] = { 0 }, aBuf[128];
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		Server()->GetClientAddr(i, aAddrStr, sizeof(aAddrStr));
		if(str_comp(aAddrStr, pMute->m_aIP) == 0)
		{
			str_format(aBuf, sizeof(aBuf), "%s has been unmuted.", Server()->ClientName(i));
			GameServer()->SendChatTarget(-1, aBuf);
			break;
		}
	}
	str_format(aBuf, sizeof(aBuf), "unmuted %s", pMute->m_aIP);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Mutes", aBuf);
}

// Console commands

void CMute::ConMute(IConsole::IResult *pResult, void *pUserData)
{
	CMute *pSelf = (CMute *) pUserData;
	int ClientID = pResult->GetInteger(0);
	if(!pSelf->GameServer()->IsValidCID(ClientID))
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Mutes", "Invalid ClientID");
		return;
	}

	pSelf->AddMute(ClientID, pResult->GetInteger(1));
}

void CMute::ConMutes(IConsole::IResult *pResult, void *pUserData)
{
	CMute *pSelf = (CMute *) pUserData;
	char aBuf[128];
	int Sec, Count = 0;

	for(int i = 0; i < pSelf->NumMutes(); i++)
	{
		Sec = (pSelf->m_Mutes.Get(i)->m_Expires - pSelf->Server()->Tick()) / pSelf->Server()->TickSpeed();
		str_format(aBuf, sizeof(aBuf), "#%d: %s for %d minutes and %d sec", Count, pSelf->m_Mutes.Get(i)->m_aIP, Sec / 60, Sec % 60);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Mutes", aBuf);
		Count++;
	}
	str_format(aBuf, sizeof(aBuf), "%d mute%c", Count, Count == 1 ? '\0' : 's');
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Mutes", aBuf);
}

void CMute::ConUnmuteID(IConsole::IResult *pResult, void *pUserData)
{
	CMute *pSelf = (CMute *) pUserData;
	int ClientID = pResult->GetInteger(0);
	if(!pSelf->GameServer()->IsValidCID(ClientID))
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Mutes", "Invalid ClientID");
		return;
	}

	CMuteEntry *pMute = pSelf->Muted(ClientID);
	if(pMute)
		pSelf->Unmute(pMute);
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Mutes", "mute not found");
}

void CMute::ConUnmuteIP(IConsole::IResult *pResult, void *pUserData)
{
	CMute *pSelf = (CMute *) pUserData;
	int MuteID = pResult->GetInteger(0);
	char aBuf[128];

	CMuteEntry *pMute = pSelf->GetMute(MuteID);

	if(pMute)
		pSelf->Unmute(pMute);
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Mutes", "mute not found");
}
