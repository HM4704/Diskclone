#pragma once

// Klasse f�r das Work Item
class CWorkItem
{
public:
    int m_iDataSize;                        // Gr�sse des Chunks
    unsigned char* m_pbyData;               // Zeiger auf Nutzdaten
    int m_iOrderNum;                        // Nummerierierung der Chunks

public:
    CWorkItem(void);
    ~CWorkItem(void);
};
