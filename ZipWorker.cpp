#include "StdAfx.h"
#include "ZipWorker.h"
#include "zlib.h"

CZipWorker::CZipWorker(int iNumWorkItems, int iDataSize, int iNumTasks) : CWorker(iNumWorkItems, iDataSize, iNumTasks)
{
}

CZipWorker::~CZipWorker(void)
{
}

DWORD CZipWorker::WorkerFunc(int i)
{
    RegisterTask();

    while (TRUE)
    {
        CWorkItem* pItem, *pItemOut;
        int iRet;

        // Input Daten aus dem Fifo holen
        // falls das Fifo leer ist, wird der Thread supendiert
        pItem = m_input.GetItemOutput();
        if (pItem && m_pOutput)
        {
            // nächstes freies Work Item aus dem Ausgangs Fifo holem
            // falls das Fifo voll ist, wird der Thread supendiert
            pItemOut = m_pOutput->GetItemInput();
            if (pItemOut)
            {
                pItemOut->m_iDataSize = m_pOutput->GetMaxDataSize();
                if ((iRet = compress(pItemOut->m_pbyData + sizeof(RLE_HEADER),   (uLongf *)&pItemOut->m_iDataSize,
                                 pItem->m_pbyData, pItem->m_iDataSize)) != Z_OK)
                {
                     return ReturnError("CZipWorker: ERROR %ld in compress()!\n");
                }
                pItemOut->m_iDataSize += sizeof(RLE_HEADER);
                ((RLE_HEADER*)pItemOut->m_pbyData)->dwSignature = RLE_SIGNATURE;
                ((RLE_HEADER*)pItemOut->m_pbyData)->dwLength    = pItemOut->m_iDataSize;
                
                // Nummer des Workitems setzen wie in dem Eingangs Workitem
                pItemOut->m_iOrderNum = pItem->m_iOrderNum;

                // Work Item im Ausgansgfifo ablegen
                m_pOutput->SetItemInputDone(pItemOut);
            }
        }
        else
            if (pItem == NULL)
            {
                printf("CZipWorker: stopping\n");
                break;  // end
            }
            else
            {
                return ReturnError("CZipWorker: ERROR no Output FIFO!\n");
            }
        m_input.SetItemOutputDone(pItem);
    }
    UnRegisterTask();

    return 0;
}

DWORD  CZipWorker::ReturnError(char* szError)
{
    printf(szError);
    UnRegisterTask();
    return -1;
}
