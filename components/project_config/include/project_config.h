/**
 * @file project_config.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Configuration header file
 * @date 2023-04-17
 * 
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __PROJECT_CONFIG_H__
#define __PROJECT_CONFIG_H__

#include <math.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"


/*************************************************
 * GPIOs
 ************************************************/
#define PIN_LCD_SCK         GPIO_NUM_18     // Serial Clock            
#define PIN_LCD_SDA         GPIO_NUM_23     // Bi-directional MOSI/MISO
#define PIN_LCD_CS          GPIO_NUM_5      // Chip Selection          
#define PIN_LCD_RES         GPIO_NUM_2      // Reset                   
#define PIN_LCD_DC          GPIO_NUM_15     // Register Selection      
#define PIN_LCD_BKL         GPIO_NUM_16     // Background light        


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
#define LCD_SIZE            hypot(LCD_HEIGHT, LCD_WIDTH)

#define PWM_LCD_GROUP       LEDC_TIMER_0
#define PWM_LCD_CHANNEL     LEDC_CHANNEL_0
#define PWM_LCD_MODE        LEDC_LOW_SPEED_MODE
#define PWM_LCD_FREQ        5000
#define PWM_LCD_RESOLUTION  LEDC_TIMER_4_BIT

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
#define SPI_LCD_DMA         SPI_DMA_DISABLED

// NUM_TRANSACTIONS = LCD_HEIGHT * LCD_WIDTH * 2 / MAX_TRANSFER_SIZE
// (Rounded up)
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
#define PX_PER_TRANSACTION  (MAX_TRANSFER_SIZE / 2)


/*************************************************
 * Game design parameters
 ************************************************/

// All blocks > 0 are solid, else they are not.
#define NUM_BLOCK_RECORDS       10
#define TIMESTEP_BUMP_BLOCK     5           // in milliseconds
#define HEIGHT_BUMP_BLOCK       3           // Bump height of a block, in pixels
#define BACKGROUND_BLOCK        (0)
#define NON_BREAKABLE_BLOCK     (1)
#define BREAKABLE_BLOCK         (2)
#define BONUS_BLOCK             (3)

#define NUM_ENEMY_RECORDS       10
#define TIMESTEP_ENEMY          15          // in milliseconds
#define KILL_ZONE_Y             5           // Height, in pixels, in which an enemy is killed
#define ENEMY_1                 (-30)
#define ENEMY_2                 (-31)
#define ENEMY_3                 (-32)

#define NUM_ITEMS               NUM_BLOCK_RECORDS
#define TIMESTEP_BUMP_COIN      5           // in milliseconds
#define HEIGHT_BUMP_COIN        36          // Bump height of a coin, in pixels
#define SHIELD_ALPHA            0.5         // Shield color transparency
#define COIN                    (1)
#define LIGHTSTAFF              (2)
#define SHIELD                  (3)

#define SPEED_INITIAL           1
#define SPEED_JUMP_INIT         2
#define SLIP_OFFSET             2           // Left/right slip offset, in pixels
#define TIMESTEP_ACCEL          200         // in milliseconds

#define IS_SOLID(x)             (x > BACKGROUND_BLOCK)
#define IS_INTERACTIVE(x)       (x > NON_BREAKABLE_BLOCK)
#define IS_ENEMY(x)             (x <= ENEMY_1)
#define MAP_BACKGROUND(x)       (x[0][2] << 8 | x[0][1])
#define MAP_ID(x)               (x[0][0])


#endif // __PROJECT_CONFIG_H__