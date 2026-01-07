#include "base_ai.h"
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include "kz_ai.h"
#include "pointer_ai.h"

CBotAI::CBotAI(CGameContext *pContext, CPlayer *pPlayer)
{
    m_pGameServer = pContext;
    m_pGameWorld = &m_pGameServer->m_World;
    m_pServer = m_pGameServer->Server();
    m_pCollision = m_pGameServer->Collision();
    m_pTuning = m_pGameServer->Tuning();
    m_pConfig = m_pGameServer->Config();

    m_pPlayer = pPlayer;
    m_pPlayer->m_Latency.m_Avg = 0;
}

CBotAI *CBotAI::CreateBot(CGameContext *pContext, CPlayer *pPlayer, int AI, int Difficulty)
{
    CBotAI* pAI = nullptr;
    
    switch(AI)
    {
    case 1:
        pAI = new CKZBotAI(pContext, pPlayer);
        break;
    case 2:
        pAI = new CPointerBotAI(pContext, pPlayer, Difficulty);
        break;
    default:
        pAI = new CBotAI(pContext, pPlayer);
        break;
    }

    pAI->GetSkin(pPlayer->m_TeeInfos);
    return pAI;
}