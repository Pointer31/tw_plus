/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_RESOURCES_H
#define GAME_CLIENT_RESOURCES_H
#include <base/vmath.h>
#include <base/tl/sorted_array.h>
#include <game/client/component.h>

#include <generated/protocol.h>

const int MAX_RESOURCE_ARRAY_SIZE = 64;
const int MAX_RESOURCES = 64;

class CResources : public CComponent
{
public:
	struct CResource
	{
		// int m_Flags;
		char m_aName[MAX_RESOURCE_ARRAY_SIZE];
		IGraphics::CTextureHandle m_Texture;

		bool operator<(const CResource &Other) { return str_comp_nocase(m_aName, Other.m_aName) < 0; }
	};

	void OnInit();
	const CResource *Get(int ResourceId);
	int Find(const char *pName);

	const CResource *GetWeaponResource(int WeaponId);
	const CResource *GetWeaponResourceCrosshair(int WeaponId);
	const CResource *GetWeaponResourceAmmo(int WeaponId);
	const CResource *GetWeaponResourceProjectile(int WeaponId);
	const int GetWeaponResourceLooksLike(int WeaponId);
	const int GetWeaponResourcePredictsLike(int WeaponId);

	void OnResourceMessage(CNetMsg_Sv_ImageResource* msg);
	void OnCustomWeaponInfoMessage(CNetMsg_Sv_CustomWeaponInfo* msg);
	
private:
	sorted_array<CResource> m_aResources;
	char ResourceMapping[MAX_RESOURCES][MAX_RESOURCE_ARRAY_SIZE];
	int WeaponMapping[MAX_RESOURCES][4];
	static int FileScan(const char *pName, int IsDir, int DirType, void *pUser);
};

#endif
