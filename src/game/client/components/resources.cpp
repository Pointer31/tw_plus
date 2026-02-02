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


// const char * const CResources::ms_apSkinPartNames[NUM_SKINPARTS] = {"body", "marking", "decoration", "hands", "feet", "eyes"}; /* Localize("body","skins");Localize("marking","skins");Localize("decoration","skins");Localize("hands","skins");Localize("feet","skins");Localize("eyes","skins"); */
// const char * const CResources::ms_apColorComponents[NUM_COLOR_COMPONENTS] = {"hue", "sat", "lgt", "alp"};

// char *CResources::ms_apSkinVariables[NUM_SKINPARTS] = {0};
// int *CResources::ms_apUCCVariables[NUM_SKINPARTS] = {0};
// int *CResources::ms_apColorVariables[NUM_SKINPARTS] = {0};

// const float MIN_EYE_BODY_COLOR_DIST = 80.f; // between body and eyes (LAB color space)

int CResources::SkinScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	if(IsDir || !str_endswith(pName, ".png"))
		return 0;

	CResources *pSelf = (CResources *)pUser;

	pSelf->Console()->Print(0, "resources", "png detected!");
	
	int PartNameSize, PartNameCount;
	str_utf8_stats(pName, str_length(pName) - str_length(".png") + 1, IO_MAX_PATH_LENGTH, &PartNameSize, &PartNameCount);
	if(PartNameSize >= MAX_SKIN_ARRAY_SIZE || PartNameCount > MAX_SKIN_LENGTH)
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

	Part.m_OrgTexture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
	
	{
		str_format(aBuf, sizeof(aBuf), "load resource %s", Part.m_aName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "resources", aBuf);
	}

	pSelf->m_aResources.add(Part);

	return 0;
}

int CResources::GetInitAmount() const
{
	return NUM_SKINPARTS*5 + 8;
}

void CResources::OnInit()
{
	for (int index = 0; index < 64; index++)
	{
		str_copy(ResourceMapping[index], "\0", 64);
	}

	m_aResources.clear();
	Storage()->ListDirectory(IStorage::TYPE_ALL, "resources", SkinScan, this);
}

// void CResources::AddSkin(const char *pSkinName)
// {
// 	CSkin Skin = m_DummySkin;
// 	Skin.m_Flags = 0;
// 	str_utf8_copy_num(Skin.m_aName, pSkinName, sizeof(Skin.m_aName), MAX_SKIN_LENGTH);
// 	for(int PartIndex = 0; PartIndex < NUM_SKINPARTS; ++PartIndex)
// 	{
// 		int SkinPart = FindSkinPart(PartIndex, ms_apSkinVariables[PartIndex], false);
// 		if(SkinPart > -1)
// 			Skin.m_apParts[PartIndex] = GetSkinPart(PartIndex, SkinPart);
// 		Skin.m_aUseCustomColors[PartIndex] = *ms_apUCCVariables[PartIndex];
// 		Skin.m_aPartColors[PartIndex] = *ms_apColorVariables[PartIndex];
// 	}
// 	int SkinIndex = Find(Skin.m_aName, false);
// 	if(SkinIndex != -1)
// 		m_aSkins[SkinIndex] = Skin;
// 	else
// 		m_aSkins.add(Skin);
// }

// void CResources::RemoveSkin(const CSkin *pSkin)
// {
// 	m_aSkins.remove(*pSkin);
// }

// int CResources::Num()
// {
// 	return m_aSkins.size();
// }

// int CResources::NumSkinPart(int Part)
// {
// 	return m_aaSkinParts[Part].size();
// }

const CResources::CResource *CResources::Get(int ResourceId)
{
	if (ResourceMapping[ResourceId] && Find(ResourceMapping[ResourceId]) >= 0)
		return &m_aResources[Find(ResourceMapping[ResourceId])];
	else
		return &m_aResources[Find("unknown")];
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

	str_format(aBuf, sizeof(aBuf), "got resource id %i, name='%s'", Id, pName);

	str_copy(ResourceMapping[Id], pName, sizeof(ResourceMapping[Id]));
	// pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "resources", aBuf);
	Console()->Print(0, "resources", aBuf);
	return;
}

// const CResources::CSkinPart *CResources::GetSkinPart(int Part, int Index)
// {
// 	int Size = m_aaSkinParts[Part].size();
// 	return &m_aaSkinParts[Part][maximum(0, Index%Size)];
// }

// int CResources::FindSkinPart(int Part, const char *pName, bool AllowSpecialPart)
// {
// 	for(int i = 0; i < m_aaSkinParts[Part].size(); i++)
// 	{
// 		if(str_comp(m_aaSkinParts[Part][i].m_aName, pName) == 0 && ((m_aaSkinParts[Part][i].m_Flags&SKINFLAG_SPECIAL) == 0 || AllowSpecialPart))
// 			return i;
// 	}
// 	return -1;
// }
