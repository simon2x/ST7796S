/*
 * SPI touch driver
 * xpt2046_touch.c
 *
 *  Version:  2020.11
 *  Author: Simon Wu
 */

#include <stdio.h>
#include <stdlib.h>
#include "xpt2046_touch.h"

void TOUCH_Delay(uint32_t delay);
void TOUCH_IO_Init(void);
void TOUCH_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void TOUCH_IO_Delay(uint32_t c);

uint16_t TS_GetX(void);
uint16_t TS_GetY(void);
void     TS_GetXY(uint16_t *x, uint16_t *y);
uint16_t TS_GetZ1(void);
uint16_t TS_GetZ2(void);
void     TS_Calculate_Z();
uint8_t  TS_IO_DetectTouch(void);
uint16_t TS_Read_Avg_XY(uint8_t cmd);
uint8_t  TS_Read_XY(uint16_t *x, uint16_t *y);

#define DMA_MAXSIZE           0xFFFE

/* SPI commands */
#define READ_X      0xD0
#define READ_Y      0x90
#define READ_Z1     0xB0
#define READ_Z2     0xC0
#define READ_TEMP1  0x80
#define READ_TEMP2  0xF0
#define CMD_POWER_DOWN 0x00
#define CMD_POWER_UP   0x01

uint16_t tx, ty;
uint16_t tz = 0;

#define BITBAND_ACCESS(a, b) *(volatile uint32_t*)(((uint32_t)&a & 0xF0000000) + \
    0x2000000 + (((uint32_t)&a & 0x000FFFFF) << 5) + (b << 2))
/*
 * GPIO mode
 */

/* values for GPIOX_MODER (io mode) */
#define MODE_DIGITAL_INPUT    0x0
#define MODE_OUT              0x1
#define MODE_ALTER            0x2
#define MODE_ANALOG_INPUT     0x3

/* values for GPIOX_OSPEEDR (output speed) */
#define MODE_SPD_LOW          0x0
#define MODE_SPD_MEDIUM       0x1
#define MODE_SPD_HIGH         0x2
#define MODE_SPD_VHIGH        0x3

/* values for GPIOX_OTYPER (output type: PP = push-pull, OD = open-drain) */
#define MODE_OT_PP            0x0
#define MODE_OT_OD            0x1

/* values for GPIOX_PUPDR (push up and down resistor) */
#define MODE_PU_NONE          0x0
#define MODE_PU_UP            0x1
#define MODE_PU_DOWN          0x2

#define GPIOX_PORT_(a, b)     GPIO ## a
#define GPIOX_PORT(a)         GPIOX_PORT_(a)

#define GPIOX_PIN_(a, b)      b
#define GPIOX_PIN(a)          GPIOX_PIN_(a)

#define GPIOX_AFR_(a,b,c)     GPIO ## b->AFR[c >> 3] = (GPIO ## b->AFR[c >> 3] & ~(0x0F << (4 * (c & 7)))) | (a << (4 * (c & 7)));
#define GPIOX_AFR(a, b)       GPIOX_AFR_(a, b)

#define GPIOX_MODER_(a,b,c)   GPIO ## b->MODER = (GPIO ## b->MODER & ~(3 << (2 * c))) | (a << (2 * c));
#define GPIOX_MODER(a, b)     GPIOX_MODER_(a, b)

#define GPIOX_OTYPER_(a,b,c)  GPIO ## b->OTYPER = (GPIO ## b->OTYPER & ~(1 << c)) | (a << c);
#define GPIOX_OTYPER(a, b)    GPIOX_OTYPER_(a, b)

#define GPIOX_OSPEEDR_(a,b,c) GPIO ## b->OSPEEDR = (GPIO ## b->OSPEEDR & ~(3 << (2 * c))) | (a << (2 * c));
#define GPIOX_OSPEEDR(a, b)   GPIOX_OSPEEDR_(a, b)

#define GPIOX_PUPDR_(a,b,c)   GPIO ## b->PUPDR = (GPIO ## b->PUPDR & ~(3 << (2 * c))) | (a << (2 * c));
#define GPIOX_PUPDR(a, b)     GPIOX_PUPDR_(a, b)

#define GPIOX_ODR_(a, b)      BITBAND_ACCESS(GPIO ## a ->ODR, b)
#define GPIOX_ODR(a)          GPIOX_ODR_(a)

#define GPIOX_IDR_(a, b)      BITBAND_ACCESS(GPIO ## a ->IDR, b)
#define GPIOX_IDR(a)          GPIOX_IDR_(a)

#define GPIOX_LINE_(a, b)     EXTI_Line ## b
#define GPIOX_LINE(a)         GPIOX_LINE_(a)

#define GPIOX_PORTSRC_(a, b)  GPIO_PortSourceGPIO ## a
#define GPIOX_PORTSRC(a)      GPIOX_PORTSRC_(a)

#define GPIOX_PINSRC_(a, b)   GPIO_PinSource ## b
#define GPIOX_PINSRC(a)       GPIOX_PINSRC_(a)

#define GPIOX_CLOCK_(a, b)    RCC_AHB1ENR_GPIO ## a ## EN
#define GPIOX_CLOCK(a)        GPIOX_CLOCK_(a)

#define GPIOX_PORTNUM_A       1
#define GPIOX_PORTNUM_B       2
#define GPIOX_PORTNUM_C       3
#define GPIOX_PORTNUM_D       4
#define GPIOX_PORTNUM_E       5
#define GPIOX_PORTNUM_F       6
#define GPIOX_PORTNUM_G       7
#define GPIOX_PORTNUM_H       8
#define GPIOX_PORTNUM_I       9
#define GPIOX_PORTNUM_J       10
#define GPIOX_PORTNUM_K       11
#define GPIOX_PORTNUM_(a, b)  GPIOX_PORTNUM_ ## a
#define GPIOX_PORTNUM(a)      GPIOX_PORTNUM_(a)

#define GPIOX_PORTNAME_(a, b) a
#define GPIOX_PORTNAME(a)     GPIOX_PORTNAME_(a)

/*
 * DMA
 */
