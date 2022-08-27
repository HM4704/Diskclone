//#define WIN_LEAN_AND_MEAN
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <Winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
#define OS_DEPENDEND_CODE
#include "li.h"
#include "v24.h"
#endif  // 0
#include "ntfsutil.h"
#include "hdman.h"



static char* strcatf(char* szDstBuf, char* pFormat, ...);
static void DisplayHexDump(word32 dwStartAddr,
                           byte* pbyData, word32 dwSize, word32 dwCntColumns);
static DisplayPartitionTable(PARTITION_TABLE* pPt);
#if 0
void WriteRMOSBootSect(void)
{
   unsigned char BL_buffer [512] = {
#include "at_ble.inc"
                                 };
   unsigned char* BSec;
   DWORD  dwErr;
   HANDLE hDisk = HdOpenPhysicalDrive(0);
   if (hDisk == INVALID_HANDLE_VALUE)
   {
     dwErr = GetLastError();
     V24XPrintf("ERROR(%ld) in HdOpenPhysicalDrive(0)\n\r", dwErr);
   }
   BSec = VirtualAlloc(NULL, 512, MEM_COMMIT, PAGE_READWRITE);
   if (BSec == NULL)
   {
      V24XPrintf("ERROR in VirtualAlloc()\n\r");
      return;
   }
   if (HdReadPhysSectors(hDisk, 63, 1, BSec))
   {
      memcpy (&BSec[0], BL_buffer, 11 );
      memcpy (&BSec[0x3e], &BL_buffer[0x3e], 512-2-0x3e );
      if (HdWritePhysSectors(hDisk, 63, 1, BSec) == 0)
      {
        V24XPrintf("ERROR in HdWritePhysSectors(0)\n\r");
      }
   }
   else
   {
     V24XPrintf("ERROR in HdReadPhysSectors(0)\n\r");
   }
}
#endif // 0

