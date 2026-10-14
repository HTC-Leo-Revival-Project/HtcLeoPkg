/*
 * Copyright (c) 2011, Shantanu Gupta <shans95g@gmail.com>
 * Copyright (c) 2026, Aljoshua Hell <aljoshua.hell@gmail.com>
 */
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h> 
#include <Library/DMA.h>

/*
 * How long to wait for a command list before giving up on it.  The NAND
 * command and data CRCIs are the ones that can hang forever: if the flash does
 * not signal them, the mover never posts a result and the firmware would sit
 * in the loop below until somebody pulls the battery.
 */
#define DMOV_TIMEOUT_SPINS 10000000

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
    UINTN spins;

    DmovPrepCH(&ch, id);

    /*
     * Throw away results of an earlier list first.  Otherwise the wait below is
     * satisfied by a stale result and the caller walks off before the mover has
     * even started, leaving the command array to be overwritten under it.
     */
    while (DMOV_STATUS_RSLT_COUNT(MmioRead32((UINTN)ch.status))) {
        MmioRead32((UINTN)ch.result);
    }

    MmioWrite32((UINTN)ch.cmd,(UINT32)(DMOV_CMD_PTR_LIST | DMOV_CMD_ADDR(paddr(ptr))));

    for (spins = 0; ; spins++) {
        n = MmioRead32((UINTN)ch.status);
        if (n & DMOV_STATUS_RSLT_VALID) {
            break;
        }
        if (spins >= DMOV_TIMEOUT_SPINS) {
            DEBUG((EFI_D_ERROR, "DMA: timeout on channel %d, status %x (flush %x)\n",
                id, n, MmioRead32((UINTN)DMOV_FLUSH0(id))));
            return -1;
        }
    }

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