#ifndef __PCOM_H_
#define __PCOM_H_

#include <Chipset/iomap.h>

#define PCOM_ENABLE 	1
#define PCOM_DISABLE 	0

#define MSM_A2M_INT(n) (MSM_CSR_BASE + 0x400 + (n) * 4)

#define APP_COMMAND (MSM_SHARED_BASE + 0x00)
#define APP_STATUS  (MSM_SHARED_BASE + 0x04)
#define APP_DATA1   (MSM_SHARED_BASE + 0x08)
#define APP_DATA2   (MSM_SHARED_BASE + 0x0C)

#define MDM_COMMAND (MSM_SHARED_BASE + 0x10)
#define MDM_STATUS  (MSM_SHARED_BASE + 0x14)
#define MDM_DATA1   (MSM_SHARED_BASE + 0x18)
#define MDM_DATA2   (MSM_SHARED_BASE + 0x1C)

/* proc_comm commands supported */
enum {
	PCOM_CMD_IDLE = 0x0,
	PCOM_CMD_DONE,
	PCOM_RESET_APPS,
	PCOM_RESET_CHIP,
	PCOM_CONFIG_NAND_MPU,
	PCOM_CONFIG_USB_CLKS,
	PCOM_GET_POWER_ON_STATUS,
	PCOM_GET_WAKE_UP_STATUS,
	PCOM_GET_BATT_LEVEL,
	PCOM_CHG_IS_CHARGING,
	PCOM_POWER_DOWN,
	PCOM_USB_PIN_CONFIG,
	PCOM_USB_PIN_SEL,
	PCOM_SET_RTC_ALARM,
	PCOM_NV_READ,
	PCOM_NV_WRITE,
	PCOM_GET_UUID_HIGH,
	PCOM_GET_UUID_LOW,
	PCOM_GET_HW_ENTROPY,
	PCOM_RPC_GPIO_TLMM_CONFIG_REMOTE,
	PCOM_CLKCTL_RPC_ENABLE,
	PCOM_CLKCTL_RPC_DISABLE,
	PCOM_CLKCTL_RPC_RESET,
	PCOM_CLKCTL_RPC_SET_FLAGS,
	PCOM_CLKCTL_RPC_SET_RATE,
	PCOM_CLKCTL_RPC_MIN_RATE,
	PCOM_CLKCTL_RPC_MAX_RATE,
	PCOM_CLKCTL_RPC_RATE,
	PCOM_CLKCTL_RPC_PLL_REQUEST,
	PCOM_CLKCTL_RPC_ENABLED,
	PCOM_VREG_SWITCH,
	PCOM_VREG_SET_LEVEL,
	PCOM_GPIO_TLMM_CONFIG_GROUP,
	PCOM_GPIO_TLMM_UNCONFIG_GROUP,
	PCOM_NV_WRITE_BYTES_4_7,
	PCOM_CONFIG_DISP,
	PCOM_GET_FTM_BOOT_COUNT,
	PCOM_RPC_GPIO_TLMM_CONFIG_EX,
	PCOM_PM_MPP_CONFIG,
	PCOM_GPIO_IN,
	PCOM_GPIO_OUT,
	PCOM_RESET_MODEM,
	PCOM_RESET_CHIP_IMM,
	PCOM_PM_VID_EN,
	PCOM_VREG_PULLDOWN,
	PCOM_GET_MODEM_VERSION,
	PCOM_CLK_REGIME_SEC_RESET,
	PCOM_CLK_REGIME_SEC_RESET_ASSERT,
	PCOM_CLK_REGIME_SEC_RESET_DEASSERT,
	PCOM_CLK_REGIME_SEC_PLL_REQUEST_WRP,
	PCOM_CLK_REGIME_SEC_ENABLE,
	PCOM_CLK_REGIME_SEC_DISABLE,
	PCOM_CLK_REGIME_SEC_IS_ON,
	PCOM_CLK_REGIME_SEC_SEL_CLK_INV,
	PCOM_CLK_REGIME_SEC_SEL_CLK_SRC,
	PCOM_CLK_REGIME_SEC_SEL_CLK_DIV,
	PCOM_CLK_REGIME_SEC_ICODEC_CLK_ENABLE,
	PCOM_CLK_REGIME_SEC_ICODEC_CLK_DISABLE,
	PCOM_CLK_REGIME_SEC_SEL_SPEED,
	PCOM_CLK_REGIME_SEC_CONFIG_GP_CLK_WRP,
	PCOM_CLK_REGIME_SEC_CONFIG_MDH_CLK_WRP,
	PCOM_CLK_REGIME_SEC_USB_XTAL_ON,
	PCOM_CLK_REGIME_SEC_USB_XTAL_OFF,
	PCOM_CLK_REGIME_SEC_SET_QDSP_DME_MODE,
	PCOM_CLK_REGIME_SEC_SWITCH_ADSP_CLK,
	PCOM_CLK_REGIME_SEC_GET_MAX_ADSP_CLK_KHZ,
	PCOM_CLK_REGIME_SEC_GET_I2C_CLK_KHZ,
	PCOM_CLK_REGIME_SEC_MSM_GET_CLK_FREQ_KHZ,
	PCOM_CLK_REGIME_SEC_SEL_VFE_SRC,
	PCOM_CLK_REGIME_SEC_MSM_SEL_CAMCLK,
	PCOM_CLK_REGIME_SEC_MSM_SEL_LCDCLK,
	PCOM_CLK_REGIME_SEC_VFE_RAIL_OFF,
	PCOM_CLK_REGIME_SEC_VFE_RAIL_ON,
	PCOM_CLK_REGIME_SEC_GRP_RAIL_OFF,
	PCOM_CLK_REGIME_SEC_GRP_RAIL_ON,
	PCOM_CLK_REGIME_SEC_VDC_RAIL_OFF,
	PCOM_CLK_REGIME_SEC_VDC_RAIL_ON,
	PCOM_CLK_REGIME_SEC_LCD_CTRL,
	PCOM_CLK_REGIME_SEC_REGISTER_FOR_CPU_RESOURCE,
	PCOM_CLK_REGIME_SEC_DEREGISTER_FOR_CPU_RESOURCE,
	PCOM_CLK_REGIME_SEC_RESOURCE_REQUEST_WRP,
	PCOM_CLK_REGIME_MSM_SEC_SEL_CLK_OWNER,
	PCOM_CLK_REGIME_SEC_DEVMAN_REQUEST_WRP,
	PCOM_GPIO_CONFIG,
	PCOM_GPIO_CONFIGURE_GROUP,
	PCOM_GPIO_TLMM_SET_PORT,
	PCOM_GPIO_TLMM_CONFIG_EX,
	PCOM_SET_FTM_BOOT_COUNT,
	PCOM_RESERVED0,
	PCOM_RESERVED1,
	PCOM_CUSTOMER_CMD1,
	PCOM_CUSTOMER_CMD2,
	PCOM_CUSTOMER_CMD3,
	PCOM_CLK_REGIME_ENTER_APPSBL_CHG_MODE,
	PCOM_CLK_REGIME_EXIT_APPSBL_CHG_MODE,
	PCOM_CLK_REGIME_SEC_RAIL_DISABLE,
	PCOM_CLK_REGIME_SEC_RAIL_ENABLE,
	PCOM_CLK_REGIME_SEC_RAIL_CONTROL,
	PCOM_SET_SW_WATCHDOG_STATE,
	PCOM_PM_MPP_CONFIG_DIGITAL_INPUT,
	PCOM_PM_MPP_CONFIG_I_SINK,
	PCOM_RESERVED_101,
	PCOM_MSM_HSUSB_PHY_RESET,
	PCOM_GET_BATT_MV_LEVEL,
	PCOM_CHG_USB_IS_PC_CONNECTED,
	PCOM_CHG_USB_IS_CHARGER_CONNECTED,
	PCOM_CHG_USB_IS_DISCONNECTED,
	PCOM_CHG_USB_IS_AVAILABLE,
	PCOM_CLK_REGIME_SEC_MSM_SEL_FREQ,
	PCOM_CLK_REGIME_SEC_SET_PCLK_AXI_POLICY,
	PCOM_CLKCTL_RPC_RESET_ASSERT,
	PCOM_CLKCTL_RPC_RESET_DEASSERT,
	PCOM_CLKCTL_RPC_RAIL_ON,
	PCOM_CLKCTL_RPC_RAIL_OFF,
	PCOM_CLKCTL_RPC_RAIL_ENABLE,
	PCOM_CLKCTL_RPC_RAIL_DISABLE,
	PCOM_CLKCTL_RPC_RAIL_CONTROL,
	PCOM_CLKCTL_RPC_MIN_MSMC1,
	PCOM_NUM_CMDS,
};

