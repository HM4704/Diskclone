#include "StdAfx.h"
#include "HdMan.h"
#include "ReadDiskWorker.h"

CReadDiskWorker::CReadDiskWorker(int iNumWorkItems, int iDataSize, int iNumTasks) : CWorker(iNumWorkItems, iDataSize, iNumTasks)
{
    m_hDisk = INVALID_HANDLE_VALUE;
}

CReadDiskWorker::~CReadDiskWorker(void)
{
}

DWORD CReadDiskWorker::WorkerFunc(int i)
{
    RegisterTask();

    while (TRUE)
    {
        CWorkItem* pItem, *pItemOut;
        DISK_JOB* pJob;

        // Daten aus Eingans Fifo holen
        pItem = m_input.GetItemOutput();
        if (pItem)
        {
            pJob = (DISK_JOB*)pItem->m_pbyData;
            switch (pJob->JobId)
            {
            case _DISK_JOB::OpenDiskId:
                m_hDisk = HdOpenPhysicalDrive(pJob->OpenDisk.ulDiskId);
                break;

            case _DISK_JOB::ReadDiskId:
                if (m_pOutput)
                {
                    // freies Work Item vom Ausgangs Fifo holen
                    pItemOut = m_pOutput->GetItemInput();

                    pItemOut->m_iDataSize = HdReadPhysSectors(m_hDisk, pJob->ReadDisk.ulStartSector,
                        pJob->ReadDisk.ulCount, pItemOut->m_pbyData);


                    // Nummerierung des Chunks weitergeben
                    pItemOut->m_iOrderNum = pItem->m_iOrderNum;

                    // Work Item in Ausgangs Fifo ablegen
                    m_pOutput->SetItemInputDone(pItemOut);
                }
                break;

            case _DISK_JOB::CloseDiskId:
                CloseHandle(m_hDisk);
                break;

            case _DISK_JOB::SetDiskHandleId:
                m_hDisk = pJob->SetDiskHandle.hDisk;
                break;
            }
            
            // Work Item im Eingangs Fifo freigeben
            m_input.SetItemOutputDone(pItem);
        }
        else
        {
            printf("CReadDiskWorker: stopping\n");
            break; // end
        }
    }

    UnRegisterTask();

    return 0;
}
