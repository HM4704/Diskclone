#pragma once

// Klasse für das Work Item
class CWorkItem
{
public:
    int m_iDataSize;                        // Grösse des Chunks
    unsigned char* m_pbyData;               // Zeiger auf Nutzdaten
    int m_iOrderNum;                        // Nummerierierung der Chunks

public:
    CWorkItem(void);
    ~CWorkItem(void);
};
