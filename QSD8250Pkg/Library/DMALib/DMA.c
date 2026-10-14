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

/*
 * The header puts STATUS at 0x200 + ch * 4, but the values that come back from
 * hardware do not look like a status register at all (they change while the
 * mover runs, yet never report a result), so on a timeout dump the handful of
 * offsets a status register could be at and let the hardware decide.
 */
#define DMOV_DUMP_STATUS 1

VOID DmovPrepCH(DMOV_CH *ch, UINTN id)
{
	ch->cmd = DMOV_CMD_PTR(id);
	ch->result = DMOV_RSLT(id);
	ch->status = DMOV_STATUS(id);
	ch->config = DMOV_CONFIG(id);
}

STATIC
VOID
DmovDumpStatus (
  IN UINTN  Id
  )
{
    STATIC CONST UINTN Offsets[] = { 0x180, 0x200, 0x240, 0x280, 0x2C0, 0x300, 0x340 };
    UINTN  Index;

    for (Index = 0; Index < ARRAY_SIZE (Offsets); Index++) {
        DEBUG ((EFI_D_ERROR, "DMA: ch %d reg %x = %x\n", Id, Offsets[Index],
                MmioRead32 ((UINTN) DMOV_SDn (Offsets[Index], Id))));
    }
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

    /*
     * Wait for a result of *this* list.
     *
     * The status register does not match its own header on this part: bit 1,
     * documented as RSLT_VALID, stays clear while the FIFO holds results, and
     * bit 0, documented as CMD_PTR_RDY, is the one that comes up together with
     * the first result.  The count in the top three bits is the one thing both
     * this part and the header agree on, and it is what tells us whether the
     * list got anywhere: it counts up as lists pile up undrained.  (Waiting for
     * the wrong bit also fills the FIFO, and a channel with a full result FIFO
     * stops accepting commands, so everything after it hangs as well.)
     */
    for (spins = 0; ; spins++) {
        n = MmioRead32((UINTN)ch.status);
        if (DMOV_STATUS_RSLT_COUNT(n)) {
            break;
        }
        if (spins >= DMOV_TIMEOUT_SPINS) {
            DEBUG((EFI_D_ERROR, "DMA: timeout on channel %d, status %x\n", id, n));
#if DMOV_DUMP_STATUS
            DmovDumpStatus (id);
#endif
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