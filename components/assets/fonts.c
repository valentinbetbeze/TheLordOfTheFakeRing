#include "fonts.h"


const uint8_t myFont[][FONT_SIZE] = {
    {   // Space
        0b000000,
        0b000000,
        0b000000,
        0b000000,
        0b000000,
        0b000000
    },
    {   // !
        0b000000,
        0b001110,
        0b001110,
        0b001110,
        0b000000,
        0b001110
    },
    {   // "
        0b110011,
        0b110011,
        0b010001,
        0b000000,
        0b000000,
        0b000000
    },
    {   /* # */   },
    {   /* $ */   },
    {   /* % */   },
    {   /* & */   },
    {   // '
        0b001100,
        0b001100,
        0b000100,
        0b000000,
        0b000000,
        0b000000
    },
    {   /* ( */   },
    {   /* ) */   },
    {   /* * */   },
    {   /* + */   },
    {   // ,
        0b000000,
        0b000000,
        0b000000,
        0b001100,
        0b000100,
        0b001000
    },
    {   /* - */   },
    {   // .
        0b000000,
        0b000000,
        0b000000,
        0b000000,
        0b001100,
        0b001100
    },
    {   /* / */   },
    {   // 0
        0b011110,
        0b110011,
        0b110011,
        0b110011,
        0b110011,
        0b011110
    },
    {   // 1
        0b001100,
        0b011100,
        0b001100,
        0b001100,
        0b001100,
        0b011110
    },
    {   // 2
        0b011110,
        0b100111,
        0b000111,
        0b011110,
        0b111000,
        0b111111
    },
    {   // 3
        0b111110,
        0b000111,
        0b011110,
        0b000111,
        0b000111,
        0b111110
    },
    {   // 4
        0b011110,
        0b110110,
        0b100110,
        0b100110,
        0b111111,
        0b000110
    },
    {   // 5
        0b111110,
        0b110000,
        0b111110,
        0b000111,
        0b100111,
        0b011110
    },
    {   // 6
        0b011110,
        0b110000,
        0b111110,
        0b110011,
        0b110011,
        0b011110
    },
    {   // 7 
        0b111111,
        0b000011,
        0b000110,
        0b001100,
        0b011100,
        0b011100
    },
    {   // 8
        0b011110,
        0b100111,
        0b011110,
        0b100111,
        0b100111,
        0b011110
    },
    {   // 9
        0b011110,
        0b100111,
        0b100111,
        0b011111,
        0b000111,
        0b011110
    },
    {   // :
        0b000000,
        0b001100,
        0b001100,
        0b000000,
        0b001100,
        0b001100
    },
    {   // ;
        0b001100,
        0b001100,
        0b000000,
        0b001100,
        0b000100,
        0b001000
    },
    {   /* < */   },
    {   // =
        0b000000,
        0b000000,
        0b111111,
        0b000000,
        0b111111,
        0b000000
    },
    {   /* > */   },
    {   // ?
        0b011110,
        0b100111,
        0b000111,
        0b001100,
        0b000000,
        0b001100
    },
    {   /* @ */   },
    {   // A
        0b011110,
        0b100111,
        0b100111,
        0b111111,
        0b100111,
        0b100111
    },
    {   // B
        0b111110,
        0b110011,
        0b111110,
        0b110011,
        0b110011,
        0b111110
    },
    {   // C
        0b011110,
        0b110011,
        0b110000,
        0b110000,
        0b110011,
        0b011110
    },
    {   // D
        0b111110,
        0b100111,
        0b100111,
        0b100111,
        0b100111,
        0b111110
    },
    {   // E
        0b111111,
        0b110000,
        0b111110,
        0b110000,
        0b110000,
        0b111111
    },
    {   // F
        0b111111,
        0b110000,
        0b110000,
        0b111110,
        0b110000,
        0b110000
    },
    {   // G
        0b011110,
        0b110011,
        0b110000,
        0b110111,
        0b110011,
        0b011111
    },
    {   // H
        0b100011,
        0b100011,
        0b111111,
        0b100011,
        0b100011,
        0b100011
    },
    {   // I
        0b011110,
        0b001100,
        0b001100,
        0b001100,
        0b001100,
        0b011110
    },
    {   // J
        0b001111,
        0b000110,
        0b000110,
        0b110110,
        0b110110,
        0b011100
    },
    {   // K
        0b110011,
        0b110110,
        0b111100,
        0b111100,
        0b110110,
        0b110011
    },
    {   // L
        0b110000,
        0b110000,
        0b110000,
        0b110000,
        0b110000,
        0b111111
    },
    {   // M
        0b100011,
        0b110111,
        0b111111,
        0b101011,
        0b100011,
        0b100011
    },
    {   // N
        0b100011,
        0b110011,
        0b111011,
        0b101111,
        0b100111,
        0b100011
    },
    {   // O
        0b011110,
        0b110011,
        0b110011,
        0b110011,
        0b110011,
        0b011110
    },
    {   // P
        0b111110,
        0b110011,
        0b110011,
        0b111110,
        0b110000,
        0b110000
    },
    {   // Q
        0b011110,
        0b110001,
        0b110001,
        0b110101,
        0b110010,
        0b011111
    },
    {   // R
        0b111110,
        0b110011,
        0b110011,
        0b111110,
        0b110100,
        0b110011
    },
    {   // S
        0b011110,
        0b110000,
        0b011110,
        0b000111,
        0b100111,
        0b011110
    },
    {   // T
        0b111111,
        0b001100,
        0b001100,
        0b001100,
        0b001100,
        0b001100
    },
    {   // U
        0b100011,
        0b100011,
        0b100011,
        0b100011,
        0b100011,
        0b011110
    },
    {   // V
        0b100011,
        0b100011,
        0b100011,
        0b100011,
        0b010110,
        0b001100
    },
    {   // W
        0b100011,
        0b100011,
        0b101011,
        0b111111,
        0b110111,
        0b100011
    },
    {   // X
        0b100011,
        0b010110,
        0b001100,
        0b011100,
        0b110010,
        0b100001
    },
    {   // Y
        0b110011,
        0b110011,
        0b011110,
        0b001100,
        0b001100,
        0b001100
    },
    {   // Z
        0b111111,
        0b000111,
        0b001110,
        0b011100,
        0b111000,
        0b111111
    },
};