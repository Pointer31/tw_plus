// +KZ
#ifndef KZ_AI_H
#define KZ_AI_H
#include "base_ai.h"
#include <base/vmath.h>

class CKZBotAI : public CBotAI
{
    int UnIntersectLineKZ(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);

    int m_TryingDirectionSmart = 0;
	bool m_TryingOppositeSmart = false;
	bool m_StopUntilTouchGround = false;
	int m_DontDoSmartTargetChase = 0;
	bool m_DoGrenadeJump = false;

    public:
    CKZBotAI(CGameContext *pContext, CPlayer *pPlayer);
    virtual void HandleInput(CNetObj_PlayerInput &Input) override;
    virtual void GetSkin(STeeInfos &TeeInfos) override;
    virtual const char * GetName() { return "Aimbot"; };
    virtual EBotAIType GetAIType() { return BOT_AI_KZ; }
};
#endif
