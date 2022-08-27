#pragma once
#include "worker.h"


// Klasse zum Einlesen einer Datei
class CReadFileWorker : public CWorker<>
{
    HANDLE m_hFile;                 // Handle auf Datei
    int m_iChunkSize;               // Gr�sse eines Chunks

public:
    CReadFileWorker(int iChunkSize);
public:
    ~CReadFileWorker(void);

    // Datei �ffnen
    bool OpenFile(char* szFile);

    virtual DWORD WorkerFunc(int i);
};
