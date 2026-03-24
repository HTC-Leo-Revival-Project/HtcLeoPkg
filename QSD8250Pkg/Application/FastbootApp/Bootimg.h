#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8

typedef struct {
    UINT8 magic[BOOT_MAGIC_SIZE];    // "ANDROID!"
    UINT32 kernel_size;              // size in bytes
    UINT32 kernel_addr;              // load address
    UINT32 ramdisk_size;             // size in bytes
    UINT32 ramdisk_addr;             // load address
    UINT32 second_size;              // size in bytes
    UINT32 second_addr;              // load address
    UINT32 tags_addr;                // physical address for ATAGS / DTB
    UINT32 page_size;                // flash page size
    UINT32 unused[2];
    UINT8 name[16];                  // board name
    UINT8 cmdline[512];              // kernel cmdline
    UINT32 id[8];
} ANDROID_BOOT_IMG_HDR;