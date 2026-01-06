#include "kz_ai.h"
#include <game/collision.h>
#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

CKZBotAI::CKZBotAI(CGameContext *pContext, CPlayer *pPlayer) :
CBotAI(pContext, pPlayer)
{
}

int CKZBotAI::UnIntersectLineKZ(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance + 1);
	vec2 Last = Pos0;
	for(int i = 0; i <= End; i++)
	{
		float a = i / (float)End;
		vec2 Pos = mix(Pos0, Pos1, a);
		// Temporary position for checking collision
		int ix = round_to_int(Pos.x);
		int iy = round_to_int(Pos.y);

		if(!Collision()->CheckPoint(ix, iy))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return Collision()->GetCollisionAt(ix, iy);
		}

		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

void CKZBotAI::HandleInput(CNetObj_PlayerInput &Input)
{
    //Spaghetti yummy

    CCharacter *pOwnChar = nullptr;
    if(!(pOwnChar = GetPlayer()->GetCharacter()))
        return;
	
	CCharacter *pClosestChar = nullptr;
	CPickup *pClosestPickup = nullptr;
	CFlag *pEnemyFlag = nullptr;
	CFlag *pTeamFlag = nullptr;
	bool targetisup = false;
	bool dontjump = false;
	bool butjumpifwall = false;
	bool jumpifgoingtofall = false;
	
	vec2 TargetPos = vec2(0,0);
	bool TargetPosSet = false;
	bool DoSmartTargetChase = false;

    const vec2* m_pPos = &pOwnChar->GetPos();
    const CCharacterCore* m_pCore = &pOwnChar->GetCore();
	
	Input.m_Fire = false;
	
	//if(str_find_nocase(GameServer()->m_pController->m_pGameType, "CTF"))
	{
		CFlag *p = (CFlag *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_FLAG);
		for(; p; p = (CFlag *)p->TypeNext())
		{
			
			if(p->GetTeam() == GetPlayer()->GetTeam())
			{
				pTeamFlag = p;
				continue;
			}
			
			pEnemyFlag = p;
		}
	}
	
	{
		float ClosestRange = 100000.0f;
		CPickup *pClosest = 0;
		
		CPickup *p = (CPickup *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_PICKUP);
		for(; p; p = (CPickup *)p->TypeNext())
		{
			if(Collision()->FastIntersectLine(*m_pPos,p->GetPos(),nullptr,nullptr))
				continue;
			
			if((p->Type() == PICKUP_HEALTH && pOwnChar->GetHealth() >= 10) || (p->Type() == PICKUP_ARMOR && pOwnChar->GetArmor() >= 10) || (p->Type() == PICKUP_SHOTGUN && pOwnChar->m_aWeapons[WEAPON_SHOTGUN].m_Ammo) || (p->Type() == PICKUP_LASER && pOwnChar->m_aWeapons[WEAPON_LASER].m_Ammo) || (p->Type() == PICKUP_GRENADE && pOwnChar->m_aWeapons[WEAPON_GRENADE].m_Ammo)  || (p->Type() == PICKUP_NINJA && pOwnChar->m_aWeapons[WEAPON_NINJA].m_Got))
				continue;
			
			if(p->GetSpawnTick() > 0)
			{
				continue;
			}
			
			
			float Len = distance(*m_pPos, p->GetPos());
			if(Len < p->GetProximityRadius() + 100000.0f)
			{
				if(Len < ClosestRange)
				{
					ClosestRange = Len;
					pClosest = p;
				}
			}
		}
		
		pClosestPickup = pClosest;
	}
	
	{
		float ClosestRange = 100000.0f;
		CCharacter *pClosest = 0;
		
		CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(CGameWorld::ENTTYPE_CHARACTER);
		for(; p; p = (CCharacter *)p->TypeNext())
		{
			if(p == pOwnChar)
				continue;
			
			if(GameServer()->m_pController->IsTeamplay() && p->GetPlayer()->GetTeam() == GetPlayer()->GetTeam())
				continue;	
			
			float Len = distance(*m_pPos, p->GetPos());
			if(Len < p->GetProximityRadius() + 100000.0f)
			{
				if(Len < ClosestRange)
				{
					ClosestRange = Len;
					pClosest = p;
				}
			}
		}
		
		pClosestChar = pClosest;
	}
	
	if(pEnemyFlag)
	{
		if(pTeamFlag && pEnemyFlag->GetCarrier() == pOwnChar)
		{
			//Input.m_Direction = pTeamFlag->GetPos()->x > m_pPos->x ? 1 : -1;
			TargetPos = pTeamFlag->GetPos();
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
		else if(!pEnemyFlag->GetCarrier())
		{
			//Input.m_Direction = pEnemyFlag->GetPos()->x > m_pPos->x ? 1 : -1;
			TargetPos = pEnemyFlag->GetPos();
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
	}
	
	if(pClosestPickup && !(pTeamFlag && pEnemyFlag && (pEnemyFlag->GetCarrier() == pOwnChar || !pEnemyFlag->GetCarrier())))
	{
		//Input.m_Direction = pClosestPickup->GetPos()->x > m_pPos->x ? 1 : -1;
		TargetPos = pClosestPickup->GetPos();
		TargetPosSet = true;
		
		/*if((pClosestPickup->GetPos()->y + pClosestPickup->GetProximityRadius() * 2.f) < m_pPos->y && !(Collision()->GetCollisionAt(m_pPos->x , m_pPos->y - GetProximityRadius() / 3.f) == TILE_DEATH))
		{
			targetisup = true;
		}*/
	}
	
	if(pClosestChar)
	{
		Input.m_TargetX = pClosestChar->GetPos().x - m_pPos->x; // aim
		Input.m_TargetY = pClosestChar->GetPos().y - m_pPos->y;
		
		if(!pClosestPickup && !(pTeamFlag && pEnemyFlag && (pEnemyFlag->GetCarrier() == pOwnChar || !pEnemyFlag->GetCarrier())))
		{
			TargetPos = pClosestChar->GetPos();
			TargetPosSet = true;
			DoSmartTargetChase = true;
		}
		
		if(pOwnChar->m_aWeapons[WEAPON_NINJA].m_Got)
		{
		}
		else if(pOwnChar->m_aWeapons[WEAPON_LASER].m_Got && pOwnChar->m_aWeapons[WEAPON_LASER].m_Ammo && distance(*m_pPos, pClosestChar->GetPos()) < GameServer()->Tuning()->m_LaserReach)
		{
			pOwnChar->SetWeapon(WEAPON_LASER);
		}
		else if(pOwnChar->m_aWeapons[WEAPON_SHOTGUN].m_Got && pOwnChar->m_aWeapons[WEAPON_SHOTGUN].m_Ammo && distance(*m_pPos, pClosestChar->GetPos()) < GameServer()->Tuning()->m_ShotgunLifetime * 3000.0f)
		{
			pOwnChar->SetWeapon(WEAPON_SHOTGUN);
		}
		else if(pOwnChar->m_aWeapons[WEAPON_HAMMER].m_Got && distance(*m_pPos, pClosestChar->GetPos()) < 40.0f)
		{
			pOwnChar->SetWeapon(WEAPON_HAMMER);
		}
		else if(pOwnChar->m_aWeapons[WEAPON_GUN].m_Got)
		{
			pOwnChar->SetWeapon(WEAPON_GUN);
		}
		
		if((pOwnChar->GetActiveWeapon() == WEAPON_LASER ? (!Collision()->FastIntersectLine(*m_pPos,pClosestChar->GetPos(),nullptr,nullptr) && distance(*m_pPos, pClosestChar->GetPos()) < GameServer()->Tuning()->m_LaserReach) : !Collision()->FastIntersectLine(*m_pPos,pClosestChar->GetPos(),nullptr,nullptr)) || pOwnChar->m_aWeapons[WEAPON_NINJA].m_Got)
		{
			if(!pOwnChar->GetLatestInput().m_Fire)
				Input.m_Fire = true;
			else
				Input.m_Fire = false;
			
			if(Server()->Tick() % Server()->TickSpeed() == 0)
			{
				Input.m_Hook = false;
			}
			else if(distance(*m_pPos, pClosestChar->GetPos()) < GameServer()->Tuning()->m_HookLength)
			{
				Input.m_Hook = true;
			}
		}
		
		if(Collision()->FastIntersectLine(*m_pPos,pClosestChar->GetPos(),nullptr,nullptr) && pOwnChar->GetActiveWeapon() == WEAPON_LASER)
		{
			bool fire = false;
			
			float halfy = (pClosestChar->GetPos().y - m_pPos->y) / 2;
			float halfx = (pClosestChar->GetPos().x - m_pPos->x) / 2;
			
			vec2 BouncePos_Right,BouncePos_Left,BouncePos_Up,BouncePos_Down;
			bool Bounced_Right = false,Bounced_Left = false,Bounced_Up = false,Bounced_Down = false;
			
			vec2 At, tempvar;
			
			//try y first
			if(Collision()->FastIntersectLine(vec2(m_pPos->x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),nullptr,&tempvar))
			{
				UnIntersectLineKZ(vec2(m_pPos->x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),&BouncePos_Right,nullptr);
				if(!Collision()->IntersectLine(*m_pPos,BouncePos_Right,nullptr,nullptr) && tempvar.x > BouncePos_Right.x)
					Bounced_Right = true;
			}
			if(Collision()->FastIntersectLine(vec2(m_pPos->x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),nullptr,&tempvar))
			{
				UnIntersectLineKZ(vec2(m_pPos->x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),&BouncePos_Left,nullptr);
				if(!Collision()->IntersectLine(*m_pPos,BouncePos_Left,nullptr,nullptr) && tempvar.x < BouncePos_Left.x)
					Bounced_Left = true;
			}
			//try x now
			if(Collision()->FastIntersectLine(vec2(halfx + m_pPos->x,m_pPos->y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),nullptr,&tempvar))
			{
				UnIntersectLineKZ(vec2(halfx + m_pPos->x,m_pPos->y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),&BouncePos_Down,nullptr);
				if(!Collision()->IntersectLine(*m_pPos,BouncePos_Down,nullptr,nullptr) && tempvar.y > BouncePos_Down.y)
					Bounced_Down = true;
			}
			if(Collision()->FastIntersectLine(vec2(halfx + m_pPos->x,m_pPos->y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),nullptr,&BouncePos_Up))
			{
				UnIntersectLineKZ(vec2(halfx + m_pPos->x,m_pPos->y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),&BouncePos_Up,nullptr);
				if(!Collision()->IntersectLine(*m_pPos,BouncePos_Up,nullptr,nullptr) && tempvar.y < BouncePos_Up.y)
					Bounced_Up = true;
			}
			
			//try y
			if(Bounced_Right && (BouncePos_Right.y > (halfy + m_pPos->y) - 5.f && BouncePos_Right.y < (halfy + m_pPos->y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Right,pClosestChar->GetPos(),nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Right, vec2(m_pPos->x,m_pPos->y + (BouncePos_Right.y - m_pPos->y)*2), 0.f, At, pOwnChar))
			{
				Input.m_TargetX = BouncePos_Right.x - m_pPos->x; // aim
				Input.m_TargetY = BouncePos_Right.y - m_pPos->y;
				fire = true;
			}
			else if(Bounced_Left && (BouncePos_Left.y > (halfy + m_pPos->y) - 5.f && BouncePos_Left.y < (halfy + m_pPos->y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Left,pClosestChar->GetPos(),nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Left, vec2(m_pPos->x,m_pPos->y + (BouncePos_Left.y - m_pPos->y)*2), 0.f, At, pOwnChar))
			{
				Input.m_TargetX = BouncePos_Left.x - m_pPos->x; // aim
				Input.m_TargetY = BouncePos_Left.y - m_pPos->y;
				fire = true;
			}
				
			//try x
			if(Bounced_Up && (BouncePos_Up.x > (halfx + m_pPos->x) - 5.f && BouncePos_Up.x < (halfx + m_pPos->x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Up,pClosestChar->GetPos(),nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Up, vec2(m_pPos->x + (BouncePos_Up.x - m_pPos->x)*2,m_pPos->y), 0.f, At, pOwnChar))
			{
				Input.m_TargetX = BouncePos_Up.x - m_pPos->x; // aim
				Input.m_TargetY = BouncePos_Up.y - m_pPos->y;
				fire = true;
			}
			else if(Bounced_Down && (BouncePos_Down.x > (halfx + m_pPos->x) - 5.f && BouncePos_Down.x < (halfx + m_pPos->x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Down,pClosestChar->GetPos(),nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Down, vec2(m_pPos->x + (BouncePos_Down.x - m_pPos->x)*2,m_pPos->y), 0.f, At, pOwnChar))
			{
				Input.m_TargetX = BouncePos_Down.x - m_pPos->x; // aim
				Input.m_TargetY = BouncePos_Down.y - m_pPos->y;
				fire = true;
			}
					
			if(!fire)
			{
				//try again but moving point
				{
					float tempfloat =0.0f;
					
					tempfloat = halfy; //save halfy
					
					//move point
					halfx = halfx - m_pPos->x > 0 ? halfx + (tempfloat/2 + tempfloat/halfx) : halfx - (tempfloat/2 + tempfloat/halfx);
					if(halfy - m_pPos->y > 0)
					{
						halfy += (halfx/2 + halfx/halfy);
						if(halfy - m_pPos->y < 0)
							halfy += (halfy - m_pPos->y)*2;
					}
					else if(halfy - m_pPos->y < 0)
					{
						halfy -= (halfx/2 + halfx/halfy);
						if(halfy - m_pPos->y > 0)
							halfy -= (halfy - m_pPos->y)*2;
					}
					
					if(halfx - m_pPos->x > 0)
					{
						halfx += (tempfloat/2 + tempfloat/halfx);
						if(halfx - m_pPos->x < 0)
						{
							halfx += (halfx - m_pPos->x)*2;
						}
					}
					else if(halfx - m_pPos->x < 0)
					{
						halfx -= (tempfloat/2 + tempfloat/halfx);
						if(halfx - m_pPos->x > 0)
						{
							halfx -= (halfx - m_pPos->x)*2;
						}
					}
				}
				
				//vec2 BouncePos_Right,BouncePos_Left,BouncePos_Up,BouncePos_Down;
				Bounced_Right = false,Bounced_Left = false,Bounced_Up = false,Bounced_Down = false;
				
				//vec2 At, tempvar;
				
				//try y first
				if(Collision()->FastIntersectLine(vec2(m_pPos->x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),nullptr,&tempvar))
				{
					UnIntersectLineKZ(vec2(m_pPos->x + GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),&BouncePos_Right,nullptr);
					if(!Collision()->IntersectLine(*m_pPos,BouncePos_Right,nullptr,nullptr) && tempvar.x > BouncePos_Right.x)
						Bounced_Right = true;
				}
				if(Collision()->FastIntersectLine(vec2(m_pPos->x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),nullptr,&tempvar))
				{
					UnIntersectLineKZ(vec2(m_pPos->x - GameServer()->Tuning()->m_LaserReach / 2.f,halfy + m_pPos->y),vec2(m_pPos->x,halfy + m_pPos->y),&BouncePos_Left,nullptr);
					if(!Collision()->IntersectLine(*m_pPos,BouncePos_Left,nullptr,nullptr) && tempvar.x < BouncePos_Left.x)
						Bounced_Left = true;
				}
				//try x now
				if(Collision()->FastIntersectLine(vec2(halfx + m_pPos->x,m_pPos->y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),nullptr,&tempvar))
				{
					UnIntersectLineKZ(vec2(halfx + m_pPos->x,m_pPos->y + GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),&BouncePos_Down,nullptr);
					if(!Collision()->IntersectLine(*m_pPos,BouncePos_Down,nullptr,nullptr) && tempvar.y > BouncePos_Down.y)
						Bounced_Down = true;
				}
				if(Collision()->FastIntersectLine(vec2(halfx + m_pPos->x,m_pPos->y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),nullptr,&BouncePos_Up))
				{
					UnIntersectLineKZ(vec2(halfx + m_pPos->x,m_pPos->y - GameServer()->Tuning()->m_LaserReach / 2.f),vec2(halfx + m_pPos->x,m_pPos->y),&BouncePos_Up,nullptr);
					if(!Collision()->IntersectLine(*m_pPos,BouncePos_Up,nullptr,nullptr) && tempvar.y < BouncePos_Up.y)
						Bounced_Up = true;
				}
				
				//try y
				if(Bounced_Right && (BouncePos_Right.y > (halfy + m_pPos->y) - 5.f && BouncePos_Right.y < (halfy + m_pPos->y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Right,pClosestChar->GetPos(),nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Right, vec2(m_pPos->x,m_pPos->y + (BouncePos_Right.y - m_pPos->y)*2), 0.f, At, pOwnChar))
				{
					Input.m_TargetX = BouncePos_Right.x - m_pPos->x; // aim
					Input.m_TargetY = BouncePos_Right.y - m_pPos->y;
					fire = true;
				}
				else if(Bounced_Left && (BouncePos_Left.y > (halfy + m_pPos->y) - 5.f && BouncePos_Left.y < (halfy + m_pPos->y) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Left,pClosestChar->GetPos(),nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Left, vec2(m_pPos->x,m_pPos->y + (BouncePos_Left.y - m_pPos->y)*2), 0.f, At, pOwnChar))
				{
					Input.m_TargetX = BouncePos_Left.x - m_pPos->x; // aim
					Input.m_TargetY = BouncePos_Left.y - m_pPos->y;
					fire = true;
				}
					
				//try x
				if(Bounced_Up && (BouncePos_Up.x > (halfx + m_pPos->x) - 5.f && BouncePos_Up.x < (halfx + m_pPos->x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Up,pClosestChar->GetPos(),nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Up, vec2(m_pPos->x + (BouncePos_Up.x - m_pPos->x)*2,m_pPos->y), 0.f, At, pOwnChar))
				{
					Input.m_TargetX = BouncePos_Up.x - m_pPos->x; // aim
					Input.m_TargetY = BouncePos_Up.y - m_pPos->y;
					fire = true;
				}
				else if(Bounced_Down && (BouncePos_Down.x > (halfx + m_pPos->x) - 5.f && BouncePos_Down.x < (halfx + m_pPos->x) + 5.f) && !Collision()->FastIntersectLine(BouncePos_Down,pClosestChar->GetPos(),nullptr,nullptr) && GameServer()->m_World.IntersectCharacter(BouncePos_Down, vec2(m_pPos->x + (BouncePos_Down.x - m_pPos->x)*2,m_pPos->y), 0.f, At, pOwnChar))
				{
					Input.m_TargetX = BouncePos_Down.x - m_pPos->x; // aim
					Input.m_TargetY = BouncePos_Down.y - m_pPos->y;
					fire = true;
				}
			}
				
			
			if(fire)
			{
				Input.m_Fire = true;
			}
				
			
			
		}
	}
	
	
	if(TargetPosSet && !m_StopUntilTouchGround)
	{
		if(DoSmartTargetChase && !m_DontDoSmartTargetChase)
		{
			if(m_TryingDirectionSmart)
			{
				Input.m_Direction = m_TryingDirectionSmart;
				if((TargetPos.y + 56.0f) < m_pPos->y && !(Collision()->GetCollisionAt(m_pPos->x , m_pPos->y -  pOwnChar->GetProximityRadius() / 3.f) == TILE_DEATH))
				{
					targetisup = true;
				}
				else
				{
					dontjump = true;
					butjumpifwall = true;
				}
				if(!(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0.f,1000.f),nullptr,nullptr)))
				{
					//danger no floor, remove directionsmart
					m_TryingOppositeSmart = m_TryingDirectionSmart = 0;
				}
			}
			else if(((TargetPos.x - m_pPos->x < 0 ? (TargetPos.y - m_pPos->y < 0 ? TargetPos.x - m_pPos->x > TargetPos.y - m_pPos->y : (TargetPos.x - m_pPos->x)*-1 < TargetPos.y - m_pPos->y) : (TargetPos.y - m_pPos->y < 0 ? TargetPos.x - m_pPos->x < (TargetPos.y - m_pPos->y)*-1 : TargetPos.x - m_pPos->x < TargetPos.y - m_pPos->y)) && TargetPos.x > m_pPos->x - 500.0f && TargetPos.x < m_pPos->x + 500.0f))
			{
				//is up/down
				
				bool left = false,right = false;
				
				if(TargetPos.y < m_pPos->y)
				{
					left = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-400.f,-400.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(0,-350.f),nullptr,nullptr);
					right = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(400.f,-400.f),nullptr,nullptr);
					
					if(left && right && !m_TryingDirectionSmart && !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0,-350.f),nullptr,nullptr))//middle
					{
						Input.m_Direction = TargetPos.x > m_pPos->x ? 1 : -1;
						targetisup = true;
					}
					else if(!m_TryingDirectionSmart && !left)//left
					{
						Input.m_Direction = -1;
						targetisup = true;
					}
					else if(!m_TryingDirectionSmart && !right)//right
					{
						Input.m_Direction = 1;
						targetisup = true;
					}
					else
					{
						//bool leftside = false,rightside = false;
						//vec2 leftcol,rightcol;
						
						
						if(!m_TryingDirectionSmart)
						{
							m_TryingDirectionSmart = TargetPos.x > m_pPos->x ? 1 : -1;
							/*leftside = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-250.f,0),nullptr,&leftcol);
							rightside = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(250.f,0),nullptr,&rightcol);
							if(!leftside && !rightside)
							{
								m_TryingDirectionSmart = TargetPos.x > m_pPos->x ? 1 : -1;
							}
							else
							{
								if(!leftside)
								{
									m_TryingDirectionSmart = -1;
								}
								else if(!rightside)
								{
									m_TryingDirectionSmart = 1;
								}
								else
								{
									float d1,d2;
									d1 = distance(*m_pPos,leftcol);
									d2 = distance(*m_pPos,rightcol);
									
									if(d1 > d2)
									{
										m_TryingDirectionSmart = 1;
									}
									else
									{
										m_TryingDirectionSmart = -1;
									}
								}
							}*/
						}
						else
						{
							Input.m_Direction = m_TryingDirectionSmart;
						}
					}
				}
				else
				{
					left = Collision()->FastIntersectLine(*m_pPos + vec2(-100.f,0),*m_pPos + vec2(-100.f,-100.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(*m_pPos + vec2(0,0),*m_pPos + vec2(0,-100.f),nullptr,nullptr);
					right = Collision()->FastIntersectLine(*m_pPos + vec2(100.f,0),*m_pPos + vec2(100.f,-100.f),nullptr,nullptr);
					
					if(left && right && !m_TryingDirectionSmart && !Collision()->FastIntersectLine(*m_pPos + vec2(0,0),*m_pPos + vec2(0,-100.f),nullptr,nullptr))//middle
					{
						m_TryingDirectionSmart = TargetPos.x > m_pPos->x ? 1 : -1;
					}
					else if(!left && !m_TryingDirectionSmart)
					{
						m_TryingDirectionSmart = -1;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!right && !m_TryingDirectionSmart)
					{
						m_TryingDirectionSmart = 1;
						dontjump = true;
						butjumpifwall = true;
					}
					else
					{
						bool leftside = false,rightside = false;
						vec2 leftcol,rightcol;
						
						
						if(!m_TryingDirectionSmart)
						{
							if(!(leftside = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-1000.f,0),nullptr,&leftcol)))
							{
								m_TryingDirectionSmart = -1;
							}
							else if(!(rightside = Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(1000.f,0),nullptr,&rightcol)))
							{
								m_TryingDirectionSmart = 1;
							}
							else
							{
								float d1,d2;
								d1 = distance(*m_pPos,leftcol);
								d2 = distance(*m_pPos,rightcol);
								
								if(d1 > d2)
								{
									m_TryingDirectionSmart = 1;
								}
								else
								{
									m_TryingDirectionSmart = -1;
								}
							}
						}
						else
						{
							Input.m_Direction = m_TryingDirectionSmart;
						}
					}
				}
			}
			else
			{
				//is still away
				
				//bool up = false,middle = false,down = false;
				
				if(TargetPos.x > m_pPos->x)
				{
					//up = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(100.f,-100.f),nullptr,nullptr) || Collision()->IntersectLine(*m_pPos + vec2(0,-100.f),*m_pPos + vec2(50.f,-150.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(150.f,0),nullptr,nullptr);
					//down = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(100.f,100.f),nullptr,nullptr)  || Collision()->IntersectLine(*m_pPos + vec2(0,100.f),*m_pPos + vec2(50.f,150.f),nullptr,nullptr);
					
					if(!Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(100.f,0),nullptr,nullptr))//middle
					{
						Input.m_Direction = 1;
						dontjump = true;
						jumpifgoingtofall = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(50.f,50.f),nullptr,nullptr))  || !(Collision()->IntersectLine(*m_pPos + vec2(0,100.f),*m_pPos + vec2(50.f,150.f),nullptr,nullptr)))//down
					{
						Input.m_Direction = 1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(50.f,-50.f),nullptr,nullptr)) || !(Collision()->IntersectLine(*m_pPos + vec2(0,-100.f),*m_pPos + vec2(50.f,-150.f),nullptr,nullptr)))//up
					{
						Input.m_Direction = 1;
						targetisup = true;
					}
					else
					{
						if(!m_TryingDirectionSmart)
						{
							bool upside = false, downside = false;
							downside = Collision()->FastIntersectLine(*m_pPos + vec2(pOwnChar->GetProximityRadius()/2,0),*m_pPos + vec2(0,300.f),nullptr,nullptr);
							upside = Collision()->FastIntersectLine(*m_pPos + vec2(pOwnChar->GetProximityRadius()/2,0),*m_pPos + vec2(0,-150.f),nullptr,nullptr);
							if(!upside)
							{
								targetisup = true;
							}
							else if(downside)
							{
								m_TryingDirectionSmart = -1;
								targetisup = true;
							}
							
						}
					}
				}
				else
				{
					//up = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(-100.f,-100.f),nullptr,nullptr) || Collision()->IntersectLine(*m_pPos + vec2(0,-100.f),*m_pPos + vec2(-50.f,-150.f),nullptr,nullptr);
					//middle = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(-150.f,0),nullptr,nullptr);
					//down = Collision()->IntersectLine(*m_pPos,*m_pPos + vec2(-100.f,100.f),nullptr,nullptr)  || Collision()->IntersectLine(*m_pPos + vec2(0,100.f),*m_pPos + vec2(-50.f,150.f),nullptr,nullptr);
					
					if(!Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-100.f,0),nullptr,nullptr))//middle
					{
						Input.m_Direction = -1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-50.f,100.f),nullptr,nullptr))  || !(Collision()->FastIntersectLine(*m_pPos + vec2(0,100.f),*m_pPos + vec2(-50.f,150.f),nullptr,nullptr)))//down
					{
						Input.m_Direction = -1;
						jumpifgoingtofall = true;
						dontjump = true;
						butjumpifwall = true;
					}
					else if(!(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-50.f,-100.f),nullptr,nullptr)) || !(Collision()->FastIntersectLine(*m_pPos + vec2(0,-100.f),*m_pPos + vec2(-50.f,-150.f),nullptr,nullptr)))//up
					{
						Input.m_Direction = -1;
						targetisup = true;
					}
					else
					{
						if(!m_TryingDirectionSmart)
						{
							bool upside = false, downside = false;
							downside = Collision()->FastIntersectLine(*m_pPos + vec2(pOwnChar->GetProximityRadius()/-2,0),*m_pPos + vec2(0,300.f),nullptr,nullptr);
							upside = Collision()->FastIntersectLine(*m_pPos + vec2(pOwnChar->GetProximityRadius()/-2,0),*m_pPos + vec2(0,-150.f),nullptr,nullptr);
							if(!upside)
							{
								targetisup = true;
							}
							else if(upside && downside)
							{
								m_TryingDirectionSmart = 1;
								targetisup = true;
							}
							
						}
					}
				}
			}
			
		}
		else
		{
			Input.m_Direction = TargetPos.x > m_pPos->x ? 1 : -1;
			
			if((TargetPos.y + 56.0f) < m_pPos->y && !(Collision()->GetCollisionAt(m_pPos->x , m_pPos->y - pOwnChar->GetProximityRadius() / 3.f) == TILE_DEATH))
			{
				targetisup = true;
			}
			else
			{
				dontjump = true;
				butjumpifwall = true;
			}
			if(m_DontDoSmartTargetChase > 0)
				m_DontDoSmartTargetChase--;
		}

		if(m_DoGrenadeJump && ((pOwnChar->GetWeaponGot(WEAPON_GRENADE) && pOwnChar->GetWeaponAmmo(WEAPON_GRENADE))))
		{
			if(pOwnChar->GetWeaponGot(WEAPON_GRENADE) && pOwnChar->GetWeaponAmmo(WEAPON_GRENADE))
				pOwnChar->SetWeapon(WEAPON_GRENADE);
			Input.m_TargetX = 0;
			Input.m_TargetY = 1;
			Input.m_Fire = 1;
		}

		m_DoGrenadeJump = false;

		if(((pOwnChar->GetWeaponGot(WEAPON_GRENADE) && pOwnChar->GetWeaponAmmo(WEAPON_GRENADE))) && (/*GameServer()->m_pController->m_IsInstagibKZ || */pOwnChar->GetHealth() >= 10 || (pOwnChar->GetHealth() >= 5 && pOwnChar->GetArmor() >= 5)))
		{
			if(m_pCore->m_Jumped < 3 && TargetPos.x > m_pPos->x - 300.f && TargetPos.x < m_pPos->x + 300.f && TargetPos.y < m_pPos->y - 150.f && pOwnChar->IsGrounded() && !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0.f,-300.f),nullptr,nullptr))
			{
				m_DoGrenadeJump = true;
				Input.m_Jump = true;
				if(pOwnChar->GetWeaponGot(WEAPON_GRENADE) && pOwnChar->GetWeaponAmmo(WEAPON_GRENADE))
					pOwnChar->SetWeapon(WEAPON_GRENADE);
			}
			else if(TargetPos.x < m_pPos->x - 700.f && pOwnChar->IsGrounded() && !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-300.f,0.f),nullptr,nullptr))
			{
				Input.m_TargetX = 1;
				Input.m_TargetY = 1;
				Input.m_Fire = 1;
				if(pOwnChar->GetWeaponGot(WEAPON_GRENADE) && pOwnChar->GetWeaponAmmo(WEAPON_GRENADE))
					pOwnChar->SetWeapon(WEAPON_GRENADE);
			}
			else if(TargetPos.x > m_pPos->x + 700.f && pOwnChar->IsGrounded() && !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(300.f,0.f),nullptr,nullptr))
			{
				Input.m_TargetX = -1;
				Input.m_TargetY = 1;
				Input.m_Fire = 1;
				if(pOwnChar->GetWeaponGot(WEAPON_GRENADE) && pOwnChar->GetWeaponAmmo(WEAPON_GRENADE))
					pOwnChar->SetWeapon(WEAPON_GRENADE);
			}
		}
	}
	
	if(m_TryingDirectionSmart && !Collision()->FastIntersectLine(*m_pPos,TargetPos,nullptr,nullptr))
	{
		m_TryingOppositeSmart = m_TryingDirectionSmart = 0;
		m_StopUntilTouchGround = true;
	}
	
	if(m_StopUntilTouchGround && pOwnChar->IsGrounded())
	{
		m_StopUntilTouchGround = false;
		m_DontDoSmartTargetChase = Server()->TickSpeed() * 2;
	}
	if(m_pCore->m_Colliding && pOwnChar->IsGrounded())
	{
		if(!m_TryingOppositeSmart)
		{
			m_TryingDirectionSmart *= -1;
			m_TryingOppositeSmart = true;
		}
		else
		{
			m_TryingDirectionSmart = 0;
			m_TryingOppositeSmart = false;
		}
	}

	if((m_pCore->m_Jumped >= 3) && !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0.f,1000.f),nullptr,nullptr))
	{
		vec2 right,left;

		Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(1000.f,1000.f),&right,nullptr);
		Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(-1000.f,1000.f),&left,nullptr);

		float rightlength = right.x - m_pPos->x;
		float leftlength = m_pPos->x - left.x;

		if(rightlength == leftlength)
		{
			Input.m_Direction = 0;
		}
		else if(rightlength > leftlength)
		{
			Input.m_Direction = -1;
		}
		else
		{
			Input.m_Direction = 1;
		}
	}
	
	
	//HELP
	if(!m_DoGrenadeJump && ((jumpifgoingtofall ? !(Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0.f,1000.f),nullptr,nullptr)) : false) || (butjumpifwall ? m_pCore->m_Colliding : false) || (!dontjump && ((Collision()->GetCollisionAt(m_pPos->x , m_pPos->y + pOwnChar->GetProximityRadius() / 3.f) == TILE_DEATH) || !Collision()->FastIntersectLine(*m_pPos,*m_pPos + vec2(0.f,1000.f),nullptr,nullptr) || m_pCore->m_Colliding || (((Collision()->CheckPoint(m_pPos->x + pOwnChar->GetProximityRadius() / 2, m_pPos->y + pOwnChar->GetProximityRadius() / 2 + 5)) && !(Collision()->CheckPoint(m_pPos->x - pOwnChar->GetProximityRadius() / 2, m_pPos->y + pOwnChar->GetProximityRadius() / 2 + 5)))) || ((!(Collision()->CheckPoint(m_pPos->x + pOwnChar->GetProximityRadius() / 2, m_pPos->y + pOwnChar->GetProximityRadius() / 2 + 5)) && (Collision()->CheckPoint(m_pPos->x - pOwnChar->GetProximityRadius() / 2, m_pPos->y + pOwnChar->GetProximityRadius() / 2 + 5)))) || targetisup))))
	{
		if(pOwnChar->IsGrounded() || (m_pCore->m_Vel.y > 0))
			Input.m_Jump = true;
		else
			Input.m_Jump = false;
	}
}

