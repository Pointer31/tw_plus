#ifndef HIDNSEK_H
#define HIDNSEK_H
#include <game/server/gamecontroller.h>
#include <generated/protocol.h>
#include <engine/shared/protocol.h>

class CGameControllerHidNSek : public IGameController
{
public:
	CGameControllerHidNSek(class CGameContext *pGameServer);
    ~CGameControllerHidNSek();
	virtual void Tick();

    virtual void OnCharacterSpawn(class CCharacter *pChr);
    virtual bool OnCharacterSnap(CCharacter *pChar, int SnappingClient) override;
    virtual bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) override;
    virtual bool CanChangeSkin(int ClientID) override;
    virtual bool CanSpecID(int ClientID) override;
    virtual bool CanFireWeapon(class CCharacter &Char) override;
	virtual void DoWincheckRound();

    int Seekers();
    void ResetSeekers();

    int OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon) override;
    virtual void HandleCharacterInput(class CCharacter &Char, CNetObj_PlayerInput *pInput, bool Predicted) override;
    virtual void HandleCharacterSnap(class CCharacter &Char, CNetObj_Character *pCharObj, int SnappingClient) override;

    class CHidNSekPlayer
    {
        private:
        int m_SelfID = -1;
        public:
        bool m_WasSeeker = false;
        bool m_IsSeeker = false;
        int m_Ball = -1;
        int m_FrozenTick = -1;
        void Reset();
        void SetSeeker(bool set);
        void SetID(int id);
    } m_HidNSekPlayers[MAX_CLIENTS];

    bool m_DoResetSeekers = false;
    bool m_ToldSeekers = false;

    private:
    void SendSkinChangeHNS(int ClientID, int TargetID, int ColorBody);
};
#endif
