// DiskClone.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include <conio.h>
#include <ctype.h>
#include "ImgFormat.h"
#include "HdMan.h"
#include "OrderedFifo.h"
#include "ReadDiskWorker.h"
#include "ZipWorker.h"
#include "DezipWorker.h"
#include "ReadFileWorker.h"
#include "WriteImageWorker.h"

#define DEBUG_
//#define WRITE_ENABLED
//#define USE_WRITE_OPT

#define CHUNK_SIZE (5*1024*1024)
#define NUM_ZIP_THREADS 2
#define NUM_WORKITEMS   10

const char* szVolumeSign[MAX_PARTITION_E] = { "\\\\.\\c:", "\\\\.\\d:" , "\\\\.\\e:", "\\\\.\\f:" };


// schreibe Partition in Image
// iDisk:       Nummer der Disk
// iPartNr:     Nummer der Partition
// szImage:     Name der Image Datei
void WritePartitionToImage(int iDisk, int iPartNr, char* szImage)
{
    DWORD dwChunkSize = CHUNK_SIZE;
    IMG_HEADER header;
    MBR mbr;
    LARGE_INTEGER liLastSect;
    LARGE_INTEGER liNextSect, liMaxSects;
    CWorkItem* pItem;
    int iOrder = 0;
    int iProgressInd, iDummy = 0;

    if (HDGetMBR(iDisk, (byte*)&mbr) == FALSE)
    {
        printf("Error in HDGetMBR(). Abort!!!\n");
        return;
    }

    printf("Writing partition(start: 0x%lx, len: 0x%lx, type: %ld) to image\n", mbr.pt.ptEntries[iPartNr].dwLbaStart,
        mbr.pt.ptEntries[iPartNr].dwPartSize, mbr.pt.ptEntries[iPartNr].byType);
    liLastSect = HdGetLastUsedVolumeSector((char*)szVolumeSign[iPartNr], mbr.pt.ptEntries[iPartNr].byType);

    // in absoluten Sektor wandeln
    liLastSect.QuadPart += mbr.pt.ptEntries[iPartNr].dwLbaStart;
    if (liLastSect.QuadPart > (mbr.pt.ptEntries[iPartNr].dwLbaStart + mbr.pt.ptEntries[iPartNr].dwPartSize))
    {
        liLastSect.QuadPart = (mbr.pt.ptEntries[iPartNr].dwLbaStart + mbr.pt.ptEntries[iPartNr].dwPartSize);
    }

    // Pipeline erstellen und starten
    CZipWorker zipWrk(NUM_WORKITEMS, dwChunkSize, NUM_ZIP_THREADS);
    CWriteImageWorker imgWrk(NUM_WORKITEMS, 2*dwChunkSize + 100);
    zipWrk.SetOutput(imgWrk.GetInput());
    imgWrk.OpenImageFile(szImage);
    zipWrk.StartTasks();
    imgWrk.StartTasks();

    HANDLE hDisk = HdOpenPhysicalDrive(iDisk);

    COrderedFifo* pZipFifo = zipWrk.GetInput();

    // Header initialisieren
    memset(&header, 0, sizeof(header));
    header.dwImageVersion   = DC_IMG_VERS;
    header.dwCloneType      = DC_CLONE_PART;
    header.PartInfo[iPartNr].dwUsed = 1;
    header.PartInfo[iPartNr].llOffset = mbr.pt.ptEntries[iPartNr].dwLbaStart;
    header.PartInfo[iPartNr].llLength = mbr.pt.ptEntries[iPartNr].dwPartSize;
    
    pItem = pZipFifo->GetItemInput();
    memcpy(pItem->m_pbyData, &header, sizeof(header));
    pItem->m_iOrderNum = iOrder;
    pItem->m_iDataSize = sizeof(header);
    pZipFifo->SetItemInputDone(pItem);
    iOrder++;

    // MBR schreiben
    pItem = pZipFifo->GetItemInput();
    memcpy(pItem->m_pbyData, &mbr, sizeof(mbr));
    pItem->m_iOrderNum = iOrder;
    pItem->m_iDataSize = sizeof(mbr);
    pZipFifo->SetItemInputDone(pItem);
    iOrder++;

    liNextSect.QuadPart = mbr.pt.ptEntries[iPartNr].dwLbaStart;
    if (hDisk != INVALID_HANDLE_VALUE)
    {
        DWORD dwSects;
        iProgressInd = 0;
        int iActInd;
        liMaxSects.QuadPart = liLastSect.QuadPart;
        while (liLastSect.QuadPart > 0)
        {
            // nächstes freies Work Item aus dem Ausgangs Fifo holem
            pItem = pZipFifo->GetItemInput();

            if (liLastSect.QuadPart < (dwChunkSize/SECT_SIZE))
                dwSects = (DWORD)liLastSect.QuadPart;
            else
                dwSects = (dwChunkSize/SECT_SIZE);

            if ((pItem->m_iDataSize = HdReadPhysSectors(hDisk, (DWORD)liNextSect.QuadPart, dwSects, pItem->m_pbyData)) == 0)
            {
                printf("\nError %ld with reading physical sectors\n", GetLastError());
                return;
            }
            liNextSect.QuadPart += pItem->m_iDataSize/SECT_SIZE; 
            liLastSect.QuadPart -= pItem->m_iDataSize/SECT_SIZE;

            // Nummer des Workitems setzen 
            pItem->m_iOrderNum = iOrder;
            iOrder++;

            // Work Item im Ausgansgfifo ablegen
            pZipFifo->SetItemInputDone(pItem);

            iActInd = (int)(((long int)((liMaxSects.QuadPart - liLastSect.QuadPart)*100)/(long int)liMaxSects.QuadPart));
            if (iActInd != iProgressInd)
            {
                iProgressInd = iActInd;
                printf("%ld done\r", iProgressInd);
            }
        }
    }
    else
        printf("\nError %ld with opening physical disk\n", GetLastError());

    // Tasks stoppen
    imgWrk.StopTasks();
    zipWrk.StopTasks();

    CloseHandle(hDisk);

    printf("\nWrite Partition to Image done successfully!!\n");

    return;

}

