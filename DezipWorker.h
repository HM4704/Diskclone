#pragma once
#include "worker.h"

class CDezipWorker :
    public CWorker<>
{
    int m_iDataSize;        // Groesse eines Data chunks
    int m_iNumWorkItems;    // Anzahl der Work Items
    int m_iNumDezipTasks;   // Anzahl der Tasks

private:
    DWORD  ReturnError(char* szError); 

public:
    CDezipWorker(int iNumWorkItems, int iDataSize, int iNumTasks = 1);
public:
    ~CDezipWorker(void);

    virtual DWORD WorkerFunc(int i);
};
