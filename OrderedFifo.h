#pragma once
#include <map>
#include <deque>
#include "Fifo.h"
#include "WorkItem.h"

using namespace std;


// Diese Klasse implementiert ein FIFO, bei dem die Reihenfolge 
// beim Auslesen durch eine eigene Nummerierung sichergestellt wird
class COrderedFifo : public IFifo
{
    typedef deque<CWorkItem*>  WIFree;      // Liste mit freien Work Items
    typedef map<int, CWorkItem*> WIDone;    // Map mit fertigen Work Items

    int m_iWorkItemCount;                   // Anzahl der Work Items
    int m_iNextItem;                        // Nummer des nächsten Work Items
    int m_iDataSize;                        // Grösse eines Chunks
    WIFree m_wiFree;                        // Liste mit freien Work Items
    WIDone m_wiProcessed;                   // Map mit fertigen Work Items

    unsigned char* m_pbyData;               // Zeiger auf Datenbuffer für die Chunks
    CWorkItem* m_pItems;                    // zeiger auf Buffer für die Work Items

    ::CRITICAL_SECTION m_csInput;           // critical section für freie Work Items
    ::CRITICAL_SECTION m_csOutput;          // critical section für fertige Work Items

    ::HANDLE m_hFreeList;                   // Handle auf Event zum Warten auf ein freies Work Item
    ::HANDLE m_hProcessedList;              // handle auf Event zum Warten auf ein fertiges Work Item

    volatile bool m_bStopped;               // Flag zum Stoppen des Fifos
    volatile long m_lCntTasksInput;         // Anzahl der wartenden Tasks am Eingang des Fifos
    volatile long m_lCntTasksOutput;        // Anzahl der wartenden Tasks am Ausgang des Fifos

private:
    // auf Leerung der Liste mit fertigen Work Items warten
    void WaitEmpty(void);

public:
    COrderedFifo();

    // Fifo erzeugen
    bool Create(int iNumWorkItems, int iDataSize);

    // Fifo stoppen
    void Stop(void);

    // naechstes freies Work Item holen
    CWorkItem* GetItemInput(void);

    // Work Item in FIFO ablegen
    void SetItemInputDone(CWorkItem* pItem);
    
    // nächstes Work Item aus Output holen
    CWorkItem* GetItemOutput(void);

    // Work Item in Output freigeben
    void SetItemOutputDone(CWorkItem*);

    // Eingestellte Chunk Grösse des Fifos zurückgeben
    int GetMaxDataSize(void) { return m_iDataSize; };

    // Nummerierung zurücksetzen
    void ResetOrder(void) { m_iNextItem = 0; };

public:
    virtual ~COrderedFifo(void);
};