// Image auf Partition schreiben
// iDisk:       Nummer der Disk
// iDestPartNr: Nummer der Partition
// szImage:     Name der Image Datei
void WriteImageToPartition(int iDisk, int iDestPartNr, char* szImage)
{
    IMG_HEADER* pHead;
    CWorkItem* pItem;
    MBR mbr, *pMbr, *piMbr;
    DWORD dwChunkSize = CHUNK_SIZE, i;
    LARGE_INTEGER liCountSects, liNextSect, liMaxSects;
    int iProgressInd;

    pMbr = (MBR*)VirtualAlloc(NULL, SECT_SIZE, MEM_COMMIT, PAGE_READWRITE);
    if (pMbr == NULL)
    {
        printf("WriteImageToPartition: Error can't get memory. Abort!!!\n");
        return;
    }
    memset(pMbr, 0, SECT_SIZE);

    printf("This action will overwrite data on partition %d. Continue(y/n)?\n", iDestPartNr);
    if (_getch() != 'y')
        return;

    if (HDGetMBR(iDisk, (byte*)pMbr) == FALSE)
    {
        printf("WriteImageToPartition: Error in HDGetMBR(). Abort!!!\n");
        return;
    }

    memcpy(&mbr, pMbr, sizeof(mbr));

    // Pipeline aufbauen und starten
    CDezipWorker dezipWrk(NUM_WORKITEMS, 2*dwChunkSize + 100, NUM_ZIP_THREADS);
    CReadFileWorker flWrk(2*1024*1024);
    COrderedFifo fifoRd;
    fifoRd.Create(NUM_WORKITEMS, 2*dwChunkSize);
    flWrk.SetOutput(dezipWrk.GetInput());
    flWrk.OpenFile(szImage);
    dezipWrk.SetOutput(&fifoRd);

    flWrk.StartTasks();
    dezipWrk.StartTasks();


    // get first chunk with header
    pItem = fifoRd.GetItemOutput();
    pHead = (IMG_HEADER*)pItem->m_pbyData;
    if (pHead->dwImageVersion != DC_IMG_VERS)
    {
        printf("WriteImageToPartition: ERROR file with wrong header. Abort.\n");
        return;
    }
    if (pHead->dwCloneType != DC_CLONE_PART)
    {
        printf("WriteImageToPartition: ERROR image type not supported yet. Abort.\n");
        return;
    }
    for (i = 0; i < DC_MAX_PARTITIONS; i++)
    {
        if (pHead->PartInfo[0].dwUsed != 0)
        {
            // Partition found
            break;
        }
    }
    if (i == DC_MAX_PARTITIONS)
    {
        printf("WriteImageToPartition: ERROR image contains no partition\n");
        return;
    }
    liNextSect.QuadPart = pHead->PartInfo[i].llOffset;
    liCountSects.QuadPart = pHead->PartInfo[i].llLength;
    printf("Image contains following partition: offset - %ld   sectors - %ld\n", 
        (DWORD)pHead->PartInfo[0].llOffset, (DWORD)pHead->PartInfo[0].llLength);
    liMaxSects.QuadPart = liCountSects.QuadPart;
    
    fifoRd.SetItemOutputDone(pItem);
    
    // get next chunk with mbr
    pItem  = fifoRd.GetItemOutput();
    piMbr  = (MBR*)pItem->m_pbyData;
    if ((pMbr->bySignature[0] != 0x55) || (pMbr->bySignature[1] != 0xaa))
    {
        printf("WriteImageToPartition: ERROR no valid mbr in image!!\n");
        return;
    }
    
    // write new mbr
    if ((mbr.bySignature[0] != 0x55) || (mbr.bySignature[1] != 0xaa))
    {
        // kein gueltiger MBR, den MBR aus dem Image komplett schreiben
        memcpy(&mbr, piMbr, sizeof(mbr));
    }
    else
    {
        memcpy(&mbr.pt.ptEntries[iDestPartNr], &piMbr->pt.ptEntries[i], sizeof(mbr.pt.ptEntries[iDestPartNr]));
    }

    memcpy(pMbr, &mbr, sizeof(mbr));

    HANDLE hDisk = HdOpenPhysicalDrive(iDisk);
#ifdef WRITE_ENABLED
    if ((pItem->m_iDataSize = HdWritePhysSectors(hDisk, (DWORD)0, 1, (byte*)pMbr)) == 0)
    {
        printf("\nError %ld with reading physical sectors\n", GetLastError());
        return;
    }
#endif // WRITE_ENABLED

    fifoRd.SetItemOutputDone(pItem);

    // write all Sectors
    if (hDisk != INVALID_HANDLE_VALUE)
    {
        DWORD dwSects;
        iProgressInd = 0;
        int iActInd;
        while (liCountSects.QuadPart > 0)
        {
            pItem = fifoRd.GetItemOutput();
            if ((pItem->m_iDataSize % SECT_SIZE) != 0)
            {
                if ((pItem->m_iDataSize % SECT_SIZE) == 1)
                {
                    printf("WriteImageToDisk: WARNING correcting chunk size.\n");
                    pItem->m_iDataSize--;
                }
                else
                {
                    printf("WriteImageToDisk: ERROR rcvd wrong chunk size 0x%lx. Abort.\n", pItem->m_iDataSize);
                    break;
                }
            }
            dwSects = pItem->m_iDataSize/SECT_SIZE;
            if (pItem->m_iDataSize > dwChunkSize)
            {
                printf("WriteImageToDisk: WARNING unexpected chunk size 0x%lx.\n", pItem->m_iDataSize);
            }

#ifdef WRITE_ENABLED
            if ((pItem->m_iDataSize = HdWritePhysSectors(hDisk, (DWORD)liNextSect.QuadPart, dwSects, pItem->m_pbyData)) == 0)
            {
                printf("\nError %ld with reading physical sectors\n", GetLastError());
                return;
            }
#endif // WRITE_ENABLED
            liNextSect.QuadPart += dwSects; 
            liCountSects.QuadPart -= dwSects;
            fifoRd.SetItemOutputDone(pItem);
            if (liCountSects.QuadPart <= 0)
            {
                if (liCountSects.QuadPart < 0)
                {
                    printf("WriteImageToDisk: ERROR rcvd more chunks as expected. Abort.\n");
                    return;
                }
                else
                    break;  // end
            }
            iActInd = (int)((long int)((long int)((liMaxSects.QuadPart - liCountSects.QuadPart)*100)/(long int)liMaxSects.QuadPart));
            if (iActInd != iProgressInd)
            {
                iProgressInd = iActInd;
                printf("%ld done\r", iProgressInd);
            }
        }
    }
    else
        printf("\nError %ld with opening physical disk\n", GetLastError());

    flWrk.StopTasks();
    dezipWrk.StopTasks();

    CloseHandle(hDisk);

    VirtualFree(pMbr, 0, MEM_RELEASE);

    printf("\nWrite Image to Disk done successfully!!\n");

    return;

}