void CKZBotAI::GetSkin(STeeInfos &TeeInfos)
{
    for (int p = 0; p < NUM_SKINPARTS; p++)
    {
        TeeInfos.m_aUseCustomColors[p] = true;

        if (p == 0) // body
        {
            str_copy(TeeInfos.m_aaSkinPartNames[p], "fox", sizeof(TeeInfos.m_aaSkinPartNames[p]));
            TeeInfos.m_aSkinPartColors[p] = 1769560;
            continue;
        }

        if (p == 1) // marking
        {
            str_copy(TeeInfos.m_aaSkinPartNames[p], "warpaint", sizeof(TeeInfos.m_aaSkinPartNames[p]));
            TeeInfos.m_aSkinPartColors[p] = 4278190080;
            continue;
        }

        if (p == 2) //decoration
        {
            str_copy(TeeInfos.m_aaSkinPartNames[p], "hair", sizeof(TeeInfos.m_aaSkinPartNames[p]));
            continue;
        }

        if (p == 5) //eyes
        {
            str_copy(TeeInfos.m_aaSkinPartNames[p], "negative", sizeof(TeeInfos.m_aaSkinPartNames[p]));
            TeeInfos.m_aSkinPartColors[p] = 65408;
            continue;
        }

        // everything else
        str_copy(TeeInfos.m_aaSkinPartNames[p], "standard", sizeof(TeeInfos.m_aaSkinPartNames[p]));
    }
}
