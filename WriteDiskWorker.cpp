#include "StdAfx.h"
#include "HdMan.h"
#include "WriteDiskWorker.h"

CWriteDiskWorker::CWriteDiskWorker(int iDisk, int iNumWorkItems, int iDataSize) : CWorker(iNumWorkItems, iDataSize, 1)
{
    m_iDisk = iDisk;
}

CWriteDiskWorker::~CWriteDiskWorker(void)
{
}

DWORD CWriteDiskWorker::WorkerFunc(int i)
{
    unsigned int dwNextSector = 0, dwWritten;
    HANDLE hDisk = HdOpenPhysicalDrive(m_iDisk);
    if (hDisk == INVALID_HANDLE_VALUE)
    {
        printf("CWriteDiskWorker: ERROR %ld with opening disk %ld!!!\n", GetLastError(), m_iDisk);
        return 0;
    }

    RegisterTask();

    while (TRUE)
    {
        CWorkItem* pItem;

        // zu schreibende Daten holen
        pItem = m_input.GetItemOutput();
        if (pItem)
        {
            if ((dwWritten = HdWritePhysSectors(hDisk, dwNextSector,
                pItem->m_iDataSize/512, pItem->m_pbyData)) == 0)
            {
                printf("CWriteDiskWorker: ERROR writing sectors to disk!!!\n");
                Sleep(INFINITE);
            }
            dwNextSector += dwWritten;
        }   

        // Work Item im Eingangs Fifo freigeben
        m_input.SetItemOutputDone(pItem);
    }

    UnRegisterTask();
    return 0;
}