// Disk in Image schreiben
// iDisk:       Nummer der Disk
// iDestPartNr: Nummer der Partition
void WriteDiskToImage(int iDisk, char* szImage)
{
    DWORD dwChunkSize = CHUNK_SIZE;
    IMG_HEADER header;
    CWorkItem* pItem;
    LARGE_INTEGER liLastSect;
    LARGE_INTEGER liDiskSize, liNextSect, liMaxSects;
    int iOrder = 0;
    int iDummy = 0;
    int iProgressInd;

    liLastSect = HdGetLastUsedVolumeSector("\\\\.\\c:", PART_TYPE_NTFS);
    liLastSect = HdGetLastUsedDiskSector(iDisk, 0);
    printf("\n\n\nLast used Sector on disk: %ld\n", liLastSect.QuadPart);
    liDiskSize = HdGetDiskSize(iDisk);
    printf("\n\nSize of disk: %ld\n", liDiskSize.QuadPart);

    if (liDiskSize.QuadPart < liLastSect.QuadPart)
        liLastSect = liDiskSize;
    printf("\n\nUsed Size for image: %ld\n", liLastSect.QuadPart);

    // Pipeline erstellen
    // Zip Worker erstellen
    CZipWorker zipWrk(NUM_WORKITEMS, dwChunkSize, NUM_ZIP_THREADS);

    // Image Writer Worker erstellen, nur 1 Thread
    CWriteImageWorker imgWrk(NUM_WORKITEMS, 2*dwChunkSize + 100);

    // Zip Worker mit Image Writer Worker verbinden
    zipWrk.SetOutput(imgWrk.GetInput());
    imgWrk.OpenImageFile(szImage);

    // Threads starten
    zipWrk.StartTasks();
    imgWrk.StartTasks();

    // Handle auf Disk holen
    HANDLE hDisk = HdOpenPhysicalDrive(iDisk);

    COrderedFifo* pZipFifo = zipWrk.GetInput();

    // Header in Image Datei schreiben
    memset(&header, 0, sizeof(header));
    header.dwImageVersion   = DC_IMG_VERS;
    header.dwCloneType      = DC_CLONE_DISK;
    header.PartInfo[0].dwUsed = 1;
    header.PartInfo[0].llOffset = 0;
    header.PartInfo[0].llLength = liLastSect.QuadPart;    
    pItem = pZipFifo->GetItemInput();
    memcpy(pItem->m_pbyData, &header, sizeof(header));
    pItem->m_iOrderNum = iOrder;
    pItem->m_iDataSize = sizeof(header);
    pZipFifo->SetItemInputDone(pItem);
    iOrder++;

    liNextSect.QuadPart = 0;
    if (hDisk != INVALID_HANDLE_VALUE)
    {
        DWORD dwSects;
        iProgressInd = 0;
        int iActInd;
        liMaxSects.QuadPart = liLastSect.QuadPart;
        while (liLastSect.QuadPart > 0)
        {
            pItem = pZipFifo->GetItemInput();
            if (liLastSect.QuadPart < (dwChunkSize/SECT_SIZE))
                dwSects = (DWORD)liLastSect.QuadPart;
            else
                dwSects = (dwChunkSize/SECT_SIZE);

            if ((pItem->m_iDataSize = HdReadPhysSectors(hDisk, (DWORD)liNextSect.QuadPart, dwSects, pItem->m_pbyData)) == 0)
            {
                printf("\nError %ld with reading physical sectors\n", GetLastError());
                return;
            }
            liNextSect.QuadPart += dwSects; 
            liLastSect.QuadPart -= dwSects;
            pItem->m_iOrderNum = iOrder;
            pItem->m_iDataSize = dwSects*SECT_SIZE;
            pZipFifo->SetItemInputDone(pItem);
            iOrder++;

            iActInd = (int)(((long int)((liMaxSects.QuadPart - liLastSect.QuadPart)*100)/(long int)liMaxSects.QuadPart));
            if (iActInd != iProgressInd)
            {
                iProgressInd = iActInd;
                printf("%ld \%\r", iProgressInd);
            }
        }
    }
    else
        printf("\nError %ld with opening physical disk\n", GetLastError());

    zipWrk.StopTasks();
    imgWrk.StopTasks();

    CloseHandle(hDisk);

    printf("\nWrite Disk to Image done successfully!!\n");

    return;
}

