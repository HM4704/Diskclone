#include "StdAfx.h"
#include "WorkItem.h"
#include "OrderedFifo.h"


COrderedFifo::~COrderedFifo(void)
{
    WIDone::iterator iter;

    // Liste mit freien Work Items leeren
    m_wiFree.clear();

    if (m_wiProcessed.empty() == false)
    {
        // Liste mit fertigen Work Items sollte leer sein
        printf("COrderedFifo: ERROR m_wiProcessed not empty!!!\n");
        m_wiProcessed.clear();
    }

    if (m_pbyData != NULL)
    {
        delete [] m_pbyData;
        m_pbyData = NULL;
    }

    if (m_pItems != NULL)
    {
        delete [] m_pItems;
        m_pItems = NULL;
    }
}


COrderedFifo::COrderedFifo()
{
    m_pbyData = NULL;
    m_pItems  = NULL;

    ::InitializeCriticalSection(&m_csInput);
    ::InitializeCriticalSection(&m_csOutput);

    m_hFreeList      = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hProcessedList = ::CreateEvent(NULL, FALSE, FALSE, NULL);


    m_iNextItem = 0;
    m_bStopped = false;

    m_lCntTasksInput = 0;
    m_lCntTasksOutput = 0;
}


// Erzeugen eines Fifos
bool COrderedFifo::Create(int iNumWorkItems, int iDataSize)
{
    // Speicher für Daten allokieren
    m_pbyData = new unsigned char[iNumWorkItems*iDataSize];

    m_iWorkItemCount = iNumWorkItems;
    m_iDataSize = iDataSize;

    // Speicher für Work Items allokieren
    m_pItems = new CWorkItem[m_iWorkItemCount];
    if (!m_pbyData || !m_pItems)
        return false;

    // Work Items initialisieren und in Free Liste ablegen
    for (int i = 0; i < m_iWorkItemCount; i++)
    {
        m_pItems[i].m_iDataSize = iDataSize;
        m_pItems[i].m_pbyData = &m_pbyData[i*iDataSize];
        m_pItems[i].m_iOrderNum = m_iWorkItemCount;
        m_wiFree.push_back(&m_pItems[i]);
    }

    return true;
}

// nächstes freies Work Item holen
CWorkItem* COrderedFifo::GetItemInput(void)
{
    while (!m_bStopped)
    {
        ::EnterCriticalSection(&m_csInput);

        if (m_wiFree.size() == 0)
        {
            // auf freies Work Item warten
            ::LeaveCriticalSection(&m_csInput);
            ::InterlockedIncrement(&m_lCntTasksInput);
            ::WaitForSingleObject(m_hFreeList, INFINITE);
            ::InterlockedDecrement(&m_lCntTasksInput);
        }
        else
        {
            // Work Item zurückgeben
            CWorkItem* &pItem = m_wiFree.front();
            m_wiFree.pop_front();
            ::LeaveCriticalSection(&m_csInput);
            return pItem;
        }
    }

    return NULL;
}

// Work Item in FIFO ablegen
void COrderedFifo::SetItemInputDone(CWorkItem* pItem)
{
    ::EnterCriticalSection(&m_csOutput);
    m_wiProcessed[pItem->m_iOrderNum] = pItem;
    ::LeaveCriticalSection(&m_csOutput);
    // Signal für neues Work Item
    ::SetEvent(m_hProcessedList);
}

// nächstes Work Item aus Output holen
CWorkItem* COrderedFifo::GetItemOutput(void)
{
    WIDone::iterator iter;

    while (!m_bStopped)
    {
        ::EnterCriticalSection(&m_csOutput);
        if (m_wiProcessed.empty())
        {
            // Liste ist leer, auf Work Item warten
            ::LeaveCriticalSection(&m_csOutput);
            ::InterlockedIncrement(&m_lCntTasksOutput);
            ::WaitForSingleObject(m_hProcessedList, INFINITE);
            ::InterlockedDecrement(&m_lCntTasksOutput);
        }
        else
        {
            iter = m_wiProcessed.begin();
            if (m_iNextItem == iter->first)
            {
                // ok, Item ist an der Reihe, aus Liste entfernen und zurückgeben
                m_iNextItem++;
                CWorkItem* pItem = (CWorkItem*)iter->second;
                m_wiProcessed.erase(iter);
                ::LeaveCriticalSection(&m_csOutput);
                return pItem;
            }
            else
            {
                // auf Item mit richtiger Nummer warten
                ::LeaveCriticalSection(&m_csOutput);
                ::InterlockedIncrement(&m_lCntTasksOutput);
                ::WaitForSingleObject(m_hProcessedList, INFINITE);
                ::InterlockedDecrement(&m_lCntTasksOutput);
            }
        }
    }

    return NULL;
}

// Work Item in Output freigeben
void COrderedFifo::SetItemOutputDone(CWorkItem* pItem)
{
    ::EnterCriticalSection(&m_csInput);
    m_wiFree.push_back(pItem);
    ::LeaveCriticalSection(&m_csInput);
    ::SetEvent(m_hFreeList);
}

void COrderedFifo::WaitEmpty(void)
{
    // auf Leerung der Liste mit fertigen Work Items warten
    while (m_wiProcessed.empty() == false)
    {
        ::Sleep(10);
    }
}

// Fifo stoppen
void COrderedFifo::Stop(void)
{
    WaitEmpty();

    // Flag zum Beenden setzen
    m_bStopped = true;

    // alle Tasks am Eingang beenden
    while (m_lCntTasksInput > 0)
    {
        ::SetEvent(m_hFreeList);
        ::Sleep(10);
    }

    // alle Tasks am Ausgang beenden
    while (m_lCntTasksOutput > 0)
    {
        ::SetEvent(m_hProcessedList);
        ::Sleep(10);
    }
}
