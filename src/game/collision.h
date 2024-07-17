/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

class CCollision
{
	class CTile *m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;
	
	struct telePos
	{
		int x;
		int y;
		bool exists = false;
	};
	
	telePos m_telePositions[4]; // positions of teleports

	bool IsTileSolid(int x, int y);
	int GetTile(int x, int y);
	int GetTileNew(int x, int y);

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,
		COLFLAG_TELEONE=8,
		COLFLAG_TELETWO=16,
		COLFLAG_TELETHREE=32,
		COLFLAG_TELEFOUR=64,
		COLFLAG_SLOWDEATH=128,
	};

	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) { return IsTileSolid(roundbyteeworlds(x), roundbyteeworlds(y)); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(roundbyteeworlds(x), roundbyteeworlds(y)); }
	int GetCollisionAtNew(float x, float y) { return GetTileNew(roundbyteeworlds(x), roundbyteeworlds(y)); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);
	int getTeleX(int index);
	int getTeleY(int index);
};

#endif