// Image auf Disk schreiben
// iDisk:       Nummer der Disk
// szImage:     Name der Image Datei
// bForce:      mit (false) oder ohne Nachfrage
void WriteImageToDisk(int iDisk, char* szImage, bool bForce)
{
    IMG_HEADER* pHead;
    CWorkItem* pItem;
    DWORD dwChunkSize = CHUNK_SIZE;
    LARGE_INTEGER liCountSects, liNextSect, liMaxSects;
    int iProgressInd;

    if (bForce == false)
    {
        printf("This action will destroy all data on disk. Continue(y/n)?\n");
        if (_getch() != 'y')
            return;
    }

    // Pipeline aufbauen
    COrderedFifo fifoRd;
    CDezipWorker dezipWrk(NUM_WORKITEMS, 2*dwChunkSize + 100, NUM_ZIP_THREADS);
    CReadFileWorker flWrk(dwChunkSize);
    fifoRd.Create(NUM_WORKITEMS, 2*dwChunkSize);
    flWrk.SetOutput(dezipWrk.GetInput());
    dezipWrk.SetOutput(&fifoRd);

    if (flWrk.OpenFile(szImage) == false)
    {
        printf("WriteImageToDisk: ERROR can't open image file. Abort.\n");
        return;
    }

    BYTE* pbyReadData = new BYTE[2*dwChunkSize];
    if (pbyReadData == NULL)
    {
        printf("WriteImageToDisk: ERROR can't allocate memory. Abort.\n");
        return;
    }

    // Tasks starten
    flWrk.StartTasks();
    dezipWrk.StartTasks();

    // get first chunk with header
    pItem = fifoRd.GetItemOutput();
    pHead = (IMG_HEADER*)pItem->m_pbyData;

    if (pHead->dwImageVersion != DC_IMG_VERS)
    {
        printf("WriteImageToDisk: ERROR file with wrong header. Abort.\n");
        return;
    }
    if (pHead->dwCloneType != DC_CLONE_DISK)
    {
        printf("WriteImageToDisk: ERROR image type not supported yet. Abort.\n");
        return;
    }
    if (pHead->PartInfo[0].dwUsed != 1)
    {
        printf("WriteImageToDisk: ERROR file contains no image. Abort.\n");
        return;
    }
    liNextSect.QuadPart = pHead->PartInfo[0].llOffset;
    liCountSects.QuadPart = pHead->PartInfo[0].llLength;
    printf("Image contains following disk: offset - %ld   sectors - %ld\n", 
        (DWORD)pHead->PartInfo[0].llOffset, (DWORD)pHead->PartInfo[0].llLength);
    liMaxSects.QuadPart = liCountSects.QuadPart;
    

    fifoRd.SetItemOutputDone(pItem);

    // write all Sectors
    HANDLE hDisk = HdOpenPhysicalDrive(iDisk);
    if (hDisk != INVALID_HANDLE_VALUE)
    {
        DWORD dwSects;
        iProgressInd = 0;
        int iActInd;
        while (liCountSects.QuadPart > 0)
        {
            pItem = fifoRd.GetItemOutput();
            if ((pItem->m_iDataSize % SECT_SIZE) != 0)
            {
                if ((pItem->m_iDataSize % SECT_SIZE) == 1)
                {
                    printf("WriteImageToDisk: WARNING correcting chunk size.\n");
                    pItem->m_iDataSize--;
                }
                else
                {
                    printf("WriteImageToDisk: ERROR rcvd wrong chunk size 0x%lx. Abort.\n", pItem->m_iDataSize);
                    break;
                }
            }
            dwSects = pItem->m_iDataSize/SECT_SIZE;
            if (pItem->m_iDataSize > dwChunkSize)
            {
                printf("WriteImageToDisk: WARNING unexpected chunk size 0x%lx.\n", pItem->m_iDataSize);
            }

#ifdef WRITE_ENABLED
#ifdef USE_WRITE_OPT
            if (HdReadPhysSectors(hDisk, (DWORD)liNextSect.QuadPart, dwSects, pbyReadData) == 0)
            {
                printf("\nError %ld with reading physical sectors\n", GetLastError());
                return;
            }
            if (memcmp(pbyReadData, pItem->m_pbyData, dwSects*SECT_SIZE) != 0)
            {
                // nur schreiben wenn Daten sich geaendert haben, da Disk Schreiben langsam ist am Bladeserver
                if ((pItem->m_iDataSize = HdWritePhysSectors(hDisk, (DWORD)liNextSect.QuadPart, dwSects, pItem->m_pbyData)) == 0)
                {
                    printf("\nError %ld with reading physical sectors\n", GetLastError());
                    return;
                }
            }
#else
            if ((pItem->m_iDataSize = HdWritePhysSectors(hDisk, (DWORD)liNextSect.QuadPart, dwSects, pItem->m_pbyData)) == 0)
            {
                printf("\nError %ld with writing physical sectors\n", GetLastError());
                return;
            }
#endif // USE_WRITE_OPT

#endif // WRITE_ENABLED
            liNextSect.QuadPart += dwSects; 
            liCountSects.QuadPart -= dwSects;
            fifoRd.SetItemOutputDone(pItem);
            if (liCountSects.QuadPart <= 0)
            {
                if (liCountSects.QuadPart < 0)
                {
                    printf("WriteImageToDisk: ERROR rcvd more chunks as expected. Abort.\n");
                    return;
                }
                else
                    break;  // end
            }
            iActInd = (int)((double)((double)(liMaxSects.QuadPart - liCountSects.QuadPart)/(double)liMaxSects.QuadPart)*100);
            if (iActInd != iProgressInd)
            {
                iProgressInd = iActInd;
                printf("%ld done\r", iProgressInd);
            }
        }
    }
    else
        printf("\nError %ld with opening physical disk\n", GetLastError());

    flWrk.StopTasks();
    dezipWrk.StopTasks();

    CloseHandle(hDisk);


    printf("\nWrite Image to Disk done successfully!!\n");

    return;
}


