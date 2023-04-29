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

#include "project_config.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "rom/ets_sys.h"


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
 * @param percent Backlight intensity in percentage.
 */
void st7735s_set_backlight(uint8_t percent);

/**
 * @brief Initialize the TFT display.
 * 
 * @param handle SPI device handle of the display.
 */
void st7735s_init_tft(spi_device_handle_t handle);

/**
 * @brief Send the frame to the ST7735S chip via SPI.
 * 
 * @param handle SPI device handle of the display.
 * @note Transmits the data per transactions of 64 bytes if DMA is 
 * disabled, 4092 bytes is enabled.
 */
void st7735s_push_frame(spi_device_handle_t handle);


#endif // __ST7735S_HAL_H__