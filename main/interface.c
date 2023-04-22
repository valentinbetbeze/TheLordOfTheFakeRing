#include "interface.h"


uint8_t scan_map(const uint8_t map[][BLOCK_SIZE], uint16_t map_x, item_t items[])
{
    if (map == NULL) {
        printf("Error: Map file does not exist.\n");
        return 0;
    }
    if (items == NULL) {
        printf("Error: `items` array does not exist.\n");
        return 0;
    }

    // Set background item
    items[0].type = BACKGROUND;
    items[0].background_color = (map[0][2] << 8) | map[0][1];

    uint8_t nitems = 1; // Initialized at 1 because of the BACKGROUND item
    uint8_t map_id = map[0][0];
    uint16_t map_index_ini = (uint16_t)(map_x / BLOCK_SIZE) + 1;
    // Iterate through each columns of the map, starting at the current index
    for (int i = map_index_ini; i < map_index_ini + NB_BLOCKS_X + 1; i++) {
        // Check each element of the map's column and create the appropriate item
        for (int j = 0; j < NB_BLOCKS_Y; j++) {
            // Get the map element
            uint8_t element = map[i][NB_BLOCKS_Y - j - 1];
            // If 'background block', go to next element
            if (element == 0) {
                continue;
            }
            /** Elements of the map can only be block sprites, other elements such
             * as specific sprites or text are added outside of the scope of this
             * function. */
            if (MAX_ITEMS - 1 < nitems) {
                printf("Error: Too many items on the map.\n                 \
                        map_index_ini   = %i\n                              \
                        map_index       = %i\n                              \
                        -> Either decrease the number of items at the given \
                        indexes, or increase MAX_ITEMS.\n",
                        map_index_ini, i);
                return 0;
            }
            items[nitems].type = SPRITE;
            items[nitems].sprite.height = BLOCK_SIZE;
            items[nitems].sprite.width = BLOCK_SIZE;
            items[nitems].sprite.pos_x = (i - 1) * BLOCK_SIZE - map_x;
            items[nitems].sprite.pos_y = j * BLOCK_SIZE;
            // Get element by type & map_id
            switch (element) {
                case 1: // Non-breakable env. blocks
                    switch (map_id) {
                        case 1: items[nitems].sprite.data = shire_block_1; break;
                        case 2: items[nitems].sprite.data = moria_block_1; break;
                        default: break;
                    }
                    break;
                case 2: // Breakable env. blocks
                    // Check if block has been destroyed
                    switch (map_id) {
                        case 1: items[nitems].sprite.data = shire_block_2; break;
                        case 2: items[nitems].sprite.data = moria_block_2; break;
                        default: break;
                    }
                    break;
                case 3: // Block with object/coins inside
                    // Check if object has been obtained
                    switch (map_id) {
                        case 2: items[nitems].sprite.data = moria_block_3; break;
                        default: break;
                    }
                    break;
                default: break;
            }
            nitems++;
        }
    }
    return nitems;
}