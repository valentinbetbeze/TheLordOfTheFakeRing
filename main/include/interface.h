/**
 * @file interface.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Includes interface functions of the game
 * @date 2023-04-21
 * 
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __INTERFACE_H__
#define __INTERFACE_H__


#include "project_config.h"
#include "st7735s_graphics.h"
#include "maps.h"
#include "fonts.h"
#include "sprites.h"


/**
 * @brief 
 * 
 * @param[in] map 
 * @param[in] map_x
 * @param[out] items 
 * @return Number of items
 */
uint8_t scan_map(const uint8_t map[][BLOCK_SIZE], uint16_t map_x, item_t items[]);


#endif // __INTERFACE_H__