#define DMA_ISR_TCIF0_Pos       (5U)
#define DMA_ISR_TCIF0           (0x1U << DMA_ISR_TCIF0_Pos)                  /*!< 0x00000020 */
#define DMA_ISR_HTIF0_Pos       (4U)
#define DMA_ISR_HTIF0           (0x1U << DMA_ISR_HTIF0_Pos)                  /*!< 0x00000010 */
#define DMA_ISR_TEIF0_Pos       (3U)
#define DMA_ISR_TEIF0           (0x1U << DMA_ISR_TEIF0_Pos)                  /*!< 0x00000008 */
#define DMA_ISR_DMEIF0_Pos      (2U)
#define DMA_ISR_DMEIF0          (0x1U << DMA_ISR_DMEIF0_Pos)                 /*!< 0x00000004 */
#define DMA_ISR_FEIF0_Pos       (0U)
#define DMA_ISR_FEIF0           (0x1U << DMA_ISR_FEIF0_Pos)                  /*!< 0x00000001 */
#define DMA_ISR_TCIF1_Pos       (11U)
#define DMA_ISR_TCIF1           (0x1U << DMA_ISR_TCIF1_Pos)                  /*!< 0x00000800 */
#define DMA_ISR_HTIF1_Pos       (10U)
#define DMA_ISR_HTIF1           (0x1U << DMA_ISR_HTIF1_Pos)                  /*!< 0x00000400 */
#define DMA_ISR_TEIF1_Pos       (9U)
#define DMA_ISR_TEIF1           (0x1U << DMA_ISR_TEIF1_Pos)                  /*!< 0x00000200 */
#define DMA_ISR_DMEIF1_Pos      (8U)
#define DMA_ISR_DMEIF1          (0x1U << DMA_ISR_DMEIF1_Pos)                 /*!< 0x00000100 */
#define DMA_ISR_FEIF1_Pos       (6U)
#define DMA_ISR_FEIF1           (0x1U << DMA_ISR_FEIF1_Pos)                  /*!< 0x00000040 */
#define DMA_ISR_TCIF2_Pos       (21U)
#define DMA_ISR_TCIF2           (0x1U << DMA_ISR_TCIF2_Pos)                  /*!< 0x00200000 */
#define DMA_ISR_HTIF2_Pos       (20U)
#define DMA_ISR_HTIF2           (0x1U << DMA_ISR_HTIF2_Pos)                  /*!< 0x00100000 */
#define DMA_ISR_TEIF2_Pos       (19U)
#define DMA_ISR_TEIF2           (0x1U << DMA_ISR_TEIF2_Pos)                  /*!< 0x00080000 */
#define DMA_ISR_DMEIF2_Pos      (18U)
#define DMA_ISR_DMEIF2          (0x1U << DMA_ISR_DMEIF2_Pos)                 /*!< 0x00040000 */
#define DMA_ISR_FEIF2_Pos       (16U)
#define DMA_ISR_FEIF2           (0x1U << DMA_ISR_FEIF2_Pos)                  /*!< 0x00010000 */
#define DMA_ISR_TCIF3_Pos       (27U)
#define DMA_ISR_TCIF3           (0x1U << DMA_ISR_TCIF3_Pos)                  /*!< 0x08000000 */
#define DMA_ISR_HTIF3_Pos       (26U)
#define DMA_ISR_HTIF3           (0x1U << DMA_ISR_HTIF3_Pos)                  /*!< 0x04000000 */
#define DMA_ISR_TEIF3_Pos       (25U)
#define DMA_ISR_TEIF3           (0x1U << DMA_ISR_TEIF3_Pos)                  /*!< 0x02000000 */
#define DMA_ISR_DMEIF3_Pos      (24U)
#define DMA_ISR_DMEIF3          (0x1U << DMA_ISR_DMEIF3_Pos)                 /*!< 0x01000000 */
#define DMA_ISR_FEIF3_Pos       (22U)
#define DMA_ISR_FEIF3           (0x1U << DMA_ISR_FEIF3_Pos)                  /*!< 0x00400000 */
#define DMA_ISR_TCIF4_Pos       (5U)
#define DMA_ISR_TCIF4           (0x1U << DMA_ISR_TCIF4_Pos)                  /*!< 0x00000020 */
#define DMA_ISR_HTIF4_Pos       (4U)
#define DMA_ISR_HTIF4           (0x1U << DMA_ISR_HTIF4_Pos)                  /*!< 0x00000010 */
#define DMA_ISR_TEIF4_Pos       (3U)
#define DMA_ISR_TEIF4           (0x1U << DMA_ISR_TEIF4_Pos)                  /*!< 0x00000008 */
#define DMA_ISR_DMEIF4_Pos      (2U)
#define DMA_ISR_DMEIF4          (0x1U << DMA_ISR_DMEIF4_Pos)                 /*!< 0x00000004 */
#define DMA_ISR_FEIF4_Pos       (0U)
#define DMA_ISR_FEIF4           (0x1U << DMA_ISR_FEIF4_Pos)                  /*!< 0x00000001 */
#define DMA_ISR_TCIF5_Pos       (11U)
#define DMA_ISR_TCIF5           (0x1U << DMA_ISR_TCIF5_Pos)                  /*!< 0x00000800 */
#define DMA_ISR_HTIF5_Pos       (10U)
#define DMA_ISR_HTIF5           (0x1U << DMA_ISR_HTIF5_Pos)                  /*!< 0x00000400 */
#define DMA_ISR_TEIF5_Pos       (9U)
#define DMA_ISR_TEIF5           (0x1U << DMA_ISR_TEIF5_Pos)                  /*!< 0x00000200 */
#define DMA_ISR_DMEIF5_Pos      (8U)
#define DMA_ISR_DMEIF5          (0x1U << DMA_ISR_DMEIF5_Pos)                 /*!< 0x00000100 */
#define DMA_ISR_FEIF5_Pos       (6U)
#define DMA_ISR_FEIF5           (0x1U << DMA_ISR_FEIF5_Pos)                  /*!< 0x00000040 */
#define DMA_ISR_TCIF6_Pos       (21U)
#define DMA_ISR_TCIF6           (0x1U << DMA_ISR_TCIF6_Pos)                  /*!< 0x00200000 */
#define DMA_ISR_HTIF6_Pos       (20U)
#define DMA_ISR_HTIF6           (0x1U << DMA_ISR_HTIF6_Pos)                  /*!< 0x00100000 */
#define DMA_ISR_TEIF6_Pos       (19U)
#define DMA_ISR_TEIF6           (0x1U << DMA_ISR_TEIF6_Pos)                  /*!< 0x00080000 */
#define DMA_ISR_DMEIF6_Pos      (18U)
#define DMA_ISR_DMEIF6          (0x1U << DMA_ISR_DMEIF6_Pos)                 /*!< 0x00040000 */
#define DMA_ISR_FEIF6_Pos       (16U)
#define DMA_ISR_FEIF6           (0x1U << DMA_ISR_FEIF6_Pos)                  /*!< 0x00010000 */
#define DMA_ISR_TCIF7_Pos       (27U)
#define DMA_ISR_TCIF7           (0x1U << DMA_ISR_TCIF7_Pos)                  /*!< 0x08000000 */
#define DMA_ISR_HTIF7_Pos       (26U)
#define DMA_ISR_HTIF7           (0x1U << DMA_ISR_HTIF7_Pos)                  /*!< 0x04000000 */
#define DMA_ISR_TEIF7_Pos       (25U)
#define DMA_ISR_TEIF7           (0x1U << DMA_ISR_TEIF7_Pos)                  /*!< 0x02000000 */
#define DMA_ISR_DMEIF7_Pos      (24U)
#define DMA_ISR_DMEIF7          (0x1U << DMA_ISR_DMEIF7_Pos)                 /*!< 0x01000000 */
#define DMA_ISR_FEIF7_Pos       (22U)
#define DMA_ISR_FEIF7           (0x1U << DMA_ISR_FEIF7_Pos)                  /*!< 0x00400000 */

