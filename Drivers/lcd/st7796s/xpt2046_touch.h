/*
 * xpt2046_touch.h
 *
 * Touchscreen SPI driver
 *
 *  Created on: Nov 30, 2020
 *      Author: Simon Wu
 */

// Note: Use SPI bus with < 2.5 Mbit speed, better ~650 Kbit to be safe.

#ifndef SRC_TOUCH_XPT2046_TOUCH_H_
#define SRC_TOUCH_XPT2046_TOUCH_H_

#include "main.h"
#include <stdbool.h>

//void TS_GetXY(uint16_t* x, uint16_t* y);

/*
 * SPI select
 * 0: software SPI driver (the pins assign are full free)
 * 1..6: hardware SPI driver (the TOUCH_SCK, TOUCH_MOSI, TOUCH_MISO pins are locked to hardware)
 */
#define TOUCH_SPI           0

/*
 * SPI duplex mode
 * 0: full duplex (SPI TX: TOUCH_MOSI, SPI RX: TOUCH_MISO)
 * 1: half duplex (TOUCH_MOSI is bidirectional pin, TOUCH_MISO is not used)
 */
#define TOUCH_SPI_MODE      0

/*
 * SPI write and read speeds
 *
 * Software SPI
 * - 0:    none
 * - 1:    NOP
 * - 2:    clock pin keeping
 * - 3..:  TOUCH_IO_Delay(TOUCH_SPI_SPD_WRITE - 3)
 *
 * Hardware SPI clock div fPCLK
 * - 0=/2, 1=/4, 2=/8, 3=/16, 4=/32, 5=/64, 6=/128, 7=/256
 *
 */
#define TOUCH_SPI_SPD_WRITE 0
#define TOUCH_SPI_SPD_READ  0

/* SPI pins alternative function assign (0..15), (Hardware SPI only) */
#define TOUCH_SPI_AFR       0

/*
 * LCD control pins assign (A..K, 0..15)
 * If hardware SPI: SCK, MOSI, MISO pin assignment is locked to hardware
 */
#define TP_IRQ             X, 0
#define TOUCH_CS           X, 0
#define TOUCH_CLK          X, 0
#define TOUCH_MOSI         X, 0
#define TOUCH_MISO         X, 0  /* Set as `X,0` if unused  */


/*
 * DMA settings (Hardware SPI only)
 * - 0..2: 0 = no DMA, 1 = DMA1, 2 = DMA2
 * - 0..7: DMA channel (DMA request mapping)
 * - 0..7: Stream (DMA request mapping)
 * - 1..3: DMA priority (0=low..3=very high)
 * */
#define TOUCH_DMA_TX        0, 0, 0, 0
#define TOUCH_DMA_RX        0, 0, 0, 0

/*
 * DMA interrupt priority
 * See NVIC_SetPriority function (default=15)
 */
#define TOUCH_DMA_IRQ_PR    15

/*
 * In DMA mode the bitmap drawing function is completed before the actual drawing.
 * If the content of the image changes (because it is in a stack), the drawing will be corrupted.
 * If you want to wait for the drawing operation to complete, set it here.
 * This will slow down the program, but will not cause a bad drawing.
 * When drawing a non-bitmap (example: FillRect), you do not wait for the end of the drawing
 * because it stores the drawing color in a static variable.
 * The "TOUCH_IO_DmaTransferStatus" variable is content this status (if 0 -> DMA transfers are completed)
   - 0: bitmap and fill drawing function end wait off
   - 1: bitmap drawing function end wait on, fill drawing function end wait off (default mode)
   - 2: bitmap and fill drawing function end wait on
 */
#define TOUCH_DMA_TXWAIT    1

/* Because there are DMA capable and DMA unable memory regions
 * here we can set what is the DMA unable region condition
 * note: where the condition is true, it is considered a DMA-unable region
 * If you delete this definition: all memory are DMA capable */
#define TOUCH_DMA_UNABLE(addr)  addr >= 0x10000000 && addr < 0x20000000

/* DMA RX buffer [byte] (only in ...24to16 function) */
#define TOUCH_DMA_RX_BUFSIZE  256

/*
 * DMA RX buffer place (only in ...24to16 function)
 * - 0: stack
 * - 1: static buffer
 * - 2: memory manager (malloc/free)
 */
#define TOUCH_DMA_RX_BUFMODE  1

/*
 * If TOUCH_DMA_RX_BUFMODE == 2 : memory management functions name
 */
#define TOUCH_DMA_RX_MALLOC   malloc
#define TOUCH_DMA_RX_FREE     free

/*
 * Updates pressure reading when TP_IRQ is active
 * - 0: disabled
 * - 1: enabled
 */
#define UPDATE_Z_PRESSURE 0


/* Include for malloc/free functions */
#include <stdlib.h>

#endif
