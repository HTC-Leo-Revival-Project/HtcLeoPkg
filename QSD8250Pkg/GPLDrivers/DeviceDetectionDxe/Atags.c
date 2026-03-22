#include <Library/Atags.h>
#include <Device/DeviceType.h>

BOOLEAN DetectCLK(char* cmdline) {
    if (cmdline == NULL) {
        return FALSE;
    }

    if (AsciiStrStr(cmdline, "clk=") != NULL &&
        AsciiStrStr(cmdline, "androidboot.hardware=leo") != NULL) {
        return TRUE;
    }

    return FALSE;
}

DeviceType DetectDevice(char *cmdline) {
    if (strstr(cmdline, "androidboot.hardware=leo"))
        return LEO;
    if (strstr(cmdline, "androidboot.hardware=schubert"))
        return SCHUBERT;
    if (strstr(cmdline, "androidboot.hardware=bravo"))
        return BRAVO;
    if (strstr(cmdline, "androidboot.hardware=passion"))
        return PASSION;

    return UNKNOWN;
}

VOID ParseAtags(UINT32 StartAddr, HtcDevice *mDevice) {
    struct tag *t = (struct tag *)(UINTN)StartAddr;

    // Must start with ATAG_CORE
    if (t->hdr.tag != ATAG_CORE) {
        DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] ERROR: First tag is not ATAG_CORE (found 0x%x)\n", t->hdr.tag));
        return;
    }

    while (1) {
        if (t->hdr.size == 0 && t->hdr.tag != ATAG_NONE) {
            DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] ERROR: Tag type=0x%x, invalid size 0, aborting\n", t->hdr.tag));
            return;
        }

        DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Tag @ 0x%p, type=0x%x, size=%d\n", t, t->hdr.tag, t->hdr.size));

        switch (t->hdr.tag) {
            case ATAG_CORE:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_CORE\n"));
                break;
            case ATAG_MEM:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_MEM\n"));
                break;
            case ATAG_VIDEOTEXT:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_VIDEOTEXT\n"));
                break;
            case ATAG_RAMDISK:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_RAMDISK\n"));
                break;
            case ATAG_INITRD2:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_INITRD2\n"));
                break;
            case ATAG_SERIAL:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_SERIAL\n"));
                break;
            case ATAG_REVISION:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_REVISION\n"));
                break;
            case ATAG_VIDEOLFB:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_VIDEOLFB\n"));
                break;
            case ATAG_CMDLINE:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_CMDLINE\n"));
                char *cmd = t->u.cmdline.cmdline;
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Command Line: %a\n", cmd));
                mDevice->cLK = DetectCLK(cmd);
                mDevice->type = DetectDevice(cmd);
                mDevice->cmdline = cmd;
                break;
            case ATAG_CLK:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_CLK\n"));
                mDevice->cLK = TRUE;
                //todo: set cLK detection to true here and parse in other DXE as needed
                break;
            case ATAG_NAND_BOOT_MAGLDR:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_NAND_BOOT_MAGLDR\n"));
                mDevice->nandBoot = TRUE;
                break;
            case ATAG_NAND_PARTION:
            case ATAG_NAND_PARTION1:
            case ATAG_NAND_PARTION2:
            case ATAG_NAND_PARTION3:
            case ATAG_NAND_PARTION4:
            case ATAG_NAND_PARTION5:
            case ATAG_NAND_PARTION6:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_NAND_PARTION\n"));
                break;
            case ATAG_NONE:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Found ATAG_NONE (end of list)\n"));
                return;
            default:
                DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Unknown tag: 0x%x\n", t->hdr.tag));
                break;
        }

        t = (struct tag *)((UINT8 *)t + (t->hdr.size * sizeof(UINT32)));
    }
}