#define DMA_IFCR_CTCIF0_Pos     (5U)
#define DMA_IFCR_CTCIF0         (0x1U << DMA_IFCR_CTCIF0_Pos)                /*!< 0x00000020 */
#define DMA_IFCR_CHTIF0_Pos     (4U)
#define DMA_IFCR_CHTIF0         (0x1U << DMA_IFCR_CHTIF0_Pos)                /*!< 0x00000010 */
#define DMA_IFCR_CTEIF0_Pos     (3U)
#define DMA_IFCR_CTEIF0         (0x1U << DMA_IFCR_CTEIF0_Pos)                /*!< 0x00000008 */
#define DMA_IFCR_CDMEIF0_Pos    (2U)
#define DMA_IFCR_CDMEIF0        (0x1U << DMA_IFCR_CDMEIF0_Pos)               /*!< 0x00000004 */
#define DMA_IFCR_CFEIF0_Pos     (0U)
#define DMA_IFCR_CFEIF0         (0x1U << DMA_IFCR_CFEIF0_Pos)                /*!< 0x00000001 */
#define DMA_IFCR_CTCIF1_Pos     (11U)
#define DMA_IFCR_CTCIF1         (0x1U << DMA_IFCR_CTCIF1_Pos)                /*!< 0x00000800 */
#define DMA_IFCR_CHTIF1_Pos     (10U)
#define DMA_IFCR_CHTIF1         (0x1U << DMA_IFCR_CHTIF1_Pos)                /*!< 0x00000400 */
#define DMA_IFCR_CTEIF1_Pos     (9U)
#define DMA_IFCR_CTEIF1         (0x1U << DMA_IFCR_CTEIF1_Pos)                /*!< 0x00000200 */
#define DMA_IFCR_CDMEIF1_Pos    (8U)
#define DMA_IFCR_CDMEIF1        (0x1U << DMA_IFCR_CDMEIF1_Pos)               /*!< 0x00000100 */
#define DMA_IFCR_CFEIF1_Pos     (6U)
#define DMA_IFCR_CFEIF1         (0x1U << DMA_IFCR_CFEIF1_Pos)                /*!< 0x00000040 */
#define DMA_IFCR_CTCIF2_Pos     (21U)
#define DMA_IFCR_CTCIF2         (0x1U << DMA_IFCR_CTCIF2_Pos)                /*!< 0x00200000 */
#define DMA_IFCR_CHTIF2_Pos     (20U)
#define DMA_IFCR_CHTIF2         (0x1U << DMA_IFCR_CHTIF2_Pos)                /*!< 0x00100000 */
#define DMA_IFCR_CTEIF2_Pos     (19U)
#define DMA_IFCR_CTEIF2         (0x1U << DMA_IFCR_CTEIF2_Pos)                /*!< 0x00080000 */
#define DMA_IFCR_CDMEIF2_Pos    (18U)
#define DMA_IFCR_CDMEIF2        (0x1U << DMA_IFCR_CDMEIF2_Pos)               /*!< 0x00040000 */
#define DMA_IFCR_CFEIF2_Pos     (16U)
#define DMA_IFCR_CFEIF2         (0x1U << DMA_IFCR_CFEIF2_Pos)                /*!< 0x00010000 */
#define DMA_IFCR_CTCIF3_Pos     (27U)
#define DMA_IFCR_CTCIF3         (0x1U << DMA_IFCR_CTCIF3_Pos)                /*!< 0x08000000 */
#define DMA_IFCR_CHTIF3_Pos     (26U)
#define DMA_IFCR_CHTIF3         (0x1U << DMA_IFCR_CHTIF3_Pos)                /*!< 0x04000000 */
#define DMA_IFCR_CTEIF3_Pos     (25U)
#define DMA_IFCR_CTEIF3         (0x1U << DMA_IFCR_CTEIF3_Pos)                /*!< 0x02000000 */
#define DMA_IFCR_CDMEIF3_Pos    (24U)
#define DMA_IFCR_CDMEIF3        (0x1U << DMA_IFCR_CDMEIF3_Pos)               /*!< 0x01000000 */
#define DMA_IFCR_CFEIF3_Pos     (22U)
#define DMA_IFCR_CFEIF3         (0x1U << DMA_IFCR_CFEIF3_Pos)                /*!< 0x00400000 */
#define DMA_IFCR_CTCIF4_Pos     (5U)
#define DMA_IFCR_CTCIF4         (0x1U << DMA_IFCR_CTCIF4_Pos)                /*!< 0x00000020 */
#define DMA_IFCR_CHTIF4_Pos     (4U)
#define DMA_IFCR_CHTIF4         (0x1U << DMA_IFCR_CHTIF4_Pos)                /*!< 0x00000010 */
#define DMA_IFCR_CTEIF4_Pos     (3U)
#define DMA_IFCR_CTEIF4         (0x1U << DMA_IFCR_CTEIF4_Pos)                /*!< 0x00000008 */
#define DMA_IFCR_CDMEIF4_Pos    (2U)
#define DMA_IFCR_CDMEIF4        (0x1U << DMA_IFCR_CDMEIF4_Pos)               /*!< 0x00000004 */
#define DMA_IFCR_CFEIF4_Pos     (0U)
#define DMA_IFCR_CFEIF4         (0x1U << DMA_IFCR_CFEIF4_Pos)                /*!< 0x00000001 */
#define DMA_IFCR_CTCIF5_Pos     (11U)
#define DMA_IFCR_CTCIF5         (0x1U << DMA_IFCR_CTCIF5_Pos)                /*!< 0x00000800 */
#define DMA_IFCR_CHTIF5_Pos     (10U)
#define DMA_IFCR_CHTIF5         (0x1U << DMA_IFCR_CHTIF5_Pos)                /*!< 0x00000400 */
#define DMA_IFCR_CTEIF5_Pos     (9U)
#define DMA_IFCR_CTEIF5         (0x1U << DMA_IFCR_CTEIF5_Pos)                /*!< 0x00000200 */
#define DMA_IFCR_CDMEIF5_Pos    (8U)
#define DMA_IFCR_CDMEIF5        (0x1U << DMA_IFCR_CDMEIF5_Pos)               /*!< 0x00000100 */
#define DMA_IFCR_CFEIF5_Pos     (6U)
#define DMA_IFCR_CFEIF5         (0x1U << DMA_IFCR_CFEIF5_Pos)                /*!< 0x00000040 */
#define DMA_IFCR_CTCIF6_Pos     (21U)
#define DMA_IFCR_CTCIF6         (0x1U << DMA_IFCR_CTCIF6_Pos)                /*!< 0x00200000 */
#define DMA_IFCR_CHTIF6_Pos     (20U)
#define DMA_IFCR_CHTIF6         (0x1U << DMA_IFCR_CHTIF6_Pos)                /*!< 0x00100000 */
#define DMA_IFCR_CTEIF6_Pos     (19U)
#define DMA_IFCR_CTEIF6         (0x1U << DMA_IFCR_CTEIF6_Pos)                /*!< 0x00080000 */
#define DMA_IFCR_CDMEIF6_Pos    (18U)
#define DMA_IFCR_CDMEIF6        (0x1U << DMA_IFCR_CDMEIF6_Pos)               /*!< 0x00040000 */
#define DMA_IFCR_CFEIF6_Pos     (16U)
#define DMA_IFCR_CFEIF6         (0x1U << DMA_IFCR_CFEIF6_Pos)                /*!< 0x00010000 */
#define DMA_IFCR_CTCIF7_Pos     (27U)
#define DMA_IFCR_CTCIF7         (0x1U << DMA_IFCR_CTCIF7_Pos)                /*!< 0x08000000 */
#define DMA_IFCR_CHTIF7_Pos     (26U)
#define DMA_IFCR_CHTIF7         (0x1U << DMA_IFCR_CHTIF7_Pos)                /*!< 0x04000000 */
#define DMA_IFCR_CTEIF7_Pos     (25U)
#define DMA_IFCR_CTEIF7         (0x1U << DMA_IFCR_CTEIF7_Pos)                /*!< 0x02000000 */
#define DMA_IFCR_CDMEIF7_Pos    (24U)
#define DMA_IFCR_CDMEIF7        (0x1U << DMA_IFCR_CDMEIF7_Pos)               /*!< 0x01000000 */
#define DMA_IFCR_CFEIF7_Pos     (22U)
#define DMA_IFCR_CFEIF7         (0x1U << DMA_IFCR_CFEIF7_Pos)                /*!< 0x00400000 */

