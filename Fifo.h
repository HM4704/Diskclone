#pragma once
#include "WorkItem.h"

// Interface Klasse für FIFO
class IFifo
{
public:

    // Fifo erzeugen
    virtual bool Create(int iNumWorkItems, int iDataSize) = 0;

    // Fifo stopppen
    virtual void Stop(void) = 0;

    // naechstes freies Work Item holen
    virtual CWorkItem* GetItemInput(void) = 0;

    // Work Item in FIFO anlegen
    virtual void SetItemInputDone(CWorkItem* pItem) = 0;
    
    // nächstes Work Item aus Output holen
    virtual CWorkItem* GetItemOutput(void) = 0;

    // Work Item in Output freigeben
    virtual void SetItemOutputDone(CWorkItem*) = 0;

    // Eingestellte Chunk Grösse des Fifos zurückgeben
    virtual int GetMaxDataSize(void) = 0;

protected:
    IFifo(void);
    virtual ~IFifo(void) = 0;
};

