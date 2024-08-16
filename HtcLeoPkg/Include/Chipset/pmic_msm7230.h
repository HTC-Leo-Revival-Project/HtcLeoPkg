#ifndef __PLATFORM_MSM7230_PMIC_HW_H
#define __PLATFORM_MSM7230_PMIC_HW_H

#include <Chipset/irqs_msm7230.h>

#define REG_MPP_BASE			0x50
#define REG_IRQ_BASE			0x1BB

/* PMIC8058 Revision */
#define PM8058_REG_REV			0x002  /* PMIC4 revision */
#define PM8058_VERSION_MASK		0xF0
#define PM8058_REVISION_MASK		0x0F
#define PM8058_VERSION_VALUE		0xE0

/* PMIC 8058 Battery Alarm SSBI registers */
#define REG_BATT_ALARM_THRESH		0x023
#define REG_BATT_ALARM_CTRL1		0x024
#define REG_BATT_ALARM_CTRL2		0x0AA
#define REG_BATT_ALARM_PWM_CTRL		0x0A3

#define REG_TEMP_ALRM_CTRL		0x1B
#define REG_TEMP_ALRM_PWM		0x9B

/* PON CNTL 1 register */
#define SSBI_REG_ADDR_PON_CNTL_1	0x01C

#define PM8058_PON_PUP_MASK		0xF0

#define PM8058_PON_WD_EN_MASK		0x08
#define PM8058_PON_WD_EN_RESET		0x08
#define PM8058_PON_WD_EN_PWR_OFF	0x00

/* PON CNTL 4 register */
#define SSBI_REG_ADDR_PON_CNTL_4 0x98
#define PM8058_PON_RESET_EN_MASK 0x01

/* PON CNTL 5 register */
#define SSBI_REG_ADDR_PON_CNTL_5 0x7B
#define PM8058_HARD_RESET_EN_MASK 0x08

/* Regulator master enable addresses */
#define SSBI_REG_ADDR_VREG_EN_MSM	0x018
#define SSBI_REG_ADDR_VREG_EN_GRP_5_4	0x1C8

/* Regulator control registers for shutdown/reset */
#define SSBI_REG_ADDR_S0_CTRL		0x004
#define SSBI_REG_ADDR_S1_CTRL		0x005
#define SSBI_REG_ADDR_S3_CTRL		0x111
#define SSBI_REG_ADDR_L21_CTRL		0x120
#define SSBI_REG_ADDR_L22_CTRL		0x121

#define REGULATOR_ENABLE_MASK		0x80
#define REGULATOR_ENABLE		0x80
#define REGULATOR_DISABLE		0x00
#define REGULATOR_PULL_DOWN_MASK	0x40
#define REGULATOR_PULL_DOWN_EN		0x40
#define REGULATOR_PULL_DOWN_DIS		0x00

/* Buck CTRL register */
#define SMPS_LEGACY_VREF_SEL		0x20
#define SMPS_LEGACY_VPROG_MASK		0x1F
#define SMPS_ADVANCED_BAND_MASK		0xC0
#define SMPS_ADVANCED_BAND_SHIFT	6
#define SMPS_ADVANCED_VPROG_MASK	0x3F

/* Buck TEST2 registers for shutdown/reset */
#define SSBI_REG_ADDR_S0_TEST2		0x084
#define SSBI_REG_ADDR_S1_TEST2		0x085
#define SSBI_REG_ADDR_S3_TEST2		0x11A

#define REGULATOR_BANK_WRITE		0x80
#define REGULATOR_BANK_MASK		0x70
#define REGULATOR_BANK_SHIFT		4
#define REGULATOR_BANK_SEL(n)		((n) << REGULATOR_BANK_SHIFT)

/* Buck TEST2 register bank 1 */
#define SMPS_LEGACY_VLOW_SEL		0x01

/* Buck TEST2 register bank 7 */
#define SMPS_ADVANCED_MODE_MASK		0x02
#define SMPS_ADVANCED_MODE		0x02
#define SMPS_LEGACY_MODE		0x00

/* SLEEP CNTL register */
#define SSBI_REG_ADDR_SLEEP_CNTL	0x02B