typedef struct
{
  __IO uint32_t ISR[2];  /*!< DMA interrupt status register,      Address offset: 0x00 */
  __IO uint32_t IFCR[2]; /*!< DMA interrupt flag clear register,  Address offset: 0x08 */
} DMA_TypeDef_Array;

#define DMANUM_(a, b, c, d)             a
#define DMANUM(a)                       DMANUM_(a)

#define DMACHN_(a, b, c, d)             b
#define DMACHN(a)                       DMACHN_(a)

#define DMASTREAM_(a, b, c, d)          c
#define DMASTREAM(a)                    DMASTREAM_(a)

#define DMAPRIORITY_(a, b, c, d)        d
#define DMAPRIORITY(a)                  DMAPRIORITY_(a)

#define DMAX_STREAMX_(a, b, c, d)       DMA ## a ## _Stream ## c
#define DMAX_STREAMX(a)                 DMAX_STREAMX_(a)

#define DMAX_STREAMX_IRQ_(a, b, c, d)   DMA ## a ## _Stream ## c ## _IRQn
#define DMAX_STREAMX_IRQ(a)             DMAX_STREAMX_IRQ_(a)

#define DMAX_STREAMX_IRQHANDLER_(a, b, c, d) DMA ## a ## _Stream ## c ## _IRQHandler
#define DMAX_STREAMX_IRQHANDLER(a)      DMAX_STREAMX_IRQHANDLER_(a)

// Interrupt event pl: if(DMAX_ISR(TOUCH_DMA_TX) & DMAX_ISR_TCIF(TOUCH_DMA_TX))...
#define DMAX_ISR_(a, b, c, d)           ((DMA_TypeDef_Array*) + DMA ## a ## _BASE)->ISR[c >> 2]
#define DMAX_ISR(a)                     DMAX_ISR_(a)

#define DMAX_ISR_TCIF_(a, b, c, d)      DMA_ISR_TCIF ## c
#define DMAX_ISR_TCIF(a)                DMAX_ISR_TCIF_(a)

#define DMAX_ISR_HTIF_(a, b, c, d)      DMA_ISR_HTIF ## c
#define DMAX_ISR_HTIF(a)                DMAX_ISR_HTIF_(a)

#define DMAX_ISR_TEIF_(a, b, c, d)      DMA_ISR_TEIF ## c
#define DMAX_ISR_TEIF(a)                DMAX_ISR_TEIF_(a)

#define DMAX_ISR_DMEIF_(a, b, c, d)     DMA_ISR_DMEIF ## c
#define DMAX_ISR_DMEIF(a)               DMAX_ISR_DMEIF_(a)

#define DMAX_ISR_FEIF_(a, b, c, d)      DMA_ISR_FEIF ## c
#define DMAX_ISR_FEIF(a)                DMAX_ISR_FEIF_(a)

// Interrupt clear pl: DMAX_IFCR(TOUCH_DMA_TX) = DMAX_IFCR_CTCIF(TOUCH_DMA_TX) | DMAX_IFCR_CFEIF(TOUCH_DMA_TX);
#define DMAX_IFCR_(a, b, c, d)          ((DMA_TypeDef_Array*) + DMA ## a ## _BASE)->IFCR[c >> 2]
#define DMAX_IFCR(a)                    DMAX_IFCR_(a)

#define DMAX_IFCR_CTCIF_(a, b, c, d)    DMA_IFCR_CTCIF ## c
#define DMAX_IFCR_CTCIF(a)              DMAX_IFCR_CTCIF_(a)

#define DMAX_IFCR_CHTIF_(a, b, c, d)    DMA_IFCR_CHTIF ## c
#define DMAX_IFCR_CHTIF(a)              DMAX_IFCR_CHTIF_(a)

#define DMAX_IFCR_CTEIF_(a, b, c, d)    DMA_IFCR_CTEIF ## c
#define DMAX_IFCR_CTEIF(a)              DMAX_IFCR_CTEIF_(a)

#define DMAX_IFCR_CDMEIF_(a, b, c, d)   DMA_IFCR_CDMEIF ## c
#define DMAX_IFCR_CDMEIF(a)             DMAX_IFCR_CDMEIF_(a)

#define DMAX_IFCR_CFEIF_(a, b, c, d)    DMA_IFCR_CFEIF ## c
#define DMAX_IFCR_CFEIF(a)              DMAX_IFCR_CFEIF_(a)

/* Chip select pin set */
#define TP_CS_ON             GPIOX_ODR(TOUCH_CS) = 0
#define TP_CS_OFF            GPIOX_ODR(TOUCH_CS) = 1

/* SPI Write delay */
#if     TOUCH_SPI_SPD_WRITE == 0
#define TOUCH_WRITE_DELAY
#elif   TOUCH_SPI_SPD_WRITE == 1
#define TOUCH_WRITE_DELAY       __NOP()
#elif   TOUCH_SPI_SPD_WRITE == 2
#define TOUCH_WRITE_DELAY       GPIOX_ODR(TOUCH_CLK) = 0
#else
#define TOUCH_WRITE_DELAY       TOUCH_IO_Delay(TOUCH_SPI_SPD_WRITE - 3)
#endif

/* SPI Read delay */
#if     TOUCH_SPI_SPD_READ == 0
#define TOUCH_READ_DELAY
#elif   TOUCH_SPI_SPD_READ == 1
#define TOUCH_READ_DELAY        __NOP()
#elif   TOUCH_SPI_SPD_READ == 2
#define TOUCH_READ_DELAY        GPIOX_ODR(TOUCH_CLK) = 0
#else
#define TOUCH_READ_DELAY        TOUCH_IO_Delay(TOUCH_SPI_SPD_READ - 3)
#endif

/* Check required pin definitions */
#if GPIOX_PORTNUM(TOUCH_CS) < GPIOX_PORTNUM_A
#error LCD CS pin is not defined
#endif

#if GPIOX_PORTNUM(TOUCH_CLK) < GPIOX_PORTNUM_A
#error LCD CLK pin is not defined
#endif

#if GPIOX_PORTNUM(TOUCH_MOSI) < GPIOX_PORTNUM_A
#error LCD MOSI pin is not defined
#endif

#if TOUCH_SPI_HALF_DUPLEX == 0 && GPIOX_PORTNUM(TOUCH_MISO) < GPIOX_PORTNUM_A
#error LCD MISO pin is not defined
#endif


#if TOUCH_SPI == 0
/*
 * Software SPI
 */
#define WaitForDmaEnd()
#define LcdSpiMode8()
#define LcdSpiMode16()
#define TOUCH_WRITE_CLK         GPIOX_ODR(TOUCH_CLK) = 1; TOUCH_WRITE_DELAY; GPIOX_ODR(TOUCH_CLK) = 0;
#define TOUCH_READ_CLK          GPIOX_ODR(TOUCH_CLK) = 0; GPIOX_ODR(TOUCH_CLK) = 1; TOUCH_READ_DELAY;

void LcdWrite8(uint8_t d8)
{
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d8, 7);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d8, 6);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d8, 5);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d8, 4);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d8, 3);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d8, 2);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d8, 1);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d8, 0);
  TOUCH_WRITE_CLK;
}

void LcdWrite16(uint16_t d16)
{
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 15);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 14);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 13);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 12);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 11);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 10);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 9);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 8);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 7);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 6);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 5);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 4);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 3);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 2);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 1);
  TOUCH_WRITE_CLK;
  GPIOX_ODR(TOUCH_MOSI) = BITBAND_ACCESS(d16, 0);
  TOUCH_WRITE_CLK;
}

