#ifndef EDITOR_H
#define EDITOR_H

#include "../../src/game.h"
#include "../../src/config.h"
#include "raylib.h"
#include <stdbool.h>

#define EDITOR_MAX_UNDO 256
#define EDITOR_PALETTE_COLS 4
#define EDITOR_PANEL_WIDTH 200

typedef struct {
    i32 x, y;
    i32 old_tile;
    i32 old_type;
    i32 new_tile;
    i32 new_type;
} EditorAction;

typedef struct {
    Map map;
    
    i32 selected_tile;
    TileType selected_type;
    
    EditorAction undo_stack[EDITOR_MAX_UNDO];
    i32 undo_count;
    i32 undo_head;
    
    char filename[256];
    bool show_grid;
    bool modified;
    
    Texture2D spritesheet;
    Font font;
    
    bool dragging;
    i32 drag_start_x, drag_start_y;
    i32 last_paint_x, last_paint_y;
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

#endif
