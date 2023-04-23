#include "st7735s_graphics.h"


/**
 * @brief Write a pixel of information to the frame, taking the
 * cartesian coordinates of the display as input.
 * 
 * @param x Position of the pixel on the x-axis (along width)
 * @param y Position of the pixel on the y-axis (along height)
 * @param data Color of the pixel
 */
static void write_to_frame(int16_t x, int16_t y, uint16_t color)
{
    // Do not write if out of frame
    if (x < 0 || y < 0) {
        return;
    }

    uint16_t row, column;
    /** Compute the amount of pixels to get to the pixel of position
     * (x, y). This parameter is required to figure out which row/column
     * of the frame the pixel will have to be placed in. */
    uint16_t npixel = LCD_HEIGHT * x + (y + 1);
    uint16_t quotient = (uint16_t) npixel / PX_PER_TRANSACTION;
    uint16_t remainder = (uint16_t) npixel % PX_PER_TRANSACTION;

    /** Case A : display pixel (x, y): (0, 62) -> npixel  = 63 pixels
     * There are 32 pixels per row, hence row 0 is full, and row 1 is filled
     * up to 63 - 32 = 31 pixels. As the column index starts at 0, we're at
     * column index 30.
     * Result: (0, 62) -> frame[1][30] */
    if (remainder) {
        row = quotient;
        column = remainder - 1;
    }
    /** Case B : display pixel (x, y): (0, 63) -> npixel  = 64 pixels
     * There are 32 pixels per row, hence both row 0 and row 1 are full.
     * The pixel of position (0, 63) is at the end of the second row of the
     * frame.
     * Note: Using the remainder here is not possible, as any multiple of
     * PX_PER_TRANSACTION will give 0, which is not the column index we want.
     * Result: (0, 63) -> frame[1][31] */
    else {
        row = quotient - 1;
        column = PX_PER_TRANSACTION - 1;
    }

    // Do not write if out of frame
    if ((NUM_TRANSACTIONS <= row) || (PX_PER_TRANSACTION <= column)) {
        return;
    }

    frame[row][column] = color;
}


/**
 * @brief Rasterize 1 point-per-octant of a circle, for all
 * 8 octants.
 * 
 * @param xc Center of the circle on the x-axis.
 * @param yc Center of the circle on the y-axis.
 * @param x Location of one point on the x-axis. 
 * @param y Location of one point on the y-axis.
 * @param color Color of the circle.
 * 
 * @note It does not matter which initial position of one of the
 * 8 points is given as input, the function will draw the 8 pixels
 * independently. However, note that the (x, y) coordinates must 
 * belong to the same point.
 */
static void rasterize_circle(uint8_t xc, uint8_t yc, uint8_t x, uint8_t y, uint16_t color)
{
    // Draw 8 pixels at once, one for each octant
    write_to_frame((int16_t)xc + x, (int16_t)yc + y, color);
    write_to_frame((int16_t)xc + y, (int16_t)yc + x, color);
    write_to_frame((int16_t)xc + y, (int16_t)yc - x, color);
    write_to_frame((int16_t)xc + x, (int16_t)yc - y, color);
    write_to_frame((int16_t)xc - x, (int16_t)yc - y, color);
    write_to_frame((int16_t)xc - y, (int16_t)yc - x, color);
    write_to_frame((int16_t)xc - y, (int16_t)yc + x, color);
    write_to_frame((int16_t)xc - x, (int16_t)yc + y, color);
}


uint16_t st7735s_rgb565(uint32_t rgb888)
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


void st7735s_fill_background(uint16_t color)
{
    for (int i = 0; i < NUM_TRANSACTIONS; i++) {
        for (int j = 0; j < PX_PER_TRANSACTION; j++) {
            frame[i][j] = color;
        }
    }
}


void st7735s_draw_rectangle(rectangle_t rectangle)
{
    for (int x = rectangle.pos_x; x < (rectangle.pos_x + rectangle.width); x++) {
        for (int y = rectangle.pos_y; y < (rectangle.pos_y + rectangle.height); y++) {
            write_to_frame(x, y, rectangle.color);
        }
    }
}


void st7735s_draw_circle(circle_t circle)
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


void st7735s_draw_text(text_t text)
{
    uint8_t px_pos_x, px_pos_y, offset = 0;

    for (int text_index = 0; text_index < text.size; text_index++) {
        // End of text
        if (text.data[text_index] == '\0') {
            break;
        }
        // New line
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
            /** Using the character ascii code (ex:65 for 'A'), get the
             * corresponding letter sprite and iterate through each layer
             * of the sprite */
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


void st7735s_draw_sprite(sprite_t sprite)
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


void st7735s_build_frame(item_t *items, int nitems)
{
    for (int i = 0; i < nitems; i++) {
        if ((items + i) == NULL) {
            printf("Error: Trying to access unauthorized memory location.\n");
            return;
        };
        switch (items[i].type) {
            case BACKGROUND:
                st7735s_fill_background(items[i].background_color);
                break;
            case RECTANGLE:
                st7735s_draw_rectangle(items[i].rectangle);
                break;
            case CIRCLE:
                st7735s_draw_circle(items[i].circle);
                break;
            case TEXT:
                st7735s_draw_text(items[i].text);
                break;
            case SPRITE:
                st7735s_draw_sprite(items[i].sprite);
                break;
            default:
                printf("Error (st7735s_build_frame): Unknown item has been given as input. Please check items[] or nitems variables.\n");
                break;
        };
        
    }
}
