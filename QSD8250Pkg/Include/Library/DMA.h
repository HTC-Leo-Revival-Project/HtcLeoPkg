#ifndef __PLATFORM_MSM_SHARED_DMOV_H
#define __PLATFORM_MSM_SHARED_DMOV_H

#define SRC_CRCI_NAND_CMD  CMD_SRC_CRCI(DMOV_NAND_CRCI_CMD)
#define DST_CRCI_NAND_CMD  CMD_DST_CRCI(DMOV_NAND_CRCI_CMD)
#define SRC_CRCI_NAND_DATA CMD_SRC_CRCI(DMOV_NAND_CRCI_DATA)
#define DST_CRCI_NAND_DATA CMD_DST_CRCI(DMOV_NAND_CRCI_DATA)

#define NAND_CFG0_RAW 0xA80420C0
#define NAND_CFG1_RAW 0x5045D

#define CFG1_WIDE_FLASH (1U << 1)

#define paddr(n) ((UINTN) (n))

typedef struct DMOV_CH DMOV_CH;
struct DMOV_CH
{
	volatile UINTN cmd;
	volatile UINTN result;
	volatile UINTN status;
	volatile UINTN config;
};


#define MSM_DMOV_BASE 0xA9700000

/* see 80-VA736-2 C pp 415-439 */

#define DMOV_SD0(off, ch) (MSM_DMOV_BASE + 0x0000 + (off) + ((ch) << 2))
#define DMOV_SD1(off, ch) (MSM_DMOV_BASE + 0x0400 + (off) + ((ch) << 2))
#define DMOV_SD2(off, ch) (MSM_DMOV_BASE + 0x0800 + (off) + ((ch) << 2))
#define DMOV_SD3(off, ch) (MSM_DMOV_BASE + 0x0C00 + (off) + ((ch) << 2))

#define DMOV_SDn DMOV_SD3



/* only security domain 3 is available to the ARM11
**
** SD0 -> mARM trusted, SD1 -> mARM nontrusted, SD2 -> aDSP, SD3 -> aARM
**
*/

#define DMOV_CMD_PTR(ch)      DMOV_SDn(0x000, ch)
#define DMOV_CMD_LIST         (0 << 29) /* does not work */
#define DMOV_CMD_PTR_LIST     (1 << 29) /* works */
#define DMOV_CMD_INPUT_CFG    (2 << 29) /* untested */
#define DMOV_CMD_OUTPUT_CFG   (3 << 29) /* untested */
#define DMOV_CMD_ADDR(addr)   ((addr) >> 3)

#define DMOV_RSLT(ch)         DMOV_SDn(0x040, ch)
#define DMOV_RSLT_VALID       (1 << 31) /* 0 == host has empties result fifo */
#define DMOV_RSLT_ERROR       (1 << 3)
#define DMOV_RSLT_FLUSH       (1 << 2)
#define DMOV_RSLT_DONE        (1 << 1)  /* top pointer done */
#define DMOV_RSLT_USER        (1 << 0)  /* command with FR force result */

#define DMOV_FLUSH0(ch)       DMOV_SDn(0x080, ch)
#define DMOV_FLUSH1(ch)       DMOV_SDn(0x0C0, ch)
#define DMOV_FLUSH2(ch)       DMOV_SDn(0x100, ch)
#define DMOV_FLUSH3(ch)       DMOV_SDn(0x140, ch)
#define DMOV_FLUSH4(ch)       DMOV_SDn(0x180, ch)
#define DMOV_FLUSH5(ch)       DMOV_SDn(0x1C0, ch)

#define DMOV_STATUS(ch)       DMOV_SDn(0x200, ch)
#define DMOV_STATUS_RSLT_COUNT(n)    (((n) >> 29))
#define DMOV_STATUS_CMD_COUNT(n)     (((n) >> 27) & 3) 
#define DMOV_STATUS_RSLT_VALID       (1 << 1)
#define DMOV_STATUS_CMD_PTR_RDY      (1 << 0)

