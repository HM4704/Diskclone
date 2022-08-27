#pragma once
#include "OrderedFifo.h"
#include <process.h>
#include "win_adapter.h"


// Basisklasse für Worker
template<class InputFifo = COrderedFifo>
class CWorker
{
    int m_iNumTasks;                    // Anzahl der Tasks
    uintptr_t* m_TaskIds;               // Zeiger auf Buffer mit den Task IDs
    volatile long m_lTasksStarted;      // Anzahl der laufenden Tasks

protected:
    InputFifo m_input;               // Input Fifo
    IFifo* m_pOutput;            // Zeiger auf Ausgangs Fifo
    unsigned int flags;                 // Flags

public:
    CWorker(int iNumWorkItems, int iDataSize, int iNumTasks = 1);
    virtual ~CWorker(void);

    // alle Tasks starten
    void StartTasks(void);

    // alle Tasks stoppen
    void StopTasks(void);

    // Funktion zum Registrieren einer Task
    void RegisterTask(void) { ::InterlockedIncrement(&m_lTasksStarted); } ;

    // Funktion zum Deregistrieren einer Task
    void UnRegisterTask(void) { ::InterlockedDecrement(&m_lTasksStarted); } ;
    
    // Zeiger auf Eingangs Fifo zurückgeben
    InputFifo* GetInput(void) { return &m_input; };

    // Verweis auf Ausgangs Fifo setzen
    void SetOutput(IFifo* pOutput) { m_pOutput = pOutput; };

    // Worker Funktion
    virtual DWORD WorkerFunc(int i) = 0;
};

template<class InputFifo>
CWorker<InputFifo>::CWorker(int iNumWorkItems, int iDataSize, int iNumTasks)
{
    m_lTasksStarted = 0;
    m_iNumTasks = iNumTasks;
    m_pOutput = NULL;
    m_TaskIds = new uintptr_t[m_iNumTasks];

    m_input.Create(iNumWorkItems, iDataSize);
}

template<class InputFifo>
CWorker<InputFifo>::~CWorker(void)
{
    if (m_TaskIds != NULL)
    {
        delete [] m_TaskIds;
        m_TaskIds = NULL;
    }
}

// alle Tasks starten
template<class InputFifo>
void CWorker<InputFifo>::StartTasks(void)
{
    for (int i = 0; i < m_iNumTasks; i++)
    {
        m_TaskIds[i] = (uintptr_t)win::beginthreadex(this, &CWorker::WorkerFunc, 0);
    }
}

// alle Tasks stoppen
template<class InputFifo>
void CWorker<InputFifo>::StopTasks(void) 
{ 
    m_input.Stop(); 
    // warten bis sich alle Tasks beendet haben
    while (m_lTasksStarted > 0)
    {
        ::Sleep(10);
    }
}
