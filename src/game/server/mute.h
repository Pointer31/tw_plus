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
	/**
	 * Internal structure where the mutes are saved
	 */
	struct CMuteEntry
	{
		char m_aIP[NETADDR_MAXSTRSIZE];
		int m_Expires;
	};
	enum { MAX_MUTES = 128 };

	/**
	 * Function for initialization
	 */
	void Init(CGameContext *pGameServer);
	/**
	 * Register console commands
	 */
	void OnConsoleInit(IConsole *pConsole);
	/**
	 * Return the number of current mutes
	 */
	inline int NumMutes();
	/**
	 * Remove expired mutes
	 */
	void PurgeMutes();
	/**
	 * Mutes a player by given ClientID for Secs seconds
	 */
	void AddMute(int ClientID, int Secs);
	/**
	 * Returns a pointer to the mute or null if not muted
	 */
	CMuteEntry *Muted(int ClientID);
	/**
	 * Get mute by index
	 */
	CMuteEntry *GetMute(int Num);


	bool CheckSpam(int ClientID, const char* msg);

private:
	/**
	 * List where our mutes are saved in
	 */
	CList<CMuteEntry> m_Mutes;
	/**
	 * Returns a pointer to the mute or null if not muted
	 */
	CMuteEntry *Muted(const char* pIP);
	/**
	 * Mute an IP for Secs seconds
	 */
	bool AddMute(const char* pIP, int Secs);
	/**
	 * Remove a mute by given MuteEntry
	 */
	void Unmute(CMuteEntry *pMute);
	int m_LastPurge;
};

#endif /* GAME_SERVER_MUTE_H */
