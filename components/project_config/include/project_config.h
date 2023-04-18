/**
 * @file project_config.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Configuration header file
 * @date 2023-04-17
 * 
 * @note The following sections can and should be modified
 *       according to the hardware setup used:
 *          - GPIOs
 *          - Display parameters
 *          - SPI parameters
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __PROJECT_CONFIG_H__
#define __PROJECT_CONFIG_H__

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"

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
#define LCD_MEMORY_BASE     0b11            /* Discplay resolution code */
#define LCD_COLOR_FORMAT    (0x05)          /* 16-bit/pixel             */
#define LCD_RTNA            0x00
#define LCD_FPA             0x06
#define LCD_BPA             0x03
#define LCD_MH              0x00
#define LCD_RGB             0x00            /* 0x00: RGB; 0x01: BGR     */
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
#define SPI_LCD_FREQUENCY   SPI_MASTER_FREQ_40M
#define SPI_LCD_FLAGS       SPI_DEVICE_3WIRE
#define SPI_LCD_QSIZE       1
#define SPI_LCD_HOST        VSPI_HOST
#define SPI_LCD_MODE        (0)
#define SPI_LCD_DMA         SPI_DMA_DISABLED

// NUM_TRANSACTIONS = LCD_HEIGHT * LCD_WIDTH * 2 / MAX_TRANSFER_SIZE
// (Rounded up)
#if (SPI_LCD_DMA)
// Maximum amount of bytes that can be sent in a single SPI transaction.
    #define MAX_TRANSFER_SIZE       (4092)
    #if (LCD_MEMORY_BASE == 0b00)
// Number of transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (11)
    #elif (LCD_MEMORY_BASE == 0b01)
// Number of transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (9)
    #elif (LCD_MEMORY_BASE == 0b11)
// Number of transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (11)
    #endif
#else
// Maximum amount of bytes that can be sent in a single SPI transaction.
    #define MAX_TRANSFER_SIZE       (64)
    #if (LCD_MEMORY_BASE == 0b00)
// Number of transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (669)
    #elif (LCD_MEMORY_BASE == 0b01)
// Number of transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (545)
    #elif (LCD_MEMORY_BASE == 0b11)
// Number of transactions required to send the frame to the display.
        #define NUM_TRANSACTIONS    (640)
    #endif
#endif
/** Maximum amount of 2-byte data sent per SPI transaction. Set for 
 * 16-bit color format.*/
#define PX_PER_TRANSACTION  (MAX_TRANSFER_SIZE / 2)


#endif // __PROJECT_CONFIG_H__