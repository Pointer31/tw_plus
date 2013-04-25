/*
 * Mute.h
 *
 *  Created on: 13.04.2013
 *      Author: Teetime
 */

#ifndef GAME_SERVER_MUTE_H
#define GAME_SERVER_MUTE_H

#include <base/List.h>
#include <engine/shared/config.h>
#include <engine/console.h>

class CMute
{
	class IServer *m_pServer;
	class CGameContext *m_pGameServer;
	class IConsole *m_pConsole;
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() { return m_pServer; }
	IConsole *Console() {return m_pConsole; }

	static void ConMute(IConsole::IResult *pResult, void *pUserData);
	static void ConUnmuteID(IConsole::IResult *pResult, void *pUserData);
	static void ConUnmuteIP(IConsole::IResult *pResult, void *pUserData);
	static void ConMutes(IConsole::IResult *pResult, void *pUserData);

public:
	CMute();
	struct CMuteEntry
	{
		char m_aIP[NETADDR_MAXSTRSIZE];
		int m_Expires;
	};
	enum { MAX_MUTES = 128 };

	void Init(CGameContext *pGameServer);
	void AddMute(int ClientID, int Secs);
	CMuteEntry *Muted(int ClientID);
	void PurgeMutes();
	inline int NumMutes();
	CMuteEntry *GetMute(int Num);

private:
	CList<CMuteEntry> m_Mutes;
	CMuteEntry *Muted(const char* pIP);
	bool AddMute(const char* pIP, int Secs);
};

#endif /* GAME_SERVER_MUTE_H */