/*
 * Full duplex SPI mode
 * Data direction is fixed, dummy read
 */
#if TOUCH_SPI_MODE == 0
#define LcdDirWrite()

/*
 * Half duplex SPI mode
 * Data direction change (MISO pin = MOSI pin)
 */
#elif TOUCH_SPI_MODE == 1
#undef  TOUCH_MISO
#define TOUCH_MISO TOUCH_MOSI
#define LcdDirWrite() GPIOX_MODER(MODE_OUT, TOUCH_MOSI)
#endif


void LcdDirRead(uint32_t d)
{
  #if TOUCH_SPI_MODE == 1
  GPIOX_MODER(MODE_DIGITAL_INPUT, TOUCH_MOSI);
  GPIOX_ODR(TOUCH_MOSI) = 0;
  #endif
  TOUCH_READ_DELAY;
  while(d--)
  {
    GPIOX_ODR(TOUCH_CLK) = 0;
    TOUCH_READ_DELAY;
    GPIOX_ODR(TOUCH_CLK) = 1;
  }
  GPIOX_ODR(TOUCH_CLK) = 0; // idle state - remove?
}

// Read 8-bits of data
uint8_t LcdRead8(void)
{
  uint8_t d8;
  GPIOX_ODR(TOUCH_CLK) = 1;
  TOUCH_READ_DELAY;
  BITBAND_ACCESS(d8, 7) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d8, 6) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d8, 5) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d8, 4) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d8, 3) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d8, 2) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d8, 1) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d8, 0) = GPIOX_IDR(TOUCH_MISO);
  GPIOX_ODR(TOUCH_CLK) = 0;
  return d8;
}

//-----------------------------------------------------------------------------
uint16_t LcdRead16(void)
{
  uint16_t d16;
  GPIOX_ODR(TOUCH_CLK) = 1;
  TOUCH_READ_DELAY;
  BITBAND_ACCESS(d16, 15) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 14) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 13) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 12) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 11) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 10) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 9) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 8) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 7) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 6) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 5) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 4) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 3) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 2) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 1) = GPIOX_IDR(TOUCH_MISO);
  TOUCH_READ_CLK;
  BITBAND_ACCESS(d16, 0) = GPIOX_IDR(TOUCH_MISO);
  GPIOX_ODR(TOUCH_CLK) = 0;
  return d16;
}


#else
/* Hardware SPI */
#if TOUCH_SPI == 1
#define SPIX                  SPI1
#define TOUCH_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI1EN_Pos) = 1
#elif TOUCH_SPI == 2
#define SPIX                  SPI2
#define TOUCH_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB1ENR, RCC_APB1ENR_SPI2EN_Pos) = 1
#elif TOUCH_SPI == 3
#define SPIX                  SPI3
#define TOUCH_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB1ENR, RCC_APB1ENR_SPI3EN_Pos) = 1
#elif TOUCH_SPI == 4
#define SPIX                  SPI4
#define TOUCH_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI4EN_Pos) = 1
#elif TOUCH_SPI == 5
#define SPIX                  SPI5
#define TOUCH_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI5EN_Pos) = 1
#elif TOUCH_SPI == 6
#define SPIX                  SPI6
#define TOUCH_SPI_RCC_EN        BITBAND_ACCESS(RCC->APB2ENR, RCC_APB2ENR_SPI6EN_Pos) = 1
#endif

// Set Data Frame Format is 8 or 16 -bit
#define LcdSpiMode8()         BITBAND_ACCESS(SPIX->CR1, SPI_CR1_DFF_Pos) = 0;
#define LcdSpiMode16()        BITBAND_ACCESS(SPIX->CR1, SPI_CR1_DFF_Pos) = 1;

/* Full duplex SPI : the direction is fixed */
#if TOUCH_SPI_MODE == 0

extern inline void LcdWrite8(uint8_t d8);
inline void LcdWrite8(uint8_t d8)
{
  while(!(BITBAND_ACCESS(SPIX->SR, SPI_SR_TXE_Pos)));
  SPIX->DR = d8;
}


extern inline uint8_t LcdRead8(void);
inline uint8_t LcdRead8(void)
{
  while(!(BITBAND_ACCESS(SPIX->SR, SPI_SR_TXE_Pos)));
  SPIX->DR = 0x00; // write dummy value to trigger clock

  while(!BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos));
  return (uint8_t) SPIX->DR;
}

extern inline void LcdDirRead(uint32_t d);
inline void LcdDirRead(uint32_t d)
{
  GPIOX_MODER(MODE_OUT, TOUCH_CLK);
  while(d--)
  {
    GPIOX_ODR(TOUCH_CLK) = 0;
    TOUCH_READ_DELAY;
    GPIOX_ODR(TOUCH_CLK) = 1;
  }
  GPIOX_MODER(MODE_ALTER, TOUCH_CLK);
  while(!BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos));
  SPIX->DR;

  //LcdRead8(); // del

  // Configure for receiving
  SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | (TOUCH_SPI_SPD_READ << SPI_CR1_BR_Pos);
}

extern inline void LcdDirWrite(void);
inline void LcdDirWrite(void)
{
  volatile uint8_t d8 __attribute__((unused));
//  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))
//    d8 = SPIX->DR;
//  SPIX->CR1 &= ~SPI_CR1_SPE;
//  SPIX->CR1 = (SPIX->CR1 & ~(SPI_CR1_BR | SPI_CR1_RXONLY)) | (TOUCH_SPI_SPD_WRITE << SPI_CR1_BR_Pos);
////  TOUCH_IO_Delay(200); // Note: adjust delay (probably increase) if no data out from slave
////  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))
////    d8 = SPIX->DR;
//  SPIX->CR1 |= SPI_CR1_SPE;
}
#endif

#endif   // #else TOUCH_SPI == 0

//-----------------------------------------------------------------------------
#if (DMANUM(TOUCH_DMA_TX) > 0 || DMANUM(TOUCH_DMA_RX) > 0) && TOUCH_SPI > 0
/* DMA transfer end check */

/* DMA status
   - 0: all DMA transfers are completed
   - 1: last DMA transfer in progress
   - 2: DMA transfer in progress */
volatile uint32_t TOUCH_IO_DmaTransferStatus = 0;

//-----------------------------------------------------------------------------
/* Waiting for all DMA processes to complete */
#ifndef osFeature_Semaphore
/* no FreeRtos */

extern inline void WaitForDmaEnd(void);
inline void WaitForDmaEnd(void)
{
  while(TOUCH_IO_DmaTransferStatus);
}

//-----------------------------------------------------------------------------
#else   // osFeature_Semaphore
/* FreeRtos */

osSemaphoreId spiDmaBinSemHandle;

extern inline void WaitForDmaEnd(void);
inline void WaitForDmaEnd(void)
{
  if(TOUCH_IO_DmaTransferStatus)
  {
    osSemaphoreWait(spiDmaBinSemHandle, 500);
    if(TOUCH_IO_DmaTransferStatus == 1)
      TOUCH_IO_DmaTransferStatus = 0;
  }
}

#endif  // #else osFeature_Semaphore

#else   // #if (DMANUM(TOUCH_DMA_TX) > 0 || DMANUM(TOUCH_DMA_RX) > 0) && TOUCH_SPI > 0

#define  WaitForDmaEnd() /* if DMA is not used -> no need to wait */

#endif  // #else (DMANUM(TOUCH_DMA_TX) > 0 || DMANUM(TOUCH_DMA_RX) > 0) && TOUCH_SPI > 0


