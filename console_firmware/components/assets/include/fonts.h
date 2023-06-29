/**
 * @file fonts.h
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

#include "st7735s_graphics.h"


/*************************************************
 * Fonts
 *************************************************/
extern const uint8_t myFont[][FONT_SIZE];


#endif // __FONTS_H__