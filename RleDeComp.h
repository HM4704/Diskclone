#pragma once

#define RLE_SIGNATURE   'orle'          // Kennung für RLE Komprimierung

typedef struct _RLE_HEADER
{
    unsigned int dwSignature;
    unsigned int dwLength;
} RLE_HEADER;

// Klasse zum Komprimieren und Dekomprimieren 
// unter Benutzung der RLE Komprimierung
class CRleDeComp
{

public:
    CRleDeComp(void);
    ~CRleDeComp(void);

    int ZipDeltaRLE( BYTE *pSource, BYTE *pD, int nByteAnz );
    int DeZipDeltaRLE ( BYTE *pSource, BYTE *pD, int nByteAnz );

};