#if DMANUM(TOUCH_DMA_TX) == 0 || TOUCH_SPI == 0

/* SPI TX no DMA */
//void TOUCH_IO_WriteMultiData8(uint8_t * pData, uint32_t Size, uint32_t dinc)
//{
//  while(Size--)
//  {
//    LcdWrite8(*pData);
//    if(dinc)
//      pData++;
//  }
//  TP_CS_OFF;
//}


#else // #if DMANUM(TOUCH_DMA_TX) == 0 || TOUCH_SPI == 0

/* SPI TX on DMA */

/* All DMA_TX interrupt flag clear */
#define DMAX_IFCRALL_TOUCH_DMA_TX                        { \
  DMAX_IFCR(TOUCH_DMA_TX) = DMAX_IFCR_CTCIF(TOUCH_DMA_TX)  | \
                          DMAX_IFCR_CHTIF(TOUCH_DMA_TX)  | \
                          DMAX_IFCR_CTEIF(TOUCH_DMA_TX)  | \
                          DMAX_IFCR_CDMEIF(TOUCH_DMA_TX) | \
                          DMAX_IFCR_CFEIF(TOUCH_DMA_TX)  ; }

//-----------------------------------------------------------------------------
void DMAX_STREAMX_IRQHANDLER(TOUCH_DMA_TX)(void)
{
  if(DMAX_ISR(TOUCH_DMA_TX) & DMAX_ISR_TCIF(TOUCH_DMA_TX))
  {
    DMAX_IFCR(TOUCH_DMA_TX) = DMAX_IFCR_CTCIF(TOUCH_DMA_TX);
    DMAX_STREAMX(TOUCH_DMA_TX)->CR = 0;
    while(DMAX_STREAMX(TOUCH_DMA_TX)->CR & DMA_SxCR_EN);
    BITBAND_ACCESS(SPIX->CR2, SPI_CR2_TXDMAEN_Pos) = 0;
    while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos));
    SPIX->CR1 &= ~SPI_CR1_SPE;
    TOUCH_IO_Delay(2 ^ TOUCH_SPI_SPD_WRITE);
    SPIX->CR1 |= SPI_CR1_SPE;

    if(TOUCH_IO_DmaTransferStatus == 1) /* last transfer end ? */
      TP_CS_OFF;

    #ifndef osFeature_Semaphore
    /* no FreeRtos */
    TOUCH_IO_DmaTransferStatus = 0;
    #else
    /* FreeRtos */
    osSemaphoreRelease(spiDmaBinSemHandle);
    #endif // #else osFeature_Semaphore
  }
  else
    DMAX_IFCRALL_TOUCH_DMA_TX;
}

//-----------------------------------------------------------------------------
void TOUCH_IO_WriteMultiData(void * pData, uint32_t Size, uint32_t dmacr)
{
  DMAX_IFCRALL_TOUCH_DMA_TX;
  SPIX->CR1 &= ~SPI_CR1_SPE;           /* SPI stop */
  DMAX_STREAMX(TOUCH_DMA_TX)->CR = 0;    /* DMA stop */
  while(DMAX_STREAMX(TOUCH_DMA_TX)->CR & DMA_SxCR_EN);
  DMAX_STREAMX(TOUCH_DMA_TX)->M0AR = (uint32_t)pData;
  DMAX_STREAMX(TOUCH_DMA_TX)->PAR = (uint32_t)&SPIX->DR;
  DMAX_STREAMX(TOUCH_DMA_TX)->NDTR = Size;
  DMAX_STREAMX(TOUCH_DMA_TX)->CR = dmacr;
  DMAX_STREAMX(TOUCH_DMA_TX)->CR |= DMA_SxCR_EN;
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_TXDMAEN_Pos) = 1;
  SPIX->CR1 |= SPI_CR1_SPE;
}

//-----------------------------------------------------------------------------
void TOUCH_IO_WriteMultiData8(uint8_t * pData, uint32_t Size, uint32_t dinc)
{
  uint32_t dmacr;
  static uint8_t d8s;
  if(!dinc)
  {
    d8s = *pData;
    pData = &d8s;
    dmacr = DMAX_STREAMX(TOUCH_DMA_TX)->CR = DMA_SxCR_TCIE |
            (0 << DMA_SxCR_MSIZE_Pos) | (0 << DMA_SxCR_PSIZE_Pos) |
            (0 << DMA_SxCR_MINC_Pos) | (1 << DMA_SxCR_DIR_Pos) |
            (DMACHN(TOUCH_DMA_TX) << DMA_SxCR_CHSEL_Pos) |
            (DMAPRIORITY(TOUCH_DMA_TX) << DMA_SxCR_PL_Pos);
  }
  else
    dmacr = DMAX_STREAMX(TOUCH_DMA_TX)->CR = DMA_SxCR_TCIE |
            (0 << DMA_SxCR_MSIZE_Pos) | (0 << DMA_SxCR_PSIZE_Pos) |
            (1 << DMA_SxCR_MINC_Pos) | (1 << DMA_SxCR_DIR_Pos) |
            (DMACHN(TOUCH_DMA_TX) << DMA_SxCR_CHSEL_Pos) |
            (DMAPRIORITY(TOUCH_DMA_TX) << DMA_SxCR_PL_Pos);

  #ifdef TOUCH_DMA_UNABLE
  if(TOUCH_DMA_UNABLE((uint32_t)(pData)))
  {
    while(Size--)
    {
      LcdWrite8(*pData);
      if(dinc)
        pData++;
    }
    TP_CS_OFF;
  }
  else
  #endif
  {
    while(Size)
    {
      if(Size <= DMA_MAXSIZE)
      {
        TOUCH_IO_DmaTransferStatus = 1;     /* last transfer */
        TOUCH_IO_WriteMultiData((void *)pData, Size, dmacr);
        Size = 0;
        #if TOUCH_DMA_TXWAIT == 1
        if(dinc)
          WaitForDmaEnd();
        #endif
      }
      else
      {
        TOUCH_IO_DmaTransferStatus = 2;     /* no last transfer */
        TOUCH_IO_WriteMultiData((void *)pData, DMA_MAXSIZE, dmacr);
        if(dinc)
          pData+= DMA_MAXSIZE;
        Size-= DMA_MAXSIZE;
        #if TOUCH_DMA_TXWAIT != 2
        WaitForDmaEnd();
        #endif
      }
      #if TOUCH_DMA_TXWAIT == 2
      WaitForDmaEnd();
      #endif
    }
  }
}
#endif // #else DMANUM(TOUCH_DMA_TX) == 0 || TOUCH_SPI == 0

#if DMANUM(TOUCH_DMA_RX) == 0 || TOUCH_SPI == 0

void TOUCH_IO_ReadMultiData8(uint8_t * pData, uint32_t Size)
{
  //uint8_t d8;
  while(Size--)
  {
    *pData++ = LcdRead8();
  }
  while(BITBAND_ACCESS(SPIX->SR, SPI_SR_BSY_Pos));
  TP_CS_OFF;
  LcdDirWrite();
}


#elif DMANUM(TOUCH_DMA_RX) > 0 && TOUCH_SPI > 0

//-----------------------------------------------------------------------------
/* SPI RX on DMA */