#define PM8058_SLEEP_SMPL_EN_MASK	0x04
#define PM8058_SLEEP_SMPL_EN_RESET	0x04
#define PM8058_SLEEP_SMPL_EN_PWR_OFF	0x00

#define PM8058_SLEEP_SMPL_SEL_MASK	0x03
#define PM8058_SLEEP_SMPL_SEL_MIN	0
#define PM8058_SLEEP_SMPL_SEL_MAX	3

/* GP_TEST1 register */
#define SSBI_REG_ADDR_GP_TEST_1		0x07A

#define PM8058_RTC_BASE			0x1E8
#define PM8058_OTHC_CNTR_BASE0		0xA0
#define PM8058_OTHC_CNTR_BASE1		0x134
#define PM8058_OTHC_CNTR_BASE2		0x137

#define PM8058_GPIOS		40
#define PM8058_MPPS		12

#define PM8058_GPIO_BLOCK_START	24
#define PM8058_MPP_BLOCK_START	16

#define PM8058_NR_IRQS		256

#define PM8058_IRQ_BLOCK_BIT(block, bit) ((block) * 8 + (bit))

/* MPPs and GPIOs [0,N) */
#define PM8058_MPP_IRQ(base, mpp)	((base) + \
					PM8058_IRQ_BLOCK_BIT(16, (mpp)))
#define PM8058_GPIO_IRQ(base, gpio)	((base) + \
					PM8058_IRQ_BLOCK_BIT(24, (gpio)))

#define BIT(x)  (1<<(x))

/* PM8058 IRQ's */
#define PM8058_VCP_IRQ			PM8058_IRQ_BLOCK_BIT(1, 0)
#define PM8058_CHGILIM_IRQ		PM8058_IRQ_BLOCK_BIT(1, 3)
#define PM8058_VBATDET_LOW_IRQ		PM8058_IRQ_BLOCK_BIT(1, 4)
#define PM8058_BATT_REPLACE_IRQ		PM8058_IRQ_BLOCK_BIT(1, 5)
#define PM8058_CHGINVAL_IRQ		PM8058_IRQ_BLOCK_BIT(1, 6)
#define PM8058_CHGVAL_IRQ		PM8058_IRQ_BLOCK_BIT(1, 7)
#define PM8058_CHG_END_IRQ		PM8058_IRQ_BLOCK_BIT(2, 0)
#define PM8058_FASTCHG_IRQ		PM8058_IRQ_BLOCK_BIT(2, 1)
#define PM8058_CHGSTATE_IRQ		PM8058_IRQ_BLOCK_BIT(2, 3)
#define PM8058_AUTO_CHGFAIL_IRQ		PM8058_IRQ_BLOCK_BIT(2, 4)
#define PM8058_AUTO_CHGDONE_IRQ		PM8058_IRQ_BLOCK_BIT(2, 5)
#define PM8058_ATCFAIL_IRQ		PM8058_IRQ_BLOCK_BIT(2, 6)
#define PM8058_ATC_DONE_IRQ		PM8058_IRQ_BLOCK_BIT(2, 7)
#define PM8058_OVP_OK_IRQ		PM8058_IRQ_BLOCK_BIT(3, 0)
#define PM8058_COARSE_DET_OVP_IRQ	PM8058_IRQ_BLOCK_BIT(3, 1)
#define PM8058_VCPMAJOR_IRQ		PM8058_IRQ_BLOCK_BIT(3, 2)
#define PM8058_CHG_GONE_IRQ		PM8058_IRQ_BLOCK_BIT(3, 3)
#define PM8058_CHGTLIMIT_IRQ		PM8058_IRQ_BLOCK_BIT(3, 4)
#define PM8058_CHGHOT_IRQ		PM8058_IRQ_BLOCK_BIT(3, 5)
#define PM8058_BATTTEMP_IRQ		PM8058_IRQ_BLOCK_BIT(3, 6)
#define PM8058_BATTCONNECT_IRQ		PM8058_IRQ_BLOCK_BIT(3, 7)
#define PM8058_BATFET_IRQ		PM8058_IRQ_BLOCK_BIT(5, 4)
#define PM8058_VBATDET_IRQ		PM8058_IRQ_BLOCK_BIT(5, 5)
#define PM8058_VBAT_IRQ			PM8058_IRQ_BLOCK_BIT(5, 6)

