#include "StdAfx.h"
#include "DezipWorker.h"
#include "DezipDecompWorker.h"

CDezipWorker::CDezipWorker(int iNumWorkItems, int iDataSize, int iNumTasks) : CWorker(iNumWorkItems, iDataSize, 1)
{
    m_iDataSize      = iDataSize;
    m_iNumWorkItems  = iNumWorkItems;
    m_iNumDezipTasks = iNumTasks;
}

CDezipWorker::~CDezipWorker(void)
{
}

DWORD CDezipWorker::WorkerFunc(int i)
{
    CWorkItem* pItem, *pItemOut;
    unsigned int dwChunkSize, dwInputLength, dwChunkPos, dwInputPos, dwCount;
    int iOrder = 0;

    RegisterTask();

    // Pipeline aufbauen
    CDezipDecompWorker zipWrk(m_iNumWorkItems, m_iDataSize, m_iNumDezipTasks);
    zipWrk.SetOutput(m_pOutput);
    m_pOutput = zipWrk.GetInput();
    zipWrk.StartTasks();

    dwChunkPos = dwInputPos = 0;

    // Item von Eingangs- und Ausgangs Fifo holen
    pItem = m_input.GetItemOutput();
    pItemOut = m_pOutput->GetItemInput();
    if (pItemOut == NULL || pItem == NULL)
    {
        return ReturnError("CDezipWorker: ERROR no item\n");
    }
    
    // Kennung haben im ersten Chunk überprüfen
    if (((RLE_HEADER*)pItem->m_pbyData)->dwSignature != RLE_SIGNATURE)
    {
        return ReturnError("CDezipWorker: ERROR no valid Signature. Halt!!!!\n");
    }
    
    // ok, Kennung passt, anschliessend folgt Länge
    dwChunkSize   = ((RLE_HEADER*)pItem->m_pbyData)->dwLength;
    dwInputLength = pItem->m_iDataSize;

    while (true)
    {
        // Laenge der zu kopierenden Daten bestimmen
        dwCount = __min((dwChunkSize-dwChunkPos), (dwInputLength - dwInputPos));

        memcpy(pItemOut->m_pbyData + dwChunkPos, pItem->m_pbyData + dwInputPos, dwCount);
        dwChunkPos += dwCount;
        dwInputPos += dwCount;
        if (dwChunkPos == dwChunkSize)
        {
            // ein Chunk ist komplett, abschliessen
            pItemOut->m_iOrderNum = iOrder++;
            pItemOut->m_iDataSize = dwChunkSize;
            m_pOutput->SetItemInputDone(pItemOut);
            pItemOut = m_pOutput->GetItemInput();
            if (pItemOut == NULL)
            {
                break;
            }
            
            dwChunkPos = 0;

            // an Input Pos muss jetzt Header Kennung sein
            if (((RLE_HEADER*)(pItem->m_pbyData+dwInputPos))->dwSignature != RLE_SIGNATURE)
            {
                if (dwInputPos == dwInputLength)
                {
                    // das war letzter Chunk, Ende
                    m_input.SetItemOutputDone(pItem);
                    printf("CDezipWorker: last chunk detected! Waiting for stop\n");
                    // muss warten bis Decomp Tasks fertig sind
                    if (m_input.GetItemOutput() != NULL)
                    {
                        return ReturnError("CDezipWorker: ERROR got unexpected input chunk!\n");
                    }
                    // Decomp Tasks beenden
                    printf("CDezipWorker: stopping tasks\n");
                    zipWrk.StopTasks();
                    break;
                }
                else
                {
                    return ReturnError("CDezipWorker: ERROR no valid Signature. Halt!!!!\n");
                }
            }            

            // ok, Kennung passt, anschliessend folgt Länge
            dwChunkSize   = ((RLE_HEADER*)(pItem->m_pbyData+dwInputPos))->dwLength;
        }
        if (dwInputPos == dwInputLength)
        {
            // Eingangsdaten verbraucht, neue holen
            m_input.SetItemOutputDone(pItem);
            pItem = m_input.GetItemOutput();
            if (pItem == NULL)
            {
                // Endekennung
                break;
            }
            dwInputPos = 0;
            dwInputLength = pItem->m_iDataSize;
        }
    }

   printf("CDezipWorker: stopping\n");

   UnRegisterTask();

   return 0;

}

DWORD  CDezipWorker::ReturnError(char* szError)
{
    printf(szError);
    UnRegisterTask();
    return -1;
}
