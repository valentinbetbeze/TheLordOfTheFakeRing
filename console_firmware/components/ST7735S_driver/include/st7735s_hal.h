/**
 * @file st7735s_hal.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Header file for the HAL functions of the ST7735S driver.
 * Compatible for use with: ESP32-WROOM-32.
 * @date 2023-04-16
 * 
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __ST7735S_HAL_H__
#define __ST7735S_HAL_H__


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "rom/ets_sys.h"


/*************************************************
 * LCD GPIO
 ************************************************/
#define PIN_LCD_SCK         GPIO_NUM_18     // Serial Clock            
#define PIN_LCD_SDA         GPIO_NUM_23     // Bi-directional MOSI/MISO
#define PIN_LCD_CS          GPIO_NUM_5      // Chip Selection          
#define PIN_LCD_RES         GPIO_NUM_2      // Reset                   
#define PIN_LCD_DC          GPIO_NUM_15     // Register Selection      
#define PIN_LCD_BKL         GPIO_NUM_17     // Background light  


/*************************************************
 * ST7735S functions (System/Panel)
 *************************************************/
#define NOP                 (0x00)          // No Operation
#define SWRESET             (0x01)          // Software Reset
#define RDDID               (0x04)          //
#define RDDST               (0x09)          //
#define RDDPM               (0x0A)          //
#define RDDMADCTL           (0x0B)          //
#define RDDCOLMOD           (0x0C)          //
#define RDDIM               (0x0D)          //
#define RDDSM               (0x0E)          //
#define RDDSDR              (0x0F)          //
#define SLPIN               (0x10)          //
#define SLPOUT              (0x11)          // Sleep Out
#define PLTON               (0x12)          //
#define NORON               (0x13)          // Normal Mode On
#define INVOFF              (0x20)          // Display Inversion Off
#define INVON               (0x21)          //
#define GAMSET              (0x26)          // Set Gamma Curve
#define DISPOFF             (0x28)          //
#define DISPON              (0x29)          // Display On
#define CASET               (0x2A)          // Column Adress Set
#define RASET               (0x2B)          // Raw Adress Set
#define RAMWR               (0x2C)          //
#define RGBSET              (0x2D)          //
#define RAMRD               (0x2E)          //
#define PTLAR               (0x30)          //
#define SCRLAR              (0x33)          //
#define TEOFF               (0x34)          //
#define TEON                (0x35)          //
#define MADCTL              (0x36)          // Memory Data Access Control
#define VSCSAD              (0x37)          //
#define IDMOFF              (0x38)          //
#define IDMON               (0x39)          //
#define COLMOD              (0x3A)          // Interface Pixel Format
#define RDID1               (0xDA)          //
#define RDID2               (0xDB)          //
#define RDID3               (0xDC)          //

#define FRMCTR1             (0xB1)          // Frame Rate Control (NM/FC)
#define FRMCTR2             (0xB2)          //           
#define FRMCTR3             (0xB3)          //
#define INVCTR              (0xB4)          //
#define PWCTR1              (0xC0)          // Power Control 1
#define PWCTR2              (0xC1)          // Power Control 2
#define PWCTR3              (0xC2)          //
#define PWCTR4              (0xC3)          //
#define PWCTR5              (0xC4)          //
#define VMCTR1              (0xC5)          //
#define VMOFCTR             (0xC7)          //
#define WRID2               (0xD1)          //
#define WRID3               (0xD2)          //
#define NVCTR1              (0xD9)          //
#define NVCTR2              (0xDE)          //
#define NVCTR3              (0xDF)          //
#define GAMCTRP1            (0xE0)          //
#define GAMCTRN1            (0xE1)          //
#define GCV                 (0xFC)          //


/*************************************************
 * Display parameters
 ************************************************/
#define LCD_MEMORY_BASE     0b11            // Display resolution code
#define LCD_COLOR_FORMAT    (0x05)          // 16-bit/pixel            
#define LCD_RTNA            0x00
#define LCD_FPA             0x06
#define LCD_BPA             0x03
#define LCD_MH              0x00
#define LCD_RGB             0x00            // 0x00: RGB; 0x01: BGR    
#define LCD_ML              0x00
#define LCD_MV              0x00
#define LCD_MX              0x00            // X-Mirror                
#define LCD_MY              0x01            // Y-Mirror                
#define LCD_GAMMA           0x08            // Gamma Curve 4           

#if (LCD_MEMORY_BASE == 0b00)
    #define LCD_HEIGHT      (132)           /* pixels */
    #define LCD_WIDTH       (162)           /* pixels */
#elif (LCD_MEMORY_BASE == 0b01)
    #define LCD_HEIGHT      (132)           /* pixels */
    #define LCD_WIDTH       (132)           /* pixels */
#elif (LCD_MEMORY_BASE == 0b11)
    #define LCD_HEIGHT      (128)           /* pixels */
    #define LCD_WIDTH       (160)           /* pixels */ 
