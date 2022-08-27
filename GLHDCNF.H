#ifndef MAKE_BC_REF
#define MBR_BSC_LENGTH   0x190   /* Boot strap loader code */
#pragma align( _BOOT_SECx = 1 )
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
#pragma align( _NTFS_BOOT_SECx = 1 )
typedef struct _NTFS_BOOT_SECx
{
    char            boot_jmp[3];
    char            disk_name[8];
    unsigned short  bytes_per_sec;
    char            sec_per_clust;
    unsigned short  res_sec;

    char            res1[3];
    char            res2[2];

    char            id_byte;

    char            res3[2];

    unsigned short  sec_per_track;
    unsigned short  sides;
    long            hidden_sec;

    char            res4[4];

    unsigned int    ntfs_id;        
    long            total_sec;      
    char            total_sec_2[4];
    long            start_c_mft;    
    char            start_c_mft_2[4];
    long            start_c_mft_mirr; 
    char            start_c_mft_mirr_2[4];
    unsigned int    clust_per_mft;
    unsigned int    clust_per_idx_block;
    long            serial_number;    
    char            serial_number_2[4];
    unsigned int    checksum;

    char            fill_block[426];
    unsigned short  boot_signature;
} NTFS_BOOT_SECx;
extern int CheckHD(byte* pbyDev, byte* pbyUnit, byte* pbySec0, PHYS_DATA* pPhysData, PART_SD* pPartTable, word32* pdwCntParts);
extern int FormatPartitions(HD_CONFIG* pHdConfig);
extern int InitHD(byte byDevId, byte byUnitId, int* piMustConf);
extern int ConfigHd(HD_CONFIG* pHdConfig, word32* pdwStatus);
extern far int DismountAllAvailableVol(int iForce);
#endif /* MAKE_BC_REF */
