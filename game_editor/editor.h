#ifndef EDITOR_H
#define EDITOR_H

#include "game.h"
#include "config.h"
#include "raylib.h"
#include <stdbool.h>

#define EDITOR_MAX_UNDO 256
#define EDITOR_PALETTE_COLS 4
#define EDITOR_PANEL_WIDTH 220
#define EDITOR_MAX_FILES 64
#define EDITOR_LEVEL_NAME_MAX 64

#define SPRITE_SHEET_ROWS 13
#define SPRITE_TOTAL_TILES (SPRITE_SHEET_COLS * SPRITE_SHEET_ROWS)

typedef enum {
    EDITOR_SECTION_LEVELS,
    EDITOR_SECTION_ENEMIES,
    EDITOR_SECTION_TURRETS
} EditorSection;

typedef enum {
    MODE_EDIT,
    MODE_BROWSE
} EditorMode;

typedef struct {
    i32 x, y;
    bool is_background;
    i32 old_tile;
    i32 old_type;
    i32 new_tile;
    i32 new_type;
} EditorAction;

typedef struct {
    char filename[256];
    char name[EDITOR_LEVEL_NAME_MAX];
} EditorFileEntry;

typedef struct {
    char name[CONFIG_NAME_LEN];
    i32 health;
    f32 speed;
    i32 reward;
    i32 damage_to_base;
    i32 sprite_id;
    i32 overlay_sprite_id;
} EditorEnemyEntry;

typedef struct {
    char name[CONFIG_NAME_LEN];
    i32 damage;
    f32 fire_rate;
    f32 range;
    i32 cost;
    i32 base_sprite_id;
    i32 gun_sprite_id;
    i32 shop_slot;
} EditorTurretEntry;

typedef struct {
    Map map;

    i32 selected_sprite;
    TileType selected_type;

    EditorAction undo_stack[EDITOR_MAX_UNDO];
    i32 undo_count;
    i32 undo_head;

    char filename[256];
    char level_name[EDITOR_LEVEL_NAME_MAX];
    bool editing_name;

    char enemies_file[256];
    char turrets_file[256];
    char wave_lines[MAX_LEVEL_WAVES][256];
    i32 wave_count;

    bool show_grid;
    bool paint_background;
    bool erase_mode;
    bool modified;

    Texture2D spritesheet;
    Font font;

    i32 last_paint_x, last_paint_y;

    i32 palette_scroll;

    EditorMode mode;
    EditorSection section;
    EditorFileEntry file_list[EDITOR_MAX_FILES];
    i32 file_count;

    EditorEnemyEntry enemy_entries[ENEMY_TYPE_COUNT];
    char enemy_filename[256];
    char enemy_config_name[EDITOR_LEVEL_NAME_MAX];
    i32 enemy_selected_row;
    i32 enemy_selected_col;

    EditorTurretEntry turret_entries[TOWER_TYPE_COUNT];
    char turret_filename[256];
    char turret_config_name[EDITOR_LEVEL_NAME_MAX];
    i32 turret_selected_row;
    i32 turret_selected_col;
} EditorState;

void InitEditor(EditorState *ed);
void UpdateEditor(EditorState *ed);
void DrawEditor(const EditorState *ed);
void CleanupEditor(EditorState *ed);

void EditorNewMap(EditorState *ed);
bool EditorSaveMap(EditorState *ed, const char *filename);
bool EditorLoadMap(EditorState *ed, const char *filename);

void EditorNewEnemies(EditorState *ed);
bool EditorSaveEnemies(EditorState *ed, const char *filename);
bool EditorLoadEnemies(EditorState *ed, const char *filename);

void EditorNewTurrets(EditorState *ed);
bool EditorSaveTurrets(EditorState *ed, const char *filename);
bool EditorLoadTurrets(EditorState *ed, const char *filename);

void EditorUndo(EditorState *ed);
void EditorRedo(EditorState *ed);
void EditorPushAction(EditorState *ed, EditorAction action);

void EditorPaintTile(EditorState *ed, i32 grid_x, i32 grid_y);
void EditorFloodFill(EditorState *ed, i32 grid_x, i32 grid_y);
void EditorRebuildSpawnBase(EditorState *ed);

void EditorScanFiles(EditorState *ed);

#endif
