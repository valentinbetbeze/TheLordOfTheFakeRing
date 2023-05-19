/**
 * @file maps.h
 * @author valentin betbeze (valentin.betbeze@gmail.com)
 * @brief Header file containing the game maps.
 * @date 2023-04-21
 * 
 * @warning Do not modify any value between parenthesis '()'.
 */

#ifndef __MAPS_H__
#define __MAPS_H__


#include <stdint.h>

#include "st7735s_graphics.h"
#include "game_engine.h"


/*************************************************
 * Maps
 *************************************************/

#define SHIRE               (1)
#define MORIA               (2)


extern const map_t map_shire;
extern const map_t map_moria;

#endif // __MAPS_H__