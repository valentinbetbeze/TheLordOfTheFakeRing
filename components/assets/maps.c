#include "maps.h"


const uint8_t shire[][NB_BLOCKS_Y] = {
    /* Map header */
    {
        0x01,               /* map id                   */
        0x6D,               /* background (MSB, RGB565) */
        0xDB,               /* background (LSB, RGB565) */
        0, 0, 0, 0, 0},     /* empty                    */

    /* Map data */
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 1, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 1, 0, 0, 1, 0},
    {1, 0, 0, 1, 0, 0, 0, 0},
    {1, 0, 0, 1, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},

    {1, 0, 0, 0, 0, 0, 0, 0},
    
    // Don't forget to add one last (empty) column at the end!
};