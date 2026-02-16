#ifndef EDITOR_H
#define EDITOR_H

#include "../../src/game.h"
#include "../../src/config.h"
#include "raylib.h"
#include <stdbool.h>

#define EDITOR_MAX_UNDO 256
#define EDITOR_PALETTE_COLS 4
#define EDITOR_PANEL_WIDTH 220
#define EDITOR_MAX_LEVELS 64
#define EDITOR_LEVEL_NAME_MAX 64

#define SPRITE_SHEET_ROWS 13
#define SPRITE_TOTAL_TILES (SPRITE_SHEET_COLS * SPRITE_SHEET_ROWS)

typedef enum {
    MODE_EDIT,
    MODE_BROWSE
} EditorMode;

typedef struct {
    i32 x, y;
    i32 old_tile;
    i32 old_type;
    i32 new_tile;
    i32 new_type;
} EditorAction;

typedef struct {
    char filename[256];
    char name[EDITOR_LEVEL_NAME_MAX];
} LevelEntry;

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

    bool show_grid;
    bool modified;

    Texture2D spritesheet;
    Font font;

    bool dragging;
    i32 drag_start_x, drag_start_y;
    i32 last_paint_x, last_paint_y;

    // Spritesheet palette scroll
    i32 palette_scroll;

    // Level browser
    EditorMode mode;
    LevelEntry level_list[EDITOR_MAX_LEVELS];
    i32 level_count;
    i32 browse_scroll;
} EditorState;

void InitEditor(EditorState *ed);
void UpdateEditor(EditorState *ed);
void DrawEditor(const EditorState *ed);
void CleanupEditor(EditorState *ed);

void EditorNewMap(EditorState *ed);
bool EditorSaveMap(EditorState *ed, const char *filename);
bool EditorLoadMap(EditorState *ed, const char *filename);

void EditorUndo(EditorState *ed);
void EditorRedo(EditorState *ed);
void EditorPushAction(EditorState *ed, EditorAction action);

void EditorPaintTile(EditorState *ed, i32 grid_x, i32 grid_y);
void EditorFloodFill(EditorState *ed, i32 grid_x, i32 grid_y);
void EditorRebuildSpawnBase(EditorState *ed);

void EditorScanLevels(EditorState *ed);

#endif
