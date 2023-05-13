#include "st7735s_graphics.h"

/**
 * @brief Get the frame indexes object
 * 
 * @param x 
 * @param y 
 * @param row 
 * @param column 
 */
static void get_frame_indexes(int16_t x, int16_t y, uint16_t *row, uint16_t *column)
{
    /*
     Compute the amount of pixels to get to the pixel of position
     (x, y). This parameter is required to figure out which row/column
     of the frame the pixel will have to be placed in.
    */
    uint16_t npixel = LCD_HEIGHT * x + (y + 1);
    uint16_t quotient = (uint16_t) npixel / PX_PER_TRANSACTION;
    uint16_t remainder = (uint16_t) npixel % PX_PER_TRANSACTION;
    /*
     Case A : display pixel (x, y): (0, 62) -> npixel  = 63 pixels
     There are 32 pixels per row, hence row 0 is full, and row 1 is filled
     up to 63 - 32 = 31 pixels, hence we're at column index 30.
     Result: (0, 62) -> frame[1][30]
    */
    if (remainder) {
        *row = quotient;
        *column = remainder - 1;
    }
    /*
     Case B : display pixel (x, y): (0, 63) -> npixel  = 64 pixels
     There are 32 pixels per row, hence both row 0 and row 1 are full.
     The pixel of position (0, 63) is at the end of the second row of the
     frame.
     Note: Using the remainder here is not possible, as any multiple of
     PX_PER_TRANSACTION will give 0, which is not the column index we want.
     Result: (0, 63) -> frame[1][31]
    */
    else {
        *row = quotient - 1;
        *column = PX_PER_TRANSACTION - 1;
    }
}


/**
 * @brief 
 * 
 * @param x 
 * @param y 
 * @return uint16_t 
 */
static uint16_t read_from_frame(int16_t x, int16_t y)
{
    if (x < 0 || LCD_WIDTH <= x || y < 0 || LCD_HEIGHT <= y ) {
        printf("Error(read_from_frame): (x, y) coordinates are out of the frame.\n");
        return 1;
    }
    uint16_t row, column;
    get_frame_indexes(x, y, &row, &column);

    return SPI_SWAP_DATA_TX(frame[row][column], 16);
}


/**
 * @brief Write a pixel of information to the frame, taking the
 * cartesian coordinates of the display as input.
 * 
 * @param x Position of the pixel on the x-axis (along width)
 * @param y Position of the pixel on the y-axis (along height)
 * @param color Color of the pixel
 * @param alpha Transparency of the pixel. 0 means no transparency, 1 means full transparency.
 * 
 * @note The @p color parameter shall be in big-endian format.
 */
static void write_to_frame(int16_t x, int16_t y, uint16_t color, float alpha)
{
    // Do not write if out of display's resolution
    if (x < 0 || LCD_WIDTH <= x || y < 0 || LCD_HEIGHT <= y ) {
        return;
    }

    uint16_t row, column;
    get_frame_indexes(x, y, &row, &column);

    // Do not write if out of the frame's range
    if ((NUM_TRANSACTIONS <= row) || (PX_PER_TRANSACTION <= column)) {
        return;
    }

    if (alpha == 0) {
        frame[row][column] = color;
        return;
    }
    // Apply transparency
    uint16_t color1 = (uint16_t)SPI_SWAP_DATA_TX(frame[row][column], 16);
    uint8_t red1 = (color1 >> 11);
    uint8_t green1 = (color1 >> 5 & 0b111111);
    uint8_t blue1 = (color1 & 0b11111);

    uint16_t color2 = (uint16_t)SPI_SWAP_DATA_TX(color, 16);
    uint8_t red2 = (color2 >> 11);
    uint8_t green2 = (color2 >> 5 & 0b111111);
    uint8_t blue2 = (color2 & 0b11111);

    uint8_t avg_red = alpha * red1 + (1-alpha) * red2;
    uint8_t avg_green = alpha * green1 + (1-alpha) * green2;
    uint8_t avg_blue = alpha * blue1 + (1-alpha) * blue2;
    uint16_t avg_color = avg_red << 11 | avg_green << 5 | avg_blue;
    avg_color = (uint16_t)SPI_SWAP_DATA_TX(avg_color, 16);

    frame[row][column] = avg_color;
}


/**
 * @brief 
 * 
 * @param color 
 * @return uint8_t 
 */
