#ifndef HIDNSEK_H
#define HIDNSEK_H
#include <game/server/gamecontroller.h>
#include <generated/protocol.h>
#include <engine/shared/protocol.h>

enum {
    SPECIAL_MODE_NONE = 0,
    SPECIAL_MODE_INFECTION,
    SPECIAL_MODE_FREEZE,
    MAX_SPECIAL_MODES,
};

class CGameControllerHidNSek : public IGameController
{
public:
	CGameControllerHidNSek(class CGameContext *pGameServer);
    ~CGameControllerHidNSek();
    virtual const char *GetGameHelpText() override;
    virtual void Tick() override;

    virtual void OnCharacterSpawn(class CCharacter *pChr) override;
    virtual bool OnCharacterSnap(CCharacter *pChar, int SnappingClient) override;
    virtual bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
    virtual bool CanChangeSkin(int ClientID) override;
    virtual bool CanSpecID(int ClientID) override;
    virtual bool CanFireWeapon(class CCharacter &Char) override;
    virtual bool DoWincheckMatch() override;
    virtual void DoWincheckRound() override;

    virtual void OnPlayerConnect(class CPlayer *pPlayer) override;

    int Seekers();
    void ResetSeekers();

    int OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon) override;
    virtual void HandleCharacterInput(class CCharacter &Char, CNetObj_PlayerInput *pInput, bool Predicted) override;
    virtual void HandleCharacterSnap(class CCharacter &Char, CNetObj_Character *pCharObj, int SnappingClient) override;
    virtual void SendChatMsg(int From, int To, int Mode, const char* pText);

    class CHidNSekPlayer
    {
        public:
        bool m_WasSeeker = false;
        bool m_IsSeeker = false;
        bool m_Infected = false;
        int m_Ball = -1;
        int m_FrozenTick = -1;
        bool m_FrozenSpecial = false;
        bool m_SentSpecialModeBroadcast = false;
    } m_HidNSekPlayers[MAX_CLIENTS];

    bool m_DoResetSeekers = false;
    bool m_ToldSeekers = false;

    static int m_SpecialMode;

    private:
    void SetPlayerSeeker(int ClientID, bool set, bool infected = false);
    void SendSkinChangeHNS(int ClientID, int TargetID, int ColorBody);
    void UpdatePlayerSkin(int ClientID);
};
#endif
