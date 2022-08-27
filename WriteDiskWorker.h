#pragma once
#include "worker.h"

// Klasse zum Beschreiben einer Disk
class CWriteDiskWorker : public CWorker<>
{
    int m_iDisk;            // Nummer der Disk
public:
    CWriteDiskWorker(int iDisk, int iNumWorkItems, int iDataSize);
    ~CWriteDiskWorker(void);

    virtual DWORD WorkerFunc(int i);
};
