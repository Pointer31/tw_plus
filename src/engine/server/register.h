/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SERVER_REGISTER_H
#define ENGINE_SERVER_REGISTER_H

#include <base/uuid.h>
#include <engine/shared/network.h>
#include <mastersrv/mastersrv.h>

class CRegister
{
	enum
	{
		STATUS_NONE = 0,
		STATUS_OK,
		STATUS_NEEDCHALLENGE,
		STATUS_NEEDINFO,
		STATUS_ERROR,
	
		PROTOCOL_IPV4 = 0,
		PROTOCOL_IPV6,
		NUM_PROTOCOLS,
	};
	static bool StatusFromString(int *pResult, const char *pString);
	static bool ProtocolFromString(int *pResult, const char *pString);
	static const char *ProtocolToScheme(int Protocol);
	static const char *ProtocolToString(int Protocol);
	static const char *ProtocolToSystem(int Protocol);

	bool m_GotServerInfo;
	char m_aVerifyPacketPrefix[sizeof(SERVERBROWSE_CHALLENGE) + UUID_MAXSTRSIZE];
	char m_aServerInfo[32 * 1024];
	char m_aConnlessTokenHex[16];

	struct CProtocol
	{
		struct CRegisterContext
		{
			CRegister *m_pParent;
			int m_Protocol;
		} m_Context;
		bool m_NewChallengeToken;
		bool m_HaveChallengeToken;
		char m_aChallengeToken[128];
		int64 m_NextRegister;

		int m_NumTotalRequests;
		int m_LastResponseStatus;
		int m_LastResponseIndex;

		void *m_Lock;
	
		array<CJob*> m_lpJobs;
	} m_aProtocols[NUM_PROTOCOLS];
	Uuid m_Secret;
	Uuid m_ChallengeSecret;

	void *m_Lock;
	int m_InfoSerial;
	int m_LastSuccessfulInfoSerial;
	int m_ServerPort;

	class IEngine *m_pEngine;
	class CConfig *m_pConfig;
	class IConsole *m_pConsole;
private:
	class IEngine *Engine() const { return m_pEngine; }
	class CConfig *Config() const { return m_pConfig; }
	class IConsole *Console() const { return m_pConsole; }

	static int SendRegister(void *pUser);
	static void SendDeleteIfRegistered(void *pUser);

	void CheckChallengeStatus(int Protocol);
	void UpdateRegister(int Protocol);
	void OnToken(int Protocol, const char *pToken);
public:
	CRegister();
	~CRegister();
	void Init(class IEngine *pEngine, class CConfig *pConfig, class IConsole *pConsole, TOKEN SecurityToken);
	void RegisterUpdate(int Nettype);
	void OnNewInfo(const char *pInfo);
	void OnShutdown();
	bool OnPacket(const struct CNetChunk *pPacket);
};

#endif
