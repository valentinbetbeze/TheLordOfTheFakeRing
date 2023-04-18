/**
 * @file st7735s.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Header file of the ST7735S driver for the ESP32-WROOM-32
 * @date 2023-04-16
 * 
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __ST7735S_H__
#define __ST7735S_H__

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
 * @brief Check if the size of the data is valid for SPI transfer.
 * 
 * @param len Amount of data in byte.
 * @return 0 on success, else 1
 */
int check_data_size(size_t len);

/**
 * @brief Send a command to the ST7735S chip.
 * 
 * @param command 8-bit command (see ST7735S datasheet p.104)
 */
void send_command(uint8_t command);

/**
 * @brief Send a byte to the ST7735S chip.
 * 
 * @param data Pointer to the data to be sent.
 * @param len Amount of data in byte.
 */
void send_byte(uint8_t *data, size_t len);

/**
 * @brief Send a WORD (2 bytes) to the ST7735S chip.
 * 
 * @param data Pointer to the data to be sent.
 * @param len Amount of data in byte.
 */
void send_word(uint16_t *data, size_t len);

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
 * @brief Check that the frame size doesn't exceed the ST7735S memory size.
 * 
 * @param len Size of the frame in bytes.
 * @return 0 on success, 1 otherwise
 */
int check_frame_memory_size(size_t len);

/**
 * @brief Legacy function. Send a 1D-array to the ST7735S chip via SPI.
 * Transmits the data by transactions of 64 bytes. 
 * 
 * @param frame Pointer to the frame to be displayed. Max LCD_HEIGHT *
 * LCD_WIDTH pixels.
 * @param len Size of the frame in bytes.
 * 
 * @note Prefer the use of push_frame_2d for better transmission speed.
 */
void push_frame_1d(uint16_t *frame, int len);

/**
 * @brief Send a 2D-array to the ST7735S chip via SPI. Transmits the data 
 * by transactions of 64 bytes.
 * 
 * @param frame Pointer to the frame to be displayed. Max LCD_HEIGHT * 
 * LCD_WIDTH pixels.
 * @param len Size of the frame in bytes.
 * 
 * @warning The 2D-frame shall have the following format :
 * `uint16_t array[NUM_TRANSACTIONS][PX_PER_TRANSACTION]` - Else the transmission 
 * will fail.
 */
void push_frame_2d(uint16_t **frame, int len);


#endif // __ST7735S_H__