void Demo(int MaxNum)
{

}

void Help(void)
{
    printf("\nDiskclone 0.1\n");
    printf("Usage: Diskclone [action] [Imagename]\n");
    printf("parameters for action:\n");
    printf("    c - create image\n");
    printf("    w - write image back to disk\n");
    printf("    f - write image back to disk without question (force write)\n");
}

int _tmain(int argc, _TCHAR* argv[])
{

    LARGE_INTEGER liLastSect;
    HDDisplayPartitionTableAndChilds(0, 0);
    liLastSect = HdGetLastUsedDiskSector(0, 0);
    printf("\n\n\nLast used Sector on disk: %ld\n", liLastSect.QuadPart);
    liLastSect = HdGetDiskSize(0);
    printf("\n\n\nSize of disk: %ld\n", liLastSect.QuadPart);

    if (argc > 2)
    {
        if (argv[1][0] == 'c')
        {
            if (argc < 4)
                WriteDiskToImage(4, argv[2]);
            else
                WritePartitionToImage(4, atoi(argv[2]), argv[3]);
        }
        else
        {
            if (argv[1][0] == 'w')
            {
                if (argc < 4)
                    WriteImageToDisk(4, argv[2], false);
                else
                    WriteImageToPartition(4, atoi(argv[2]), argv[3]);
            }
            else
            if (argv[1][0] == 'f')
            {
                if (argc < 4)
                    WriteImageToDisk(4, argv[2], true);
                else
                    WriteImageToPartition(4, atoi(argv[2]), argv[3]);
            }
            else
                Help();
        }
    }
    else
    {
        Help();
    }

	return 0;
}

