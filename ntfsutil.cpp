#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <Winioctl.h>
#include <stdio.h>


// Funktion gibt Anzahl der Bytes pro Cluster zurück
DWORD NtfsGetBytesPerCluster(HANDLE hVolume)
{
    NTFS_VOLUME_DATA_BUFFER buf;
    DWORD dwRead;
    DWORD dwBytesPerSec = 0;

    if (hVolume == INVALID_HANDLE_VALUE)
        return 0;

    if (DeviceIoControl(
      (HANDLE) hVolume,             // handle to volume
      FSCTL_GET_NTFS_VOLUME_DATA,   // dwIoControlCode
      (LPVOID) &buf,                 // input buffer
      (DWORD) sizeof(buf),        // size of input buffer
      (LPVOID) &buf,         // output buffer
      (DWORD) sizeof(buf),       // size of output buffer
      (LPDWORD) &dwRead,    // number of bytes returned
      NULL
    ) == FALSE)
    {
        printf("NtfsGetBytesPerCluster: ERROR %ld in DeviceIoControl()\n", GetLastError());
        dwBytesPerSec = 0;
    }
    else
    {
        dwBytesPerSec = buf.BytesPerCluster;
    }

    return dwBytesPerSec;
}