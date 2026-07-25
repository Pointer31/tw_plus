/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <generated/server_data.h>
#include <game/server/gamecontext.h>

#include "structure.h"
#include "projectile.h"
#include "lasertrap.h"	

CStructure::CStructure(CGameWorld *pGameWorld, int Type, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, Pos, 14)
{
	m_Type = Type;
	m_Angle = FigureOutAngle(Pos);
	m_TicksBeforeNextSpawn = 100;

	Reset();

	GameWorld()->InsertEntity(this);
}

vec2 CStructure::FigureOutAngle(vec2 Pos)
{
	int x = 0;
	int y = 0;
	if (GameServer()->Collision()->CheckPoint(Pos.x - 32.0f, Pos.y))
		x++;
	if (GameServer()->Collision()->CheckPoint(Pos.x + 32.0f, Pos.y))
		x--;
	if (GameServer()->Collision()->CheckPoint(Pos.x, Pos.y - 32.0f))
		y++;
	if (GameServer()->Collision()->CheckPoint(Pos.x, Pos.y + 32.0f))
		y--;
	return {(float)x, (float)y};
}

void CStructure::Reset()
{

}

void CStructure::Tick()
{
	m_TicksBeforeNextSpawn--;
	if(m_TicksBeforeNextSpawn <= 0)
	{
		m_TicksBeforeNextSpawn = Config()->m_SvStructureFireDelay;

		vec2 Direction = normalize(m_Angle);

		if (m_Type == 0)
		{
			new CProjectile(GameWorld(), WEAPON_GRENADE,
				PLAYER_TEAM_WORLD,
				m_Pos,
				Direction,
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
				0, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
		}
		else if (m_Type == 1)
		{

			new CLaserTrap(GameWorld(), 	
				m_Pos,
				Direction,
				(int)(GameServer()->Tuning()->m_LaserReach),
				PLAYER_TEAM_WORLD);
		}
	}
}

void CStructure::TickPaused()
{

}

void CStructure::Snap(int SnappingClient)
{
	if(m_SpawnTick != -1 || NetworkClipped(SnappingClient))
		return;

}
