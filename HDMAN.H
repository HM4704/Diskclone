#ifndef HDMAN_H_27042003
#define HDMAN_H_27042003


typedef unsigned long word32;
typedef unsigned char  byte;
#define V24XPrintf printf

#define SECT_SIZE                   512
#define PART_TABLE_START_OFFS       0x1be       // Start-Offset einer Partitions Tabelle
#define MAX_PARTITION_E             4

#define PART_TYPE_EXT              0x05
#define PART_TYPE_EXT_LBA          0x0f
#define PART_TYPE_FAT16            0x06
#define PART_TYPE_FAT32            0x0B
#define PART_TYPE_FAT32_LBA        0x0C
#define PART_TYPE_NTFS             0x07
#define PART_TYPE_LINUX_SWAP       0x82
#define PART_TYPE_LINUX_NATIVE     0x83

#pragma pack(1) 

typedef struct _PARTITION_TABLE_E
{
    byte byBootInd;
    byte byHeadStart;
    union
    {
        byte bySectCylStart;
        struct {
            byte SectStart : 6;
            byte CylStart1 : 2;
        } b;
    } scs;
    byte byCylStart2;
    byte byType;
    byte byHeadEnd;
    union
    {
        byte bySectCylEnd;
        struct {
            byte SectEnd : 6;
            byte CylEnd1 : 2;
        } b;
    } sce;
    byte byCylEnd2;
    word32 dwLbaStart;
    word32 dwPartSize;
} PARTITION_TABLE_E;

#pragma pack(1) 
typedef struct _PARTITION_TABLE
{
    PARTITION_TABLE_E  ptEntries[MAX_PARTITION_E];
} PARTITION_TABLE;

#pragma pack(1) 
typedef struct _MBR
{
    byte byBootCode[PART_TABLE_START_OFFS];
    PARTITION_TABLE pt;
    byte bySignature[2];
} MBR;

#pragma pack(1) 
typedef struct _BOOT_SECx
{
    char            boot_jmp[3];
    char            disk_name[8];
    unsigned short  bytes_per_sec;
    char            sec_per_clust;
    unsigned short  res_sec;
    char            no_of_fats;
    unsigned short  root_entries;
    unsigned short  partition_sec1;
    char            id_byte;
    unsigned short  sec_per_fat;
    unsigned short  sec_per_track;
    unsigned short  sides;
    long            hidden_sec;
    long            partition_sec2;
    char            drive_number;
    char            head_number;
    char            boot_signat;
    long            volume_id;
    char            label_name[11];
    char            fat_type[8];
    char            fill_block[448];
    unsigned short  boot_signature;
} MS_BOOT_SECx;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

BOOL HDDisplayPhysSectors(int iDisk, DWORD dwSectAddr, DWORD dwCntSects);
BOOL HDDisplayMBR(int iDisk, DWORD dwFlags);


HANDLE HdOpenPhysicalDrive(int iNum);
DWORD  HdReadPhysSectors(HANDLE hPhysDisk, DWORD dwStartSect,
                      DWORD dwCntSects, PBYTE pbyData);
DWORD  HdWritePhysSectors(HANDLE hPhysDrive, DWORD dwStartSect,
                      DWORD dwCntSects, PBYTE pbyData);
BOOL HdEraseSectors(HANDLE hPhysDisk, DWORD dwStartSect,
                      DWORD dwCntSects, BYTE byPattern);
BOOL HdEraseDisk(int iDisk, BYTE byPattern);
BOOL HDGetMBR(int iDisk, byte* pbyData);
BOOL HDDisplayPartitionTableAndChilds(int iDisk, word32 dwSectForPT);
BOOL HDDeleteAllPartitionTables(int iDisk, word32 dwSectForPT);
void WriteRMOSBootSect(void);
HANDLE HdOpenVolume(char cDrive, DWORD dwAccessFlags);
LARGE_INTEGER HdGetLastUsedDiskSector(int iDisk, word32 dwSectForPT);
LARGE_INTEGER HdGetDiskSize(int iDisk);
LARGE_INTEGER HdGetLastUsedVolumeSector(char* szVolume, int iFSType);
BOOL HdLockVolume(char cVol);
BOOL HdDismountVolume(char cVol);


#ifdef __cplusplus
}
#endif // __cplusplus
#endif
