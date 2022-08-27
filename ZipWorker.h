#pragma once
#include "RleDeComp.h"
#include "worker.h"


// Worker Klasse zum Komprimieren
class CZipWorker : public CWorker<>
{
    CRleDeComp m_comp;                  // Komprimierer

private:
    DWORD  ReturnError(char* szError);

public:
    CZipWorker(int iNumWorkItems, int iDataSize, int iNumTasks = 1);
    ~CZipWorker(void);

    virtual DWORD WorkerFunc(int i);
};
