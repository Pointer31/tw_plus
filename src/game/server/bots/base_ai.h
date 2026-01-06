// +KZ
#ifndef BASE_AI_H
#define BASE_AI_H
#include <generated/protocol.h>

class CGameContext;
class CGameWorld;
class CConfig;
class CTuningParams;
class IServer;
class CCollision;
class CPlayer;
struct STeeInfos;

class CBotAI
{
    int m_Difficulty;

	CCollision* m_pCollision = nullptr;
	CGameWorld* m_pGameWorld = nullptr;
	CGameContext* m_pGameServer = nullptr;
	IServer* m_pServer = nullptr;
    CTuningParams* m_pTuning = nullptr;
    CConfig* m_pConfig = nullptr;

    CPlayer* m_pPlayer = nullptr;

    protected:

	CGameWorld *GameWorld() { return m_pGameWorld; }
	CTuningParams *Tuning() { return m_pTuning; }
	CConfig *Config() { return m_pConfig; }
	CGameContext *GameServer() { return m_pGameServer; }
	IServer *Server() { return m_pServer; }
	CCollision *Collision() { return m_pCollision; }
    CPlayer *GetPlayer() { return m_pPlayer; };

    public:

    enum EBotAIType {
        BOT_AI_NONE = 0,
        BOT_AI_KZ,
        BOT_AI_POINTER,
    };

    CBotAI(CGameContext *pContext, CPlayer *pPlayer);
    virtual ~CBotAI() {};
    virtual void HandleInput(CNetObj_PlayerInput &Input) {};
    virtual void GetSkin(STeeInfos &TeeInfos) {};
    virtual const char * GetName() { return "Dummy"; };
    virtual EBotAIType GetAIType() { return BOT_AI_NONE; }

    static CBotAI *CreateBot(CGameContext *pContext, CPlayer *pPlayer, int AI, int Difficulty);
};
#endif
