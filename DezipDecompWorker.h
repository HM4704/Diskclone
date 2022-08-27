#pragma once
#include "worker.h"
#include "RleDeComp.h"


class CDezipDecompWorker :
    public CWorker<>
{
    CRleDeComp m_comp;      // Objekt mit Kompromierungsalgorithmen
public:
    CDezipDecompWorker(int iNumWorkItems, int iDataSize, int iNumTasks = 1);
    ~CDezipDecompWorker(void);

    virtual DWORD WorkerFunc(int i);
};