/*proc_comm status */
enum {
	PCOM_INVALID_STATUS = 0x0,
	PCOM_READY,
	PCOM_CMD_RUNNING,
	PCOM_CMD_SUCCESS,
	PCOM_CMD_FAIL,
	PCOM_CMD_FAIL_FALSE_RETURNED,
	PCOM_CMD_FAIL_CMD_OUT_OF_BOUNDS_SERVER,
	PCOM_CMD_FAIL_CMD_OUT_OF_BOUNDS_CLIENT,
	PCOM_CMD_FAIL_CMD_UNREGISTERED,
	PCOM_CMD_FAIL_CMD_LOCKED,
	PCOM_CMD_FAIL_SERVER_NOT_YET_READY,
	PCOM_CMD_FAIL_BAD_DESTINATION,
	PCOM_CMD_FAIL_SERVER_RESET,
	PCOM_CMD_FAIL_SMSM_NOT_INIT,
	PCOM_CMD_FAIL_PROC_COMM_BUSY,
	PCOM_CMD_FAIL_PROC_COMM_NOT_INIT,
};

/* List of VREGs that support the Pull Down Resistor setting. */
enum vreg_pdown_id {
	PM_VREG_PDOWN_MSMA_ID,
	PM_VREG_PDOWN_MSMP_ID,
	PM_VREG_PDOWN_MSME1_ID,	/* Not supported in Panoramix */
	PM_VREG_PDOWN_MSMC1_ID,	/* Not supported in PM6620 */
	PM_VREG_PDOWN_MSMC2_ID,	/* Supported in PM7500 only */
	PM_VREG_PDOWN_GP3_ID,	/* Supported in PM7500 only */
	PM_VREG_PDOWN_MSME2_ID,	/* Supported in PM7500 and Panoramix only */
	PM_VREG_PDOWN_GP4_ID,	/* Supported in PM7500 only */
	PM_VREG_PDOWN_GP1_ID,	/* Supported in PM7500 only */
	PM_VREG_PDOWN_TCXO_ID,
	PM_VREG_PDOWN_PA_ID,
	PM_VREG_PDOWN_RFTX_ID,
	PM_VREG_PDOWN_RFRX1_ID,
	PM_VREG_PDOWN_RFRX2_ID,
	PM_VREG_PDOWN_SYNT_ID,
	PM_VREG_PDOWN_WLAN_ID,
	PM_VREG_PDOWN_USB_ID,
	PM_VREG_PDOWN_MMC_ID,
	PM_VREG_PDOWN_RUIM_ID,
	PM_VREG_PDOWN_MSMC0_ID,	/* Supported in PM6610 only */
	PM_VREG_PDOWN_GP2_ID,	/* Supported in PM7500 only */
	PM_VREG_PDOWN_GP5_ID,	/* Supported in PM7500 only */
	PM_VREG_PDOWN_GP6_ID,	/* Supported in PM7500 only */
	PM_VREG_PDOWN_RF_ID,
	PM_VREG_PDOWN_RF_VCO_ID,
	PM_VREG_PDOWN_MPLL_ID,
	PM_VREG_PDOWN_S2_ID,
	PM_VREG_PDOWN_S3_ID,
	PM_VREG_PDOWN_RFUBM_ID,

