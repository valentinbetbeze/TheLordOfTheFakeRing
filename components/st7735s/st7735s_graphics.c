#include "st7735s_graphics.h"


void write_to_frame(uint8_t x, uint8_t y, uint16_t data)
{
    uint16_t row, column;
    uint16_t npixel = LCD_HEIGHT * x + (y + 1);
    uint16_t quotient = (uint16_t) npixel / PX_PER_TRANSACTION;
    uint16_t remainder = (uint16_t) npixel % PX_PER_TRANSACTION;

    if (remainder) {
        row = quotient;
        column = remainder - 1;
    }
    else {
        row = quotient - 1;
        column = PX_PER_TRANSACTION - 1;
    }

    // Do not *try* to write if out of frame
    if ((NUM_TRANSACTIONS <= row) || (PX_PER_TRANSACTION <= column)) {
        return;
    }

    frame[row][column] = data;
}


uint16_t rgb565(uint32_t rgb888)
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
    for (int x = rectangle.pos_x; x < (rectangle.pos_x + rectangle.width); x++) {
        for (int y = rectangle.pos_y; y < (rectangle.pos_y + rectangle.height); y++) {
            write_to_frame(x, y, rectangle.color);
        }
    }
}


void draw_circle(circle_t circle)
{
    uint8_t y_limit;
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
            write_to_frame(x, y, circle.color);
            // Top-left quarter
            write_to_frame(x-2*(x-circle.pos_x), y, circle.color);
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
            write_to_frame(x, y, circle.color);
            // Bottom-left quarter
            write_to_frame(x-2*(x-circle.pos_x), y, circle.color);
        }
    }
}


void draw_text(text_t text)
{
    const uint8_t MSB = FONT_SIZE - 1;
    uint8_t px_pos_x, px_pos_y;

    for (int text_index = 0; text_index < text.size; text_index++) {

        if ((text.data[text_index] < FIRST_ASCII) || (LAST_ASCII < text.data[text_index])) {
            if (text.data[text_index]) {
                printf("Error: `%c`(0x%x) has no font sprite.\n",
                        text.data[text_index], text.data[text_index]);
            }
            break;
        }
        uint8_t char_index = text.data[text_index]-FIRST_ASCII;
        /** Using the character ascii code (ex:65 for 'A'), get the corresponding
         * letter sprite and iterate through each layer of the sprite */
        for (int layer_index = 0; layer_index < FONT_SIZE; layer_index++) {
            uint8_t layer = myFont[char_index][layer_index];
            px_pos_y = text.pos_y + layer_index;
            // Extract each bit from bit field and write the pixel to the frame
            for (int bit = MSB; bit >= 0; bit--) {
                if ((layer >> bit) & 1) {
                    px_pos_x = text.pos_x + text_index*(FONT_SIZE+FONT_PADDING) + (MSB-bit);
                    write_to_frame(px_pos_x, px_pos_y, text.color);
                }
            }
        }
    }
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

