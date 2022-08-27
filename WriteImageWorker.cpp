#include "StdAfx.h"
#include "WriteImageWorker.h"

CWriteImageWorker::CWriteImageWorker(int iNumWorkItems, int iDataSize) : CWorker(iNumWorkItems, iDataSize)
{
    m_hImage = INVALID_HANDLE_VALUE;
    m_iFlags = 0;
}

CWriteImageWorker::~CWriteImageWorker(void)
{
}

DWORD CWriteImageWorker::WorkerFunc(int i)
{
    RegisterTask();
    while (TRUE)
    {
        CWorkItem* pItem;
        DWORD dwWritten;

        // zu schreibende Daten holen
        pItem = m_input.GetItemOutput();
        if (pItem)
        {
            if (m_hImage)
            {
                if (::WriteFile(m_hImage, pItem->m_pbyData, pItem->m_iDataSize, &dwWritten, NULL) == FALSE)
                {
                    printf("CWriteImageWorker: error %ld in WriteFile()\n!!!", GetLastError());
                }
            }

            // Work Item im Eingangs Fifo freigeben
            m_input.SetItemOutputDone(pItem);
        }
        else
        {
            printf("CWriteImageWorker: stopping\n");
            break;
        }
    }

    UnRegisterTask();
    return 0;
}

bool CWriteImageWorker::OpenImageFile(char* szFile)
{
    m_hImage = ::CreateFile(szFile, 
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);
    if (INVALID_HANDLE_VALUE == m_hImage)
        return false;
    else
        return true;
}