	/* new for HAN */
	PM_VREG_PDOWN_RF1_ID,
	PM_VREG_PDOWN_RF2_ID,
	PM_VREG_PDOWN_RFA_ID,
	PM_VREG_PDOWN_CDC2_ID,
	PM_VREG_PDOWN_RFTX2_ID,
	PM_VREG_PDOWN_USIM_ID,
	PM_VREG_PDOWN_USB2P6_ID,
	PM_VREG_PDOWN_USB3P3_ID,
	PM_VREG_PDOWN_INVALID_ID,

	/* backward compatible enums only */
	PM_VREG_PDOWN_CAM_ID = PM_VREG_PDOWN_GP1_ID,
	PM_VREG_PDOWN_MDDI_ID = PM_VREG_PDOWN_GP2_ID,
	PM_VREG_PDOWN_RUIM2_ID = PM_VREG_PDOWN_GP3_ID,
	PM_VREG_PDOWN_AUX_ID = PM_VREG_PDOWN_GP4_ID,
	PM_VREG_PDOWN_AUX2_ID = PM_VREG_PDOWN_GP5_ID,
	PM_VREG_PDOWN_BT_ID = PM_VREG_PDOWN_GP6_ID,

	PM_VREG_PDOWN_MSME_ID = PM_VREG_PDOWN_MSME1_ID,
	PM_VREG_PDOWN_MSMC_ID = PM_VREG_PDOWN_MSMC1_ID,
	PM_VREG_PDOWN_RFA1_ID = PM_VREG_PDOWN_RFRX2_ID,
	PM_VREG_PDOWN_RFA2_ID = PM_VREG_PDOWN_RFTX2_ID,
	PM_VREG_PDOWN_XO_ID = PM_VREG_PDOWN_TCXO_ID
};

