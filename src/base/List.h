/*
 * List.h
 *
 *  Created on: 14.04.2013
 *      Author: Teetime
 */

#ifndef BASE_LIST_H
#define BASE_LIST_H

#include<cstring>
#include<cstdlib>
#ifdef LIST_DEBUG
#include<cstdio>
#endif

#ifdef LIST_DEBUG
static size_t gs_ByteAlloc = 0;
static size_t gs_ByteFreed = 0;
#endif

template <class T>
class CList
{
private:
	typedef T CItem;

	class CNode
	{
	public:
		CNode() { memset(this, 0, sizeof(*this)); }
		CItem m_DataItem;
		CNode *m_pNext;
		CNode *m_pPrev;
		//TODO: Maybe something for fast-forward like access to every tenth node, so we speed it up if we have many nodes

#ifdef LIST_DEBUG
		void *operator new(size_t size)
		{
			gs_ByteAlloc += size;
			return static_cast<CNode *> (malloc(size));
		}
		void operator delete(void *pPtr)
		{
			gs_ByteFreed += sizeof(CNode);
			free(pPtr);
		}
#endif
	};

	CNode *m_pStart;
	CNode *m_pEnd;
	unsigned m_NumNodes;
	unsigned m_UpperLimit;

	void RemoveItem(CNode *pItem)
	{
		if(pItem->m_pNext)
			pItem->m_pNext->m_pPrev = pItem->m_pPrev;
		if(pItem->m_pPrev)
			pItem->m_pPrev->m_pNext = pItem->m_pNext;
		else
			m_pStart = pItem->m_pNext;

		delete pItem;
		pItem = 0;
		m_NumNodes--;
	}

	void SetUpperLimit(unsigned NewLimit)
	{
		m_UpperLimit = NewLimit;
	}

	CNode *GetNode(unsigned Pos)
	{
		if(Pos >= m_NumNodes)
			return 0;

		//If we request access to Pos > NumNodes/2 we go from behind
		if(m_NumNodes/2 < Pos)
		{
			for(CNode *pNode = m_pStart; pNode; pNode = pNode->m_pNext, Pos--)
				if(Pos == 0)
					return pNode;
		}
		else
		{
			Pos = m_NumNodes - Pos - 1;
			for(CNode *pNode = m_pEnd; pNode; pNode = pNode->m_pPrev, Pos--)
				if(Pos == 0)
					return pNode;
		}
		return 0;
	}

public:
	typedef bool (*ArgItemCallback)(const CItem *pItem);

	CList(int UpperLimit = 128)
	{
		memset(this, 0, sizeof(*this));
		SetUpperLimit(UpperLimit);
	}

	~CList()
	{
		CNode *pNext = m_pStart;
		while(m_pStart)
		{
			pNext = m_pStart->m_pNext;
			delete m_pStart;
			m_pStart = pNext;
		}
	}

#ifdef LIST_DEBUG
	static void PrintMemStats()
	{
		printf("Total alloc: %d\n", gs_ByteAlloc);
		printf("Total freed: %d\n", gs_ByteFreed);
		printf("       Diff: %d\n", gs_ByteAlloc - gs_ByteFreed);
	}
#endif

	unsigned Num()
	{
		return m_NumNodes;
	}

	bool AddItem(CItem &Item, bool ToFront = false)
	{
		if(m_UpperLimit > 0 && m_NumNodes >= m_UpperLimit)
			return false;

		CNode *pNewItem = new CNode();
		memcpy(&pNewItem->m_DataItem, &Item, sizeof(pNewItem->m_DataItem));

		//Insert
		if(!m_pStart)
		{
			m_pStart = pNewItem;
			m_pEnd = pNewItem;
		}
		else
		{
			if(ToFront)
			{
				pNewItem->m_pNext = m_pStart;
				m_pStart->m_pPrev = pNewItem;
				m_pStart = pNewItem;
			}
			else
			{
				m_pEnd->m_pNext = pNewItem;
				pNewItem->m_pPrev = m_pEnd;
				m_pEnd = pNewItem;
			}
		}

		m_NumNodes++;
		return true;
	}

	bool Add(CItem &Item)
	{
		return AddItem(Item);
	}

	const CItem *Get(unsigned int Pos)
	{
		CNode *pNode = GetNode(Pos);
		if(pNode)
		{
			return &pNode->m_DataItem;
		}
		return 0;
	}

	bool Remove(unsigned int Pos)
	{
		CNode *pNode = GetNode(Pos);
		if(!pNode)
			return false;

		RemoveItem(pNode);
		return true;
	}

	int Remove(CItem &Item)
	{
		int NumDeleted = 0;
		CNode *pCurr, *pNext;
		for(pNext = pCurr = m_pStart; pCurr; pCurr = pNext)
		{
			pNext = pCurr->m_pNext;
			if(!memcmp(&pCurr->m_DataItem, &Item, sizeof(CItem)))
			{
				RemoveItem(pCurr);
				NumDeleted++;
			}
		}
		return NumDeleted;
	}

	int Remove(ArgItemCallback pfnCallback)
	{
		int NumDeleted = 0;
		CNode *pCurr, *pNext;
		for(pNext = pCurr = m_pStart; pCurr; pCurr = pNext)
		{
			pNext = pCurr->m_pNext;
			if(pfnCallback(&pCurr->m_DataItem))
			{
				RemoveItem(pCurr);
				NumDeleted++;
			}
		}
		return NumDeleted;
	}
};

#endif