#define PM8058_RTC_IRQ			PM8058_IRQ_BLOCK_BIT(6, 5)
#define PM8058_RTC_ALARM_IRQ		PM8058_IRQ_BLOCK_BIT(4, 7)
#define PM8058_PWRKEY_REL_IRQ		PM8058_IRQ_BLOCK_BIT(6, 2)
#define PM8058_PWRKEY_PRESS_IRQ		PM8058_IRQ_BLOCK_BIT(6, 3)
#define PM8058_KEYPAD_IRQ		PM8058_IRQ_BLOCK_BIT(9, 2)
#define PM8058_KEYSTUCK_IRQ		PM8058_IRQ_BLOCK_BIT(9, 3)
#define PM8058_BATT_ALARM_IRQ		PM8058_IRQ_BLOCK_BIT(5, 6)
#define PM8058_SW_0_IRQ			PM8058_IRQ_BLOCK_BIT(7, 1)
#define PM8058_IR_0_IRQ			PM8058_IRQ_BLOCK_BIT(7, 0)
#define PM8058_SW_1_IRQ			PM8058_IRQ_BLOCK_BIT(7, 3)
#define PM8058_IR_1_IRQ			PM8058_IRQ_BLOCK_BIT(7, 2)
#define PM8058_SW_2_IRQ			PM8058_IRQ_BLOCK_BIT(7, 5)
#define PM8058_IR_2_IRQ			PM8058_IRQ_BLOCK_BIT(7, 4)
#define PM8058_TEMPSTAT_IRQ		PM8058_IRQ_BLOCK_BIT(6, 7)
#define PM8058_OVERTEMP_IRQ		PM8058_IRQ_BLOCK_BIT(4, 2)
#define PM8058_ADC_IRQ			PM8058_IRQ_BLOCK_BIT(9, 4)
#define PM8058_OSCHALT_IRQ		PM8058_IRQ_BLOCK_BIT(4, 6)
#define PM8058_CBLPWR_IRQ		PM8058_IRQ_BLOCK_BIT(4, 3)
#define PM8058_RESOUT_IRQ		PM8058_IRQ_BLOCK_BIT(6, 4)

/* PMIC version specific silicon revisions */
#define PM8XXX_REVISION_8058_TEST	0
#define PM8XXX_REVISION_8058_1p0	1
#define PM8XXX_REVISION_8058_2p0	2
#define PM8XXX_REVISION_8058_2p1	3

#define PM8XXX_REVISION_8901_TEST	0
#define PM8XXX_REVISION_8901_1p0	1
#define PM8XXX_REVISION_8901_1p1	2
#define PM8XXX_REVISION_8901_2p0	3
#define PM8XXX_REVISION_8901_2p1	4

#define PM8XXX_REVISION_8921_TEST	0
#define PM8XXX_REVISION_8921_1p0	1
#define PM8XXX_REVISION_8921_1p1	2
#define PM8XXX_REVISION_8921_2p0	3
#define PM8XXX_REVISION_8921_3p0	4

#define PM8XXX_REVISION_8821_TEST	0
#define PM8XXX_REVISION_8821_1p0	1
#define PM8XXX_REVISION_8821_2p0	2
#define PM8XXX_REVISION_8821_2p1	3

#define PM8XXX_REVISION_8018_TEST	0
#define PM8XXX_REVISION_8018_1p0	1
#define PM8XXX_REVISION_8018_1p1	2
#define PM8XXX_REVISION_8018_2p0	3

#define PM8XXX_REVISION_8922_TEST	0
#define PM8XXX_REVISION_8922_1p0	1
#define PM8XXX_REVISION_8922_1p1	2
#define PM8XXX_REVISION_8922_2p0	3

enum pon_config{
	DISABLE_HARD_RESET = 0,
	SHUTDOWN_ON_HARD_RESET,
	RESTART_ON_HARD_RESET,
	MAX_PON_CONFIG,
};