enum {
        PCOM_CLKRGM_APPS_RESET_USB_PHY  = 34,
        PCOM_CLKRGM_APPS_RESET_USBH     = 37,
};

/* ---- PROC_COMM_PM_MPP_CONFIG command related ----*/
/* MPP pins available */
enum
{
	PM_MPP_1,
	PM_MPP_2,
	PM_MPP_3,
	PM_MPP_4,
	PM_MPP_5,
	PM_MPP_6,
	PM_MPP_7,
	PM_MPP_8,
	PM_MPP_9,
	PM_MPP_10,
	PM_MPP_11,
	PM_MPP_12,
	PM_MPP_13,
	PM_MPP_14,
	PM_MPP_15,
	PM_MPP_16,
	PM_MPP_17,
	PM_MPP_18,
	PM_MPP_19,
	PM_MPP_20,
	PM_MPP_21,
	PM_MPP_22,
	PM_MPP_INVALID,
	PM_NUM_MPP_HAN = PM_MPP_4 + 1, 		/* Max num of MPP's for PM7500 */
	PM_NUM_MPP_PM7500 = PM_MPP_22 + 1, 	/* Max num of MPP's for PM6650 */
	PM_NUM_MPP_PM6650 = PM_MPP_12 + 1,	/* Max num of MPP's for PANORAMIX and PM6640 */
	PM_NUM_MPP_PANORAMIX = PM_MPP_2 + 1,
	PM_NUM_MPP_PM6640 = PM_NUM_MPP_PANORAMIX,
	PM_NUM_MPP_PM6620 = PM_NUM_MPP_PANORAMIX
};

/* MPP Logic Level */
enum
{
	PM_MPP__DLOGIC__LVL_MSME,
	PM_MPP__DLOGIC__LVL_MSMP,
	PM_MPP__DLOGIC__LVL_RUIM,
	PM_MPP__DLOGIC__LVL_MMC,
	PM_MPP__DLOGIC__LVL_VDD,
	PM_MPP__DLOGIC__LVL_INVALID
};

/* MPP Output control */
enum
{

	PM_MPP__DLOGIC_OUT__CTRL_LOW, 	/* MPP OUTPUT= LOGIC LOW */
	PM_MPP__DLOGIC_OUT__CTRL_HIGH, 	/* MPP OUTPUT= LOGIC HIGH */
	PM_MPP__DLOGIC_OUT__CTRL_MPP, 	/* MPP OUTPUT= CORRESPONDING MPP INPUT */
	PM_MPP__DLOGIC_OUT__CTRL_NOT_MPP, /* MPP OUTPUT= CORRESPONDING INVERTED MPP INPUT*/
	PM_MPP__DLOGIC_OUT__CTRL_INVALID,
};

/* GPIO TLMM: Direction */
enum {
	PCOM_GPIO_CFG_INPUT,
	PCOM_GPIO_CFG_OUTPUT,
};

/* GPIO TLMM: Pullup/Pulldown */
enum {
	PCOM_GPIO_CFG_NO_PULL,
	PCOM_GPIO_CFG_PULL_DOWN,
	PCOM_GPIO_CFG_KEEPER,
	PCOM_GPIO_CFG_PULL_UP,
};

/* GPIO TLMM: Drive Strength */
enum {
	PCOM_GPIO_CFG_2MA,
	PCOM_GPIO_CFG_4MA,
	PCOM_GPIO_CFG_6MA,
	PCOM_GPIO_CFG_8MA,
	PCOM_GPIO_CFG_10MA,
	PCOM_GPIO_CFG_12MA,
	PCOM_GPIO_CFG_14MA,
	PCOM_GPIO_CFG_16MA,
};

enum {
	PCOM_GPIO_CFG_ENABLE,
	PCOM_GPIO_CFG_DISABLE,
};