static uint8_t is_color_dark(uint16_t color)
{
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;
    // https://en.wikipedia.org/wiki/Luma_(video)#Rec._601_luma_versus_Rec._709_luma_coefficients
    float luma = sqrt(0.299 * r * r + 0.587 * g * g + 0.114 * b * b);
    // Check if the perceived brightness is below the threshold
    return (luma < PERCEIVED_BRIGHTNESS_THRESHOLD);
}


/**
 * @brief 
 * 
 * @param background 
 * @param color 
 * @return uint16_t 
 */
static uint16_t adapt_color(uint16_t background, uint16_t color)
{
    if (is_color_dark(background)) {
        color = WHITE;
    }
    return color;
}


/**
 * @brief Rasterize 1 point-per-octant of a circle, for all
 * 8 octants.
 * 
 * @param circle Circle object to rasterize.
 * @param x Location of one point on the x-axis. 
 * @param y Location of one point on the y-axis.

 * 
 * @note It does not matter which initial position of one of the
 * 8 points is given as input, the function will draw the 8 pixels
 * independently. However, note that the (x, y) coordinates must 
 * belong to the same point.
 */
static void rasterize_circle(circle_t circle, uint8_t x, uint8_t y)
{
    // Draw 8 pixels at once, one for each octant
    write_to_frame((int16_t)circle.pos_x + x, (int16_t)circle.pos_y + y, circle.color, circle.alpha);
    write_to_frame((int16_t)circle.pos_x + y, (int16_t)circle.pos_y + x, circle.color, circle.alpha);
    write_to_frame((int16_t)circle.pos_x + y, (int16_t)circle.pos_y - x, circle.color, circle.alpha);
    write_to_frame((int16_t)circle.pos_x + x, (int16_t)circle.pos_y - y, circle.color, circle.alpha);
    write_to_frame((int16_t)circle.pos_x - x, (int16_t)circle.pos_y - y, circle.color, circle.alpha);
    write_to_frame((int16_t)circle.pos_x - y, (int16_t)circle.pos_y - x, circle.color, circle.alpha);
    write_to_frame((int16_t)circle.pos_x - y, (int16_t)circle.pos_y + x, circle.color, circle.alpha);
    write_to_frame((int16_t)circle.pos_x - x, (int16_t)circle.pos_y + y, circle.color, circle.alpha);
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
            write_to_frame(x, y, rectangle.color, rectangle.alpha);
        }
    }
}


void st7735s_draw_circle(circle_t circle)
{
    // https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
    uint8_t y_out, y_in;

    // No thickness means fully filled circle
    if (circle.thickness == 0) {
        for (uint8_t x = 0; x < circle.radius; x++) {
            write_to_frame(circle.pos_x + x, circle.pos_y, circle.color, circle.alpha);
            write_to_frame(circle.pos_x - x, circle.pos_y, circle.color, circle.alpha);
            y_out = round(sqrt(pow(circle.radius, 2) - pow(x, 2)));
            for (uint8_t y = 1; y < y_out; y++) {
                if (x == 0) {
                    write_to_frame(circle.pos_x, circle.pos_y + y, circle.color, circle.alpha);
                    write_to_frame(circle.pos_x, circle.pos_y - y, circle.color, circle.alpha);
                }
                else {
                    write_to_frame(circle.pos_x + x, circle.pos_y + y, circle.color, circle.alpha);
                    write_to_frame(circle.pos_x + x, circle.pos_y - y, circle.color, circle.alpha);
                    write_to_frame(circle.pos_x - x, circle.pos_y + y, circle.color, circle.alpha);
                    write_to_frame(circle.pos_x - x, circle.pos_y - y, circle.color, circle.alpha);
                }
            }
        }
        return;
    }
    // Else, draw all 8 octants simultaneously
    uint8_t x_end = circle.radius * 0.707 + 1;
    for (uint8_t x = 0; x < x_end; x++) {
        // Outer circle
        y_out = round(sqrt(pow(circle.radius, 2) - pow(x, 2)));
        rasterize_circle(circle, x, y_out);
        y_in = round(sqrt(pow(circle.radius - circle.thickness, 2) - pow(x, 2)));
        rasterize_circle(circle, x, y_in);
        for (uint8_t y = y_in + 1; y < y_out; y++) {
            rasterize_circle(circle, x, y);
        }
    }
}