#define DMOV_ISR              DMOV_SDn(0x380, 0)

#define DMOV_CONFIG(ch)       DMOV_SDn(0x300, ch)
#define DMOV_CONFIG_FORCE_TOP_PTR_RSLT (1 << 2)
#define DMOV_CONFIG_FOREC_FLUSH_RSLT   (1 << 1)
#define DMOV_CONFIG_IRQ_EN             (1 << 0)

/* channel assignments - from qc/dmov_7500.h */

#define DMOV_NAND_CHAN        7
#define DMOV_NAND_CRCI_CMD    5
#define DMOV_NAND_CRCI_DATA   4

#define DMOV_SDC1_CHAN        8
#define DMOV_SDC1_CRCI        6

#define DMOV_SDC2_CHAN        8
#define DMOV_SDC2_CRCI        7

#define DMOV_TSIF_CHAN        10
#define DMOV_TSIF_CRCI        10

#define DMOV_USB_CHAN         11

/* no client rate control ifc (eg, ram) */
#define DMOV_NONE_CRCI        0


/* If the CMD_PTR register has CMD_PTR_LIST selected, the data mover
** is going to walk a list of 32bit pointers as described below.  Each
** pointer points to a *array* of DMOV_S, etc structs.  The last pointer
** in the list is marked with CMD_PTR_LP.  The last struct in each array
** is marked with CMD_LC (see below).
*/
#define CMD_PTR_ADDR(addr)  ((addr) >> 3)
#define CMD_PTR_LP          (1 << 31) /* last pointer */
#define CMD_PTR_PT          (3 << 29) /* ? */


/* Single Item Mode -- seems to work as expected */
typedef struct {
    UINTN cmd;
    UINTN src;
    UINTN dst;
    UINTN len;
} DMOV_S;

/* Scatter/Gather Mode -- does this work?*/
typedef struct {
    UINTN cmd;
    UINTN src_dscr;
    UINTN dst_dscr;
    UINTN _reserved;
} DMOV_SG;

/* bits for the cmd field of the above structures */

#define CMD_LC      (1 << 31)  /* last command */
#define CMD_FR      (1 << 22)  /* force result -- does not work? */
#define CMD_OCU     (1 << 21)  /* other channel unblock */
#define CMD_OCB     (1 << 20)  /* other channel block */
#define CMD_TCB     (1 << 19)  /* ? */
#define CMD_DAH     (1 << 18)  /* destination address hold -- does not work?*/
#define CMD_SAH     (1 << 17)  /* source address hold -- does not work? */

#define CMD_MODE_SINGLE     (0 << 0) /* DMOV_S structure used */
#define CMD_MODE_SG         (1 << 0) /* untested */
#define CMD_MODE_IND_SG     (2 << 0) /* untested */
#define CMD_MODE_BOX        (3 << 0) /* untested */

#define CMD_DST_SWAP_BYTES  (1 << 14) /* exchange each byte n with byte n+1 */
#define CMD_DST_SWAP_SHORTS (1 << 15) /* exchange each short n with short n+1 */
#define CMD_DST_SWAP_WORDS  (1 << 16) /* exchange each word n with word n+1 */

#define CMD_SRC_SWAP_BYTES  (1 << 11) /* exchange each byte n with byte n+1 */
#define CMD_SRC_SWAP_SHORTS (1 << 12) /* exchange each short n with short n+1 */
#define CMD_SRC_SWAP_WORDS  (1 << 13) /* exchange each word n with word n+1 */

#define CMD_DST_CRCI(n)     (((n) & 15) << 7)
#define CMD_SRC_CRCI(n)     (((n) & 15) << 3)

VOID DmovPrepCH(DMOV_CH *ch, UINTN id);
INTN DmovExecCmdPtr(UINTN id, UINTN *ptr);


#endif /* __PLATFORM_MSM_SHARED_DMOV_H */