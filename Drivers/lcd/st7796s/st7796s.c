/*
 * ST7796S LCD driver (optional built-in touchscreen driver)
 * 2020.11
*/

#include <string.h>
#include "main.h"
#include "lcd.h"
#include "bmp.h"
#include "st7796s.h"

#if  ST7796S_TOUCH == 1
#include "ts.h"
#endif

/* Lcd */
void     st7796s_Init(void);
uint16_t st7796s_ReadID(void);
void     st7796s_DisplayOn(void);
void     st7796s_DisplayOff(void);
void     st7796s_SetCursor(uint16_t Xpos, uint16_t Ypos);
void     st7796s_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGB_Code);
uint16_t st7796s_ReadPixel(uint16_t Xpos, uint16_t Ypos);
void     st7796s_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height);
void     st7796s_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     st7796s_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length);
void     st7796s_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode);
uint16_t st7796s_GetLcdPixelWidth(void);
uint16_t st7796s_GetLcdPixelHeight(void);
void     st7796s_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp);
void     st7796s_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t *pData);
void     st7796s_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t *pData);
void     st7796s_Scroll(int16_t Scroll, uint16_t TopFix, uint16_t BottonFix);

/* Touchscreen */
void     st7796s_ts_Init(uint16_t DeviceAddr);
uint8_t  st7796s_ts_DetectTouch(uint16_t DeviceAddr);
void     st7796s_ts_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y);

LCD_DrvTypeDef   st7796s_drv =
{
  st7796s_Init,
  st7796s_ReadID,
  st7796s_DisplayOn,
  st7796s_DisplayOff,
  st7796s_SetCursor,
  st7796s_WritePixel,
  st7796s_ReadPixel,
  st7796s_SetDisplayWindow,
  st7796s_DrawHLine,
  st7796s_DrawVLine,
  st7796s_GetLcdPixelWidth,
  st7796s_GetLcdPixelHeight,
  st7796s_DrawBitmap,
  st7796s_DrawRGBImage,
  st7796s_FillRect,
  st7796s_ReadRGBImage,
  st7796s_Scroll,
};

LCD_DrvTypeDef  *lcd_drv = &st7796s_drv;

#define ST7796S_NOP            0x00
#define ST7796S_SWRESET        0x01

#define ST7796S_RDDID          0x04
#define ST7796S_RDDST          0x09
#define ST7796S_RDMODE         0x0A
#define ST7796S_RDMADCTL       0x0B
#define ST7796S_RDPIXFMT       0x0C
#define ST7796S_RDIMGFMT       0x0D
#define ST7796S_RDSELFDIAG     0x0F

#define ST7796S_SLPIN          0x10
#define ST7796S_SLPOUT         0x11
#define ST7796S_PTLON          0x12
#define ST7796S_NORON          0x13

#define ST7796S_INVOFF         0x20
#define ST7796S_INVON          0x21
//#define ST7796S_GAMMASET       0x26
#define ST7796S_DISPOFF        0x28
#define ST7796S_DISPON         0x29

#define ST7796S_CASET          0x2A
#define ST7796S_PASET          0x2B
#define ST7796S_RAMWR          0x2C
#define ST7796S_RAMRD          0x2E

#define ST7796S_PTLAR          0x30
#define ST7796S_VSCRDEF        0x33
#define ST7796S_MADCTL         0x36
#define ST7796S_VSCRSADD       0x37     /* Vertical Scrolling Start Address */
#define ST7796S_PIXFMT         0x3A     /* COLMOD: Pixel Format Set */

#define ST7796S_RGB_INTERFACE  0xB0     /* RGB Interface Signal Control */
#define ST7796S_FRMCTR1        0xB1
#define ST7796S_FRMCTR2        0xB2
#define ST7796S_FRMCTR3        0xB3
#define ST7796S_INVCTR         0xB4
#define ST7796S_DFUNCTR        0xB6     /* Display Function Control */

