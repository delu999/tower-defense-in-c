#include "editor.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define EDITOR_SCREEN_WIDTH 1280
#define EDITOR_SCREEN_HEIGHT 720
#define EDITOR_MAP_OFFSET_X EDITOR_PANEL_WIDTH
#define EDITOR_MAP_OFFSET_Y 40

static const char *TILE_TYPE_NAMES[] = {
    [TILE_GROUND]    = "Ground",
    [TILE_BUILDABLE] = "Buildable",
    [TILE_BLOCKED]   = "Blocked",
    [TILE_SPAWN]     = "Spawn",
    [TILE_BASE]      = "Base"
};

typedef struct {
    i32 sprite_id;
    TileType type;
    const char *name;
} PaletteEntry;

static const PaletteEntry PALETTE[] = {
    { SPRITE_GROUND,      TILE_GROUND,    "Ground" },
    { SPRITE_GRASS,       TILE_BLOCKED,   "Grass" },
    { SPRITE_TREE,        TILE_BLOCKED,   "Tree" },
    { SPRITE_ROCK,        TILE_BLOCKED,   "Rock" },
    { SPRITE_SAND,        TILE_BUILDABLE, "Sand" },
    { TILE_NUM(100),      TILE_BUILDABLE, "Buildable" },
    { TILE_NUM(0),        TILE_SPAWN,     "Spawn" },
    { TILE_NUM(1),        TILE_BASE,      "Base" },
};
#define PALETTE_COUNT (i32)(sizeof(PALETTE) / sizeof(PALETTE[0]))

