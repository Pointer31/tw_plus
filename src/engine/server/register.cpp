#include <base/system.h>
#include <engine/shared/network.h>
#include <engine/shared/config.h>
#include <engine/console.h>
#include <engine/engine.h>
#include <engine/masterserver.h>
#include <engine/message.h>
#include <engine/shared/config.h>
#include <engine/shared/jsonparser.h>
#include <engine/shared/http_request.h>
#include <engine/shared/network.h>

#include <mastersrv/mastersrv.h>

#include "register.h"

void CRegister::CheckChallengeStatus(int Protocol)
{
	lock_wait(m_aProtocols[Protocol].m_Lock);

	if(m_aProtocols[Protocol].m_LastResponseIndex == m_aProtocols[Protocol].m_NumTotalRequests - 1)
	{
		switch(m_aProtocols[Protocol].m_LastResponseStatus)
		{
		case STATUS_NEEDCHALLENGE:
			if(m_aProtocols[Protocol].m_NewChallengeToken)
			{
				// Immediately resend if we got the token.
				m_aProtocols[Protocol].m_NextRegister = time_get();
			}
			break;
		case STATUS_NEEDINFO:
			// Act immediately if the master requests more info.
			m_aProtocols[Protocol].m_NextRegister = time_get();
			break;
		}
	}
	

	lock_unlock(m_aProtocols[Protocol].m_Lock);
}

void CRegister::UpdateRegister(int Protocol)
{
	CheckChallengeStatus(Protocol);
	if(time_get() >= m_aProtocols[Protocol].m_NextRegister)
	{
		int Index = m_aProtocols[Protocol].m_lpJobs.add(new CJob());
		Engine()->AddJob(m_aProtocols[Protocol].m_lpJobs[Index], CRegister::SendRegister, &m_aProtocols[Protocol].m_Context);
		for(int i = 0; i < m_aProtocols[Protocol].m_lpJobs.size(); i++)
		{
			if(m_aProtocols[Protocol].m_lpJobs[i]->Status() == CJob::STATE_DONE)
			{
				delete m_aProtocols[Protocol].m_lpJobs[i];
				m_aProtocols[Protocol].m_lpJobs.remove_index_fast(i);
			}
		}
	}
}

