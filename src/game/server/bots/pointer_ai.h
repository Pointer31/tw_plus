// Pointer 0.6
#ifndef POINTER_AI_H
#define POINTER_AI_H
#include "base_ai.h"
#include <base/vmath.h>

class CPointerBotAI : public CBotAI
{
    int m_isBot = 0;
    int m_botAggro = -1;
	int m_ticksSinceFire = 0;
	int m_botDirection = 1;

    public:
    CPointerBotAI(CGameContext *pContext, CPlayer *pPlayer, int Difficulty);
    virtual void HandleInput(CNetObj_PlayerInput &Input) override;
    virtual void GetSkin(STeeInfos &TeeInfos) override;
    virtual const char * GetName() { return "Bot"; };
    virtual EBotAIType GetAIType() { return BOT_AI_POINTER; }
};
#endif
