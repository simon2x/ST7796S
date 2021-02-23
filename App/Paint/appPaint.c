#include <stdlib.h>
#include "main.h"
#include "stm32_adafruit_lcd.h"
#include "stm32_adafruit_ts.h"
#include <string.h>

uint16_t boxsize;
uint16_t pensize = 1;

typedef void (*MENU_HANDLER)(int);
MENU_HANDLER menu_handler = 0;
uint16_t oldcolor, currentcolor;
void color_menu_handler(int n);
void option_menu_handler(int n);
void pen_menu_handler(int n);


void clear_menu()
{
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_FillRect(0, 0, BSP_LCD_GetXSize(), boxsize);
}


void clear_canvas()
{
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_FillRect(0, boxsize, BSP_LCD_GetXSize(), BSP_LCD_GetYSize() - boxsize);
}

void show_color_menu()
{
  clear_menu();
  BSP_LCD_SetTextColor(LCD_COLOR_RED);
  BSP_LCD_FillRect(0, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
  BSP_LCD_FillRect(boxsize, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
  BSP_LCD_FillRect(boxsize * 2, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
  BSP_LCD_FillRect(boxsize * 3, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  BSP_LCD_FillRect(boxsize * 4, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_MAGENTA);
  BSP_LCD_FillRect(boxsize * 5, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_FillRect(boxsize * 6, 0, boxsize, boxsize);
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_DisplayStringAt(boxsize * 7.5, boxsize / 2, (uint8_t *)"Menu", CENTER_MODE);
  menu_handler = &color_menu_handler;
  BSP_LCD_SetTextColor(oldcolor);
}

void show_pen_menu()
{
  clear_menu();
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

  BSP_LCD_DrawRect(boxsize * 7, 0, boxsize, boxsize);
  BSP_LCD_DisplayStringAt(boxsize * 0.5, boxsize / 2, (uint8_t *)"Size:", CENTER_MODE);
  BSP_LCD_DisplayStringAt(boxsize * 1.5, boxsize / 2, (uint8_t *)"1", CENTER_MODE);
  BSP_LCD_DisplayStringAt(boxsize * 2.5, boxsize / 2, (uint8_t *)"2", CENTER_MODE);
  BSP_LCD_DisplayStringAt(boxsize * 3.5, boxsize / 2, (uint8_t *)"3", CENTER_MODE);
  BSP_LCD_DisplayStringAt(boxsize * 4.5, boxsize / 2, (uint8_t *)"4", CENTER_MODE);
  BSP_LCD_DisplayStringAt(boxsize * 5.5, boxsize / 2, (uint8_t *)"5", CENTER_MODE);
  BSP_LCD_DisplayStringAt(boxsize * 7.5, boxsize / 2, (uint8_t *)"Menu", CENTER_MODE);
  menu_handler = &pen_menu_handler;

  switch (pensize) {
    case (1):
      BSP_LCD_DrawRect(boxsize * 1, 0, boxsize, boxsize);
      break;
    case (2):
      BSP_LCD_DrawRect(boxsize * 2, 0, boxsize, boxsize);
      break;
    case (3):
      BSP_LCD_DrawRect(boxsize * 3, 0, boxsize, boxsize);
      break;
    case (4):
      BSP_LCD_DrawRect(boxsize * 4, 0, boxsize, boxsize);
      break;
    case (5):
      BSP_LCD_DrawRect(boxsize * 5, 0, boxsize, boxsize);
      break;
    default:
      break;
  }
}

void show_option_menu()
{
  clear_menu();
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_DrawRect(0, 0, boxsize, boxsize);
  BSP_LCD_DrawRect(boxsize, 0, boxsize, boxsize);
  BSP_LCD_DrawRect(boxsize * 2, 0, boxsize, boxsize);

  BSP_LCD_DisplayStringAt(boxsize * 0.5, boxsize / 2, (uint8_t *)"Pen", CENTER_MODE);
  BSP_LCD_DisplayStringAt(boxsize * 1.5, boxsize / 2, (uint8_t *)"Color", CENTER_MODE);
  BSP_LCD_DisplayStringAt(boxsize * 2.5, boxsize / 2, (uint8_t *)"Clear", CENTER_MODE);
  menu_handler = &option_menu_handler;
}

void pen_menu_handler(int n)
{
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  oldcolor = currentcolor;
  switch (n) {
      case (1): {
      pensize = 1;
      break;
    } case (2): {
      pensize = 2;
      break;
    } case (3): {
      pensize = 3;
      break;
    } case (4): {
      pensize = 4;
      break;
    } case (5): {
      pensize = 5;
      break;
    } case (7): {
      show_option_menu();
      return;
    }
    default:
      break;
  }

  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  for (int i = 1; i < 6; i++) {
    if (i == pensize) continue;
    BSP_LCD_DrawRect(boxsize * i, 0, boxsize, boxsize);
  }

  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_DrawRect(boxsize * pensize, 0, boxsize, boxsize);
}

void color_menu_handler(int n)
{
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  oldcolor = currentcolor;
  switch (n) {
    case (0): {
      currentcolor = LCD_COLOR_RED;
      BSP_LCD_DrawRect(0, 0, boxsize, boxsize);
      break;
    } case (1): {
      currentcolor = LCD_COLOR_YELLOW;
      BSP_LCD_DrawRect(boxsize, 0, boxsize, boxsize);
      break;
    } case (2): {
      currentcolor = LCD_COLOR_GREEN;
      BSP_LCD_DrawRect(boxsize*2, 0, boxsize, boxsize);
      break;
    } case (3): {
      currentcolor = LCD_COLOR_CYAN;
      BSP_LCD_DrawRect(boxsize*3, 0, boxsize, boxsize);
      break;
    } case (4): {
      currentcolor = LCD_COLOR_BLUE;
      BSP_LCD_DrawRect(boxsize*4, 0, boxsize, boxsize);
      break;
    } case (5): {
      currentcolor = LCD_COLOR_MAGENTA;
      BSP_LCD_DrawRect(boxsize*5, 0, boxsize, boxsize);
      break;
    } case (6): {
      currentcolor = LCD_COLOR_WHITE;
      BSP_LCD_DrawRect(boxsize*6, 0, boxsize, boxsize);
      break;
    } case (7): {
      show_option_menu();
      break;
    }
    default:
      break;
  }

  // Remove white-box around old color
  if (oldcolor != currentcolor) {
    BSP_LCD_SetTextColor(oldcolor);
    if (oldcolor == LCD_COLOR_RED)
      BSP_LCD_FillRect(0, 0, boxsize, boxsize);
    else if (oldcolor == LCD_COLOR_YELLOW)
      BSP_LCD_FillRect(boxsize, 0, boxsize, boxsize);
    else if (oldcolor == LCD_COLOR_GREEN)
      BSP_LCD_FillRect(boxsize * 2, 0, boxsize, boxsize);
    else if (oldcolor == LCD_COLOR_CYAN)
      BSP_LCD_FillRect(boxsize * 3, 0, boxsize, boxsize);
    else if (oldcolor == LCD_COLOR_BLUE)
      BSP_LCD_FillRect(boxsize * 4, 0, boxsize, boxsize);
    else if (oldcolor == LCD_COLOR_MAGENTA)
      BSP_LCD_FillRect(boxsize * 5, 0, boxsize, boxsize);
    else if (oldcolor == LCD_COLOR_WHITE)
      BSP_LCD_FillRect(boxsize * 6, 0, boxsize, boxsize);
  }
}

void option_menu_handler(int n)
{
  switch (n) {
    case (0): {
      show_pen_menu();
      break;
    }
    case (1): {
      show_color_menu();
      break;
    }
    case (2): {
      clear_canvas();
      break;
    }
    default:
      break;
  }

}


/**
 * Draw circle point depending on pen size
 */
void Draw_Point(int x, int y)
{
  int r = pensize;
  for (int y2 = -r;  y2 <= r; y2++) {
    for (int x2 = -r; x2 <= r; x2++) {
      if (!(x2 * x2 + y2 * y2 <= r * r)) continue;
      // Prevent drawing out of Y-bounds
      if (y + y2 >= BSP_LCD_GetYSize() || y + y2 <= boxsize) continue;
      BSP_LCD_DrawPixel(x + x2, y + y2, currentcolor);
    }
  }

}


#ifdef osCMSIS
void StartDefaultTask(void const * argument)
#else
void mainApp(void)
#endif
{
  TS_StateTypeDef ts;
  boxsize = BSP_LCD_GetXSize() / 8;
  BSP_LCD_Init();
  BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  BSP_LCD_SetFont(&Font12);
  show_color_menu();
  currentcolor = LCD_COLOR_RED;

  while(1)
  {
    BSP_TS_GetState(&ts);
    if(ts.TouchDetected)
    {
      if(ts.Y < boxsize)
      {
        int choice;
        if (ts.X < boxsize) choice = 0;
        else if (ts.X < boxsize * 2) choice = 1;
        else if (ts.X < boxsize * 3) choice = 2;
        else if (ts.X < boxsize * 4) choice = 3;
        else if (ts.X < boxsize * 5) choice = 4;
        else if (ts.X < boxsize * 6) choice = 5;
        else if (ts.X < boxsize * 7) choice = 6;
        else if (ts.X < boxsize * 8) choice = 7;
        menu_handler(choice);
      }
      else
      {
        Draw_Point(ts.X, ts.Y);
      }
    }
    HAL_Delay(1);
  }
}
