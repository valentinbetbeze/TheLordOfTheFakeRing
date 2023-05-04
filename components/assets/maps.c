#include "maps.h"

const int8_t shire[][NUM_BLOCKS_Y] = {
    // Map header
    {
        0x01,               // map id                   
        0xDF,               // background (MSB, RGB565)
        0x1B,               // background (LSB, RGB565)
        0, 0, 0, 0, 0},     // empty                    

    // Map data
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 2, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 1, 0, 0, 0, 0},
    {1, 0, 0, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 1, 0, 0}, // 10

    {1, 0, 0, 0, 0, 1, 0, 0},
    {1, 0, 0, 0, 0, 1, -30, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1},
    // Don't forget to add one last (empty) column at the end!
};