static void DrawSpriteAtGrid(const EditorState *ed, i32 sprite_id, i32 grid_x, i32 grid_y) {
    i32 src_x = (sprite_id % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
    i32 src_y = (sprite_id / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
    Rectangle src = {src_x, src_y, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
    Rectangle dest = {
        EDITOR_MAP_OFFSET_X + grid_x * TILE_SIZE,
        EDITOR_MAP_OFFSET_Y + grid_y * TILE_SIZE,
        TILE_SIZE, TILE_SIZE
    };
    DrawTexturePro(ed->spritesheet, src, dest, (Vector2){0, 0}, 0, WHITE);
}

static void DrawEditorGrid(const EditorState *ed) {
    for (i32 y = 0; y <= ed->map.height; y++) {
        i32 py = EDITOR_MAP_OFFSET_Y + y * TILE_SIZE;
        DrawLine(EDITOR_MAP_OFFSET_X, py, 
                 EDITOR_MAP_OFFSET_X + ed->map.width * TILE_SIZE, py, 
                 ColorAlpha(GRAY, 0.3f));
    }
    for (i32 x = 0; x <= ed->map.width; x++) {
        i32 px = EDITOR_MAP_OFFSET_X + x * TILE_SIZE;
        DrawLine(px, EDITOR_MAP_OFFSET_Y, 
                 px, EDITOR_MAP_OFFSET_Y + ed->map.height * TILE_SIZE, 
                 ColorAlpha(GRAY, 0.3f));
    }
}

static void DrawPalette(const EditorState *ed) {
    i32 panel_x = 0;
    i32 panel_y = EDITOR_MAP_OFFSET_Y;
    i32 tile_size = 48;
    
    DrawRectangle(panel_x, 0, EDITOR_PANEL_WIDTH, EDITOR_SCREEN_HEIGHT, (Color){40, 40, 40, 255});
    DrawRectangle(panel_x, 0, EDITOR_PANEL_WIDTH, EDITOR_MAP_OFFSET_Y, (Color){60, 60, 60, 255});
    DrawTextEx(ed->font, "Palette", (Vector2){10, 10}, 20, 1, WHITE);
    
    for (i32 i = 0; i < PALETTE_COUNT; i++) {
        i32 col = i % EDITOR_PALETTE_COLS;
        i32 row = i / EDITOR_PALETTE_COLS;
        i32 tx = panel_x + 10 + col * (tile_size + 4);
        i32 ty = panel_y + 10 + row * (tile_size + 4);
        
        Rectangle rect = {tx, ty, tile_size, tile_size};
        
        if (i == ed->selected_tile) {
            DrawRectangleLinesEx(rect, 3, YELLOW);
        } else {
            DrawRectangleLinesEx(rect, 1, GRAY);
        }
        
        i32 src_x = (PALETTE[i].sprite_id % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
        i32 src_y = (PALETTE[i].sprite_id / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
        Rectangle src = {src_x, src_y, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
        Rectangle dest = {tx, ty, tile_size, tile_size};
        DrawTexturePro(ed->spritesheet, src, dest, (Vector2){0, 0}, 0, WHITE);
        
        if (PALETTE[i].type == TILE_SPAWN) {
            DrawTextEx(ed->font, "S", (Vector2){tx + tile_size - 14, ty + 2}, 16, 1, GREEN);
        } else if (PALETTE[i].type == TILE_BASE) {
            DrawTextEx(ed->font, "B", (Vector2){tx + tile_size - 14, ty + 2}, 16, 1, RED);
        }
    }
    
    i32 info_y = panel_y + 10 + ((PALETTE_COUNT + EDITOR_PALETTE_COLS - 1) / EDITOR_PALETTE_COLS) * (tile_size + 4) + 20;
    DrawTextEx(ed->font, TextFormat("Selected: %s", PALETTE[ed->selected_tile].name), 
               (Vector2){10, info_y}, 16, 1, LIGHTGRAY);
    DrawTextEx(ed->font, TextFormat("Type: %s", TILE_TYPE_NAMES[PALETTE[ed->selected_tile].type]), 
               (Vector2){10, info_y + 20}, 16, 1, LIGHTGRAY);
    
    DrawTextEx(ed->font, "Controls:", (Vector2){10, info_y + 60}, 16, 1, YELLOW);
    DrawTextEx(ed->font, "Click/Drag: Paint", (Vector2){10, info_y + 80}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, "Right-click: Flood fill", (Vector2){10, info_y + 96}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, "G: Toggle grid", (Vector2){10, info_y + 112}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, "Ctrl+S: Save", (Vector2){10, info_y + 128}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, "Ctrl+L: Load", (Vector2){10, info_y + 144}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, "Ctrl+N: New", (Vector2){10, info_y + 160}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, "Ctrl+Z: Undo", (Vector2){10, info_y + 176}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, "Ctrl+Y: Redo", (Vector2){10, info_y + 192}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, "Esc: Quit", (Vector2){10, info_y + 208}, 14, 1, LIGHTGRAY);
}

static void DrawStatusBar(const EditorState *ed) {
    i32 bar_y = EDITOR_SCREEN_HEIGHT - 30;
    DrawRectangle(0, bar_y, EDITOR_SCREEN_WIDTH, 30, (Color){30, 30, 30, 255});
    
    char status[512];
    snprintf(status, sizeof(status), "Map: %dx%d | File: %s | %s", 
             ed->map.width, ed->map.height, 
             ed->filename[0] ? ed->filename : "(unsaved)",
             ed->modified ? "[Modified]" : "");
    DrawTextEx(ed->font, status, (Vector2){10, bar_y + 6}, 14, 1, LIGHTGRAY);
    
    char undo_status[64];
    snprintf(undo_status, sizeof(undo_status), "Undo: %d | Redo: %d", 
             ed->undo_head, ed->undo_count - ed->undo_head);
    DrawTextEx(ed->font, undo_status, (Vector2){EDITOR_SCREEN_WIDTH - 150, bar_y + 6}, 14, 1, LIGHTGRAY);
}

static void DrawToolBar(const EditorState *ed) {
    DrawRectangle(EDITOR_PANEL_WIDTH, 0, EDITOR_SCREEN_WIDTH - EDITOR_PANEL_WIDTH, EDITOR_MAP_OFFSET_Y, (Color){50, 50, 50, 255});
    
    DrawTextEx(ed->font, "Tower Defense Level Editor", (Vector2){EDITOR_PANEL_WIDTH + 10, 10}, 20, 1, WHITE);
    
    i32 map_info_x = EDITOR_SCREEN_WIDTH - 250;
    DrawTextEx(ed->font, TextFormat("Spawns: %d | Bases: %d", ed->map.spawn_count, ed->map.base_count), 
               (Vector2){map_info_x, 10}, 16, 1, LIGHTGRAY);
}

void InitEditor(EditorState *ed) {
    memset(ed, 0, sizeof(EditorState));
    
    ed->show_grid = true;
    ed->selected_tile = 0;
    ed->selected_type = PALETTE[0].type;
    ed->modified = false;
    
    EditorNewMap(ed);
    
    ed->spritesheet = LoadTexture("assets/sprites/towerDefense_tilesheet@2.png");
    if (ed->spritesheet.id == 0) {
        printf("Failed to load spritesheet!\n");
    }
    SetTextureFilter(ed->spritesheet, TEXTURE_FILTER_BILINEAR);
    
    ed->font = LoadFontEx("assets/fonts/Poppins-Regular.ttf", 96, 0, 0);
    if (ed->font.texture.id == 0) {
        printf("Failed to load font!\n");
    }
    SetTextureFilter(ed->font.texture, TEXTURE_FILTER_BILINEAR);
    
    printf("Editor initialized.\n");
}

void CleanupEditor(EditorState *ed) {
    if (ed->spritesheet.id != 0) {
        UnloadTexture(ed->spritesheet);
    }
    if (ed->font.texture.id != 0) {
        UnloadFont(ed->font);
    }
}

void EditorNewMap(EditorState *ed) {
    memset(&ed->map, 0, sizeof(Map));
    ed->map.width = MAP_WIDTH;
    ed->map.height = MAP_HEIGHT;
    
    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            ed->map.tiles[y][x] = SPRITE_GROUND;
            ed->map.cell_types[y][x] = TILE_BUILDABLE;
        }
    }
    
    ed->undo_count = 0;
    ed->undo_head = 0;
    ed->filename[0] = '\0';
    ed->modified = true;
}

void EditorPushAction(EditorState *ed, EditorAction action) {
    if (ed->undo_head < ed->undo_count) {
        ed->undo_count = ed->undo_head + 1;
    }
    
    if (ed->undo_count >= EDITOR_MAX_UNDO) {
        memmove(ed->undo_stack, ed->undo_stack + 1, sizeof(EditorAction) * (EDITOR_MAX_UNDO - 1));
        ed->undo_count = EDITOR_MAX_UNDO - 1;
        ed->undo_head = ed->undo_count;
    }
    
    ed->undo_stack[ed->undo_count] = action;
    ed->undo_count++;
    ed->undo_head = ed->undo_count;
    ed->modified = true;
}

void EditorUndo(EditorState *ed) {
    if (ed->undo_head > 0) {
        ed->undo_head--;
        EditorAction *a = &ed->undo_stack[ed->undo_head];
        ed->map.tiles[a->y][a->x] = a->old_tile;
        ed->map.cell_types[a->y][a->x] = a->old_type;
        ed->modified = true;
        
        if (a->old_type == TILE_SPAWN || ed->map.cell_types[a->y][a->x] == TILE_SPAWN) {
            EditorRebuildSpawnBase(ed);
        }
    }
}

void EditorRedo(EditorState *ed) {
    if (ed->undo_head < ed->undo_count) {
        EditorAction *a = &ed->undo_stack[ed->undo_head];
        ed->map.tiles[a->y][a->x] = a->new_tile;
        ed->map.cell_types[a->y][a->x] = a->new_type;
        ed->undo_head++;
        ed->modified = true;
        
        if (a->new_type == TILE_SPAWN || a->new_type == TILE_BASE) {
            EditorRebuildSpawnBase(ed);
        }
    }
}

void EditorRebuildSpawnBase(EditorState *ed) {
    ed->map.spawn_count = 0;
    ed->map.base_count = 0;
    
    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            if (ed->map.cell_types[y][x] == TILE_SPAWN && ed->map.spawn_count < 16) {
                ed->map.spawn_points[ed->map.spawn_count++] = (Vector2){x, y};
            } else if (ed->map.cell_types[y][x] == TILE_BASE && ed->map.base_count < 16) {
                ed->map.base_points[ed->map.base_count++] = (Vector2){x, y};
            }
        }
    }
}

void EditorPaintTile(EditorState *ed, i32 grid_x, i32 grid_y) {
    if (grid_x < 0 || grid_x >= ed->map.width || grid_y < 0 || grid_y >= ed->map.height) return;
    if (grid_x == ed->last_paint_x && grid_y == ed->last_paint_y) return;
    
    i32 new_tile = PALETTE[ed->selected_tile].sprite_id;
    TileType new_type = PALETTE[ed->selected_tile].type;
    
    if (ed->map.tiles[grid_y][grid_x] == new_tile && 
        ed->map.cell_types[grid_y][grid_x] == new_type) return;
    
    EditorAction action = {
        .x = grid_x, .y = grid_y,
        .old_tile = ed->map.tiles[grid_y][grid_x],
        .old_type = ed->map.cell_types[grid_y][grid_x],
        .new_tile = new_tile,
        .new_type = new_type
    };
    
    ed->map.tiles[grid_y][grid_x] = new_tile;
    ed->map.cell_types[grid_y][grid_x] = new_type;
    
    EditorPushAction(ed, action);
    ed->last_paint_x = grid_x;
    ed->last_paint_y = grid_y;
    
    if (new_type == TILE_SPAWN || new_type == TILE_BASE) {
        EditorRebuildSpawnBase(ed);
    }
}

static void FloodFillRecursive(EditorState *ed, i32 x, i32 y, i32 target_tile, TileType target_type, 
                               i32 replace_tile, TileType replace_type) {
    if (x < 0 || x >= ed->map.width || y < 0 || y >= ed->map.height) return;
    if (ed->map.tiles[y][x] != target_tile || ed->map.cell_types[y][x] != target_type) return;
    if (target_tile == replace_tile && target_type == replace_type) return;
    
    EditorAction action = {
        .x = x, .y = y,
        .old_tile = target_tile,
        .old_type = target_type,
        .new_tile = replace_tile,
        .new_type = replace_type
    };
    EditorPushAction(ed, action);
    
    ed->map.tiles[y][x] = replace_tile;
    ed->map.cell_types[y][x] = replace_type;
    
    FloodFillRecursive(ed, x + 1, y, target_tile, target_type, replace_tile, replace_type);
    FloodFillRecursive(ed, x - 1, y, target_tile, target_type, replace_tile, replace_type);
    FloodFillRecursive(ed, x, y + 1, target_tile, target_type, replace_tile, replace_type);
    FloodFillRecursive(ed, x, y - 1, target_tile, target_type, replace_tile, replace_type);
}

void EditorFloodFill(EditorState *ed, i32 grid_x, i32 grid_y) {
    if (grid_x < 0 || grid_x >= ed->map.width || grid_y < 0 || grid_y >= ed->map.height) return;
    
    i32 target_tile = ed->map.tiles[grid_y][grid_x];
    TileType target_type = ed->map.cell_types[grid_y][grid_x];
    i32 replace_tile = PALETTE[ed->selected_tile].sprite_id;
    TileType replace_type = PALETTE[ed->selected_tile].type;
    
    FloodFillRecursive(ed, grid_x, grid_y, target_tile, target_type, replace_tile, replace_type);
    
    EditorRebuildSpawnBase(ed);
}

bool EditorSaveMap(EditorState *ed, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Failed to open file for writing: %s\n", filename);
        return false;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"width\": %d,\n", ed->map.width);
    fprintf(f, "  \"height\": %d,\n", ed->map.height);
    
    fprintf(f, "  \"tiles\": [\n");
    for (i32 y = 0; y < ed->map.height; y++) {
        fprintf(f, "    [");
        for (i32 x = 0; x < ed->map.width; x++) {
            fprintf(f, "%d", ed->map.tiles[y][x]);
            if (x < ed->map.width - 1) fprintf(f, ", ");
        }
        fprintf(f, "]");
        if (y < ed->map.height - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    fprintf(f, "  ],\n");
    
    fprintf(f, "  \"cell_types\": [\n");
    for (i32 y = 0; y < ed->map.height; y++) {
        fprintf(f, "    [");
        for (i32 x = 0; x < ed->map.width; x++) {
            fprintf(f, "%d", ed->map.cell_types[y][x]);
            if (x < ed->map.width - 1) fprintf(f, ", ");
        }
        fprintf(f, "]");
        if (y < ed->map.height - 1) fprintf(f, ",");
        fprintf(f, "\n");
    }
    fprintf(f, "  ],\n");
    
    fprintf(f, "  \"spawn_points\": [");
    for (i32 i = 0; i < ed->map.spawn_count; i++) {
        fprintf(f, "[%d, %d]", (i32)ed->map.spawn_points[i].x, (i32)ed->map.spawn_points[i].y);
        if (i < ed->map.spawn_count - 1) fprintf(f, ", ");
    }
    fprintf(f, "],\n");
    
    fprintf(f, "  \"base_points\": [");
    for (i32 i = 0; i < ed->map.base_count; i++) {
        fprintf(f, "[%d, %d]", (i32)ed->map.base_points[i].x, (i32)ed->map.base_points[i].y);
        if (i < ed->map.base_count - 1) fprintf(f, ", ");
    }
    fprintf(f, "]\n");
    
    fprintf(f, "}\n");
    
    fclose(f);
    strncpy(ed->filename, filename, sizeof(ed->filename) - 1);
    ed->modified = false;
    printf("Map saved to: %s\n", filename);
    return true;
}

static bool ParseJsonInt(const char *json, const char *key, i32 *value) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char *pos = strstr(json, pattern);
    if (!pos) return false;
    pos += strlen(pattern);
    while (*pos == ' ' || *pos == '\n' || *pos == '\r' || *pos == '\t') pos++;
    *value = atoi(pos);
    return true;
}

static bool ParseJsonArray2D(const char *json, const char *key, i32 (*arr)[MAP_WIDTH], i32 height, i32 width) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\": [", key);
    const char *pos = strstr(json, pattern);
    if (!pos) return false;
    
    pos += strlen(pattern);
    
    for (i32 y = 0; y < height; y++) {
        while (*pos != '[' && *pos != '\0') pos++;
        if (*pos == '[') pos++;
        
        for (i32 x = 0; x < width; x++) {
            while (*pos == ' ' || *pos == '\n' || *pos == '\r' || *pos == '\t') pos++;
            arr[y][x] = atoi(pos);
            while (*pos != ',' && *pos != ']' && *pos != '\0') pos++;
            if (*pos == ',') pos++;
        }
        
        while (*pos != ']' && *pos != '\0') pos++;
        if (*pos == ']') pos++;
        while (*pos != ',' && *pos != ']' && *pos != '\0') pos++;
        if (*pos == ',') pos++;
    }
    
    return true;
}

