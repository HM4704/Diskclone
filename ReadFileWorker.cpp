#include "StdAfx.h"
#include "ReadFileWorker.h"

CReadFileWorker::CReadFileWorker(int iChunkSize) : CWorker(1, 1)
{
    m_iChunkSize = iChunkSize;
    m_hFile = INVALID_HANDLE_VALUE;
}

CReadFileWorker::~CReadFileWorker(void)
{
    if (m_hFile != INVALID_HANDLE_VALUE)
        ::CloseHandle(m_hFile);
}


// Datei öffnen
bool CReadFileWorker::OpenFile(char* szFile)
{
    m_hFile = ::CreateFile(szFile, 
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);
    if (INVALID_HANDLE_VALUE == m_hFile)
        return false;
    else
        return true;
}

DWORD CReadFileWorker::WorkerFunc(int i)
{
    int iOrder = 0;

    if (m_pOutput == NULL)
    {
        printf("CReadFileWorker: ERROR no output fifo!!!\n");
        return 0;
    }

    RegisterTask();

    while (true)
    {
        CWorkItem* pItem;
        DWORD dwRead;

        // freies Work Item vom Ausgangs Fifo holen
        pItem = m_pOutput->GetItemInput();
        if (pItem)
        {
            if (m_hFile)
            {
                if (::ReadFile(m_hFile, pItem->m_pbyData, m_iChunkSize, &dwRead, NULL) == FALSE)
                {
                    printf("CWriteImageWorker: error %ld in WriteFile()\n!!!", GetLastError());
                }
                if (dwRead == 0)
                {
                    // Dateiende erreicht
                    printf("CReadFileWorker: end of file reached(%ld). Exiting. Order=%ld\n", dwRead, iOrder);
                    break;
                }
                if (dwRead != m_iChunkSize)
                {
//                    printf("CReadFileWorker: not enough bytes read. Halt.\n");
//                    Sleep(INFINITE);
                }
                pItem->m_iDataSize = dwRead;

                // Nummer für Work Item setzen
                pItem->m_iOrderNum = iOrder++;

                // Work Item in Ausgangs Fifo ablegen
                m_pOutput->SetItemInputDone(pItem);
            }
        }
        else
        {
            printf("CReadFileWorker: stopping\n");
            break; // end
        }
    }
    
    UnRegisterTask();

    return 0;
}
