/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/color.h>
#include <base/math.h>
#include <base/system.h>

#include <engine/graphics.h>
#include <engine/storage.h>

#include <engine/shared/config.h>
#include <engine/shared/jsonparser.h>
#include <engine/shared/jsonwriter.h>

#include <generated/protocol.h>

#include "menus.h"
#include "resources.h"

int CResources::FileScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	if(IsDir || !str_endswith(pName, ".png"))
		return 0;

	CResources *pSelf = (CResources *)pUser;
	
	int PartNameSize, PartNameCount;
	str_utf8_stats(pName, str_length(pName) - str_length(".png") + 1, IO_MAX_PATH_LENGTH, &PartNameSize, &PartNameCount);
	if(PartNameSize >= MAX_RESOURCE_ARRAY_SIZE)
	{
		char aBuf[IO_MAX_PATH_LENGTH + 64];
		str_format(aBuf, sizeof(aBuf), "failed to load resource '%s': name too long", pName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "resources", aBuf);
		return 0;
	}

	CResource Part;
	str_copy(Part.m_aName, pName, minimum<int>(PartNameSize + 1, sizeof(Part.m_aName)));

	char aBuf[IO_MAX_PATH_LENGTH];
	str_format(aBuf, sizeof(aBuf), "resources/%s", pName);
	CImageInfo Info;
	if(!pSelf->Graphics()->LoadPNG(&Info, aBuf, DirType))
	{
		str_format(aBuf, sizeof(aBuf), "failed to load resource '%s'", pName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "resources", aBuf);
		return 0;
	}
	if(Info.m_Format != CImageInfo::FORMAT_RGBA)
	{
		str_format(aBuf, sizeof(aBuf), "failed to load resource '%s': must be RGBA format", pName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "resources", aBuf);
		return 0;
	}

	Part.m_Texture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
	
	{
		str_format(aBuf, sizeof(aBuf), "load resource %s", Part.m_aName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "resources", aBuf);
	}

	pSelf->m_aResources.add(Part);

	return 0;
}

void CResources::OnInit()
{
	for (int index = 0; index < 64; index++)
	{
		str_copy(ResourceMapping[index], "\0", 64);
	}

	m_aResources.clear();
	Storage()->ListDirectory(IStorage::TYPE_ALL, "resources", FileScan, this);
	
	dbg_assert(Find("unknown") >= 0, "data/resources/unknown.png has not been loaded");
}

const CResources::CResource *CResources::Get(int ResourceId)
{
	if (ResourceId < 0 || ResourceId >= MAX_RESOURCES)
		return &m_aResources[Find("unknown")];
	else if (ResourceMapping[ResourceId] && Find(ResourceMapping[ResourceId]) >= 0)
		return &m_aResources[Find(ResourceMapping[ResourceId])];
	else
		return &m_aResources[Find("unknown")];
}

const CResources::CResource *CResources::GetWeaponResource(int WeaponId)
{
	if (WeaponId < 0 || WeaponId >= MAX_RESOURCES)
		return Get(-1);
	else
		return Get(WeaponMapping[WeaponId][0]);
}

const CResources::CResource *CResources::GetWeaponResourceProjectile(int WeaponId)
{
	if (WeaponId < 0 || WeaponId >= MAX_RESOURCES)
		return Get(-1);
	else
		return Get(WeaponMapping[WeaponId][1]);
}

const CResources::CResource *CResources::GetWeaponResourceCrosshair(int WeaponId)
{
	if (WeaponId < 0 || WeaponId >= MAX_RESOURCES)
		return Get(-1);
	else
		return Get(WeaponMapping[WeaponId][2]);
}

const CResources::CResource *CResources::GetWeaponResourceAmmo(int WeaponId)
{
	if (WeaponId < 0 || WeaponId >= MAX_RESOURCES)
		return Get(-1);
	else
		return Get(WeaponMapping[WeaponId][3]);
}

int CResources::Find(const char *pName)
{
	for(int i = 0; i < m_aResources.size(); i++)
	{
		if(str_comp(m_aResources[i].m_aName, pName) == 0)
			return i;
	}
	return -1;
}

void CResources::OnResourceMessage(CNetMsg_Sv_ImageResource* msg)
{

	char aBuf[IO_MAX_PATH_LENGTH];

	int Id = msg->m_Id;
	const char* pName = msg->m_pName;

	if (!pName[0])
	{
		str_format(aBuf, sizeof(aBuf), "got invalid resource id %i, name='%s'", Id, pName);
		Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "resources", aBuf);
		return;
	}
	if (Id < 0 || Id >= MAX_RESOURCES)
	{
		str_format(aBuf, sizeof(aBuf), "got out of bounds resource id %i, name='%s'", Id, pName);
		Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "resources", aBuf);
		return;
	}
	str_format(aBuf, sizeof(aBuf), "got resource id %i, name='%s'", Id, pName);
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "resources", aBuf);

	str_copy(ResourceMapping[Id], pName, sizeof(ResourceMapping[Id]));
	return;
}

void CResources::OnCustomWeaponInfoMessage(CNetMsg_Sv_CustomWeaponInfo* msg)
{
	char aBuf[IO_MAX_PATH_LENGTH];

	const int WeaponId = msg->m_WeaponId;

	if (WeaponId < 0 || WeaponId >= MAX_RESOURCES)
	{
		str_format(aBuf, sizeof(aBuf), "got out of bounds weapon id %i", WeaponId);
		Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "resources", aBuf);
		return;
	}
	str_format(aBuf, sizeof(aBuf), "got weapon id %i", WeaponId);
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "resources", aBuf);

	// msg->m_LooksLike;
	// msg->m_PredictsLike;
	WeaponMapping[WeaponId][0] = msg->m_ResourceIdWeapon;
	WeaponMapping[WeaponId][1] = msg->m_ResourceIdProjectile;
	WeaponMapping[WeaponId][2] = msg->m_ResourceIdCrosshair;
	WeaponMapping[WeaponId][3] = msg->m_ResourceIdAmmo;
}