#define ST7796S_PWCTR1         0xC0
#define ST7796S_PWCTR2         0xC1
#define ST7796S_PWCTR3         0xC2
#define ST7796S_PWCTR4         0xC3
#define ST7796S_PWCTR5         0xC4
#define ST7796S_VMCTR1         0xC5

#define ST7796S_RDID1          0xDA
#define ST7796S_RDID2          0xDB
#define ST7796S_RDID3          0xDC
#define ST7796S_RDID4          0xDD

#define ST7796S_GMCTRP1        0xE0
#define ST7796S_GMCTRN1        0xE1
#define ST7796S_DGCTR1         0xE2
#define ST7796S_DGCTR2         0xE3

//-----------------------------------------------------------------------------
#define ST7796S_MAD_RGB        0x08
#define ST7796S_MAD_BGR        0x00

#define ST7796S_MAD_VERTICAL   0x20
#define ST7796S_MAD_X_LEFT     0x00
#define ST7796S_MAD_X_RIGHT    0x40
#define ST7796S_MAD_Y_UP       0x80
#define ST7796S_MAD_Y_DOWN     0x00

#if ST7796S_COLORMODE == 0
#define ST7796S_MAD_COLORMODE  ST7796S_MAD_RGB
#else
#define ST7796S_MAD_COLORMODE  ST7796S_MAD_BGR
#endif

#if (ST7796S_ORIENTATION == 0)
#define ST7796S_SIZE_X                     ST7796S_LCD_PIXEL_WIDTH
#define ST7796S_SIZE_Y                     ST7796S_LCD_PIXEL_HEIGHT
#define ST7796S_MAD_DATA_RIGHT_THEN_UP     ST7796S_MAD_COLORMODE | ST7796S_MAD_X_RIGHT | ST7796S_MAD_Y_UP
#define ST7796S_MAD_DATA_RIGHT_THEN_DOWN   ST7796S_MAD_COLORMODE | ST7796S_MAD_X_RIGHT | ST7796S_MAD_Y_DOWN
#define ST7796S_MAD_DATA_RGBMODE           ST7796S_MAD_COLORMODE | ST7796S_MAD_X_LEFT  | ST7796S_MAD_Y_DOWN
#elif (ST7796S_ORIENTATION == 1)
#define ST7796S_SIZE_X                     ST7796S_LCD_PIXEL_HEIGHT
#define ST7796S_SIZE_Y                     ST7796S_LCD_PIXEL_WIDTH
#define ST7796S_MAD_DATA_RIGHT_THEN_UP     ST7796S_MAD_COLORMODE | ST7796S_MAD_X_RIGHT | ST7796S_MAD_Y_DOWN | ST7796S_MAD_VERTICAL
#define ST7796S_MAD_DATA_RIGHT_THEN_DOWN   ST7796S_MAD_COLORMODE | ST7796S_MAD_X_LEFT  | ST7796S_MAD_Y_DOWN | ST7796S_MAD_VERTICAL
#define ST7796S_MAD_DATA_RGBMODE           ST7796S_MAD_COLORMODE | ST7796S_MAD_X_RIGHT | ST7796S_MAD_Y_DOWN
#elif (ST7796S_ORIENTATION == 2)
#define ST7796S_SIZE_X                     ST7796S_LCD_PIXEL_WIDTH
#define ST7796S_SIZE_Y                     ST7796S_LCD_PIXEL_HEIGHT
#define ST7796S_MAD_DATA_RIGHT_THEN_UP     ST7796S_MAD_COLORMODE | ST7796S_MAD_X_LEFT  | ST7796S_MAD_Y_DOWN
#define ST7796S_MAD_DATA_RIGHT_THEN_DOWN   ST7796S_MAD_COLORMODE | ST7796S_MAD_X_LEFT  | ST7796S_MAD_Y_UP
#define ST7796S_MAD_DATA_RGBMODE           ST7796S_MAD_COLORMODE | ST7796S_MAD_X_RIGHT | ST7796S_MAD_Y_UP
#elif (ST7796S_ORIENTATION == 3)
#define ST7796S_SIZE_X                     ST7796S_LCD_PIXEL_HEIGHT
#define ST7796S_SIZE_Y                     ST7796S_LCD_PIXEL_WIDTH
#define ST7796S_MAD_DATA_RIGHT_THEN_UP     ST7796S_MAD_COLORMODE | ST7796S_MAD_X_LEFT  | ST7796S_MAD_Y_UP   | ST7796S_MAD_VERTICAL
#define ST7796S_MAD_DATA_RIGHT_THEN_DOWN   ST7796S_MAD_COLORMODE | ST7796S_MAD_X_RIGHT | ST7796S_MAD_Y_UP   | ST7796S_MAD_VERTICAL
#define ST7796S_MAD_DATA_RGBMODE           ST7796S_MAD_COLORMODE | ST7796S_MAD_X_LEFT  | ST7796S_MAD_Y_UP
#endif

