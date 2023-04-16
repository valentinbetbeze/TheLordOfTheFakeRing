#include <stdio.h>
#include <string.h>
#include <math.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "rom/ets_sys.h"

#define HIGH                (1)
#define LOW                 (0)

/*************************************************
 * GPIOs
 ************************************************/
#define PIN_LCD_SCK         GPIO_NUM_18     /* Serial Clock              */
#define PIN_LCD_SDA         GPIO_NUM_23     /* Bi-directional MOSI/MISO  */
#define PIN_LCD_CS          GPIO_NUM_5      /* Chip Selection            */
#define PIN_LCD_RES         GPIO_NUM_2      /* Reset                     */
#define PIN_LCD_DC          GPIO_NUM_15     /* Register Selection        */
#define PIN_LCD_BKL         GPIO_NUM_16     /* Background light          */

/*************************************************
 * Display options
 ************************************************/
#define PWM_LCD_GROUP       LEDC_TIMER_0
#define PWM_LCD_CHANNEL     LEDC_CHANNEL_0
#define PWM_LCD_MODE        LEDC_LOW_SPEED_MODE
#define PWM_LCD_FREQ        5000
#define PWM_LCD_RESOLUTION  LEDC_TIMER_4_BIT

/*************************************************
 * System functions 
 *************************************************/
#define NOP                 (0x00)          /* No Operation */
#define SWRESET             (0x01)          /* Software Reset */
#define RDDID               (0x04)          /*  */
#define RDDST               (0x09)          /*  */
#define RDDPM               (0x0A)          /*  */
#define RDDMADCTL           (0x0B)          /*  */
#define RDDCOLMOD           (0x0C)          /*  */
#define RDDIM               (0x0D)          /*  */
#define RDDSM               (0x0E)          /*  */
#define RDDSDR              (0x0F)          /*  */
#define SLPIN               (0x10)          /*  */
#define SLPOUT              (0x11)          /* Sleep Out */
#define PLTON               (0x12)          /*  */
#define NORON               (0x13)          /* Normal Mode On */
#define INVOFF              (0x20)          /* Display Inversion Off */
#define INVON               (0x21)          /*  */
#define GAMSET              (0x26)          /* Set Gamma Curve */
#define DISPOFF             (0x28)          /*  */
#define DISPON              (0x29)          /* Display On */
#define CASET               (0x2A)          /* Column Adress Set */
#define RASET               (0x2B)          /* Raw Adress Set */
#define RAMWR               (0x2C)          /*  */
#define RGBSET              (0x2D)          /*  */
#define RAMRD               (0x2E)          /*  */
#define PTLAR               (0x30)          /*  */
#define SCRLAR              (0x33)          /*  */
#define TEOFF               (0x34)          /*  */
#define TEON                (0x35)          /*  */
#define MADCTL              (0x36)          /* Memory Data Access Control */
#define VSCSAD              (0x37)          /*  */
#define IDMOFF              (0x38)          /*  */
#define IDMON               (0x39)          /*  */
#define COLMOD              (0x3A)          /* Interface Pixel Format */
#define RDID1               (0xDA)          /*  */
#define RDID2               (0xDB)          /*  */
#define RDID3               (0xDC)          /*  */

/*************************************************
 * Panel functions 
 ************************************************/
#define FRMCTR1             (0xB1)          /* Frame Rate Control (NM/FC) */
#define FRMCTR2             (0xB2)          /*  */           
#define FRMCTR3             (0xB3)          /*  */
#define INVCTR              (0xB4)          /*  */
#define PWCTR1              (0xC0)          /* Power Control 1 */
#define PWCTR2              (0xC1)          /* Power Control 2 */
#define PWCTR3              (0xC2)          /*  */
#define PWCTR4              (0xC3)          /*  */
#define PWCTR5              (0xC4)          /*  */
#define VMCTR1              (0xC5)          /*  */
#define VMOFCTR             (0xC7)          /*  */
#define WRID2               (0xD1)          /*  */
#define WRID3               (0xD2)          /*  */
#define NVCTR1              (0xD9)          /*  */
#define NVCTR2              (0xDE)          /*  */
#define NVCTR3              (0xDF)          /*  */
#define GAMCTRP1            (0xE0)          /*  */
#define GAMCTRN1            (0xE1)          /*  */
#define GCV                 (0xFC)          /*  */

/*************************************************
 * Some color codes (RGB565)
 ************************************************/
#define RED                 (0xF800)
#define GREEN               (0x07E0)
#define BLUE                (0x001F)
#define WHITE               (0xFFFF)
#define BLACK               (0x0000)

/**
 * @note Set as global variable to avoid having to pass it as an 
 * arguments every time an SPI communication takes place. Declared
 * in the main source file (TheLordOfTheFakeRing.c).
 */
extern spi_device_handle_t tft_handle;



void init_pwm_backlight(void);
void set_backlight(uint8_t percent);
void init_spi(void);
void send_command(uint8_t command);
void send_data(uint8_t *data, int len);
void init_tft(void);
void set_write_area(uint8_t xs, uint8_t ys, uint8_t xe, uint8_t ye);
uint16_t RGB888_to_RGB565(uint8_t red, uint8_t green, uint8_t blue);
uint16_t hex_RGB888_to_RGB565(uint32_t rgb888);