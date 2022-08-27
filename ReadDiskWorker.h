#pragma once
#include "worker.h"

typedef struct _DISK_JOB
{
    enum _JobId { OpenDiskId, ReadDiskId, CloseDiskId, SetDiskHandleId } JobId;
    union 
    {
        struct {
            unsigned long ulDiskId;
        } OpenDisk;
        struct {
            unsigned long ulStartSector;
            unsigned long ulCount;
        } ReadDisk;
        struct {
            HANDLE hDisk;
        } SetDiskHandle;
    };
} DISK_JOB;

// Klasse zum Lesen von Disk
class CReadDiskWorker :
    public CWorker<>
{
    HANDLE m_hDisk;                             // Handle auf Disk

public:
    CReadDiskWorker(int iNumWorkItems, int iDataSize, int iNumTasks = 1);
public:
    ~CReadDiskWorker(void);

    virtual DWORD WorkerFunc(int i);
};