BOOL HDDisplayPhysSectors(int iDisk, DWORD dwSectAddr, DWORD dwCntSects)
{
   HANDLE hPhysDisk;
   DWORD dwBytesRead;
   byte* pbyData;

   V24XPrintf("**** Sectors %ld to %ld on Disk %ld\n", dwSectAddr, dwSectAddr+dwCntSects,
       iDisk);
   if ((hPhysDisk = HdOpenPhysicalDrive(iDisk)) != INVALID_HANDLE_VALUE)
   {
      pbyData = VirtualAlloc(NULL, dwCntSects*SECT_SIZE, MEM_RESERVE|MEM_COMMIT,
          PAGE_READWRITE);
      if (pbyData == NULL)
      {
         V24XPrintf("ERROR in VirtualAlloc()");
         CloseHandle(hPhysDisk);
         return FALSE;
      }
      dwBytesRead = HdReadPhysSectors(hPhysDisk, dwSectAddr,
                        dwCntSects, pbyData);
      if (dwBytesRead != dwCntSects*SECT_SIZE)
      {
         V24XPrintf("WARNING: only %ld bytes read instead of requested %ld\n",
             dwBytesRead, dwCntSects*SECT_SIZE);
      }
      DisplayHexDump(dwSectAddr, pbyData, dwBytesRead, 16);

      VirtualFree(pbyData, 0, MEM_RELEASE);
   }
   else
   {
      V24XPrintf("ERROR in HdOpenPhysicalDrive()");
      return FALSE;
   }
   CloseHandle(hPhysDisk);
   return TRUE;
}
BOOL HDDisplayPartitionTableAndChilds(int iDisk, word32 dwSectForPT)
{
   byte* pbyData;
   HANDLE hPhysDisk;
   word32 dwBytesRead;
   PARTITION_TABLE* pPt;
   int i;
   if ((hPhysDisk = HdOpenPhysicalDrive(iDisk)) != INVALID_HANDLE_VALUE)
   {
      pbyData = VirtualAlloc(NULL, 1*SECT_SIZE, MEM_RESERVE|MEM_COMMIT,
          PAGE_READWRITE);
      if (pbyData == NULL)
      {
         V24XPrintf("ERROR in VirtualAlloc()");
         CloseHandle(hPhysDisk);
         return FALSE;
      }
      dwBytesRead = HdReadPhysSectors(hPhysDisk, dwSectForPT,
                        1, pbyData);
      if (dwBytesRead != 1*SECT_SIZE)
      {
         V24XPrintf("WARNING: only %ld bytes read instead of requested %ld\n",
             dwBytesRead, 1*SECT_SIZE);
          CloseHandle(hPhysDisk);
          VirtualFree(pbyData, 0, MEM_RELEASE);
          return FALSE;
      }
   }
   else
   {
      V24XPrintf("ERROR in HdOpenPhysicalDrive()");
      return FALSE;
   }
   CloseHandle(hPhysDisk);

   pPt = &((MBR*)pbyData)->pt;
   DisplayPartitionTable(pPt);
   for (i=0; i<MAX_PARTITION_E; i++)
   {
      if (pPt->ptEntries[i].byType == PART_TYPE_EXT_LBA ||
          pPt->ptEntries[i].byType == PART_TYPE_EXT)
      {
         HDDisplayPartitionTableAndChilds(iDisk, dwSectForPT + pPt->ptEntries[i].dwLbaStart);
      }
   }
   VirtualFree(pbyData, 0, MEM_RELEASE);
   return TRUE;
}
BOOL HDDeleteAllPartitionTables(int iDisk, word32 dwSectForPT)
{
   byte* pbyData;
   HANDLE hPhysDisk;
   word32 dwBytesRead;
   PARTITION_TABLE* pPt;
   int i;
   if ((hPhysDisk = HdOpenPhysicalDrive(iDisk)) != INVALID_HANDLE_VALUE)
   {
      pbyData = VirtualAlloc(NULL, 1*SECT_SIZE, MEM_RESERVE|MEM_COMMIT,
          PAGE_READWRITE);
      if (pbyData == NULL)
      {
         V24XPrintf("ERROR in VirtualAlloc()");
         CloseHandle(hPhysDisk);
         return FALSE;
      }
      dwBytesRead = HdReadPhysSectors(hPhysDisk, dwSectForPT,
                        1, pbyData);
      if (dwBytesRead != 1*SECT_SIZE)
      {
         V24XPrintf("WARNING: only %ld bytes read instead of requested %ld\n",
             dwBytesRead, 1*SECT_SIZE);
          CloseHandle(hPhysDisk);
          VirtualFree(pbyData, 0, MEM_RELEASE);
          return FALSE;
      }
   }
   else
   {
      V24XPrintf("ERROR in HdOpenPhysicalDrive()");
      return FALSE;
   }
   HdEraseSectors(hPhysDisk, dwSectForPT, 1, 0);
   CloseHandle(hPhysDisk);

   pPt = &((MBR*)pbyData)->pt;
   for (i=0; i<MAX_PARTITION_E; i++)
   {
      if (pPt->ptEntries[i].byType == PART_TYPE_EXT_LBA ||
          pPt->ptEntries[i].byType == PART_TYPE_EXT)
      {
         HDDeleteAllPartitionTables(iDisk, dwSectForPT + pPt->ptEntries[i].dwLbaStart);
      }
   }
   VirtualFree(pbyData, 0, MEM_RELEASE);
   return TRUE;
}
BOOL HDDisplayMBR(int iDisk, DWORD dwFlags)
{
   byte* pbyData;
   int i = sizeof(PARTITION_TABLE_E);


   V24XPrintf("**** MBR on Disk %ld\n", iDisk);
   pbyData = VirtualAlloc(NULL, 1*SECT_SIZE, MEM_RESERVE|MEM_COMMIT,
       PAGE_READWRITE);
   if (pbyData == NULL)
   {
      V24XPrintf("ERROR in VirtualAlloc()");
      return FALSE;
   }
   if (HDGetMBR(iDisk, pbyData) == FALSE)
   {
      VirtualFree(pbyData, 0, MEM_RELEASE);
      return FALSE;
   }
   if (dwFlags & 0x01)
   {
      DisplayHexDump(0, pbyData, sizeof(MBR), 16);
   }
   if (dwFlags & 0x02)
   {
      DisplayPartitionTable(&((MBR*)pbyData)->pt);
   }
   VirtualFree(pbyData, 0, MEM_RELEASE);
   return TRUE;
}
BOOL HDGetMBR(int iDisk, byte* pbyData)
{
   HANDLE hPhysDisk;
   word32 dwBytesRead = 0;
   if ((hPhysDisk = HdOpenPhysicalDrive(iDisk)) != INVALID_HANDLE_VALUE)
   {
      dwBytesRead = HdReadPhysSectors(hPhysDisk, 0,
                        1, pbyData);
      if (dwBytesRead != 1*SECT_SIZE)
      {
         V24XPrintf("ERROR: only %ld bytes read instead of requested %ld\n",
             1*512, dwBytesRead);
         CloseHandle(hPhysDisk);
         return FALSE;
      }
   }
   else
   {
      V24XPrintf("ERROR in HdOpenPhysicalDrive()");
      return FALSE;
   }
   CloseHandle(hPhysDisk);
   return TRUE;
}
HANDLE HdOpenPhysicalDrive(int iNum)
{
   HANDLE hDisk;
   char szFileName[MAX_PATH];
   word32 dwErr;
#if 1
   HANDLE            hToken;
   TOKEN_PRIVILEGES  tkp;
   HANDLE            hCurrentProcess;
   hCurrentProcess = GetCurrentProcess ();
   if (!OpenProcessToken (hCurrentProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
   {
      dwErr = GetLastError ();
      V24XPrintf("HDMAN: ERROR(%ld) in OpenProcessToken()\n", dwErr);
      return NULL;
   }
   else
   {
   }

   LookupPrivilegeValue (NULL, SE_RESTORE_NAME, &tkp.Privileges[0].Luid);

   tkp.PrivilegeCount = 1;                             
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

   dwErr = GetLastError ();
   if (dwErr != ERROR_SUCCESS)
   {
      V24XPrintf("HDMAN: ERROR(%ld) in AdjustTokenPrivileges()\n", dwErr);
      return INVALID_HANDLE_VALUE;
   }
   else
   {
   }
#endif
   sprintf(szFileName, "\\\\.\\PhysicalDrive%d", iNum);
   hDisk = CreateFile(szFileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH|FILE_FLAG_RANDOM_ACCESS|FILE_FLAG_NO_BUFFERING, NULL);
   if (hDisk == INVALID_HANDLE_VALUE)
   {
      DWORD dwErr = GetLastError();
      V24XPrintf("HDMAN: ERROR(%ld) cannot open physical disk %d\n", dwErr, iNum);
      return hDisk;
   }
   return hDisk;
}
word32 HdGetDiskCntSectors(const char* szVol)
{
   ULARGE_INTEGER liFreeBytes, liCntBytes;
   if (GetDiskFreeSpaceEx(
     szVol,                 
     &liFreeBytes,    
     &liCntBytes,    
     NULL 
   ) == FALSE)
   {
      return 0;
   }
   liCntBytes.QuadPart /= SECT_SIZE;
   return liCntBytes.LowPart;
}
BOOL LockVolume(HANDLE hDisk)
{
 unsigned long count;   

 if (DeviceIoControl(hDisk,FSCTL_LOCK_VOLUME, NULL,
  0,NULL,0,&count,NULL) == FALSE)
 {
    word32 dwErr = GetLastError();
    V24XPrintf("UnlockVolume: ERROR (%ld)in DeviceIoControl()!\n\r", dwErr);
    return FALSE;
 }
 return TRUE;
}

BOOL UnlockVolume(HANDLE hDisk)
{
 unsigned long count;
 if (DeviceIoControl(hDisk,FSCTL_UNLOCK_VOLUME, NULL,
  0,NULL,0,&count,NULL) == FALSE)
 {
    word32 dwErr = GetLastError();
    V24XPrintf("UnlockVolume: ERROR(%ld) in DeviceIoControl()!\n\r", dwErr);
    return FALSE;
 }
 return TRUE;
}
DWORD HdReadPhysSectors(HANDLE hPhysDisk, DWORD dwStartSect,
                      DWORD dwCntSects, PBYTE pbyData)
{
   LARGE_INTEGER  lStart, lNew;
   DWORD dwErr, dwCntRead, dwPtr;

   lStart.QuadPart = dwStartSect;
   lStart.QuadPart *= SECT_SIZE;
   dwPtr = SetFilePointerEx(hPhysDisk, lStart, &lNew, FILE_BEGIN);
   dwErr = GetLastError();
   if (dwPtr == INVALID_SET_FILE_POINTER && dwErr != NO_ERROR)
   {
      V24XPrintf("HDMAN: ERROR(%ld) cannot set file pointer\n", dwErr);
      return 0;
   }
   if (ReadFile(hPhysDisk, pbyData, dwCntSects*SECT_SIZE, &dwCntRead, NULL) == FALSE)
   {
      dwErr = GetLastError();
      V24XPrintf("HDMAN: ERROR(%ld) cannot read file\n", dwErr);
      return 0;
   }
   return dwCntRead;
}
BOOL HdEraseSectors(HANDLE hPhysDisk, DWORD dwStartSect,
                      DWORD dwCntSects, BYTE byPattern)
{
   word32 i;
   BYTE* pbyData;

   pbyData = VirtualAlloc(NULL, dwCntSects*SECT_SIZE, MEM_RESERVE|MEM_COMMIT,
       PAGE_READWRITE);
   if (pbyData == NULL)
   {
      V24XPrintf("ERROR in VirtualAlloc()");
      return FALSE;
   }
   memset(pbyData, byPattern, dwCntSects*SECT_SIZE);
   for (i=0; i<dwCntSects; i++)
   {
      if (HdWritePhysSectors(hPhysDisk, dwStartSect, dwCntSects, pbyData) == 0)
      {
         VirtualFree(pbyData, 0, MEM_RELEASE);
         return FALSE;
      }
   }
   VirtualFree(pbyData, 0, MEM_RELEASE);
   return TRUE;
}
BOOL HdEraseDisk(int iDisk, BYTE byPattern)
{
   HANDLE hPhysDisk;
   BOOL  bRet;
   word32 dwCntSects, dwStartSect;
   if ((hPhysDisk = HdOpenPhysicalDrive(iDisk)) != INVALID_HANDLE_VALUE)
   {
   }
   else
   {
      V24XPrintf("ERROR in HdOpenPhysicalDrive()");
      return FALSE;
   }
   dwStartSect = 0;
   dwCntSects  = 100;
   while (TRUE)
   {
      bRet = HdEraseSectors(hPhysDisk, dwStartSect,
                            dwCntSects, byPattern);
      if (bRet = TRUE)
      {
         dwStartSect += dwCntSects;
      }
      else
      {
         break;
      }
   }
   CloseHandle(hPhysDisk);
   return TRUE;
}
DWORD HdWritePhysSectors(HANDLE hPhysDisk, DWORD dwStartSect,
                      DWORD dwCntSects, PBYTE pbyData)
{
   LARGE_INTEGER  lStart, lNew;
   DWORD dwErr, dwCntRead, dwPtr;
#if 1
//   if (LockVolume(hPhysDisk) == FALSE)
//   {
//   }
   lStart.QuadPart = 0;
   lStart.QuadPart = dwStartSect;
   lStart.QuadPart *= SECT_SIZE;
   dwPtr = SetFilePointerEx(hPhysDisk, lStart, &lNew, FILE_BEGIN);
   dwErr = GetLastError();
   if (dwPtr == INVALID_SET_FILE_POINTER && dwErr != NO_ERROR)
   {
      V24XPrintf("HDMAN: ERROR(%ld) cannot set file pointer\n", dwErr);
//      UnlockVolume(hPhysDisk);
      return 0;
   }
   if (WriteFile(hPhysDisk, pbyData, dwCntSects*SECT_SIZE, &dwCntRead, NULL) == FALSE)
   {
      dwErr = GetLastError();
      V24XPrintf("HDMAN: ERROR(%ld) cannot write disk: ofs=%ld len=%ld\n", dwErr, lStart.LowPart, dwCntSects);
//      UnlockVolume(hPhysDisk);
      return 0;
   }
#endif
//   UnlockVolume(hPhysDisk);
   return dwCntRead;
}
static void DisplayHexDump(word32 dwStartAddr,
                           byte* pbyData, word32 dwSize, word32 dwCntColumns)
{
   word32 i;
   char szBuf[MAX_PATH];

   i = 0;
   sprintf(szBuf, "", i);
   for (i = 0; i <= dwSize; i++)
   {
      if (i%dwCntColumns == 0)
      {
         strcat(szBuf, "\n\r");
         V24XPrintf(szBuf);
         sprintf(szBuf, "0x%08x    ", i + dwStartAddr);
      }
      strcatf(szBuf, "%02x ", pbyData[i]);
   }
}
static int BytesToCyl(byte byCyl1, byte byCyl2)
{
   return (int)(byCyl2 | (byCyl1<<8));
}
static DisplayPartitionTable(PARTITION_TABLE* pPt)
{
   word32 i;

   V24XPrintf("=========================Partition Table==============================\n");
   V24XPrintf("Partition         -----Begin----    -----End------     Start       Num\n");
   V24XPrintf("          # Boot  Cyl Head Sect FS  Cyl Head Sect       Sect     Sects\n");
   V24XPrintf("--------- - ---- ---- ---- ---- -- ---- ---- ---- ---------- ---------\n");
   for (i = 0; i < MAX_PARTITION_E; i++)
   {
      V24XPrintf("          %d %02x   %4d %4d %4d %02x %4d %4d %4d   %8ld  %8lu\n",
          i, pPt->ptEntries[i].byBootInd,
          BytesToCyl(pPt->ptEntries[i].scs.b.CylStart1, pPt->ptEntries[i].byCylStart2),
          pPt->ptEntries[i].byHeadStart, pPt->ptEntries[i].scs.b.SectStart,
           pPt->ptEntries[i].byType,
          BytesToCyl(pPt->ptEntries[i].sce.b.CylEnd1, pPt->ptEntries[i].byCylEnd2),
           pPt->ptEntries[i].byHeadEnd, pPt->ptEntries[i].sce.b.SectEnd,
           pPt->ptEntries[i].dwLbaStart, pPt->ptEntries[i].dwPartSize);
   }
}
static char* strcatf(char* szDstBuf, char* pFormat, ...)
{
   va_list argptr;
   char szBuf[MAX_PATH];

   va_start (argptr, pFormat);
   vsprintf (szBuf, pFormat, argptr);
   va_end   (argptr);

   return strcat(szDstBuf, szBuf);
}
HANDLE HdOpenVolume(char cVol, DWORD dwAccessFlags)
{
   HANDLE hVol;
   char szFileName[MAX_PATH];
   word32 dwErr;
#if 1
   HANDLE            hToken;
   TOKEN_PRIVILEGES  tkp;
   HANDLE            hCurrentProcess;
   hCurrentProcess = GetCurrentProcess ();
   if (!OpenProcessToken (hCurrentProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
   {
      dwErr = GetLastError ();
      V24XPrintf("HDMAN: ERROR(%ld) in OpenProcessToken()\n", dwErr);
      return NULL;
   }
   else
   {
   }

   LookupPrivilegeValue (NULL, SE_RESTORE_NAME, &tkp.Privileges[0].Luid);

   tkp.PrivilegeCount = 1;                             
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;


   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

   dwErr = GetLastError ();
   if (dwErr != ERROR_SUCCESS)
   {
      V24XPrintf("HDMAN: ERROR(%ld) in AdjustTokenPrivileges()\n", dwErr);
      return INVALID_HANDLE_VALUE;
   }
   else
   {
   }
#endif
   sprintf(szFileName, "\\\\.\\%c:", cVol);
   hVol = CreateFile(szFileName, dwAccessFlags, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH|FILE_FLAG_RANDOM_ACCESS|FILE_FLAG_NO_BUFFERING, NULL);
   if (hVol == INVALID_HANDLE_VALUE)
   {
      DWORD dwErr = GetLastError();
      V24XPrintf("HDMAN: ERROR(%ld) cannot open volume %c\n", dwErr, cVol);
      return hVol;
   }
   return hVol;
}


LARGE_INTEGER HdGetDiskSize(int iDisk)
{
   HANDLE hPhysDisk;
   DWORD dwRet, dwSize = sizeof(DISK_GEOMETRY_EX)+512;
   LARGE_INTEGER liSectCount;
   DISK_GEOMETRY_EX* pbyData;

   liSectCount.QuadPart = 0;
   if ((hPhysDisk = HdOpenPhysicalDrive(iDisk)) != INVALID_HANDLE_VALUE)
   {
        pbyData = (DISK_GEOMETRY_EX*)VirtualAlloc(NULL, dwSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        if (pbyData == NULL)
        {
            V24XPrintf("ERROR in VirtualAlloc()");
            CloseHandle(hPhysDisk);
            return liSectCount;
        }
        if (DeviceIoControl(
          hPhysDisk,                        // handle to device
          IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, // dwIoControlCode
          NULL,                             // lpInBuffer
          0,                                // nInBufferSize
          (LPVOID) pbyData,                 // output buffer
          (DWORD) dwSize,                   // size of output buffer
          (LPDWORD) &dwRet,                 // number of bytes returned
          NULL) == FALSE)
        {
            V24XPrintf("ERROR %ld in DeviceIoControl()\n", GetLastError());
            CloseHandle(hPhysDisk);
            VirtualFree(pbyData, 0, MEM_RELEASE);
            return liSectCount;
        }
        CloseHandle(hPhysDisk);
        liSectCount.QuadPart = pbyData->DiskSize.QuadPart/512;
        VirtualFree(pbyData, 0, MEM_RELEASE);
   }
   return liSectCount;
}

LARGE_INTEGER HdGetLastUsedDiskSector(int iDisk, word32 dwSectForPT)
{
   byte* pbyData;
   HANDLE hPhysDisk;
   word32 dwBytesRead;
   PARTITION_TABLE* pPt;
   int i;
   LARGE_INTEGER liLastSect;

   liLastSect.QuadPart = 0;
   if ((hPhysDisk = HdOpenPhysicalDrive(iDisk)) != INVALID_HANDLE_VALUE)
   {
      pbyData = VirtualAlloc(NULL, 1*SECT_SIZE, MEM_RESERVE|MEM_COMMIT,
          PAGE_READWRITE);
      if (pbyData == NULL)
      {
         V24XPrintf("ERROR in VirtualAlloc()");
         CloseHandle(hPhysDisk);
         return liLastSect;
      }
      dwBytesRead = HdReadPhysSectors(hPhysDisk, 0,
                        1, pbyData);
      if (dwBytesRead != 1*SECT_SIZE)
      {
         V24XPrintf("WARNING: only %ld bytes read instead of requested %ld\n",
             dwBytesRead, 1*SECT_SIZE);
          CloseHandle(hPhysDisk);
          VirtualFree(pbyData, 0, MEM_RELEASE);
          return liLastSect;
      }
   }
   else
   {
      V24XPrintf("ERROR in HdOpenPhysicalDrive()");
      return liLastSect;
   }
   CloseHandle(hPhysDisk);

   pPt = &((MBR*)pbyData)->pt;

   for (i=0; i<MAX_PARTITION_E; i++)
   {
      if (pPt->ptEntries[i].byType == 0)
      {
          break;
      }
   }
   if (i > 0)
   {
       i--;
       if (pPt->ptEntries[i].byType == PART_TYPE_EXT_LBA ||
           pPt->ptEntries[i].byType == PART_TYPE_EXT)
       {
          // letzte Partition ist eine extended, weiter suchen
           liLastSect = HdGetLastUsedDiskSector(iDisk, dwSectForPT + pPt->ptEntries[i].dwLbaStart);
       }
       else
       {
           liLastSect.QuadPart = dwSectForPT + pPt->ptEntries[i].dwLbaStart + pPt->ptEntries[i].dwPartSize;
       }
   }

   VirtualFree(pbyData, 0, MEM_RELEASE);
   return liLastSect;
}

LARGE_INTEGER HdGetLastUsedVolumeSector(char* szVolume, int iFSType)
{
    LARGE_INTEGER liLastSect, liSecLastSect, liTrdLastSect;

    liLastSect.QuadPart = 0;

    if (iFSType == PART_TYPE_NTFS)
    {
        STARTING_LCN_INPUT_BUFFER startLcn;
        DWORD dwErr, dwRead, dwIdx, i;
        VOLUME_BITMAP_BUFFER* pBitBuf;
        BOOL bStop = FALSE;
        BYTE byMask;

        HANDLE hVolume = CreateFile(szVolume, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
            FILE_FLAG_WRITE_THROUGH|FILE_FLAG_RANDOM_ACCESS|FILE_FLAG_NO_BUFFERING, NULL);

        if (hVolume == INVALID_HANDLE_VALUE)
            return liLastSect;

        liSecLastSect.QuadPart = 0;
        startLcn.StartingLcn.QuadPart = 0;  // von vorne
        pBitBuf = VirtualAlloc(NULL, 1000+16, MEM_COMMIT, PAGE_READWRITE);
        while (bStop == FALSE)
        {

            if (DeviceIoControl(
              (HANDLE) hVolume,           // handle to volume
              FSCTL_GET_VOLUME_BITMAP,    // dwIoControlCode
              (LPVOID) &startLcn,        // input buffer
              (DWORD) sizeof(startLcn),      // size of input buffer
              (LPVOID) pBitBuf,       // output buffer
              (DWORD) 1000+16,     // size of output buffer
              (LPDWORD) &dwRead,  // number of bytes returned
              NULL // OVERLAPPED structure
            ) == FALSE)
            {
                dwErr = GetLastError();
                if (dwErr == ERROR_MORE_DATA)
                {
                    // ok noch  mehr Daten
                }
                else
                {
                    // Fehler
                    pBitBuf->StartingLcn.QuadPart = 0;
                    break;
                }
            }
            else
            {
                // ok, letzter Chunk
                bStop = TRUE;
            }

            byMask = 0x01;
            dwIdx = 0;
            // Daten auswerten
            for (i = 0; i < (dwRead-16)*8; i++)
            {
                if (pBitBuf->Buffer[dwIdx] & byMask)
                {
                    liTrdLastSect.QuadPart = liSecLastSect.QuadPart;
                    liSecLastSect.QuadPart = liLastSect.QuadPart;
                    liLastSect.QuadPart = pBitBuf->StartingLcn.QuadPart + i;
                }
                byMask <<= 1;

                if (byMask == 0x00)
                {
                    // Ueberlauf, von vorne
                    byMask = 0x01;
                    dwIdx++;
                }
            }
            
            startLcn.StartingLcn.QuadPart = pBitBuf->StartingLcn.QuadPart + (dwRead-16)*8;
        }

        if (bStop == TRUE)
        {
            // kein Fehler
            // jetzt Sektor ermitteln

            DWORD dwBytePerCluster = NtfsGetBytesPerCluster(hVolume);
            if (dwBytePerCluster)
            {
                liLastSect.QuadPart *= (dwBytePerCluster/512);
            }
            else
            {
                // Fehler
                liLastSect.QuadPart = 0;
            }
        }
        VirtualFree(pBitBuf, 0, MEM_RELEASE);
        CloseHandle(hVolume);
    }


    return liLastSect;
}

BOOL HdLockVolume(char cVol)
{
    DWORD dwRead;
    HANDLE hVol;

    hVol = HdOpenVolume(cVol, GENERIC_READ|GENERIC_WRITE);
    if (hVol == INVALID_HANDLE_VALUE)
        return FALSE;

    if (DeviceIoControl(
              (HANDLE) hVol,               // handle to a volume
              FSCTL_LOCK_VOLUME,           // dwIoControlCode
              NULL,                        // lpInBuffer
              0,                           // nInBufferSize
              NULL,                        // lpOutBuffer
              0,                           // nOutBufferSize
              &dwRead,                     // number of bytes returned
              NULL                         // OVERLAPPED structure
              ) == FALSE)
    {
        printf("HdLockVolume: ERROR %ld in DeviceIoControl\n", GetLastError());
        CloseHandle(hVol);
        return FALSE;
    }

    CloseHandle(hVol);

    return TRUE;
}

BOOL HdDismountVolume(char cVol)
{
    DWORD dwRead;
    HANDLE hVol;

    hVol = HdOpenVolume(cVol, GENERIC_READ|GENERIC_WRITE);
    if (hVol == INVALID_HANDLE_VALUE)
        return FALSE;

    if (DeviceIoControl(
              (HANDLE) hVol,               // handle to a volume
              FSCTL_DISMOUNT_VOLUME ,           // dwIoControlCode
              NULL,                        // lpInBuffer
              0,                           // nInBufferSize
              NULL,                        // lpOutBuffer
              0,                           // nOutBufferSize
              &dwRead,                     // number of bytes returned
              NULL                         // OVERLAPPED structure
              ) == FALSE)
    {
        printf("HdLockVolume: ERROR %ld in DeviceIoControl\n", GetLastError());
        CloseHandle(hVol);
        return FALSE;
    }

    CloseHandle(hVol);

    return TRUE;
}