#define ST7796S_SETCURSOR(x, y)            {LCD_IO_WriteCmd8(ST7796S_CASET); LCD_IO_WriteData16_to_2x8(x); LCD_IO_WriteData16_to_2x8(x); \
                                            LCD_IO_WriteCmd8(ST7796S_PASET); LCD_IO_WriteData16_to_2x8(y); LCD_IO_WriteData16_to_2x8(y); }

//-----------------------------------------------------------------------------
#define ST7796S_LCD_INITIALIZED      0x01
#define ST7796S_LCD_IO_INITIALIZED   0x02
#define ST7796S_TS_IO_INITIALIZED    0x04
static uint8_t Is_st7796s_Initialized = 0;

#if      ST7796S_MULTITASK_MUTEX == 1 && ST7796S_TOUCH == 1
volatile uint8_t io_lcd_busy = 0;
volatile uint8_t io_ts_busy = 0;
#define  ST7796S_LCDMUTEX_PUSH()    while(io_ts_busy); io_lcd_busy++;
#define  ST7796S_LCDMUTEX_POP()     io_lcd_busy--
#else
#define  ST7796S_LCDMUTEX_PUSH()
#define  ST7796S_LCDMUTEX_POP()
#endif

//-----------------------------------------------------------------------------
#if ST7796S_TOUCH == 1

// Touch parameters
#define TOUCHMINPRESSRC    8192
#define TOUCHMAXPRESSRC    4096
#define TOUCHMINPRESTRG       0
#define TOUCHMAXPRESTRG     255
#define TOUCH_FILTER         8 // 16?

// fixpoints Z indexes (16bit integer, 16bit fractional)
#define ZINDEXA  ((65536 * (TOUCHMAXPRESTRG - TOUCHMINPRESTRG)) / (TOUCHMAXPRESSRC - TOUCHMINPRESSRC))
#define ZINDEXB  (-ZINDEXA * TOUCHMINPRESSRC)

#define ABS(N)   (((N)<0) ? (-(N)) : (N))

TS_DrvTypeDef   st7796s_ts_drv =
{
  st7796s_ts_Init,
  0,
  0,
  0,
  st7796s_ts_DetectTouch,
  st7796s_ts_GetXY,
  0,
  0,
  0,
  0,
};

TS_DrvTypeDef *ts_drv = &st7796s_ts_drv;

#if (ST7796S_ORIENTATION == 0)
int32_t  ts_cindex[] = TS_CINDEX_0;
#elif (ST7796S_ORIENTATION == 1)
int32_t  ts_cindex[] = TS_CINDEX_1;
#elif (ST7796S_ORIENTATION == 2)
int32_t  ts_cindex[] = TS_CINDEX_2;
#elif (ST7796S_ORIENTATION == 3)
int32_t  ts_cindex[] = TS_CINDEX_3;
#endif

uint16_t tx, ty;

/* Link function for Touchscreen */
uint8_t   TS_IO_DetectTouch(void);
//uint16_t  TS_IO_GetX(void);
//uint16_t  TS_IO_GetY(void);
//uint16_t  TS_IO_GetZ1(void);
//uint16_t  TS_IO_GetZ2(void);