int CRegister::SendRegister(void *pUser)
{
	CProtocol::CRegisterContext *pContext = static_cast<CProtocol::CRegisterContext*>(pUser);
	int Protocol = pContext->m_Protocol;

	int64 Now = time_get();
	int64 Freq = time_freq();

	char aAddress[64];
	str_format(aAddress, sizeof(aAddress), "%sconnecting-address.invalid:%d", ProtocolToScheme(Protocol), pContext->m_pParent->m_ServerPort);

	char aSecret[UUID_MAXSTRSIZE];
	FormatUuid(pContext->m_pParent->m_Secret, aSecret, sizeof(aSecret));

	char aChallengeUuid[UUID_MAXSTRSIZE];
	FormatUuid(pContext->m_pParent->m_ChallengeSecret, aChallengeUuid, sizeof(aChallengeUuid));

	char aChallengeSecret[64];
	str_format(aChallengeSecret, sizeof(aChallengeSecret), "%s:%s", aChallengeUuid, ProtocolToString(Protocol));
	int InfoSerial;
	bool SendInfo;

	{
		lock_wait(pContext->m_pParent->m_Lock);
		InfoSerial = pContext->m_pParent->m_InfoSerial;
		SendInfo = InfoSerial > pContext->m_pParent->m_LastSuccessfulInfoSerial;
		lock_unlock(pContext->m_pParent->m_Lock);
	}

	CHttpRequest Register("POST", pContext->m_pParent->Config()->m_SvRegisterUrl, 15L, Protocol == PROTOCOL_IPV4 ? HTTP_IPRESOLVE_IPV4ONLY : HTTP_IPRESOLVE_IPV6ONLY);
	if(SendInfo)
	{
		Register.PostJson(pContext->m_pParent->m_aServerInfo);
	}
	char aHeader[256];
	str_format(aHeader, sizeof(aHeader), "Address: %s", aAddress);
	Register.AddHeader(aHeader);

	str_format(aHeader, sizeof(aHeader), "Secret: %s", aSecret);
	Register.AddHeader(aHeader);

	str_format(aHeader, sizeof(aHeader), "Connless-Token: %s", pContext->m_pParent->m_aConnlessTokenHex);
	Register.AddHeader(aHeader);

	str_format(aHeader, sizeof(aHeader), "Challenge-Secret: %s", aChallengeSecret);
	Register.AddHeader(aHeader);
	if(pContext->m_pParent->m_aProtocols[Protocol].m_HaveChallengeToken)
	{
		str_format(aHeader, sizeof(aHeader), "Challenge-Token: %s", pContext->m_pParent->m_aProtocols[Protocol].m_aChallengeToken);
		Register.AddHeader(aHeader);
	}
	str_format(aHeader, sizeof(aHeader), "Info-Serial: %d", InfoSerial);
	Register.AddHeader(aHeader);

	if(pContext->m_pParent->Config()->m_SvRegisterCommunityToken[0])
	{
		str_format(aHeader, sizeof(aHeader), "Community-Token: %s", pContext->m_pParent->Config()->m_SvRegisterCommunityToken);
		Register.AddHeader(aHeader);
	}

	int RequestIndex;
	{
		lock_wait(pContext->m_pParent->m_aProtocols[Protocol].m_Lock);
		if(pContext->m_pParent->m_aProtocols[Protocol].m_LastResponseStatus != STATUS_OK)
		{
			pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, ProtocolToSystem(Protocol), "registering...");
		}
		RequestIndex = pContext->m_pParent->m_aProtocols[Protocol].m_NumTotalRequests;
		pContext->m_pParent->m_aProtocols[Protocol].m_NumTotalRequests++;	
		lock_unlock(pContext->m_pParent->m_aProtocols[Protocol].m_Lock);
	}

	pContext->m_pParent->m_aProtocols[Protocol].m_NewChallengeToken = false;
	pContext->m_pParent->m_aProtocols[Protocol].m_NextRegister = Now + 15 * Freq;

	Register.StartRunBlocking();

	char aBuf[256];
	if(Register.Result() != 0)
	{
		// TODO: exponential backoff
		pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, ProtocolToSystem(Protocol), "error sending request to master");
		return -1;
	}
	CJsonParser Parser;
	json_value *pJson = Parser.ParseData(Register.ReceivedData(), Register.ReceivedDataSize());
	if(!pJson)
	{
		pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, ProtocolToSystem(Protocol), "non-JSON response from master");
		return -2;
	}
	const json_value &Json = *pJson;
	const json_value &StatusString = Json["status"];
	if(StatusString.type != json_string)
	{
		pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, ProtocolToSystem(Protocol), "invalid JSON response from master");
		return -3;
	}
	int Status;
	if(StatusFromString(&Status, StatusString))
	{
		pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, ProtocolToSystem(Protocol), "invalid status from master: %s", (const char *) StatusString);
		return -4;
	}
	if(Status == STATUS_ERROR)
	{
		const json_value &Message = Json["message"];
		if(Message.type != json_string)
		{
			pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, ProtocolToSystem(Protocol), "invalid JSON error response from master");
			return -5;
		}
		str_format(aBuf, sizeof(aBuf), "error response from master: %d: %s", Register.ResponseCode(), (const char *) Message);
		pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, ProtocolToSystem(Protocol), aBuf);
		return -6;
	}
	if(Register.ResponseCode() >= 400)
	{
		str_format(aBuf, sizeof(aBuf), "non-success status code %d from master without error code", Register.ResponseCode());
		pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, ProtocolToSystem(Protocol), aBuf);
		return -7;
	}
	{
		lock_wait(pContext->m_pParent->m_aProtocols[Protocol].m_Lock);
		if(Status != pContext->m_pParent->m_aProtocols[Protocol].m_LastResponseStatus)
		{
			if(Status != STATUS_OK)
			{
				if(pContext->m_pParent->Config()->m_Debug)
					dbg_msg(ProtocolToSystem(Protocol), "status: %s", (const char *)StatusString);
			}
			else
			{
				pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, ProtocolToSystem(Protocol), "successfully registered");
			}
		}
		if(Status == pContext->m_pParent->m_aProtocols[Protocol].m_LastResponseStatus && Status == STATUS_NEEDCHALLENGE)
		{
			pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, ProtocolToSystem(Protocol), "ERROR: the master server reports that clients can not connect to this server.");
			str_format(aBuf, sizeof(aBuf), "ERROR: configure your firewall/nat to let through udp on port %d.", pContext->m_pParent->m_ServerPort);
			pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, ProtocolToSystem(Protocol), aBuf);
		}
		if(RequestIndex > pContext->m_pParent->m_aProtocols[Protocol].m_LastResponseIndex)
		{
			pContext->m_pParent->m_aProtocols[Protocol].m_LastResponseIndex = RequestIndex;
			pContext->m_pParent->m_aProtocols[Protocol].m_LastResponseStatus = Status;
		}
		lock_unlock(pContext->m_pParent->m_aProtocols[Protocol].m_Lock);
	}
	if(Status == STATUS_OK)
	{
		lock_wait(pContext->m_pParent->m_Lock);
		if(InfoSerial > pContext->m_pParent->m_LastSuccessfulInfoSerial)
		{
			pContext->m_pParent->m_LastSuccessfulInfoSerial = InfoSerial;
		}
		lock_unlock(pContext->m_pParent->m_Lock);
	}
	else if(Status == STATUS_NEEDINFO)
	{
		lock_wait(pContext->m_pParent->m_Lock);
		if(InfoSerial == pContext->m_pParent->m_LastSuccessfulInfoSerial)
		{
			// Tell other requests that they need to send the info again.
			pContext->m_pParent->m_LastSuccessfulInfoSerial -= 1;
		}
		lock_unlock(pContext->m_pParent->m_Lock);
	}
	return 0;
}

