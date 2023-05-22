#include "musics.h"

// Start menu music
const uint16_t intro_data[] = {
    500, 500, 900, 900, 900, 900, 500, 900, 500, 900, 500, 900,
    900, 900, 150, 150, 150
};

music_t music_intro = {
    .duration = 100,
    .num_notes = (uint16_t)sizeof(intro_data) / sizeof(intro_data[0]),
    .data = intro_data
};


// Unbreakable block sound
const uint16_t unbr_block_data[] = {
    100
};
music_t music_unbr_block = {
    .duration = 50,
    .num_notes = (uint16_t)sizeof(unbr_block_data) / sizeof(unbr_block_data[0]),
    .data = unbr_block_data
};


// Breakable block sound
const uint16_t brkl_block_data[] = {
    50
};
music_t music_brkl_block = {
    .duration = 50,
    .num_notes = (uint16_t)sizeof(brkl_block_data) / sizeof(brkl_block_data[0]),
    .data = brkl_block_data
};


// Bonus block sound
const uint16_t bnus_block_data[] = {
     170, 60
};
music_t music_bnus_block = {
    .duration = 25,
    .num_notes = (uint16_t)sizeof(bnus_block_data) / sizeof(bnus_block_data[0]),
    .data = bnus_block_data
};


// Ring sound
const uint16_t ring_data[] = {
    1500, 2000, 2500, 3000
};
music_t music_ring = {
    .duration = 20,
    .num_notes = (uint16_t)sizeof(ring_data) / sizeof(ring_data[0]),
    .data = ring_data
};


// Enemy kill sound
const uint16_t enemy_data[] = {
    50, 100, 150
};
music_t music_enemy = {
    .duration = 30,
    .num_notes = (uint16_t)sizeof(enemy_data) / sizeof(enemy_data[0]),
    .data = enemy_data
};