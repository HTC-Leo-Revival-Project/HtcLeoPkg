/*
 * Copyright (c) 2011, Shantanu Gupta <shans95g@gmail.com>
 * Copyright (c) 2026, Aljoshua Hell <aljoshua.hell@gmail.com>
 */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h> 
#include <Library/DMA.h>


VOID DmovPrepCH(DMOV_CH *ch, UINTN id)
{
	ch->cmd = DMOV_CMD_PTR(id);
	ch->result = DMOV_RSLT(id);
	ch->status = DMOV_STATUS(id);
	ch->config = DMOV_CONFIG(id);
}

INTN DmovExecCmdPtr(UINTN id, UINTN *ptr)
{
    DMOV_CH ch;
    UINTN n;

    DmovPrepCH(&ch, id);

    MmioWrite32((UINTN)ch.cmd,(UINT32)(DMOV_CMD_PTR_LIST | DMOV_CMD_ADDR(paddr(ptr))));

    // while(!(readl(ch.status) & DMOV_STATUS_RSLT_VALID))
    while (!(MmioRead32((UINTN)ch.status) & DMOV_STATUS_RSLT_VALID)) {
    }

    n = MmioRead32((UINTN)ch.status);

    while (DMOV_STATUS_RSLT_COUNT(n)) {

        n = MmioRead32((UINTN)ch.result);

        if (n != 0x80000002) {
            DEBUG((EFI_D_ERROR, "DMA: ERROR: result: %x\n", n));
            DEBUG((EFI_D_ERROR, "DMA: ERROR: flush: %x %x %x %x\n",
                MmioRead32((UINTN)DMOV_FLUSH0(DMOV_NAND_CHAN)),
                MmioRead32((UINTN)DMOV_FLUSH1(DMOV_NAND_CHAN)),
                MmioRead32((UINTN)DMOV_FLUSH2(DMOV_NAND_CHAN)),
                MmioRead32((UINTN)DMOV_FLUSH3(DMOV_NAND_CHAN))
            ));
            return -1;
        }

        n = MmioRead32((UINTN)ch.status);
    }

    return 0;
}