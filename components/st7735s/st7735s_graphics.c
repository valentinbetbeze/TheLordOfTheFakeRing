#include "st7735s_graphics.h"


void get_frame_indexes(uint8_t x, uint8_t y, uint16_t *row, uint16_t *column)
{
    uint16_t npixel = LCD_WIDTH * x + (y + 1);
    uint16_t quotient = (uint16_t) npixel / PX_PER_TRANSACTION;
    uint16_t remainder = (uint16_t) npixel % PX_PER_TRANSACTION;

    if (remainder) {
        *row = quotient;
        *column = remainder - 1;
    }
    else {
        *row = quotient - 1;
        *column = PX_PER_TRANSACTION - 1;
    }
}


uint16_t RGB565(uint32_t rgb888)
{
    // Convert each color in the new format
    uint8_t red = (uint8_t) (((rgb888 >> 16) * 31) / 255);
    uint8_t green = (uint8_t) (((rgb888 >> 8 & 0xFF) * 63) / 255);
    uint8_t blue = (uint8_t) (((rgb888 & 0xFF) * 31) / 255);
    // Compile in one code
    uint16_t rgb565 = ((red << 11) & RED) | ((green << 5) & GREEN) | (blue & BLUE);
    /* Swap data to send MSB first (required for compatibility with ST7735S).
     See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html#transactions-with-integers-other-than-uint8-t
    */
    return SPI_SWAP_DATA_TX(rgb565, 16);
}


void fill_background(uint16_t color)
{
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        for (int j = 0; j < PX_PER_TRANSACTION; j++) {
            frame[i][j] = color;
        }
    }
}


void draw_rectangle(rectangle_t rectangle)
{
    uint16_t row, column;
    for (int x = rectangle.pos_x; x < (rectangle.pos_x + rectangle.width); x++) {
        for (int y = rectangle.pos_y; y < (rectangle.pos_y + rectangle.height); y++) {
            get_frame_indexes(x, y, &row, &column);
            frame[row][column] = rectangle.color;
        }
    }
}


void draw_circle(circle_t circle)
{
    uint8_t y_limit;
    uint16_t row, column;
    // Leveraging symmetry to lower the number of iterations
    for (int x = circle.pos_x; x < (circle.pos_x + circle.radius); x++) {
        uint8_t delta_y = (uint8_t) sqrt(pow(circle.radius, 2) - pow(x - circle.pos_x, 2));
        // Top-half of the circle
        if (circle.thickness) {
            y_limit = circle.pos_y - delta_y + circle.thickness;
        }
        else {
            y_limit = circle.pos_y + 1;
        } 
        for (uint8_t y = (circle.pos_y - delta_y); y < y_limit; y++) {
            // Top-right quarter
            get_frame_indexes(x, y, &row, &column);
            frame[row][column] = circle.color;
            // Top-left quarter
            get_frame_indexes(x-2*(x-circle.pos_x), y, &row, &column);
            frame[row][column] = circle.color;
        }
        // Bottom-half of the circle
        if (circle.thickness) {
            y_limit = circle.pos_y + delta_y - circle.thickness;
        }
        else {
            y_limit = circle.pos_y - 1;
        }   
        for (uint8_t y = (circle.pos_y + delta_y); y > y_limit; y--) {
            // Bottom-right quarter
            get_frame_indexes(x, y, &row, &column);
            frame[row][column] = circle.color;
            // Bottom-left quarter
            get_frame_indexes(x-2*(x-circle.pos_x), y, &row, &column);
            frame[row][column] = circle.color;
        }
    }
}


void draw_text(text_t text)
{
    
}


void draw_sprite(sprite_t sprite)
{

}


void build_frame(item_t *item, int nitems)
{
    for (int i = 0; i < nitems; i++) {
        switch (item[i].type) {
            case BACKGROUND:
                fill_background(item[i].background_color);
                break;
            case RECTANGLE:
                draw_rectangle(item[i].rectangle);
                break;
            case CIRCLE:
                draw_circle(item[i].circle);
                break;
            case TEXT:
                draw_text(item[i].text);
                break;
            case SPRITE:
                draw_sprite(item[i].sprite);
                break;
            default:
                printf("Error: Item of index %i is unknown.\n", i);
                break;
        };
    }
}