#endif   /* #if ST7796S_TOUCH == 1 */

//-----------------------------------------------------------------------------
/* Link functions for LCD IO peripheral */
void     LCD_Delay (uint32_t delay);
void     LCD_IO_Init(void);
void     LCD_IO_Bl_OnOff(uint8_t Bl);

void     LCD_IO_WriteCmd8(uint8_t Cmd);
//void     LCD_IO_WriteCmd16(uint16_t Cmd);
void     LCD_IO_WriteData8(uint8_t Data);
void     LCD_IO_WriteData16(uint16_t Data);

void     LCD_IO_WriteCmd8DataFill16(uint8_t Cmd, uint16_t Data, uint32_t Size);
void     LCD_IO_WriteCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size);
void     LCD_IO_WriteCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size);
void     LCD_IO_ReadCmd8MultipleData8(uint8_t Cmd, uint8_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd8MultipleData16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);
void     LCD_IO_ReadCmd8MultipleData24to16(uint8_t Cmd, uint16_t *pData, uint32_t Size, uint32_t DummySize);

#define  LCD_IO_WriteData16_to_2x8(dt)    {LCD_IO_WriteData8((dt) >> 8); LCD_IO_WriteData8(dt); }

//-----------------------------------------------------------------------------

static  uint16_t  yStart, yEnd;

//-----------------------------------------------------------------------------
/**
  * @brief  Enables the Display.
  * @param  None
  * @retval None
  */
void st7796s_DisplayOn(void)
{
  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ST7796S_SLPOUT);    // Exit Sleep
  ST7796S_LCDMUTEX_POP();
  LCD_IO_Bl_OnOff(1);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
void st7796s_DisplayOff(void)
{
  LCD_IO_Bl_OnOff(0);
  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ST7796S_SLPIN);    // Sleep
  ST7796S_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Width.
  * @param  None
  * @retval The Lcd Pixel Width
  */