enum {
	PCOM_GPIO_OWNER_BASEBAND,
	PCOM_GPIO_OWNER_ARM11,
};

/*---- Clk control related ----*/
enum
{
	PCOM_ACPU_CLK,	  	/* Applications processor clock */
	PCOM_ADM_CLK,	  	/* Applications data mover clock */
	PCOM_ADSP_CLK,	  	/* ADSP clock */
	PCOM_EBI1_CLK,	  	/* External bus interface 1 clock */
	PCOM_EBI2_CLK,	  	/* External bus interface 2 clock */
	PCOM_ECODEC_CLK,	/* External CODEC clock */
	PCOM_EMDH_CLK,	  	/* External MDDI host clock */
	PCOM_GP_CLK,	  	/* General purpose clock */
	PCOM_GRP_CLK,	  	/* Graphics clock */
	PCOM_I2C_CLK,	  	/* I2C clock */
	PCOM_ICODEC_RX_CLK, /* Internal CODEX RX clock */
	PCOM_ICODEC_TX_CLK, /* Internal CODEX TX clock */
	PCOM_IMEM_CLK,	  	/* Internal graphics memory clock */
	PCOM_MDC_CLK,	 	/* MDDI client clock */
	PCOM_MDP_CLK,	  	/* Mobile display processor clock */
	PCOM_PBUS_CLK,	  	/* Peripheral bus clock */
	PCOM_PCM_CLK,	  	/* PCM clock */
	PCOM_PMDH_CLK,	  	/* Primary MDDI host clock */
	PCOM_SDAC_CLK,	  	/* Stereo DAC clock */
	PCOM_SDC1_CLK,	  	/* Secure Digital Card clocks */
	PCOM_SDC1_PCLK,
	PCOM_SDC2_CLK,
	PCOM_SDC2_PCLK,
	PCOM_SDC3_CLK,
	PCOM_SDC3_PCLK,
	PCOM_SDC4_CLK,
	PCOM_SDC4_PCLK,
	PCOM_TSIF_CLK,	  	/* Transport Stream Interface clocks */
	PCOM_TSIF_REF_CLK,
	PCOM_TV_DAC_CLK,	/* TV clocks */
	PCOM_TV_ENC_CLK,
	PCOM_UART1_CLK,	  	/* UART clocks */
	PCOM_UART2_CLK,
	PCOM_UART3_CLK,
	PCOM_UART1DM_CLK,
	PCOM_UART2DM_CLK,
	PCOM_USB_HS_CLK,	/* High speed USB core clock */
	PCOM_USB_HS_PCLK,	/* High speed USB pbus clock */
	PCOM_USB_OTG_CLK,	/* Full speed USB clock */
	PCOM_VDC_CLK,	  	/* Video controller clock */
	PCOM_VFE_MDC_CLK,	/* Camera / Video Front End clock */
	PCOM_VFE_CLK,	  	/* VFE MDDI client clock */
	PCOM_MDP_LCDC_PCLK_CLK,
	PCOM_MDP_LCDC_PAD_PCLK_CLK,
	PCOM_MDP_VSYNC_CLK,
	PCOM_SPI_CLK,
	PCOM_VFE_AXI_CLK,
	PCOM_USB_HS2_CLK,	/* High speed USB 2 core clock */
	PCOM_USB_HS2_PCLK,	/* High speed USB 2 pbus clock */
	PCOM_USB_HS3_CLK,	/* High speed USB 3 core clock */
	PCOM_USB_HS3_PCLK,	/* High speed USB 3 pbus clock */
	PCOM_GRP_PCLK,	  	/* Graphics pbus clock */
	PCOM_NR_CLKS,
};

void msm_pcom_init(void);

int msm_pcom_wait_for_modem_ready(void);
int msm_proc_comm(unsigned cmd, unsigned *data1, unsigned *data2);

int pcom_gpio_tlmm_config(unsigned config, unsigned disable);
int pcom_vreg_set_level(unsigned id, unsigned mv);
int pcom_vreg_enable(unsigned id);
int pcom_vreg_disable(unsigned id);

int pcom_clock_enable(unsigned id);
int pcom_clock_disable(unsigned id);
int pcom_clock_set_rate(unsigned id, unsigned rate);
int pcom_clock_get_rate(unsigned id);
int pcom_clock_is_enabled(unsigned id);
int pcom_set_clock_flags(unsigned id, unsigned flags);

#endif //__PCOM_H_