void st7735s_draw_text(text_t text)
{
    uint8_t px_pos_x, px_pos_y, offset = 0;
    for (uint8_t char_index = 0; char_index < text.size; char_index++) {
        if (text.data[char_index] == '\0') {
            break;
        }
        else if (text.data[char_index] == '\n') {
            text.pos_y += FONT_SIZE + TEXT_PADDING_Y;
            offset = char_index + 1;
        }
        else if (text.data[char_index] < FIRST_ASCII ||
                 text.data[char_index] > LAST_ASCII) {
            printf("Error: `%c`(0x%x) has no font sprite.\n",
            text.data[char_index], text.data[char_index]);
        }
        else {
            /* Using the character ascii code (ex:65 for 'A'), get the
             corresponding letter sprite and iterate through each layer
             of the sprite. */
            uint8_t sprite_index = text.data[char_index]-FIRST_ASCII;

            for (uint8_t layer_index = 0; layer_index < FONT_SIZE; layer_index++) {
                uint8_t layer = myFont[sprite_index][layer_index];
                px_pos_y = text.pos_y + layer_index;
                // Extract each bit from bit field and write the pixel to the frame
                for (int8_t bit = FONT_SIZE - 1; bit >= 0; bit--) {
                    px_pos_x    = text.pos_x
                                + (char_index - offset) * (FONT_SIZE + TEXT_PADDING_X)
                                + (FONT_SIZE - 1 - bit);
                    if (((layer >> bit) & 1) && text.adaptive) {
                        uint16_t bg_color = read_from_frame(px_pos_x, px_pos_y);
                        uint16_t color = adapt_color(bg_color, text.color);
                        write_to_frame(px_pos_x, px_pos_y, SPI_SWAP_DATA_TX(color, 16), text.alpha);
                    }
                    else if ((layer >> bit) & 1) {
                        write_to_frame(px_pos_x, px_pos_y, text.color, text.alpha);
                    }
                    else if (text.background) {
                        write_to_frame(px_pos_x, px_pos_y, text.background, text.alpha);
                    }
                }
                // Fill background on x-padding
                if (text.background) {
                    for (uint8_t i = 0; i < TEXT_PADDING_X; i++) {
                        px_pos_x =  text.pos_x
                                    + (char_index - offset) * (FONT_SIZE + TEXT_PADDING_X)
                                    - i - 1;
                        write_to_frame(px_pos_x, px_pos_y, text.background, text.alpha);
                        px_pos_x =  text.pos_x
                                    + (char_index - offset) * (FONT_SIZE + TEXT_PADDING_X)
                                    + FONT_SIZE + i;
                        write_to_frame(px_pos_x, px_pos_y, text.background, text.alpha);
                    }
                }
            }
            // Fill background on y-padding
            if (text.background) {
                for (uint8_t i = 0; i < TEXT_PADDING_Y; i++) {
                    for (uint8_t j = 0; j < FONT_SIZE + 2 * TEXT_PADDING_X; j++) {
                        px_pos_x =  text.pos_x
                                    + (char_index - offset) * (FONT_SIZE + TEXT_PADDING_X)
                                    - TEXT_PADDING_X + j;
                        px_pos_y = text.pos_y - i - 1;
                        write_to_frame(px_pos_x, px_pos_y, text.background, text.alpha);
                        px_pos_y = text.pos_y + FONT_SIZE + i;
                        write_to_frame(px_pos_x, px_pos_y, text.background, text.alpha);
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
            if (color == BLACK) { // Use black as transparency for sprites only
                continue;
            }

            int16_t pos_x, pos_y;
            if (sprite.flip_x) {
                pos_x = sprite.pos_x + sprite.width - 1 - x;
            }
            else {
                pos_x = sprite.pos_x + x;
            }
            if (sprite.flip_y) {
                pos_y = sprite.pos_y + sprite.height - 1 - y;
            }
            else {
                pos_y = sprite.pos_y + y;
            }

            if (sprite.CW_90) {
                pos_x = sprite.pos_x + sprite.height - 1 - y;
                pos_y = sprite.pos_y + x;
            }
            else if (sprite.ACW_90) {
                pos_x = sprite.pos_x + y;
                pos_y = sprite.pos_y + sprite.width - 1 - x;
            }

            write_to_frame(pos_x, pos_y, color, sprite.alpha);
        }
    }
}