uint16_t st7796s_GetLcdPixelWidth(void)
{
  return ST7796S_SIZE_X;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the LCD pixel Height.
  * @param  None
  * @retval The Lcd Pixel Height
  */
uint16_t st7796s_GetLcdPixelHeight(void)
{
  return ST7796S_SIZE_Y;
}

//-----------------------------------------------------------------------------
/**
  * @brief  Get the ILI9486 ID.
  * @param  None
  * @retval The ILI9486 ID
  */
uint16_t st7796s_ReadID(void)
{
  uint32_t id = 0;
  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_ReadCmd8MultipleData8(0xD3, (uint8_t *)&id, 3, 1);
  ST7796S_LCDMUTEX_POP();
  if(id == 0x869400)
    return 0x9486;
  else
    return 0;
}

//-----------------------------------------------------------------------------
/**
  * @brief  ST7796S initialization
  * @param  None
  * @retval None
  */
void st7796s_Init(void)
{
  if((Is_st7796s_Initialized & ST7796S_LCD_INITIALIZED) == 0)
  {
    Is_st7796s_Initialized |= ST7796S_LCD_INITIALIZED;
    if((Is_st7796s_Initialized & ST7796S_LCD_IO_INITIALIZED) == 0)
      LCD_IO_Init();
    Is_st7796s_Initialized |= ST7796S_LCD_IO_INITIALIZED;
  }
  LCD_Delay(1);
  LCD_IO_WriteCmd8(ST7796S_SWRESET);
  LCD_Delay(5);

  // Command Set Control - enable command 2 part I
  LCD_IO_WriteCmd8MultipleData8(0xF0, (uint8_t *)"\xC3", 1);
  // Command Set Control - enable command 2 part II
  LCD_IO_WriteCmd8MultipleData8(0xF0, (uint8_t *)"\x96", 1);

  LCD_IO_WriteCmd8MultipleData8(ST7796S_MADCTL, (uint8_t *)"\x68", 1);
  LCD_IO_WriteCmd8MultipleData8(ST7796S_PIXFMT, (uint8_t *) "\x05", 1);

  // Interface Mode Control
  LCD_IO_WriteCmd8MultipleData8(0xB0, (uint8_t *) "\x80", 1);
  // Display Function Control
  LCD_IO_WriteCmd8MultipleData8(0xB6, (uint8_t *) "\x20\x02", 2);
  // Blanking Porch Control
  LCD_IO_WriteCmd8MultipleData8(0xB5, (uint8_t *) "\x02\x03\x00\x04", 4);

  // Frame Control
  LCD_IO_WriteCmd8MultipleData8(ST7796S_FRMCTR1, (uint8_t *)"\x80\x10", 2);

  // Display Inversion Control
  LCD_IO_WriteCmd8MultipleData8(ST7796S_INVCTR, (uint8_t *)"\x00", 1);

  // Entry Mode Set
  LCD_IO_WriteCmd8MultipleData8(0xB7, (uint8_t *)"\xC6", 1);

  // VCOM Control
  LCD_IO_WriteCmd8MultipleData8(ST7796S_VMCTR1, (uint8_t *)"\x24", 1);

  // VCOM Control
  LCD_IO_WriteCmd8MultipleData8(0xE4, (uint8_t *)"\x31", 1);

  // Display Output Ctrl Adjust
  LCD_IO_WriteCmd8MultipleData8(0xE8, (uint8_t *) "\x40\x8A\x00\x00\x29\x19\xA5\x33", 8);

  // Power Control 3
  LCD_IO_WriteCmd8MultipleData8(ST7796S_PWCTR3, (uint8_t *)"\xA7", 1);

  // Positive gamma control
  LCD_IO_WriteCmd8MultipleData8(ST7796S_GMCTRP1,
		  (uint8_t *)"\xF0\x09\x13\x12\x12\x2B\x3C\x44\x4B\x1B\x18\x17\x1D\x21", 14);

  // Negative gamma control
  LCD_IO_WriteCmd8MultipleData8(ST7796S_GMCTRN1,
		  (uint8_t *)"\xF0\x09\x13\x0C\x0D\x27\x3B\x44\x4D\x0B\x17\x17\x1D\x21", 14);

  // LCD_IO_WriteCmd8MultipleData8(ST7796S_RGB_INTERFACE, (uint8_t *)"\x00", 1); // RGB mode off (0xB0)

  LCD_IO_WriteCmd8(ST7796S_MADCTL);
  LCD_IO_WriteData8(ST7796S_MAD_DATA_RIGHT_THEN_DOWN);

  // Command Set Control - enable command 2 part I
  LCD_IO_WriteCmd8MultipleData8(0xF0, (uint8_t *)"\xC3", 1);
  // Command Set Control - disable command 2 part II
  LCD_IO_WriteCmd8MultipleData8(0xF0, (uint8_t *)"\x69", 1);

  // Normal display on
  LCD_IO_WriteCmd8(ST7796S_NORON);
  // Display inversion off
  LCD_IO_WriteCmd8(ST7796S_INVOFF);
  // Exit Sleep
  LCD_IO_WriteCmd8(ST7796S_SLPOUT);
  LCD_Delay(200);
  // Display on
  LCD_IO_WriteCmd8(ST7796S_DISPON);
  LCD_Delay(10);
  st7796s_FillRect(0, 0, ST7796S_SIZE_X, ST7796S_SIZE_Y, 0x0000);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Set Cursor position.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @retval None
  */
void st7796s_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
  ST7796S_LCDMUTEX_PUSH();
  ST7796S_SETCURSOR(Xpos, Ypos);
  ST7796S_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Write pixel.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @param  RGBCode: the RGB pixel color
  * @retval None
  */
void st7796s_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode)
{
  ST7796S_LCDMUTEX_PUSH();
  ST7796S_SETCURSOR(Xpos, Ypos);
  LCD_IO_WriteCmd8(ST7796S_RAMWR);
  LCD_IO_WriteData16(RGBCode);
  ST7796S_LCDMUTEX_POP();
}


//-----------------------------------------------------------------------------
/**
  * @brief  Read pixel.
  * @param  None
  * @retval the RGB pixel color
  */
uint16_t st7796s_ReadPixel(uint16_t Xpos, uint16_t Ypos)
{
  uint16_t ret;
  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8MultipleData8(ST7796S_PIXFMT, (uint8_t *)"\x66", 1); // Read: only 24bit pixel mode
  ST7796S_SETCURSOR(Xpos, Ypos);
  LCD_IO_ReadCmd8MultipleData24to16(ST7796S_RAMRD, (uint16_t *)&ret, 1, 1);
  LCD_IO_WriteCmd8MultipleData8(ST7796S_PIXFMT, (uint8_t *)"\x55", 1); // Return to 16bit pixel mode
  ST7796S_LCDMUTEX_POP();
  return(ret);
}

//-----------------------------------------------------------------------------
/**
  * @brief  Sets a display window
  * @param  Xpos:   specifies the X bottom left position.
  * @param  Ypos:   specifies the Y bottom left position.
  * @param  Height: display window height.
  * @param  Width:  display window width.
  * @retval None
  */
void st7796s_SetDisplayWindow(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  yStart = Ypos; yEnd = Ypos + Height - 1;
  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ST7796S_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Width - 1);
  LCD_IO_WriteCmd8(ST7796S_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Height - 1);
  ST7796S_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Draw vertical line.
  * @param  RGBCode: Specifies the RGB color
  * @param  Xpos:     specifies the X position.
  * @param  Ypos:     specifies the Y position.
  * @param  Length:   specifies the Line length.
  * @retval None
  */
void st7796s_DrawHLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ST7796S_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Length - 1);
  LCD_IO_WriteCmd8(ST7796S_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos);
  LCD_IO_WriteCmd8DataFill16(ST7796S_RAMWR, RGBCode, Length);
  ST7796S_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Draw vertical line.
  * @param  RGBCode: Specifies the RGB color
  * @param  Xpos:     specifies the X position.
  * @param  Ypos:     specifies the Y position.
  * @param  Length:   specifies the Line length.
  * @retval None
  */
