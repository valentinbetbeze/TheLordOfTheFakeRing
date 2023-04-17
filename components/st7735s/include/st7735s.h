/**
 * @file st7735s.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Header file of the ST7735S driver for the ESP32-WROOM-32
 * @version 1.0.3
 * @date 2023-04-16
 * 
 * @note The following sections can and should be modified
 *       according to the hardware setup used:
*           - GPIOs
*           - SPI parameters
*           - Display parameters
 * @warning Do not modify any value between parenthesis '()'.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "rom/ets_sys.h"

/*************************************************
 * GPIOs
 ************************************************/
#define PIN_LCD_SCK         GPIO_NUM_18     /* Serial Clock             */
#define PIN_LCD_SDA         GPIO_NUM_23     /* Bi-directional MOSI/MISO */
#define PIN_LCD_CS          GPIO_NUM_5      /* Chip Selection           */
#define PIN_LCD_RES         GPIO_NUM_2      /* Reset                    */
#define PIN_LCD_DC          GPIO_NUM_15     /* Register Selection       */
#define PIN_LCD_BKL         GPIO_NUM_16     /* Background light         */

/*************************************************
 * Display parameters
 ************************************************/
#define LCD_MEMORY_BASE     0b11            /* Resolution parameter     */
#define LCD_COLOR_FORMAT    (0x05)          /* 16-bit/pixel             */
#define LCD_RTNA            0x00
#define LCD_FPA             0x06
#define LCD_BPA             0x03
#define LCD_MH              0x00
#define LCD_RGB             0x00
#define LCD_ML              0x00
#define LCD_MV              0x00
#define LCD_MX              0x00
#define LCD_MY              0x00
#define LCD_GAMMA           0x01            /* Gamma Curve 0            */

#define PWM_LCD_GROUP       LEDC_TIMER_0
#define PWM_LCD_CHANNEL     LEDC_CHANNEL_0
#define PWM_LCD_MODE        LEDC_LOW_SPEED_MODE
#define PWM_LCD_FREQ        5000
#define PWM_LCD_RESOLUTION  LEDC_TIMER_4_BIT

#if (LCD_MEMORY_BASE == 0b00)
    #define LCD_HEIGHT      (162)           /* pixels                   */
    #define LCD_WIDTH       (132)           /* pixels                   */
#elif (LCD_MEMORY_BASE == 0b01)
    #define LCD_HEIGHT      (132)           /* pixels                   */
    #define LCD_WIDTH       (132)           /* pixels                   */
#elif (LCD_MEMORY_BASE == 0b11)
    #define LCD_HEIGHT      (160)           /* pixels                   */
    #define LCD_WIDTH       (128)           /* pixels                   */
#else
    #error "LCD_MEMORY_BASE not recognized. Consult ST7735S datasheet."
#endif

/*************************************************
 * SPI parameters
 ************************************************/
#define SPI_LCD_FREQUENCY   SPI_MASTER_FREQ_26M
#define SPI_LCD_FLAGS       SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX
#define SPI_LCD_QSIZE       1
#define SPI_LCD_HOST        VSPI_HOST
#define SPI_LCD_MODE        (0)
#define SPI_LCD_DMA         SPI_DMA_DISABLED

// NUM_TRANSACTION = (LCD_HEIGHT * LCD_WIDTH * 2 / MAX_TRANSFER_SIZE) (rounded up)
#if (SPI_LCD_DMA)
    #define MAX_TRANSFER_SIZE       (4092)
    #if (LCD_MEMORY_BASE == 0b00)
        #define NUM_TRANSACTIONS    (11)
    #elif (LCD_MEMORY_BASE == 0b01)
        #define NUM_TRANSACTIONS    (9)
    #elif (LCD_MEMORY_BASE == 0b11)
        #define NUM_TRANSACTIONS    (11)
    #endif
#else
    #define MAX_TRANSFER_SIZE       (64)
    #if (LCD_MEMORY_BASE == 0b00)
        #define NUM_TRANSACTIONS    (669)
    #elif (LCD_MEMORY_BASE == 0b01)
        #define NUM_TRANSACTIONS    (545)
    #elif (LCD_MEMORY_BASE == 0b11)
        #define NUM_TRANSACTIONS    (640)
    #endif
#endif
#define PX_PER_TRANSACTION  (MAX_TRANSFER_SIZE / 2)

/*************************************************
 * ST7735S functions (System/Panel)
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


/**
 * @brief TFT display handle on the SPI bus
 * @note Set as global variable to avoid having to pass it as an 
 * arguments every time an SPI communication takes place. Declared
 * in the main source file (TheLordOfTheFakeRing.c).
 */
extern spi_device_handle_t tft_handle;


/**
 * @brief Initialize the spi bus and device interface for the TFT
 * LCD display.
 * @note DMA is disabled on this version. 
 */
void init_spi(void);

/**
 * @brief Send a command to the ST7735S chip.
 * 
 * @param command 8-bit command (see ST7735S datasheet p.104)
 */
void send_command(uint8_t command);

/**
 * @brief Send data to the ST7735S chip.
 * 
 * @param data Pointer to the data to be sent. Preferably an 8-bit
 * array of size 64 for maximum efficiency.
 * @param len Size of the array in bytes.
 * 
 * @note With DMA disabled, a maximum of 64 bytes can be sent with 1 SPI
 * transaction.
 */
void send_data(uint8_t *data, int len);

/**
 * @brief Initialize the TFT display.
 */
void init_tft(void);

/**
 * @brief Initialize the ESP32 PWM channel for the control of the 
 * TFT backlight intensity.
 */
void init_pwm_backlight(void);

/**
 * @brief Set the backlight intensity of the TFT display.
 * 
 * @param percent Backlight intensity in percentage.
 */
void set_backlight(uint8_t percent);

/**
 * @brief Set the TFT display area onto which writing data.
 * 
 * @param xs Start column
 * @param xe End column
 * @param ys Start row
 * @param ye End row
 * 
 * @warning This function is dependant on the display orientation.
 */
void set_display_area(uint8_t xs, uint8_t xe, uint8_t ys, uint8_t ye);

/**
 * @brief Check that the frame size doesn't exceed the ST7725S memory size.
 * 
 * @param len Size of the frame in bytes.
 * @return 0 if success, 1 if error/warning
 */
int check_frame_size(int len);

/**
 * @brief Send a frame to the ST7735S chip via SPI. Transmits the data 
 * by transactions of 64 bytes.
 * 
 * @param frame Pointer to the frame to be displayed. Max LCD_HEIGHT * 
 * LCD_WIDTH pixels.
 * @param len Size of the frame in bytes.
 */
void push_frame(uint16_t *frame, int len);