bool EditorLoadMap(EditorState *ed, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Failed to open file for reading: %s\n", filename);
        return false;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *json = malloc(size + 1);
    if (!json) {
        fclose(f);
        return false;
    }
    
    fread(json, 1, size, f);
    json[size] = '\0';
    fclose(f);
    
    memset(&ed->map, 0, sizeof(Map));
    
    ParseJsonInt(json, "width", &ed->map.width);
    ParseJsonInt(json, "height", &ed->map.height);
    ParseJsonArray2D(json, "tiles", ed->map.tiles, ed->map.height, ed->map.width);
    ParseJsonArray2D(json, "cell_types", (i32 (*)[MAP_WIDTH])ed->map.cell_types, ed->map.height, ed->map.width);
    
    EditorRebuildSpawnBase(ed);
    
    free(json);
    
    strncpy(ed->filename, filename, sizeof(ed->filename) - 1);
    ed->undo_count = 0;
    ed->undo_head = 0;
    ed->modified = false;
    printf("Map loaded from: %s\n", filename);
    return true;
}

void UpdateEditor(EditorState *ed) {
    Vector2 mouse = GetMousePosition();
    i32 grid_x = (i32)((mouse.x - EDITOR_MAP_OFFSET_X) / TILE_SIZE);
    i32 grid_y = (i32)((mouse.y - EDITOR_MAP_OFFSET_Y) / TILE_SIZE);
    
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        if (mouse.x >= EDITOR_PANEL_WIDTH && mouse.y >= EDITOR_MAP_OFFSET_Y) {
            EditorPaintTile(ed, grid_x, grid_y);
        }
    } else {
        ed->last_paint_x = -1;
        ed->last_paint_y = -1;
    }
    
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        if (mouse.x >= EDITOR_PANEL_WIDTH && mouse.y >= EDITOR_MAP_OFFSET_Y) {
            EditorFloodFill(ed, grid_x, grid_y);
        }
    }
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse.x < EDITOR_PANEL_WIDTH) {
        i32 tile_size = 48;
        i32 panel_y = EDITOR_MAP_OFFSET_Y;
        
        for (i32 i = 0; i < PALETTE_COUNT; i++) {
            i32 col = i % EDITOR_PALETTE_COLS;
            i32 row = i / EDITOR_PALETTE_COLS;
            i32 tx = 10 + col * (tile_size + 4);
            i32 ty = panel_y + 10 + row * (tile_size + 4);
            
            Rectangle rect = {tx, ty, tile_size, tile_size};
            if (CheckCollisionPointRec(mouse, rect)) {
                ed->selected_tile = i;
                ed->selected_type = PALETTE[i].type;
                break;
            }
        }
    }
    
    if (IsKeyPressed(KEY_G)) {
        ed->show_grid = !ed->show_grid;
    }
    
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyPressed(KEY_S)) {
            if (ed->filename[0]) {
                EditorSaveMap(ed, ed->filename);
            } else {
                static char default_name[] = "levels/custom_level.json";
                EditorSaveMap(ed, default_name);
            }
        }
        if (IsKeyPressed(KEY_L)) {
            static char default_name[] = "levels/custom_level.json";
            EditorLoadMap(ed, default_name);
        }
        if (IsKeyPressed(KEY_N)) {
            EditorNewMap(ed);
        }
        if (IsKeyPressed(KEY_Z)) {
            EditorUndo(ed);
        }
        if (IsKeyPressed(KEY_Y)) {
            EditorRedo(ed);
        }
    }
    
    if (IsKeyPressed(KEY_ESCAPE)) {
        CloseWindow();
    }
}

