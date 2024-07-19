#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

#include <Chipset/iomap.h>
#include <Library/reg.h>
#include <Library/FrameBufferConfigLib.h>

#include <Resources/FbColor.h>

UINTN Width = FixedPcdGet32(PcdMipiFrameBufferWidth);
UINTN Height = FixedPcdGet32(PcdMipiFrameBufferHeight);
UINTN FbAddr = FixedPcdGet32(PcdMipiFrameBufferAddress);
UINTN GlobalBpp = 0;

VOID
PaintScreen(
  IN  UINTN   BgColor
)
{
    // Code from FramebufferSerialPortLib
	char* Pixels = (void*)FbAddr;

    if(GlobalBpp == 0) {
        GlobalBpp = FixedPcdGet32(PcdMipiFrameBufferPixelBpp);
    }

	// Set color.
	for (UINTN i = 0; i < Width; i++)
	{
		for (UINTN j = 0; j < Height; j++)
		{
			// Set pixel bit
			for (UINTN p = 0; p < (GlobalBpp / 8); p++)
			{
				*Pixels = (unsigned char)BgColor;
				BgColor = BgColor >> 8;
				Pixels++;
			}
		}
	}
}

VOID
ReconfigFb(
    IN UINTN Bpp
)
{
  UINT32 dma_cfg = 0;
  GlobalBpp = Bpp;

  // Paint screen to black
  PaintScreen(0);

  // Stop any previous transfers
  MmioWrite32(MDP_LCDC_EN, 0);

  //ArmInstructionSynchronizationBarrier();
  //ArmDataMemoryBarrier();

  // Format
  // https://github.com/marc1706/hd2_kernel/blob/f4951cda4525e4cba87a3de83fd00aee61bb2897/drivers/video/msm/mdp_lcdc.c#L152
  dma_cfg |= (DMA_PACK_ALIGN_MSB |
          DMA_PACK_PATTERN_RGB |
          DMA_DITHER_EN);
  
  dma_cfg |= DMA_OUT_SEL_LCDC; // Select the DMA channel for LCDC
  
  // Format
  if(Bpp == 16) {
      dma_cfg |= DMA_IBUF_FORMAT_RGB565;
      dma_cfg &= ~DMA_DST_BITS_MASK;
      dma_cfg |= DMA_DSTC0G_6BITS | DMA_DSTC1B_5BITS | DMA_DSTC2R_5BITS;
  }
  else if(Bpp == 24) {
      dma_cfg |= DMA_IBUF_FORMAT_RGB888;
      dma_cfg &= ~DMA_DST_BITS_MASK;
      dma_cfg |= DMA_DSTC0G_8BITS|DMA_DSTC1B_8BITS|DMA_DSTC2R_8BITS;
  }
  else if(Bpp == 32) {
      dma_cfg |= DMA_IBUF_FORMAT_XRGB8888;
      dma_cfg &= ~DMA_DST_BITS_MASK;
      dma_cfg |= DMA_DSTC0G_8BITS|DMA_DSTC1B_8BITS|DMA_DSTC2R_8BITS;
  }

  MmioWrite32(MDP_DMA_P_CONFIG, dma_cfg);

  // Stride
  MmioWrite32(MDP_DMA_P_IBUF_Y_STRIDE, (Bpp / 8) * Width);

  // Write fb addr (relocates fb to 0x02A00000 on schubert)
  MmioWrite32(MDP_DMA_P_IBUF_ADDR, FixedPcdGet32(PcdMipiFrameBufferAddress));

  // Ensure all transfers finished
  //ArmInstructionSynchronizationBarrier();
  //ArmDataMemoryBarrier();

  // Enable LCDC
  MmioWrite32(MDP_LCDC_EN, 0x1);
}