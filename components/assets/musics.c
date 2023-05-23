#include "musics.h"


// Start menu music
const uint16_t intro_data[] = {
    NOTE_B6, NOTE_B6, NOTE_B6, NOTE_B6,
    NOTE_C7, NOTE_C7, NOTE_C7, NOTE_C7,
    NOTE_C7, NOTE_C7, NOTE_B6, NOTE_C7,
    NOTE_B6, NOTE_A6, NOTE_C7, NOTE_B6,
    NOTE_B6, NOTE_B6, NOTE_B6, NOTE_E6, 
    NOTE_E6, NOTE_E6, NOTE_E6,
    0, 0, 0, 0,
    NOTE_B6, NOTE_B6, NOTE_B6, NOTE_B6,
    NOTE_C7, NOTE_C7, NOTE_C7, NOTE_C7,
    NOTE_C7, NOTE_C7, NOTE_B6, NOTE_C7,
    NOTE_B6, NOTE_A6, NOTE_C7, NOTE_B6,
    NOTE_B6, NOTE_B6, NOTE_B6,
};
music_t music_intro = {
    .duration = 250,
    .num_notes = (uint16_t)sizeof(intro_data) / sizeof(intro_data[0]),
    .data = intro_data
};


// Unbreakable block sound
const uint16_t unbr_block_data[] = {
    NOTE_GS2, NOTE_D3
};
music_t music_unbr_block = {
    .duration = 50,
    .num_notes = (uint16_t)sizeof(unbr_block_data) / sizeof(unbr_block_data[0]),
    .data = unbr_block_data
};


// Breakable block sound
const uint16_t brkl_block_data[] = {
    NOTE_A4, 0, NOTE_A4, 0
};
music_t music_brkl_block = {
    .duration = 25,
    .num_notes = (uint16_t)sizeof(brkl_block_data) / sizeof(brkl_block_data[0]),
    .data = brkl_block_data
};


// Bonus block sound
const uint16_t bnus_block_data[] = {
    NOTE_E4, NOTE_F4, NOTE_G4,
    NOTE_A4, 0, NOTE_GS5,
};
music_t music_bnus_block = {
    .duration = 25,
    .num_notes = (uint16_t)sizeof(bnus_block_data) / sizeof(bnus_block_data[0]),
    .data = bnus_block_data
};


// Enemy kill sound
const uint16_t enemy_data[] = {
    NOTE_A5, 0,
    NOTE_A4, 0
};
music_t music_enemy = {
    .duration = 50,
    .num_notes = (uint16_t)sizeof(enemy_data) / sizeof(enemy_data[0]),
    .data = enemy_data
};


const uint16_t glamdring_blast_data[] = {
    NOTE_DS6, NOTE_E6, 0, 0,
    0, 0, NOTE_CS4, NOTE_C3
};
music_t music_glamdring_blast = {
    .duration = 100,
    .num_notes = (uint16_t)sizeof(glamdring_blast_data) / sizeof(glamdring_blast_data[0]),
    .data = glamdring_blast_data
};


// Ring sound
const uint16_t ring_data[] = {
    NOTE_E5, NOTE_E6, 0,
    NOTE_A6, 0, 0,
    NOTE_D7, 0, 0,
    NOTE_G7, 0, 0,
    NOTE_C8
};
music_t music_ring = {
    .duration = 50,
    .num_notes = (uint16_t)sizeof(ring_data) / sizeof(ring_data[0]),
    .data = ring_data
};