#else
    #error "LCD_MEMORY_BASE not recognized. Consult ST7735S datasheet."
#endif
#define LCD_NPIX            (LCD_HEIGHT * LCD_WIDTH)    // Number of pixels on the display
#define LCD_SIZE            hypot(LCD_HEIGHT, LCD_WIDTH)

#define PWM_LCD_MODE        LEDC_HIGH_SPEED_MODE
#define PWM_LCD_RESOLUTION  LEDC_TIMER_4_BIT
#define PWM_LCD_TIMER       LEDC_TIMER_0
#define PWM_LCD_FREQ        5000
#define PWM_LCD_CHANNEL     LEDC_CHANNEL_0

#define TEXT_PADDING_X      1               // pixels                  
#define TEXT_PADDING_Y      3               // pixels   


/*************************************************
 * SPI parameters
 ************************************************/
#define SPI_LCD_FREQUENCY   SPI_MASTER_FREQ_40M
#define SPI_LCD_FLAGS       SPI_DEVICE_3WIRE
#define SPI_LCD_QSIZE       1
#define SPI_LCD_HOST        VSPI_HOST
#define SPI_LCD_MODE        (0)
/**
 * @warning Update the macros below from the enum spi_common_dma_t
 * in spi_common.h if any value has changed due to future API updates.
 * Current API version used: v5.0.1
 */
#define SPI_DMA_DISABLED    (0)             // Not updated automatically
#define SPI_DMA_CH1         (1)             // Not updated automatically
#define SPI_DMA_CH2         (2)             // Not updated automatically
#define SPI_DMA_CH_AUTO     (3)             // Not updated automatically

#define SPI_LCD_DMA         SPI_DMA_CH_AUTO


/*************************************************
 * Frame dimensions
 ************************************************/
// NUM_TRANSACTIONS (Rounded up) = LCD_HEIGHT * LCD_WIDTH * 2 / MAX_TRANSFER_SIZE
#if (SPI_LCD_DMA)
// Maximum amount of bytes that can be sent in a single SPI transaction.
    #define MAX_TRANSFER_SIZE       (4092)
    #if (LCD_MEMORY_BASE == 0b00)
// Number of SPI transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (11)
    #elif (LCD_MEMORY_BASE == 0b01)
// Number of SPI transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (9)
    #elif (LCD_MEMORY_BASE == 0b11)
// Number of SPI transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (11)
    #endif
#else
// Maximum amount of bytes that can be sent in a single SPI transaction.
    #define MAX_TRANSFER_SIZE       (64)
    #if (LCD_MEMORY_BASE == 0b00)
// Number of SPI transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (669)
    #elif (LCD_MEMORY_BASE == 0b01)
// Number of SPI transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (545)
    #elif (LCD_MEMORY_BASE == 0b11)
// Number of SPI transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (640)
    #endif
#endif
/* Maximum amount of 2-byte data sent per SPI transaction. Set for 
 16-bit color format.*/
#define PX_PER_TRANSACTION  (MAX_TRANSFER_SIZE / sizeof(uint16_t))
/* Check how many pixels will remain in the last transaction
 (if 0, the last transaction is full)*/ 
#define REMAINING_PIXELS    (LCD_NPIX % (MAX_TRANSFER_SIZE / sizeof(uint16_t)))


/*************************************************
 * Extern variables
 *************************************************/

/**
 * @brief 2D-array representing the display frame to be sent to the
 * TFT LCD screen. 
 * 
 * @note The array is sized specifically to get optimized
 * SPI transfer speed, effectively allowing the maximum amount of
 * bytes to be sent per transaction.
 * Both row and column sizes are determined at compile-time in
 * project_config.h, depending on the chosen SPI parameters and
 * display resolution.
 */
extern uint16_t frame[NUM_TRANSACTIONS][PX_PER_TRANSACTION];


/*************************************************
 * Prototypes
 *************************************************/

/**
 * @brief Initialize the ESP32 PWM channel for the control of the 
 * TFT backlight intensity.
 */
void st7735s_init_pwm_backlight(void);

/**
 * @brief Set the backlight intensity of the TFT display.
 * 
 * @param[in] percentage Backlight intensity in percentage.
 */
void st7735s_set_backlight(uint8_t percentage);

/**
 * @brief Initialize the bus and handle to enable SPI communication
 * with the TFT display.
 * 
 * @param handle Handle of the TFT display for the SPI bus.
 */
void st7735s_init_spi(spi_device_handle_t *handle);

/**
 * @brief Initialize the TFT display.
 * 
 * @param[in] handle SPI device handle of the display.
 */
void st7735s_init_tft(const spi_device_handle_t handle);

/**
 * @brief Send the frame to the ST7735S chip via SPI.
 * 
 * @param[in] handle SPI device handle of the display.
 * @note Transmits the data per transactions of 64 bytes if DMA is 
 * disabled, 4092 bytes is enabled.
 */
void st7735s_push_frame(const spi_device_handle_t handle);


#endif // __ST7735S_HAL_H__