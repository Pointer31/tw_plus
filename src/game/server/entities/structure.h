/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

class CStructure : public CEntity
{
public:
	CStructure(CGameWorld *pGameWorld, int Type, int Owner, vec2 Pos, vec2 Dir);

	// vec2 GetPos(float Time);
	void FillInfo(CNetObj_Projectile *pProj);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_Direction;
	float m_LifeSpan;
	int m_Owner;
	int m_Type;
	int m_Damage;
	int m_SoundImpact;
	int m_Weapon;
	float m_Force;
	int m_StartTick;
	bool m_Explosive;
	bool m_inTele;

	int m_Bounces;
};