/* All DMA_RX interrupt flag clear */
#define DMAX_IFCRALL_TOUCH_DMA_RX                        { \
  DMAX_IFCR(TOUCH_DMA_RX) = DMAX_IFCR_CTCIF(TOUCH_DMA_RX)  | \
                          DMAX_IFCR_CHTIF(TOUCH_DMA_RX)  | \
                          DMAX_IFCR_CTEIF(TOUCH_DMA_RX)  | \
                          DMAX_IFCR_CDMEIF(TOUCH_DMA_RX) | \
                          DMAX_IFCR_CFEIF(TOUCH_DMA_RX)  ; }

//-----------------------------------------------------------------------------
void DMAX_STREAMX_IRQHANDLER(TOUCH_DMA_RX)(void)
{
  volatile uint8_t d8 __attribute__((unused));
  if(DMAX_ISR(TOUCH_DMA_RX) & DMAX_ISR_TCIF(TOUCH_DMA_RX))
  {
    DMAX_IFCR(TOUCH_DMA_RX) = DMAX_IFCR_CTCIF(TOUCH_DMA_RX);
    BITBAND_ACCESS(SPIX->CR2, SPI_CR2_RXDMAEN_Pos) = 0; /* SPI DMA tilt */
    while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))
      d8 = SPIX->DR;
    SPIX->CR1 &= ~SPI_CR1_SPE;
    #if   TOUCH_SPI_MODE == 1
    SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | ((TOUCH_SPI_SPD_WRITE << SPI_CR1_BR_Pos) | SPI_CR1_BIDIOE);
    #elif TOUCH_SPI_MODE == 0
    SPIX->CR1 = (SPIX->CR1 & ~SPI_CR1_BR) | (TOUCH_SPI_SPD_WRITE << SPI_CR1_BR_Pos);
    #endif
    TOUCH_IO_Delay(2 ^ TOUCH_SPI_SPD_READ);
    while(BITBAND_ACCESS(SPIX->SR, SPI_SR_RXNE_Pos))
      d8 = SPIX->DR;
    SPIX->CR1 |= SPI_CR1_SPE;
    DMAX_STREAMX(TOUCH_DMA_RX)->CR = 0;
    while(DMAX_STREAMX(TOUCH_DMA_RX)->CR & DMA_SxCR_EN);

    #ifndef osFeature_Semaphore
    /* no FreeRtos */
    TOUCH_IO_DmaTransferStatus = 0;
    #else  // #ifndef osFeature_Semaphore
    /* FreeRtos */
    osSemaphoreRelease(spiDmaBinSemHandle);
    #endif // #else osFeature_Semaphore
  }
  else
    DMAX_IFCRALL_TOUCH_DMA_RX;
}

//-----------------------------------------------------------------------------
void TOUCH_IO_ReadMultiData(void * pData, uint32_t Size, uint32_t dmacr)
{
  DMAX_IFCRALL_TOUCH_DMA_RX;
  DMAX_STREAMX(TOUCH_DMA_RX)->CR = 0;  /* DMA stop */
  while(DMAX_STREAMX(TOUCH_DMA_RX)->CR & DMA_SxCR_EN);
  DMAX_STREAMX(TOUCH_DMA_RX)->M0AR = (uint32_t)pData;  /* memory addr */
  DMAX_STREAMX(TOUCH_DMA_RX)->PAR = (uint32_t)&SPIX->DR; /* periph addr */
  DMAX_STREAMX(TOUCH_DMA_RX)->NDTR = Size;           /* number of data */
  DMAX_STREAMX(TOUCH_DMA_RX)->CR = dmacr;
  DMAX_STREAMX(TOUCH_DMA_RX)->CR |= DMA_SxCR_EN;  /* DMA start */
  BITBAND_ACCESS(SPIX->CR2, SPI_CR2_RXDMAEN_Pos) = 1; /* SPI DMA eng */
}

//-----------------------------------------------------------------------------
void TOUCH_IO_ReadMultiData8(uint8_t * pData, uint32_t Size)
{
  uint32_t dmacr;
  dmacr = DMA_SxCR_TCIE | (0 << DMA_SxCR_MSIZE_Pos) |
          (0 << DMA_SxCR_PSIZE_Pos) | DMA_SxCR_MINC |
          (DMACHN(TOUCH_DMA_RX) << DMA_SxCR_CHSEL_Pos) |
          (DMAPRIORITY(TOUCH_DMA_RX) << DMA_SxCR_PL_Pos) | (0 << DMA_SxCR_MBURST_Pos);

  while(Size)
  {
    if(Size > DMA_MAXSIZE)
    {
      TOUCH_IO_DmaTransferStatus = 2;     /* no last transfer */
      TOUCH_IO_ReadMultiData((void *)pData, DMA_MAXSIZE, dmacr);
      Size-= DMA_MAXSIZE;
      pData+= DMA_MAXSIZE;
    }
    else
    {
      TOUCH_IO_DmaTransferStatus = 1;     /* last transfer */
      TOUCH_IO_ReadMultiData((void *)pData, Size, dmacr);
      Size = 0;
    }
    WaitForDmaEnd();
  }
  TP_CS_OFF;
  LcdDirWrite();
}

//-----------------------------------------------------------------------------
void TOUCH_IO_ReadMultiData16(uint16_t * pData, uint32_t Size)
{
  uint32_t dmacr;
  dmacr = DMA_SxCR_TCIE | (1 << DMA_SxCR_MSIZE_Pos) |
          (1 << DMA_SxCR_PSIZE_Pos) | DMA_SxCR_MINC |
          (DMACHN(TOUCH_DMA_RX) << DMA_SxCR_CHSEL_Pos) |
          (DMAPRIORITY(TOUCH_DMA_RX) << DMA_SxCR_PL_Pos) | (0 << DMA_SxCR_MBURST_Pos);

  while(Size)
  {
    if(Size > DMA_MAXSIZE)
    {
      TOUCH_IO_DmaTransferStatus = 2;     /* no last transfer */
      TOUCH_IO_ReadMultiData((void *)pData, DMA_MAXSIZE, dmacr);
      Size-= DMA_MAXSIZE;
      pData+= DMA_MAXSIZE;
    }
    else
    {
      TOUCH_IO_DmaTransferStatus = 1;     /* last transfer */
      TOUCH_IO_ReadMultiData((void *)pData, Size, dmacr);
      Size = 0;
    }
    WaitForDmaEnd();
  }
  TP_CS_OFF;
  LcdDirWrite();
}


#endif // #elif DMANUM(TOUCH_DMA_RX) > 0 && TOUCH_SPI > 0

//=============================================================================
#ifdef  __GNUC__
#pragma GCC push_options
#pragma GCC optimize("O0")
#elif   defined(__CC_ARM)
#pragma push
#pragma O0
#endif
void TOUCH_IO_Delay(uint32_t c)
{
  while(c--);
}
#ifdef  __GNUC__
#pragma GCC pop_options
#elif   defined(__CC_ARM)
#pragma pop
#endif

/*
 * Public functions
 *
 */

void TOUCH_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}


/**
 * Initialize SPI
 */