enum pm8058_smpl_delay {
	PM8058_SMPL_DELAY_0p5,
	PM8058_SMPL_DELAY_1p0,
	PM8058_SMPL_DELAY_1p5,
	PM8058_SMPL_DELAY_2p0,
};

enum pm8xxx_version {
	PM8XXX_VERSION_8058,
	PM8XXX_VERSION_8901,
	PM8XXX_VERSION_8921,
	PM8XXX_VERSION_8821,
	PM8XXX_VERSION_8018,
	PM8XXX_VERSION_8922,
};

/* PMIC8xxx IRQ */

//used to be SSBI_REG_ADDR_IRQ_*(x) (x+y)
#define SSBI_REG_ADDR_IRQ_ROOT(base)		(base + 0)
#define SSBI_REG_ADDR_IRQ_M_STATUS1(base)	(base + 1)
#define SSBI_REG_ADDR_IRQ_M_STATUS2(base)	(base + 2)
#define SSBI_REG_ADDR_IRQ_M_STATUS3(base)	(base + 3)
#define SSBI_REG_ADDR_IRQ_M_STATUS4(base)	(base + 4)
#define SSBI_REG_ADDR_IRQ_BLK_SEL(base)		(base + 5)
#define SSBI_REG_ADDR_IRQ_IT_STATUS(base)	(base + 6)
#define SSBI_REG_ADDR_IRQ_CONFIG(base)		(base + 7)
#define SSBI_REG_ADDR_IRQ_RT_STATUS(base)	(base + 8)

#define	PM_IRQF_LVL_SEL			0x01	/* level select */
#define	PM_IRQF_MASK_FE			0x02	/* mask falling edge */
#define	PM_IRQF_MASK_RE			0x04	/* mask rising edge */
#define	PM_IRQF_CLR			0x08	/* clear interrupt */
#define	PM_IRQF_BITS_MASK		0x70
#define	PM_IRQF_BITS_SHIFT		4
#define	PM_IRQF_WRITE			0x80

#define	PM_IRQF_MASK_ALL		(PM_IRQF_MASK_FE | \
					PM_IRQF_MASK_RE)

#define	MAX_PM_IRQ		256


//GPIO STUFF

/* GPIO registers */
#define	SSBI_REG_ADDR_GPIO_BASE		0x150
#define	SSBI_REG_ADDR_GPIO(n)		(SSBI_REG_ADDR_GPIO_BASE + n)

/* GPIO */
#define	PM_GPIO_BANK_MASK		0x70
#define	PM_GPIO_BANK_SHIFT		4
#define	PM_GPIO_WRITE			0x80

/* Bank 0 */
#define	PM_GPIO_VIN_MASK		0x0E
#define	PM_GPIO_VIN_SHIFT		1
#define	PM_GPIO_MODE_ENABLE		0x01

/* Bank 1 */
#define	PM_GPIO_MODE_MASK		0x0C
#define	PM_GPIO_MODE_SHIFT		2
#define	PM_GPIO_OUT_BUFFER		0x02
#define	PM_GPIO_OUT_INVERT		0x01

#define	PM_GPIO_MODE_OFF		3
#define	PM_GPIO_MODE_OUTPUT		2
#define	PM_GPIO_MODE_INPUT		0
#define	PM_GPIO_MODE_BOTH		1

/* Bank 2 */
#define	PM_GPIO_PULL_MASK		0x0E
#define	PM_GPIO_PULL_SHIFT		1

/* Bank 3 */
#define	PM_GPIO_OUT_STRENGTH_MASK	0x0C
#define	PM_GPIO_OUT_STRENGTH_SHIFT	2
#define PM_GPIO_PIN_ENABLE		0x00
#define	PM_GPIO_PIN_DISABLE		0x01

/* Bank 4 */
#define	PM_GPIO_FUNC_MASK		0x0E
#define	PM_GPIO_FUNC_SHIFT		1

/* Bank 5 */
#define	PM_GPIO_NON_INT_POL_INV	0x08
#define PM_GPIO_BANKS		6


#endif