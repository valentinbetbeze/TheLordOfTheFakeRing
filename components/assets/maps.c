#include "maps.h"


const uint8_t shire[][BLOCK_SIZE] = {
    /* Map header */
    {
        1,              /* map id                   */
        0x6D,           /* background (MSB, RGB565) */
        0xDB,           /* background (LSB, RGB565) */
        0, 0, 0, 0, 0}, /* empty                    */
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