void CRegister::SendDeleteIfRegistered(void *pUser)
{
	CProtocol::CRegisterContext *pContext = static_cast<CProtocol::CRegisterContext*>(pUser);
	int Protocol = pContext->m_Protocol;
	{
		const bool ShouldSendDelete = pContext->m_pParent->m_aProtocols[Protocol].m_LastResponseStatus == STATUS_OK;
		pContext->m_pParent->m_aProtocols[Protocol].m_LastResponseStatus = STATUS_NONE;
		if(!ShouldSendDelete)
			return;
	}

	char aAddress[64];
	str_format(aAddress, sizeof(aAddress), "%sconnecting-address.invalid:%d", ProtocolToScheme(Protocol), pContext->m_pParent->m_ServerPort);

	char aSecret[UUID_MAXSTRSIZE];
	FormatUuid(pContext->m_pParent->m_Secret, aSecret, sizeof(aSecret));

	CHttpRequest Request("POST", pContext->m_pParent->Config()->m_SvRegisterUrl, 15, Protocol == PROTOCOL_IPV4 ? HTTP_IPRESOLVE_IPV4ONLY : HTTP_IPRESOLVE_IPV6ONLY);
	Request.AddHeader("Action: delete");

	char aHeader[256];
	str_format(aHeader, sizeof(aHeader), "Address: %s", aAddress);
	Request.AddHeader(aHeader);

	str_format(aHeader, sizeof(aHeader), "Secret: %s", aSecret);
	Request.AddHeader(aHeader);

	pContext->m_pParent->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, ProtocolToSystem(Protocol), "deleting...");

	Request.StartRun(pContext->m_pParent->Engine());
}

void CRegister::OnToken(int Protocol, const char *pToken)
{
}

CRegister::CRegister()
{
	m_pEngine = 0;
	m_pConfig = 0;
	m_pConsole = 0;

	m_GotServerInfo = false;

	m_Secret = RandomUuid();
	m_ChallengeSecret = RandomUuid();

	for(int i = 0; i < NUM_PROTOCOLS; i++)
	{
		m_aProtocols[i].m_Context.m_pParent = this;
		m_aProtocols[i].m_Context.m_Protocol = i;
		m_aProtocols[i].m_NewChallengeToken = false;
		m_aProtocols[i].m_HaveChallengeToken = false;
		m_aProtocols[i].m_aChallengeToken[0] = '\0';
		m_aProtocols[i].m_NumTotalRequests = 0;
		m_aProtocols[i].m_LastResponseStatus = STATUS_NONE;
		m_aProtocols[i].m_LastResponseIndex = -1;
		m_aProtocols[i].m_NextRegister = -1;
		m_aProtocols[i].m_Lock = lock_create();
	}
	m_Lock = lock_create();
	m_InfoSerial = -1;
	m_LastSuccessfulInfoSerial = -1;

	static const int HEADER_LEN = sizeof(SERVERBROWSE_CHALLENGE);
	mem_copy(m_aVerifyPacketPrefix, SERVERBROWSE_CHALLENGE, HEADER_LEN);
	FormatUuid(m_ChallengeSecret, m_aVerifyPacketPrefix + HEADER_LEN, sizeof(m_aVerifyPacketPrefix) - HEADER_LEN);
	m_aVerifyPacketPrefix[HEADER_LEN + UUID_MAXSTRSIZE - 1] = ':';
}

CRegister::~CRegister()
{
	for(int i = 0; i < NUM_PROTOCOLS; i++)
	{
		lock_destroy(m_aProtocols[i].m_Lock);
	}
	lock_destroy(m_Lock);
}

