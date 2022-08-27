#pragma once
#include "worker.h"

// Klasse zum Schreiben einer Image Datei
class CWriteImageWorker : public CWorker<>
{
    HANDLE m_hImage;            // Handle auf Image Datei
    int m_iFlags;               // Flags

public:
    CWriteImageWorker(int iNumWorkItems, int iDataSize);
    ~CWriteImageWorker(void);

    // Image Datei öffnen
    bool OpenImageFile(char* szFile);

    virtual DWORD WorkerFunc(int i);
};
