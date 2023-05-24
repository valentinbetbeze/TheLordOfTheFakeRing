#include "game_engine.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "musics.h"


void feed_watchdog_timer(void)
{
    TIMERG0.wdtwprotect.wdt_wkey = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdtfeed.wdt_feed = 1;
    TIMERG0.wdtwprotect.wdt_wkey = 0;
}

/*************************************************
 * Resets
 *************************************************/

void reset_player(game_t *game, player_t *player)
{
    if (game == NULL) {
        printf("Error(reset_player): game_t pointer is NULL.\n");
        assert(game);
    }
    if (player == NULL) {
        printf("Error(reset_player): player_t pointer is NULL.\n");
        assert(player);
    }
    player->shield                       = 0;
    player->lightstaff                   = 0;
    player->forward                      = 1;
    player->power_used                   = 0;
    player->spell_radius                 = 0;
    player->timer_x                      = 0;
    player->timer_y                      = 0;
    player->coins                        = 0;
    player->physics.pos_x                = game->map->start_row * BLOCK_SIZE;
    player->physics.pos_y                = (NUM_BLOCKS_Y - game->map->start_column - 1) * BLOCK_SIZE;
    player->physics.speed_x              = SPEED_INITIAL;
    player->physics.speed_y              = SPEED_INITIAL;
    player->physics.falling              = 0;
    player->physics.jumping              = 0;
    player->physics.top_collision        = 0;
    player->physics.bottom_collision     = 0;
    player->physics.left_collision       = 0;
    player->physics.right_collision      = 0;
}


void reset_game_flags(game_t *game)
{
    if (game == NULL) {
        printf("Error(reset_game_flags): game_t pointer is NULL.\n");
        assert(game);
    }
    game->cam_moving = 0;
    game->cam_pos_x = 0;
    game->cam_row = 0;
    game->init = 0;
    game->reset = 0;
}


void reset_hit_flag_blocks(void)
{
    for (uint8_t i = 0; i < NUM_BLOCK_RECORDS; i++) {
        blocks[i].is_hit = 0;
    }
}


void reset_records(void)
{
    // Blocks
    for (int i = 0; i < NUM_BLOCK_RECORDS; i++) {
        blocks[i].row          = -1; // Empty slot identifier
        blocks[i].column       = -1;
        blocks[i].is_hit       = 0;
        blocks[i].destroyed    = 0;
        blocks[i].item_given   = 0;
        blocks[i].bumping      = 0;
    }
    // Enemies
    for (uint8_t i = 0; i < NUM_ENEMY_RECORDS; i++) {
        memset(&enemies[i], 0, sizeof(enemies[i]));
    }
    // Items
    for (uint8_t i = 0; i < NUM_ITEMS; i++) {
        memset(&items[i], 0, sizeof(items[i]));
    }
    // Projectiles
    for (uint8_t i = 0; i < MAX_PROJECTILES; i++) {
        memset(&items[i], 0, sizeof(items[i]));
    }
    // Platforms
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        platforms[i].start_row = -1;
        platforms[i].start_column = -1;
        platforms[i].end_row = platforms[i].start_row;
        platforms[i].end_column = platforms[i].start_column;
    }
}


/*************************************************
 * Music
 *************************************************/

void flush_music(music_t *music)
{
    if (music == NULL) {
        /* Not an error, it can happen as an empty music_t pointer means 
        that there is no music to play */
        return;
    }
    music->timer = 0;
    music->playing = 0;
    music->note_index = 0;
}


void cue_music(music_t **music, block_t *block, const int8_t block_type)
{
    if (music == NULL) {
        printf("Error(cue_music): music_t pointer does not exist (no adress).\n");
        assert(music);
    }
    block_t empty_block = {0};
    if (block == NULL) {
        block = &empty_block;
    }
    // Flush the previous music
    flush_music(*music);
    // Cue the new music
    if (IS_ENEMY(block_type)) {
        *music = &music_enemy;
    }
    else if (block_type == BREAKABLE_BLOCK) {
        *music = &music_brkl_block;
    }
    else if (block_type == BONUS_BLOCK && !block->item_given) {
        *music = &music_bnus_block;
    }
    else if (block_type == RING) {
        *music = &music_ring;
    }
    else if (IS_SOLID(block_type) || (block_type == BONUS_BLOCK && block->item_given)) {
        // Unbreakable block types remaining
        *music = &music_unbr_block;
    }
}


uint8_t play_music(const game_t *game, music_t *music)
{
    if (music == NULL) {
        printf("Error(play_music): music_t pointer is NULL.\n");
        assert(music);
    }
    if (music->data == NULL) {
        printf("Error(play_music): music.data pointer is NULL.\n");
        assert(music->data);
    }
    if (game == NULL) {
        printf("Error(play_music): game_t pointer is NULL.\n");
        assert(game);
    }
    // Check music state
    if (!music->playing) {
        //printf("tag\n");
        music->playing = 1;
        music->timer = game->timer;     // Initialize music timer
        if (music->note_index != 0) {
            printf("Error(play_music): note_index must be set to 0 before playing a music.\n");
            assert(music->note_index != 0);
        }
        mhfmd_set_frequency(music->data[music->note_index]);
        mhfmd_set_buzzer(1);            // Switch the buzzer on
    }
    // Change the note if the previous note duration has expired
    if ((uint64_t)(game->timer - music->timer) / music->duration) {
        mhfmd_set_buzzer(0);
        // Music is fully played
        if (music->note_index == music->num_notes - 1) {
            flush_music(music);
            return 1;
        }
        // If next frequency is non-null, play it
        if (music->data[++music->note_index]) {
            mhfmd_set_frequency(music->data[music->note_index]);
            mhfmd_set_buzzer(1);
        }
        music->timer = (uint32_t)game->timer;
    }
    return 0;
}
