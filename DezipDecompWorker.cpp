#include "StdAfx.h"
#include "DezipDecompWorker.h"
#include "zlib.h"

CDezipDecompWorker::CDezipDecompWorker(int iNumWorkItems, int iDataSize, int iNumTasks) : CWorker(iNumWorkItems, iDataSize, iNumTasks)
{
}

CDezipDecompWorker::~CDezipDecompWorker(void)
{
}

DWORD CDezipDecompWorker::WorkerFunc(int i)
{
    RegisterTask();

    while (TRUE)
    {
        CWorkItem* pItem, *pItemOut;
        int iRet;

        // zu komprimierende Daten holen
        pItem = m_input.GetItemOutput();
        if (pItem)
        {
            // freies Work Item vom Ausgangs Fifo holen
            pItemOut = m_pOutput->GetItemInput();
            if (pItemOut)
            {
                // Ueberprüfung der Eingangsdaten
                if (((RLE_HEADER*)pItem->m_pbyData)->dwSignature != RLE_SIGNATURE)
                {
                    printf("CDezipDecompWorker: ERROR no valid Signature. Halt!!!!\n");
                    Sleep(INFINITE);
                }
                if (((RLE_HEADER*)pItem->m_pbyData)->dwLength != pItem->m_iDataSize)
                {
                    printf("CDezipDecompWorker: ERROR invalid length in Header. Halt!!!!\n");
                    Sleep(INFINITE);
                }
#ifdef USE_RLE
                pItemOut->m_iDataSize = m_comp.DeZipDeltaRLE(pItem->m_pbyData + sizeof(RLE_HEADER), pItemOut->m_pbyData, 
                                                             pItem->m_iDataSize - sizeof(RLE_HEADER));
#else  // USE_RLE
                // Komprimieren
                pItemOut->m_iDataSize = m_pOutput->GetMaxDataSize();
                if ((iRet = uncompress(pItemOut->m_pbyData, (uLongf *)&pItemOut->m_iDataSize, pItem->m_pbyData + sizeof(RLE_HEADER), 
                        pItem->m_iDataSize - sizeof(RLE_HEADER))) != Z_OK)
                {
                    printf("CDezipDecompWorker: ERROR %ld in uncompress() num=%ld. Halt!!!!\n", iRet, pItem->m_iOrderNum);
                    Sleep(INFINITE);
                }
#endif // USE_RLE
                // Nummerierung des Chunks weitergeben
                pItemOut->m_iOrderNum = pItem->m_iOrderNum;

                // Work Item in Ausgangs Fifo ablegen
                m_pOutput->SetItemInputDone(pItemOut);
            }
            else
            {
                printf("CDezipDecompWorker: stopping\n");
                break;  // end
            }
        }
        else
        {
            printf("CDezipDecompWorker: stopping\n");
            break;
        }

        // Work Item im Eingangs Fifo freigeben
        m_input.SetItemOutputDone(pItem);
    }

    UnRegisterTask();
    return 0;
}