void TOUCH_IO_Init(void)
{
  /* MISO pin clock */
  #if GPIOX_PORTNUM(TOUCH_MISO) >= GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_TOUCH_MISO  GPIOX_CLOCK(TOUCH_MISO)
  #else
  #define GPIOX_CLOCK_TOUCH_MISO  0
  #endif

  /* DMA clock */
  #if TOUCH_SPI == 0
  #define DMA1_CLOCK_TX         0
  #define DMA1_CLOCK_RX         0
  #else
  #if DMANUM(TOUCH_DMA_TX) == 1
  #define DMA1_CLOCK_TX         RCC_AHB1ENR_DMA1EN
  #elif DMANUM(TOUCH_DMA_TX) == 2
  #define DMA1_CLOCK_TX         RCC_AHB1ENR_DMA2EN
  #else
  #define DMA1_CLOCK_TX         0
  #endif
  #if DMANUM(TOUCH_DMA_RX) == 1
  #define DMA1_CLOCK_RX         RCC_AHB1ENR_DMA1EN
  #elif DMANUM(TOUCH_DMA_RX) == 2
  #define DMA1_CLOCK_RX         RCC_AHB1ENR_DMA2EN
  #else
  #define DMA1_CLOCK_RX         0
  #endif
  #endif

  /* GPIO, DMA Clocks */
  RCC->AHB1ENR |= GPIOX_CLOCK(TOUCH_CS) | GPIOX_CLOCK(TOUCH_CLK) | GPIOX_CLOCK(TOUCH_MOSI) |
                  GPIOX_CLOCK_TOUCH_MISO | DMA1_CLOCK_TX | DMA1_CLOCK_RX;

  TP_CS_OFF;
  GPIOX_MODER(MODE_OUT, TOUCH_CS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, TOUCH_CS);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, TOUCH_CLK);
  GPIOX_OSPEEDR(MODE_SPD_VHIGH, TOUCH_MOSI);
  GPIOX_ODR(TOUCH_CLK) = 0;

  #if TOUCH_SPI == 0  /* Software SPI */
  GPIOX_MODER(MODE_OUT, TOUCH_CLK);
  GPIOX_MODER(MODE_OUT, TOUCH_MOSI);
  GPIOX_MODER(MODE_DIGITAL_INPUT, TOUCH_MISO);
  #else /* Hardware SPI */
  TOUCH_SPI_RCC_EN;
  GPIOX_AFR(TOUCH_SPI_AFR, TOUCH_CLK);
  GPIOX_MODER(MODE_ALTER, TOUCH_CLK);
  GPIOX_AFR(TOUCH_SPI_AFR, TOUCH_MOSI);
  GPIOX_MODER(MODE_ALTER, TOUCH_MOSI);
  GPIOX_AFR(TOUCH_SPI_AFR, TOUCH_MISO);
  GPIOX_MODER(MODE_ALTER, TOUCH_MISO);
  /* TX or full duplex */
  SPIX->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM
      | SPI_CR1_SSI | (TOUCH_SPI_SPD_WRITE << SPI_CR1_BR_Pos);

  //SPIX->CR1 |= SPI_CR1_CPHA;
  //SPIX->CR1 |= SPI_CR1_CPOL; // Idle CLK high
  SPIX->CR1 |= SPI_CR1_SPE; // SPI enable
  #endif

  TOUCH_Delay(10);

  #if (DMANUM(TOUCH_DMA_TX) > 0 || DMANUM(TOUCH_DMA_RX) > 0) && TOUCH_SPI > 0
  #if DMANUM(TOUCH_DMA_TX) > 0
  NVIC_SetPriority(DMAX_STREAMX_IRQ(TOUCH_DMA_TX), TOUCH_DMA_IRQ_PR);
  NVIC_EnableIRQ(DMAX_STREAMX_IRQ(TOUCH_DMA_TX));
  #endif
  #if DMANUM(TOUCH_DMA_RX) > 0
  NVIC_SetPriority(DMAX_STREAMX_IRQ(TOUCH_DMA_RX), TOUCH_DMA_IRQ_PR);
  NVIC_EnableIRQ(DMAX_STREAMX_IRQ(TOUCH_DMA_RX));
  #endif
  #ifdef osFeature_Semaphore
  osSemaphoreDef(spiDmaBinSem);
  spiDmaBinSemHandle = osSemaphoreCreate(osSemaphore(spiDmaBinSem), 1);
  osSemaphoreWait(spiDmaBinSemHandle, 1);
  #endif
  #endif  // #if DMANUM(TOUCH_DMA_RX) > 0
}


void TOUCH_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize)
{
  WaitForDmaEnd();
  LcdSpiMode8();
  TP_CS_ON;
  LcdWrite8(Cmd);
  DummySize = (DummySize << 3);
  LcdDirRead(DummySize);
  TOUCH_IO_ReadMultiData8(pData, Size);
}

/**
 * Transmit CMD and return the 12-bit value from a 16-bit read
 */
uint16_t TS_Read_Cmd_12bit(uint8_t cmd)
{
  uint8_t rx_data[2];
  TOUCH_IO_ReadCmd8MultipleData8(cmd, rx_data, 2, 0);
  return rx_data[0] << 5 | rx_data[1] >> 3;
}


/* read the X position */
uint16_t TS_GetX(void)
{
  return TS_Read_Cmd_12bit(READ_X);
}


/* read the Y position */
uint16_t TS_GetY(void)
{
  return TS_Read_Cmd_12bit(READ_Y);
}

/* read the Z1 value */
uint16_t TS_GetZ1(void)
{
  return TS_Read_Cmd_12bit(READ_Z1);
}


/* read the Z2 value */
uint16_t TS_GetZ2(void)
{
  return TS_Read_Cmd_12bit(READ_Z2);
}


/**
 * 12-bit z-coefficient calculation
 */
void TS_Update_Z()
{
  uint16_t z1 = TS_GetZ1();
  uint16_t z2 = TS_GetZ2();
  uint16_t x = TS_GetX();

  // z-coefficient
  tz = x * ((z2 / z1) - 1);

//  printf("z1=%d, z2=%d, x=%d\r\n", z1, z2, x);
//  printf("z-coefficient: %d\r\n", tz);
}


#define COORD_ERR_RANGE 10
/**
 * @function   Read touch screen coordinates twice in a row,
 *             an acceptable deviation of these two coordinates
 *             can not exceed the ERR_RANGE
 * @parameters x Read x coordinate of the touch screen
 *             y Read y coordinate of the touch screen
 * @retvalue   0 - fail, 1 - success
 */
uint8_t TS_Read_XY(uint16_t *x, uint16_t *y)
{
  uint16_t x1, y1, x2, y2;
  x1 = TS_GetX();
  y1 = TS_GetY();
  x2 = TS_GetX();
  y2 = TS_GetY();
  if (((x2 <= x1 && x1 < x2 + ERR_RANGE) || (x1 <= x2 && x2 < x1 + COORD_ERR_RANGE))
      && ((y2 <= y1 && y1 < y2 + ERR_RANGE) || (y1 <= y2 && y2 < y1 + COORD_ERR_RANGE))) {
      *x = (x1 + x2) / 2;
      *y = (y1 + y2) / 2;
      return 1;
  } else {
      return 0;
  }
}


/**
 * Return z coefficient from latest touch update
 */
void TS_GetZ(uint16_t *z)
{
  *z = tz;
}


/**
 * Return raw XY values from latest touch update
 */
void TS_GetXY(uint16_t *x, uint16_t *y)
{
  *x = tx;
  *y = ty;
}


/**
 * Update detected touch
 */
uint8_t TS_Update()
{
  return TS_Read_XY(&tx, &ty);
}


/**
 * Touch detected when touch interrupt pin `TP_IRQ` active
 *
 */
uint8_t TS_IO_DetectTouch(void)
{
  uint8_t ret = 0;
  if (GPIOX_IDR(TP_IRQ)) {
    return ret;
  }

#if UPDATE_Z_PRESSURE
  TS_Update_Z();
#endif
  // User code - add pressure-based threshold touch?
  // ...
  // if (tz > Z_THRESHOLD) ret = 1
  //
  ret = 1;
  return ret;
}
