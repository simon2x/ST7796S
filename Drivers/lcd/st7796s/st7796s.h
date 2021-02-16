/* Interface mode
   - 1: SPI or parallel interface mode
   - 2: RGB mode (LTDC hardware, HSYNC, VSYNC, pixel clock, RGB bits data, framebuffer memory)
*/
#define  ST7796S_INTERFACE_MODE   1

/* Orientation:
   - 0: 320x480 8pin connecting at the top (portrait)
   - 1: 480x320 8pin connecting left (landscape)
   - 2: 320x480 8pin connecting below (portrait)
   - 3: 480x320 8pin connecting to the right (landscape)
*/
#define  ST7796S_ORIENTATION       0

/* Color mode
   - 0: RGB565 (R:bit15..11, G:bit10..5, B:bit4..0)
   - 1: BRG565 (B:bit15..11, G:bit10..5, R:bit4..0)
*/
#define  ST7796S_COLORMODE         0

/* To clear the screen before display turning on ?
   - 0: does not clear
   - 1: clear
*/
#define  ST7796S_INITCLEAR        1

/* Analog touchscreen (only INTERFACE_MODE == 1, 8bit parallel IO mode)
   - 0: touchscreen disabled
   - 1: touchscreen enabled
*/
#define  ST7796S_TOUCH            1

/* Touchscreen calibration data for 4 orientations */
#define  TS_CINDEX_0        {-76088, -83042, 151, 964366, 240, -84004, 1327176}
#define  TS_CINDEX_1        {-76088, 240, -84004, 1327176, 83042, -151, -25236438}
#define  TS_CINDEX_2        {-76088, 83042, -151, -25236438, -240, 84004, -37773328}
#define  TS_CINDEX_3        {-76088, -240, 84004, -37773328, -83042, 151, 964366}


/* For multi-threaded or intermittent use, Lcd and Touchscreen simultaneous use can cause confusion (since it uses common I/O resources)
   Lcd functions wait for the touchscreen header, the touchscreen query is not executed when Lcd is busy.
   Note: If the priority of the Lcd is higher than that of the Touchscreen, it may end up in an infinite loop!
   - 0: multi-threaded protection disabled
   - 1: multi-threaded protection enabled
*/
#define ST7796S_MULTITASK_MUTEX 0

#if ST7796S_INTERFACE_MODE == 2

/* please see in the main.c what is the LTDC_HandleTypeDef name */
extern LTDC_HandleTypeDef hltdc;

/* Frambuffer memory alloc, free */
#define  ST7796S_MALLOC           malloc
#define  ST7796S_FREE             free

/* include for memory alloc/free */
#include <stdlib.h>

#endif  /* #if ST7796S_INTERFACE_MODE == 2 */

//-----------------------------------------------------------------------------
// ST7796S physical resolution (in 0 orientation)
#define  ST7796S_LCD_PIXEL_WIDTH   320
#define  ST7796S_LCD_PIXEL_HEIGHT  480
