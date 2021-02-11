/*
 * 8 bit parallel LCD/TOUCH GPIO driver for STM32F4
 * 5 control pins (CS, RS, WR, RD, RST) + 8 data pins + backlight pin

 * note: since the LCD and touch are on the same pins, 
 * the simultaneous use of these should be excluded.
 * In multi-threaded interruption mode, collision prevention must be avoided!
 */

//=============================================================================
/* Lcd control pins assign (A..K, 0..15) */
#define LCD_CS            C, 0
#define LCD_RS            C, 1
#define LCD_WR            C, 13
#define LCD_RST           C, 14  // set to `X, 0` if unused
#define LCD_RD            C, 15  // set to `X, 0` if unused
/* Lcd data pins assign (A..K, 0..15) */
#define LCD_D0            E, 0
#define LCD_D1            E, 1
#define LCD_D2            E, 2
#define LCD_D3            E, 3
#define LCD_D4            E, 4
#define LCD_D5            E, 5
#define LCD_D6            E, 6
#define LCD_D7            E, 7

/* Backlight control
   - BL: A..K, 0..15 (if not used -> X, 0)
   - BL_ON: 0 = active low level, 1 = active high level */
#define LCD_BL            X, 0  // set to `X, 0` if unused
#define LCD_BLON          0


/* Wait time for LCD write and read pulse and touchscreen AD converter
   - First, give 10, 20, 500 values, then lower them to speed up the program.
     (values also depend on processor speed and LCD display speed) */
//#define LCD_WRITE_DELAY  10
//#define LCD_READ_DELAY   20
//#define TS_AD_DELAY     500

/*=============================================================================
I/O group optimization so that GPIO operations are not performed bit by bit:
Note: If the pins are in order, they will automatically optimize.
The example belongs to the following pins:
      LCD_D0<-D14, LCD_D1<-D15, LCD_D2<-D0, LCD_D3<-D1
      LCD_D4<-E7,  LCD_D5<-E8,  LCD_D6<-E9, LCD_D7<-E10 */
#if 0
/* datapins setting to output (data direction: STM32 -> LCD) */
#define LCD_DIRWRITE { /* D0..D1, D14..D15, E7..E10 <- 0b01 */ \
GPIOD->MODER = (GPIOD->MODER & ~0b11110000000000000000000000001111) | 0b01010000000000000000000000000101; \
GPIOE->MODER = (GPIOE->MODER & ~0b00000000001111111100000000000000) | 0b00000000000101010100000000000000; }
/* datapins setting to input (data direction: STM32 <- LCD) */
#define LCD_DIRREAD { /* D0..D1, D14..D15, E7..E10 <- 0b00 */ \
GPIOD->MODER = (GPIOD->MODER & ~0b11110000000000000000000000001111); \
GPIOE->MODER = (GPIOE->MODER & ~0b00000000001111111100000000000000); }
/* datapins write, STM32 -> LCD (write I/O pins from dt data) */
#define LCD_WRITE(dt) { /* D14..15 <- dt0..1, D0..1 <- dt2..3, E7..10 <- dt4..7 */ \
GPIOD->ODR = (GPIOD->ODR & ~0b1100000000000011) | (((dt & 0b00000011) << 14) | ((dt & 0b00001100) >> 2)); \
GPIOE->ODR = (GPIOE->ODR & ~0b0000011110000000) | ((dt & 0b11110000) << 3); }
/* datapins read, STM32 <- LCD (read from I/O pins and store to dt data) */
#define LCD_READ(dt) { /* dt0..1 <- D14..15, dt2..3 <- D0..1, dt4..7 <- E7..10 */ \
dt = ((GPIOD->IDR & 0b1100000000000000) >> 14) | ((GPIOD->IDR & 0b0000000000000011) << 2) | \
     ((GPIOE->IDR & 0b0000011110000000) >> 3); }
/* Note: the keil compiler cannot use binary numbers, convert it to hexadecimal */	 
#endif
