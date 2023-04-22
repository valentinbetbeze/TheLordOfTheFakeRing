#include "st7735s_graphics.h"


void write_to_frame(int16_t x, int16_t y, uint16_t data)
{
    if (x < 0 || y < 0) {
        return;
    }

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


void rasterize_circle(uint8_t xc, uint8_t yc, uint8_t x, uint8_t y, uint16_t color) {
    write_to_frame((int16_t)xc + x, (int16_t)yc + y, color);
    write_to_frame((int16_t)xc + y, (int16_t)yc + x, color);
    write_to_frame((int16_t)xc + y, (int16_t)yc - x, color);
    write_to_frame((int16_t)xc + x, (int16_t)yc - y, color);
    write_to_frame((int16_t)xc - x, (int16_t)yc - y, color);
    write_to_frame((int16_t)xc - y, (int16_t)yc - x, color);
    write_to_frame((int16_t)xc - y, (int16_t)yc + x, color);
    write_to_frame((int16_t)xc - x, (int16_t)yc + y, color);
}


void draw_circle(circle_t circle)
{
    // https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
    uint8_t y_out, y_in;
    uint8_t x_end = circle.radius * 0.707 + 1;

    // Draw all 8 octants simultaneously
    for (uint8_t x = 0; x < x_end; x++) {
        // Outer circle
        y_out = round(sqrt(pow(circle.radius, 2) - pow(x, 2)));
        rasterize_circle(circle.pos_x, circle.pos_y, x, y_out, circle.color);
        // No thickness means fully filled circle
        if (circle.thickness == 0) {
            for (uint8_t y = 0; y < y_out; y++) {
                rasterize_circle(circle.pos_x, circle.pos_y, x, y, circle.color);
            }
        }
        else if (1 < circle.thickness){
            y_in = round(sqrt(pow(circle.radius - circle.thickness, 2) - pow(x, 2)));
            rasterize_circle(circle.pos_x, circle.pos_y, x, y_in, circle.color);
            for (uint8_t y = y_in + 1; y < y_out; y++) {
                rasterize_circle(circle.pos_x, circle.pos_y, x, y, circle.color);
            }
        }
    }
}


void draw_text(text_t text)
{
    uint8_t px_pos_x, px_pos_y, offset = 0;

    for (int text_index = 0; text_index < text.size; text_index++) {
        if (text.data[text_index] == '\0') {
            break;
        }
        else if (text.data[text_index] == '\n') {
            text.pos_y += FONT_SIZE + TEXT_PADDING_Y;
            offset = text_index + 1;
        }
        else if (text.data[text_index] < FIRST_ASCII ||
                 text.data[text_index] > LAST_ASCII) {
            printf("Error: `%c`(0x%x) has no font sprite.\n",
            text.data[text_index], text.data[text_index]);
        }
        else {
            /** Using the character ascii code (ex:65 for 'A'), get the corresponding
             * letter sprite and iterate through each layer of the sprite */
            uint8_t char_index = text.data[text_index]-FIRST_ASCII;

            for (int layer_index = 0; layer_index < FONT_SIZE; layer_index++) {
                uint8_t layer = myFont[char_index][layer_index];
                px_pos_y = text.pos_y + layer_index;

                // Extract each bit from bit field and write the pixel to the frame
                for (int bit = FONT_SIZE - 1; bit >= 0; bit--) {
                    if ((layer >> bit) & 1) {
                        px_pos_x    = text.pos_x
                                    + (text_index - offset) * (FONT_SIZE + TEXT_PADDING_X)
                                    + (FONT_SIZE - 1 - bit);
                        write_to_frame(px_pos_x, px_pos_y, text.color);
                    }
                }
            }
        }
    }
}


void draw_sprite(sprite_t sprite)
{
    for (uint8_t y = 0; y < sprite.height; y++) {
        for (uint8_t x = 0; x < sprite.width; x++) {
            uint16_t color = sprite.data[y * sprite.width + x];
            if (color != TRANSPARENT) {
                write_to_frame(x + sprite.pos_x, y + sprite.pos_y, color);
            }
        }
    }
}


void build_frame(item_t *items, int nitems)
{
    for (int i = 0; i < nitems; i++) {
        /*
        if ((items + i) == NULL) {
            printf("Error: Trying to access unauthorized memory location\n \
                    Make sure `nitems` input is correct.\n");
            return;
        };
        */
        switch (items[i].type) {
            case BACKGROUND:
                fill_background(items[i].background_color);
                break;
            case RECTANGLE:
                draw_rectangle(items[i].rectangle);
                break;
            case CIRCLE:
                draw_circle(items[i].circle);
                break;
            case TEXT:
                draw_text(items[i].text);
                break;
            case SPRITE:
                draw_sprite(items[i].sprite);
                break;
            default:
                printf("Error: Item of index %i is unknown.\n", i);
                break;
        };
        
    }
}