bool CRegister::StatusFromString(int *pResult, const char *pString)
{
	if(str_comp(pString, "success") == 0)
	{
		*pResult = STATUS_OK;
	}
	else if(str_comp(pString, "need_challenge") == 0)
	{
		*pResult = STATUS_NEEDCHALLENGE;
	}
	else if(str_comp(pString, "need_info") == 0)
	{
		*pResult = STATUS_NEEDINFO;
	}
	else if(str_comp(pString, "error") == 0)
	{
		*pResult = STATUS_ERROR;
	}
	else
	{
		*pResult = -1;
		return true;
	}
	return false;
}

bool CRegister::ProtocolFromString(int *pResult, const char *pString)
{
	if(str_comp(pString, "tw0.7/ipv6") == 0)
	{
		*pResult = PROTOCOL_IPV6;
	}
	else if(str_comp(pString, "tw0.7/ipv4") == 0)
	{
		*pResult = PROTOCOL_IPV4;
	}
	else
	{
		*pResult = -1;
		return true;
	}
	return false;
}

const char *CRegister::ProtocolToScheme(int Protocol)
{
	switch(Protocol)
	{
		case PROTOCOL_IPV4: return "tw-0.7+udp://";
		case PROTOCOL_IPV6: return "tw-0.7+udp://";
	}
	return "invalid protocol";
}

const char *CRegister::ProtocolToString(int Protocol)
{
	switch(Protocol)
	{
		case PROTOCOL_IPV4: return "tw0.7/ipv6";
		case PROTOCOL_IPV6: return "tw0.7/ipv4";
	}
	return "invalid protocol";
}

const char *CRegister::ProtocolToSystem(int Protocol)
{
	switch(Protocol)
	{
	case PROTOCOL_IPV6: return "register/ipv6";
	case PROTOCOL_IPV4: return "register/ipv4";
	}
	return "invalid protocol";
}

void CRegister::Init(IEngine *pEngine, CConfig *pConfig, IConsole *pConsole, TOKEN SecurityToken)
{
	m_pEngine = pEngine;
	m_pConfig = pConfig;
	m_pConsole = pConsole;
	m_ServerPort = m_pConfig->m_SvPort;

	str_format(m_aConnlessTokenHex, sizeof(m_aConnlessTokenHex), "%08x", SecurityToken);
}

void CRegister::RegisterUpdate(int Nettype)
{
	if(!Config()->m_SvRegister)
		return;
	if(!m_GotServerInfo)
		return;
	if(Nettype & NETTYPE_IPV4)
		UpdateRegister(PROTOCOL_IPV4);
	if(Nettype & NETTYPE_IPV6)
		UpdateRegister(PROTOCOL_IPV6);
}

void CRegister::OnNewInfo(const char *pInfo)
{
	if(m_GotServerInfo && str_comp(m_aServerInfo, pInfo) == 0)
	{
		return;
	}
	m_GotServerInfo = true;
	str_copy(m_aServerInfo, pInfo, sizeof(m_aServerInfo));
	{
		lock_wait(m_Lock);
		m_InfoSerial++;
		lock_unlock(m_Lock);
	}
}

void CRegister::OnShutdown()
{
	for(int i = 0; i < NUM_PROTOCOLS; i++)
	{
		SendDeleteIfRegistered(&m_aProtocols[i].m_Context);
	}
}

bool CRegister::OnPacket(const CNetChunk *pPacket)
{
	if((pPacket->m_Flags & NETSENDFLAG_CONNLESS) == 0)
		return false;

	if(pPacket->m_DataSize >= (int)sizeof(m_aVerifyPacketPrefix) &&
		mem_comp(pPacket->m_pData, m_aVerifyPacketPrefix, sizeof(m_aVerifyPacketPrefix)) == 0)
	{
		CUnpacker Unpacker;
		Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);
		Unpacker.GetRaw(sizeof(m_aVerifyPacketPrefix));
		const char *pProtocol = Unpacker.GetString(0);
		const char *pToken = Unpacker.GetString(0);
		if(Unpacker.Error())
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", "got erroneous challenge packet from master");
			return true;
		}

		if(Config()->m_Debug)
			dbg_msg("register", "got challenge token, protocol='%s' token='%s'", pProtocol, pToken);

		int Protocol;
		if(ProtocolFromString(&Protocol, pProtocol))
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "register", "got challenge packet with unknown protocol");
			return true;
		}
		OnToken(Protocol, pToken);
		return true;
	}
	return false;
}
