/**
 * @file st7735s_fonts.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Header file containing fonts to use in the st7735s_graphics
 * functions.
 * @date 2023-04-19
 * 
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __FONTS_H__
#define __FONTS_H__


#include <stdint.h>

#define FONT_SIZE           (6)         /* in pixel      */
#define FONT_PADDING        1           /* in pixel      */
#define FIRST_ASCII         (32)
#define LAST_ASCII          (90)          

extern const uint8_t myFont[][FONT_SIZE];


#endif // __FONTS_H__