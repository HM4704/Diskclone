#pragma once

#define DC_IMG_VERS         'DISK'

#define DC_MAX_PARTITIONS   0x04        // max.4 Partitionen sichern

// Art des Klons
#define DC_CLONE_DISK    0x01       // Image enthält eine ganze Disk
#define DC_CLONE_PART    0x02       // Image enthält geklonte Partitions

typedef struct _PART_INFO
{
    unsigned long dwUsed;
    long long     llOffset;         // Offset in Sektoren
    long long     llLength;         // Laenge in Sektoren
} PART_INFO;

typedef struct _IMG_HEADER
{
    unsigned long dwImageVersion;       // Image Version
    unsigned long dwCloneType;          // Klon Typ (DISK oder PART)
    PART_INFO PartInfo[DC_MAX_PARTITIONS];
} IMG_HEADER;


    