void st7796s_DrawVLine(uint16_t RGBCode, uint16_t Xpos, uint16_t Ypos, uint16_t Length)
{
  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ST7796S_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos);
  LCD_IO_WriteCmd8(ST7796S_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Length - 1);
  LCD_IO_WriteCmd8DataFill16(ST7796S_RAMWR, RGBCode, Length);
  ST7796S_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Draw Filled rectangle
  * @param  Xpos:     specifies the X position.
  * @param  Ypos:     specifies the Y position.
  * @param  Xsize:    specifies the X size
  * @param  Ysize:    specifies the Y size
  * @param  RGBCode:  specifies the RGB color
  * @retval None
  */
void st7796s_FillRect(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t RGBCode)
{
  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ST7796S_CASET); LCD_IO_WriteData16_to_2x8(Xpos); LCD_IO_WriteData16_to_2x8(Xpos + Xsize - 1);
  LCD_IO_WriteCmd8(ST7796S_PASET); LCD_IO_WriteData16_to_2x8(Ypos); LCD_IO_WriteData16_to_2x8(Ypos + Ysize - 1);
  LCD_IO_WriteCmd8DataFill16(ST7796S_RAMWR, RGBCode, Xsize * Ysize);
  ST7796S_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Displays a 16bit bitmap picture..
  * @param  BmpAddress: Bmp picture address.
  * @param  Xpos:  Bmp X position in the LCD
  * @param  Ypos:  Bmp Y position in the LCD
  * @retval None
  * @brief  Draw direction: right then up
  */