void DrawEditor(const EditorState *ed) {
    ClearBackground(BG_COLOR);
    
    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            DrawSpriteAtGrid(ed, ed->map.tiles[y][x], x, y);
            
            if (ed->map.cell_types[y][x] == TILE_SPAWN) {
                DrawRectangleLinesEx(
                    (Rectangle){EDITOR_MAP_OFFSET_X + x * TILE_SIZE, 
                                EDITOR_MAP_OFFSET_Y + y * TILE_SIZE, 
                                TILE_SIZE, TILE_SIZE}, 2, GREEN);
            } else if (ed->map.cell_types[y][x] == TILE_BASE) {
                DrawRectangleLinesEx(
                    (Rectangle){EDITOR_MAP_OFFSET_X + x * TILE_SIZE, 
                                EDITOR_MAP_OFFSET_Y + y * TILE_SIZE, 
                                TILE_SIZE, TILE_SIZE}, 2, RED);
            }
        }
    }
    
    if (ed->show_grid) {
        DrawEditorGrid(ed);
    }
    
    Vector2 mouse = GetMousePosition();
    i32 grid_x = (i32)((mouse.x - EDITOR_MAP_OFFSET_X) / TILE_SIZE);
    i32 grid_y = (i32)((mouse.y - EDITOR_MAP_OFFSET_Y) / TILE_SIZE);
    
    if (grid_x >= 0 && grid_x < ed->map.width && grid_y >= 0 && grid_y < ed->map.height) {
        DrawRectangleLinesEx(
            (Rectangle){EDITOR_MAP_OFFSET_X + grid_x * TILE_SIZE, 
                        EDITOR_MAP_OFFSET_Y + grid_y * TILE_SIZE, 
                        TILE_SIZE, TILE_SIZE}, 2, YELLOW);
    }
    
    DrawPalette(ed);
    DrawToolBar(ed);
    DrawStatusBar(ed);
}