void st7796s_DrawBitmap(uint16_t Xpos, uint16_t Ypos, uint8_t *pbmp)
{
  uint32_t index, size;
  /* Read bitmap size */
  size = ((BITMAPSTRUCT *)pbmp)->fileHeader.bfSize;
  /* Get bitmap data address offset */
  index = ((BITMAPSTRUCT *)pbmp)->fileHeader.bfOffBits;
  size = (size - index) / 2;
  pbmp += index;

  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8(ST7796S_MADCTL); LCD_IO_WriteData8(ST7796S_MAD_DATA_RIGHT_THEN_UP);
  LCD_IO_WriteCmd8(ST7796S_PASET); LCD_IO_WriteData16_to_2x8(ST7796S_SIZE_Y - 1 - yEnd); LCD_IO_WriteData16_to_2x8(ST7796S_SIZE_Y - 1 - yStart);
  LCD_IO_WriteCmd8MultipleData16(ST7796S_RAMWR, (uint16_t *)pbmp, size);
  LCD_IO_WriteCmd8(ST7796S_MADCTL); LCD_IO_WriteData8(ST7796S_MAD_DATA_RIGHT_THEN_DOWN);
  ST7796S_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Displays 16bit/pixel picture..
  * @param  pdata: picture address.
  * @param  Xpos:  Image X position in the LCD
  * @param  Ypos:  Image Y position in the LCD
  * @param  Xsize: Image X size in the LCD
  * @param  Ysize: Image Y size in the LCD
  * @retval None
  * @brief  Draw direction: right then down
  */
void st7796s_DrawRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t *pData)
{
  st7796s_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8MultipleData16(ST7796S_RAMWR, pData, Xsize * Ysize);
  ST7796S_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Read 16bit/pixel picture from Lcd and store to RAM
  * @param  pdata: picture address.
  * @param  Xpos:  Image X position in the LCD
  * @param  Ypos:  Image Y position in the LCD
  * @param  Xsize: Image X size in the LCD
  * @param  Ysize: Image Y size in the LCD
  * @retval None
  * @brief  Draw direction: right then down
  */
void st7796s_ReadRGBImage(uint16_t Xpos, uint16_t Ypos, uint16_t Xsize, uint16_t Ysize, uint16_t *pData)
{
  st7796s_SetDisplayWindow(Xpos, Ypos, Xsize, Ysize);
  ST7796S_LCDMUTEX_PUSH();
  LCD_IO_WriteCmd8MultipleData8(ST7796S_PIXFMT, (uint8_t *)"\x66", 1); // Read: only 24bit pixel mode
  LCD_IO_ReadCmd8MultipleData24to16(ST7796S_RAMRD, pData, Xsize * Ysize, 1);
  LCD_IO_WriteCmd8MultipleData8(ST7796S_PIXFMT, (uint8_t *)"\x55", 1); // Return to 16bit pixel mode
  ST7796S_LCDMUTEX_POP();
}

//-----------------------------------------------------------------------------
/**
  * @brief  Set display scroll parameters
  * @param  Scroll    : Scroll size [pixel]
  * @param  TopFix    : Top fix size [pixel]
  * @param  BottonFix : Botton fix size [pixel]
  * @retval None
  */
void st7796s_Scroll(int16_t Scroll, uint16_t TopFix, uint16_t BottonFix)
{
  static uint16_t scrparam[4] = {0, 0, 0, 0};
  ST7796S_LCDMUTEX_PUSH();
  #if (ST7796S_ORIENTATION == 0)
  if((TopFix != scrparam[1]) || (BottonFix != scrparam[3]))
  {
    scrparam[1] = TopFix;
    scrparam[3] = BottonFix;
    scrparam[2] = ST7796S_LCD_PIXEL_HEIGHT - TopFix - BottonFix;
    LCD_IO_WriteCmd8MultipleData16(ST7796S_VSCRDEF, &scrparam[1], 3);
  }
  Scroll = (0 - Scroll) % scrparam[2];
  if(Scroll < 0)
    Scroll = scrparam[2] + Scroll + scrparam[1];
  else
    Scroll = Scroll + scrparam[1];
  #elif (ST7796S_ORIENTATION == 1)
  if((TopFix != scrparam[1]) || (BottonFix != scrparam[3]))
  {
    scrparam[1] = TopFix;
    scrparam[3] = BottonFix;
    scrparam[2] = ST7796S_LCD_PIXEL_HEIGHT - TopFix - BottonFix;
    LCD_IO_WriteCmd8MultipleData16(ST7796S_VSCRDEF, &scrparam[1], 3);
  }
  Scroll = (0 - Scroll) % scrparam[2];
  if(Scroll < 0)
    Scroll = scrparam[2] + Scroll + scrparam[1];
  else
    Scroll = Scroll + scrparam[1];
  #elif (ST7796S_ORIENTATION == 2)
  if((TopFix != scrparam[3]) || (BottonFix != scrparam[1]))
  {
    scrparam[3] = TopFix;
    scrparam[1] = BottonFix;
    scrparam[2] = ST7796S_LCD_PIXEL_HEIGHT - TopFix - BottonFix;
    LCD_IO_WriteCmd8MultipleData16(ST7796S_VSCRDEF, &scrparam[1], 3);
  }
  Scroll %= scrparam[2];
  if(Scroll < 0)
    Scroll = scrparam[2] + Scroll + scrparam[1];
  else
    Scroll = Scroll + scrparam[1];
  #elif (ST7796S_ORIENTATION == 3)
  if((TopFix != scrparam[3]) || (BottonFix != scrparam[1]))
  {
    scrparam[3] = TopFix;
    scrparam[1] = BottonFix;
    scrparam[2] = ST7796S_LCD_PIXEL_HEIGHT - TopFix - BottonFix;
    LCD_IO_WriteCmd8MultipleData16(ST7796S_VSCRDEF, &scrparam[1], 3);
  }
  Scroll %= scrparam[2];
  if(Scroll < 0)
    Scroll = scrparam[2] + Scroll + scrparam[1];
  else
    Scroll = Scroll + scrparam[1];
  #endif
  if(Scroll != scrparam[0])
  {
    scrparam[0] = Scroll;
    LCD_IO_WriteCmd8DataFill16(ST7796S_VSCRSADD, scrparam[0], 1);
  }
  ST7796S_LCDMUTEX_POP();
}


#if ST7796S_TOUCH == 1
void st7796s_ts_Init(uint16_t DeviceAddr)
{
//  if((Is_st7796s_Initialized & ST7796S_LCD_IO_INITIALIZED) == 0) {
//    LCD_IO_Init();
//    Is_st7796s_Initialized |= ST7796S_LCD_IO_INITIALIZED;
//  }

  if((Is_st7796s_Initialized & ST7796S_TS_IO_INITIALIZED) == 0) {
    TOUCH_IO_Init();
    Is_st7796s_Initialized |= ST7796S_TS_IO_INITIALIZED;
  }
}


uint8_t st7796s_ts_DetectTouch(uint16_t DeviceAddr)
{
  static uint8_t tp = 0;

  #if  ST7796S_MULTITASK_MUTEX == 1
  io_ts_busy = 1;

  if (io_lcd_busy) {
    io_ts_busy = 0;
    return tp;
  }
  #endif

  if (TS_IO_DetectTouch()) {
    tp = TS_Update();
  } else {
    tp = 0;
  }

  #if  ST7796S_MULTITASK_MUTEX == 1
  io_ts_busy = 0;
  #endif

  return tp;
}

//-----------------------------------------------------------------------------
void st7796s_ts_GetXY(uint16_t DeviceAddr, uint16_t *X, uint16_t *Y)
{
  TS_GetXY(X, Y);
  switch (ST7796S_ORIENTATION) {
      case (0):
      case (2):
          *X = (*X * ST7796S_SIZE_X) / 4095;
          *Y = (ST7796S_SIZE_Y - ((*Y * ST7796S_SIZE_Y) / 4095));
          break;
      case (1):
      case (3):
        *X = ((*X * ST7796S_SIZE_Y) / 4095);
        *Y = ST7796S_SIZE_X - ((*Y * ST7796S_SIZE_X) / 4095);
        break;
    }
}
#endif /* #if ST7796S_TOUCH == 1 */
