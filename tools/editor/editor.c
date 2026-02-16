#include "editor.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

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

// --- Spritesheet palette ---

#define PAL_TILE_SIZE 36
#define PAL_MARGIN 2
#define PAL_START_Y 70
#define PAL_VISIBLE_ROWS 10

static void DrawPalette(const EditorState *ed) {
    i32 panel_w = EDITOR_PANEL_WIDTH;

    DrawRectangle(0, 0, panel_w, EDITOR_SCREEN_HEIGHT, (Color){40, 40, 40, 255});

    // Level name
    DrawTextEx(ed->font, "Name:", (Vector2){4, 4}, 14, 1, LIGHTGRAY);
    Rectangle name_box = {4, 20, panel_w - 8, 20};
    DrawRectangleRec(name_box, ed->editing_name ? (Color){70, 70, 70, 255} : (Color){55, 55, 55, 255});
    DrawRectangleLinesEx(name_box, 1, ed->editing_name ? YELLOW : GRAY);
    DrawTextEx(ed->font, ed->level_name, (Vector2){8, 22}, 14, 1, WHITE);

    // Type buttons
    DrawTextEx(ed->font, "Type:", (Vector2){4, 46}, 14, 1, LIGHTGRAY);
    const char *type_labels[] = {"Gnd", "Bld", "Blk", "Spn", "Bas"};
    Color type_colors[] = {GRAY, GREEN, RED, (Color){0,200,255,255}, (Color){255,100,0,255}};
    for (i32 i = 0; i < 5; i++) {
        i32 bx = 4 + i * 42;
        i32 by = 58;
        Rectangle r = {bx, by, 40, 14};
        bool sel = (ed->selected_type == i);
        DrawRectangleRec(r, sel ? type_colors[i] : (Color){60, 60, 60, 255});
        DrawRectangleLinesEx(r, 1, sel ? WHITE : GRAY);
        DrawTextEx(ed->font, type_labels[i], (Vector2){bx + 2, by + 1}, 12, 0, sel ? BLACK : LIGHTGRAY);
    }

    // Spritesheet grid
    i32 cols = EDITOR_PALETTE_COLS;
    i32 cell = PAL_TILE_SIZE + PAL_MARGIN;
    i32 total_rows = (SPRITE_TOTAL_TILES + cols - 1) / cols;
    i32 visible = PAL_VISIBLE_ROWS;
    i32 scroll = ed->palette_scroll;
    if (scroll > total_rows - visible) scroll = total_rows - visible;
    if (scroll < 0) scroll = 0;

    // Clip area
    i32 grid_y0 = PAL_START_Y;
    i32 grid_h = visible * cell;

    BeginScissorMode(0, grid_y0, panel_w, grid_h);
    for (i32 row = scroll; row < scroll + visible + 1 && row < total_rows; row++) {
        for (i32 col = 0; col < cols; col++) {
            i32 idx = row * cols + col;
            if (idx >= SPRITE_TOTAL_TILES) break;

            i32 tx = 4 + col * cell;
            i32 ty = grid_y0 + (row - scroll) * cell;

            // Draw sprite
            i32 src_x = (idx % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            i32 src_y = (idx / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            Rectangle src = {src_x, src_y, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
            Rectangle dest = {tx, ty, PAL_TILE_SIZE, PAL_TILE_SIZE};
            DrawTexturePro(ed->spritesheet, src, dest, (Vector2){0, 0}, 0, WHITE);

            if (idx == ed->selected_sprite) {
                DrawRectangleLinesEx(dest, 2, YELLOW);
            }
        }
    }
    EndScissorMode();

    // Scroll indicator
    if (total_rows > visible) {
        i32 bar_x = panel_w - 8;
        i32 bar_h = grid_h;
        f32 thumb_h = (f32)visible / total_rows * bar_h;
        f32 thumb_y = grid_y0 + (f32)scroll / total_rows * bar_h;
        DrawRectangle(bar_x, grid_y0, 6, bar_h, (Color){60, 60, 60, 255});
        DrawRectangle(bar_x, (i32)thumb_y, 6, (i32)thumb_h, LIGHTGRAY);
    }

    // Info below palette
    i32 info_y = grid_y0 + grid_h + 8;
    DrawTextEx(ed->font, TextFormat("Sprite: %d", ed->selected_sprite),
               (Vector2){4, info_y}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, TextFormat("Type: %s", TILE_TYPE_NAMES[ed->selected_type]),
               (Vector2){4, info_y + 16}, 14, 1, LIGHTGRAY);

    // Controls help
    i32 help_y = info_y + 44;
    DrawTextEx(ed->font, "Controls:", (Vector2){4, help_y}, 14, 1, YELLOW);
    const char *help[] = {
        "Click/Drag: Paint",
        "Right-click: Flood fill",
        "S: Set type Spawn",
        "B: Set type Base",
        "G: Toggle grid",
        "Ctrl+S: Save",
        "Ctrl+B: Browse levels",
        "Ctrl+N: New",
        "Ctrl+Z/Y: Undo/Redo",
        "Scroll: Palette",
        "Esc: Quit"
    };
    for (i32 i = 0; i < 11; i++) {
        DrawTextEx(ed->font, help[i], (Vector2){4, help_y + 16 + i * 14}, 12, 1, LIGHTGRAY);
    }
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

    DrawTextEx(ed->font, TextFormat("Level Editor - %s", ed->level_name),
               (Vector2){EDITOR_PANEL_WIDTH + 10, 10}, 20, 1, WHITE);

    i32 map_info_x = EDITOR_SCREEN_WIDTH - 250;
    DrawTextEx(ed->font, TextFormat("Spawns: %d | Bases: %d", ed->map.spawn_count, ed->map.base_count),
               (Vector2){map_info_x, 10}, 16, 1, LIGHTGRAY);
}

// --- Level browser ---

static void DrawBrowser(const EditorState *ed) {
    DrawRectangle(0, 0, EDITOR_SCREEN_WIDTH, EDITOR_SCREEN_HEIGHT, (Color){30, 30, 30, 255});

    DrawTextEx(ed->font, "Level Browser", (Vector2){20, 20}, 28, 1, WHITE);
    DrawTextEx(ed->font, "Click to load | Del to delete | Ctrl+B to close | Ctrl+N for new",
               (Vector2){20, 55}, 14, 1, LIGHTGRAY);

    i32 y = 90;
    i32 item_h = 36;
    for (i32 i = 0; i < ed->level_count && y < EDITOR_SCREEN_HEIGHT - 40; i++) {
        Rectangle row = {20, y, EDITOR_SCREEN_WIDTH - 40, item_h - 2};
        bool hover = CheckCollisionPointRec(GetMousePosition(), row);
        DrawRectangleRec(row, hover ? (Color){60, 60, 60, 255} : (Color){45, 45, 45, 255});
        DrawRectangleLinesEx(row, 1, GRAY);

        DrawTextEx(ed->font, ed->level_list[i].name,
                   (Vector2){30, y + 4}, 18, 1, WHITE);
        DrawTextEx(ed->font, ed->level_list[i].filename,
                   (Vector2){400, y + 6}, 14, 1, LIGHTGRAY);
        y += item_h;
    }

    if (ed->level_count == 0) {
        DrawTextEx(ed->font, "No .conf files found in levels/", (Vector2){20, 100}, 18, 1, LIGHTGRAY);
    }
}

// --- Init ---

void InitEditor(EditorState *ed) {
    memset(ed, 0, sizeof(EditorState));

    ed->show_grid = true;
    ed->selected_sprite = SPRITE_GROUND;
    ed->selected_type = TILE_BUILDABLE;
    ed->modified = false;
    ed->mode = MODE_EDIT;
    strncpy(ed->level_name, "Untitled", EDITOR_LEVEL_NAME_MAX - 1);

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
    strncpy(ed->level_name, "Untitled", EDITOR_LEVEL_NAME_MAX - 1);
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

        if (a->old_type == TILE_SPAWN || a->new_type == TILE_SPAWN ||
            a->old_type == TILE_BASE || a->new_type == TILE_BASE) {
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

    i32 new_tile = ed->selected_sprite;
    TileType new_type = ed->selected_type;

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

    if (new_type == TILE_SPAWN || new_type == TILE_BASE ||
        action.old_type == TILE_SPAWN || action.old_type == TILE_BASE) {
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
    i32 replace_tile = ed->selected_sprite;
    TileType replace_type = ed->selected_type;

    FloodFillRecursive(ed, grid_x, grid_y, target_tile, target_type, replace_tile, replace_type);

    EditorRebuildSpawnBase(ed);
}

// --- .conf Save/Load ---

bool EditorSaveMap(EditorState *ed, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Failed to open file for writing: %s\n", filename);
        return false;
    }

    fprintf(f, "name=%s\n", ed->level_name);
    fprintf(f, "width=%d\n", ed->map.width);
    fprintf(f, "height=%d\n", ed->map.height);

    fprintf(f, "tiles\n");
    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            if (x > 0) fprintf(f, " ");
            fprintf(f, "%d", ed->map.tiles[y][x]);
        }
        fprintf(f, "\n");
    }

    fprintf(f, "types\n");
    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            if (x > 0) fprintf(f, " ");
            fprintf(f, "%d", ed->map.cell_types[y][x]);
        }
        fprintf(f, "\n");
    }

    fclose(f);

    // Validation warnings
    EditorRebuildSpawnBase(ed);
    if (ed->map.spawn_count == 0) {
        printf("WARNING: No spawn points set!\n");
    }
    if (ed->map.base_count == 0) {
        printf("WARNING: No base points set!\n");
    }

    strncpy(ed->filename, filename, sizeof(ed->filename) - 1);
    ed->modified = false;
    printf("Map saved to: %s\n", filename);
    return true;
}

bool EditorLoadMap(EditorState *ed, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Failed to open file for reading: %s\n", filename);
        return false;
    }

    memset(&ed->map, 0, sizeof(Map));
    strncpy(ed->level_name, "Untitled", EDITOR_LEVEL_NAME_MAX - 1);

    char line[1024];
    enum { SECTION_HEADER, SECTION_TILES, SECTION_TYPES } section = SECTION_HEADER;
    i32 row = 0;

    while (fgets(line, sizeof(line), f)) {
        // Strip newline
        i32 len = (i32)strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';

        if (section == SECTION_HEADER) {
            if (strncmp(line, "name=", 5) == 0) {
                strncpy(ed->level_name, line + 5, EDITOR_LEVEL_NAME_MAX - 1);
            } else if (strncmp(line, "width=", 6) == 0) {
                ed->map.width = atoi(line + 6);
            } else if (strncmp(line, "height=", 7) == 0) {
                ed->map.height = atoi(line + 7);
            } else if (strcmp(line, "tiles") == 0) {
                section = SECTION_TILES;
                row = 0;
            }
        } else if (section == SECTION_TILES) {
            if (strcmp(line, "types") == 0) {
                section = SECTION_TYPES;
                row = 0;
                continue;
            }
            if (row < ed->map.height) {
                char *p = line;
                for (i32 x = 0; x < ed->map.width && *p; x++) {
                    ed->map.tiles[row][x] = (i32)strtol(p, &p, 10);
                }
                row++;
            }
        } else if (section == SECTION_TYPES) {
            if (row < ed->map.height) {
                char *p = line;
                for (i32 x = 0; x < ed->map.width && *p; x++) {
                    ed->map.cell_types[row][x] = (TileType)strtol(p, &p, 10);
                }
                row++;
            }
        }
    }

    fclose(f);

    EditorRebuildSpawnBase(ed);

    strncpy(ed->filename, filename, sizeof(ed->filename) - 1);
    ed->undo_count = 0;
    ed->undo_head = 0;
    ed->modified = false;
    printf("Map loaded from: %s\n", filename);
    return true;
}

// --- Level browser scanning ---

void EditorScanLevels(EditorState *ed) {
    ed->level_count = 0;

    DIR *dir = opendir("levels");
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir)) && ed->level_count < EDITOR_MAX_LEVELS) {
        i32 len = (i32)strlen(ent->d_name);
        if (len < 6 || strcmp(ent->d_name + len - 5, ".conf") != 0) continue;

        LevelEntry *le = &ed->level_list[ed->level_count];
        snprintf(le->filename, sizeof(le->filename), "levels/%s", ent->d_name);

        // Parse name from file
        strncpy(le->name, ent->d_name, EDITOR_LEVEL_NAME_MAX - 1);
        FILE *f = fopen(le->filename, "r");
        if (f) {
            char line[256];
            if (fgets(line, sizeof(line), f)) {
                i32 l = (i32)strlen(line);
                while (l > 0 && (line[l-1] == '\n' || line[l-1] == '\r')) line[--l] = '\0';
                if (strncmp(line, "name=", 5) == 0) {
                    strncpy(le->name, line + 5, EDITOR_LEVEL_NAME_MAX - 1);
                }
            }
            fclose(f);
        }

        ed->level_count++;
    }

    closedir(dir);
}

// --- Update ---

void UpdateEditor(EditorState *ed) {
    // Handle name editing
    if (ed->editing_name) {
        i32 key = GetCharPressed();
        while (key > 0) {
            i32 len = (i32)strlen(ed->level_name);
            if (key >= 32 && key < 127 && len < EDITOR_LEVEL_NAME_MAX - 1) {
                ed->level_name[len] = (char)key;
                ed->level_name[len + 1] = '\0';
                ed->modified = true;
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            i32 len = (i32)strlen(ed->level_name);
            if (len > 0) {
                ed->level_name[len - 1] = '\0';
                ed->modified = true;
            }
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
            ed->editing_name = false;
        }
        return; // Don't process other input while editing name
    }

    // Browse mode
    if (ed->mode == MODE_BROWSE) {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
            if (IsKeyPressed(KEY_B)) {
                ed->mode = MODE_EDIT;
                return;
            }
            if (IsKeyPressed(KEY_N)) {
                EditorNewMap(ed);
                ed->mode = MODE_EDIT;
                return;
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            ed->mode = MODE_EDIT;
            return;
        }

        // Click to load
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            i32 y = 90;
            i32 item_h = 36;
            for (i32 i = 0; i < ed->level_count; i++) {
                Rectangle row = {20, y, EDITOR_SCREEN_WIDTH - 40, item_h - 2};
                if (CheckCollisionPointRec(mouse, row)) {
                    EditorLoadMap(ed, ed->level_list[i].filename);
                    ed->mode = MODE_EDIT;
                    return;
                }
                y += item_h;
            }
        }

        // Delete key to remove hovered level
        if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
            Vector2 mouse = GetMousePosition();
            i32 y = 90;
            i32 item_h = 36;
            for (i32 i = 0; i < ed->level_count; i++) {
                Rectangle row = {20, y, EDITOR_SCREEN_WIDTH - 40, item_h - 2};
                if (CheckCollisionPointRec(mouse, row)) {
                    remove(ed->level_list[i].filename);
                    printf("Deleted: %s\n", ed->level_list[i].filename);
                    EditorScanLevels(ed);
                    return;
                }
                y += item_h;
            }
        }

        return;
    }

    // --- Edit mode ---
    Vector2 mouse = GetMousePosition();
    i32 grid_x = (i32)((mouse.x - EDITOR_MAP_OFFSET_X) / TILE_SIZE);
    i32 grid_y = (i32)((mouse.y - EDITOR_MAP_OFFSET_Y) / TILE_SIZE);

    // Painting on map
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        if (mouse.x >= EDITOR_PANEL_WIDTH && mouse.y >= EDITOR_MAP_OFFSET_Y &&
            mouse.y < EDITOR_SCREEN_HEIGHT - 30) {
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

    // Click on palette: sprite selection
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse.x < EDITOR_PANEL_WIDTH) {
        // Check name box click
        Rectangle name_box = {4, 20, EDITOR_PANEL_WIDTH - 8, 20};
        if (CheckCollisionPointRec(mouse, name_box)) {
            ed->editing_name = true;
            return;
        }

        // Check type buttons
        for (i32 i = 0; i < 5; i++) {
            Rectangle r = {4 + i * 42, 58, 40, 14};
            if (CheckCollisionPointRec(mouse, r)) {
                ed->selected_type = (TileType)i;
                return;
            }
        }

        // Check palette grid
        i32 cell = PAL_TILE_SIZE + PAL_MARGIN;
        i32 cols = EDITOR_PALETTE_COLS;
        i32 total_rows = (SPRITE_TOTAL_TILES + cols - 1) / cols;
        i32 scroll = ed->palette_scroll;
        if (scroll > total_rows - PAL_VISIBLE_ROWS) scroll = total_rows - PAL_VISIBLE_ROWS;
        if (scroll < 0) scroll = 0;

        if (mouse.y >= PAL_START_Y && mouse.y < PAL_START_Y + PAL_VISIBLE_ROWS * cell) {
            i32 col = ((i32)mouse.x - 4) / cell;
            i32 row_in_view = ((i32)mouse.y - PAL_START_Y) / cell;
            i32 row = row_in_view + scroll;
            if (col >= 0 && col < cols) {
                i32 idx = row * cols + col;
                if (idx >= 0 && idx < SPRITE_TOTAL_TILES) {
                    ed->selected_sprite = idx;
                }
            }
        }
    }

    // Palette scroll with mouse wheel
    if (mouse.x < EDITOR_PANEL_WIDTH) {
        i32 wheel = (i32)GetMouseWheelMove();
        if (wheel != 0) {
            ed->palette_scroll -= wheel * 2;
            i32 total_rows = (SPRITE_TOTAL_TILES + EDITOR_PALETTE_COLS - 1) / EDITOR_PALETTE_COLS;
            if (ed->palette_scroll > total_rows - PAL_VISIBLE_ROWS) ed->palette_scroll = total_rows - PAL_VISIBLE_ROWS;
            if (ed->palette_scroll < 0) ed->palette_scroll = 0;
        }
    }

    // Keyboard shortcuts for type
    if (IsKeyPressed(KEY_S) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        ed->selected_type = TILE_SPAWN;
    }
    if (IsKeyPressed(KEY_B) && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL)) {
        ed->selected_type = TILE_BASE;
    }

    if (IsKeyPressed(KEY_G)) {
        ed->show_grid = !ed->show_grid;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyPressed(KEY_S)) {
            if (!ed->filename[0]) {
                // Auto-generate filename
                for (i32 n = 1; n < 100; n++) {
                    char buf[256];
                    snprintf(buf, sizeof(buf), "levels/level%d.conf", n);
                    FILE *test = fopen(buf, "r");
                    if (!test) {
                        EditorSaveMap(ed, buf);
                        break;
                    }
                    fclose(test);
                }
            } else {
                EditorSaveMap(ed, ed->filename);
            }
        }
        if (IsKeyPressed(KEY_B)) {
            EditorScanLevels(ed);
            ed->mode = MODE_BROWSE;
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

    if (ed->mode == MODE_BROWSE) {
        DrawBrowser(ed);
        return;
    }

    // Draw map tiles
    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            DrawSpriteAtGrid(ed, ed->map.tiles[y][x], x, y);

            if (ed->map.cell_types[y][x] == TILE_SPAWN) {
                DrawRectangleLinesEx(
                    (Rectangle){EDITOR_MAP_OFFSET_X + x * TILE_SIZE,
                                EDITOR_MAP_OFFSET_Y + y * TILE_SIZE,
                                TILE_SIZE, TILE_SIZE}, 2, GREEN);
                DrawTextEx(ed->font, "S",
                           (Vector2){EDITOR_MAP_OFFSET_X + x * TILE_SIZE + 2,
                                     EDITOR_MAP_OFFSET_Y + y * TILE_SIZE + 2}, 16, 1, GREEN);
            } else if (ed->map.cell_types[y][x] == TILE_BASE) {
                DrawRectangleLinesEx(
                    (Rectangle){EDITOR_MAP_OFFSET_X + x * TILE_SIZE,
                                EDITOR_MAP_OFFSET_Y + y * TILE_SIZE,
                                TILE_SIZE, TILE_SIZE}, 2, RED);
                DrawTextEx(ed->font, "B",
                           (Vector2){EDITOR_MAP_OFFSET_X + x * TILE_SIZE + 2,
                                     EDITOR_MAP_OFFSET_Y + y * TILE_SIZE + 2}, 16, 1, RED);
            }
        }
    }

    if (ed->show_grid) {
        DrawEditorGrid(ed);
    }

    // Cursor highlight
    Vector2 mouse = GetMousePosition();
    i32 grid_x = (i32)((mouse.x - EDITOR_MAP_OFFSET_X) / TILE_SIZE);
    i32 grid_y = (i32)((mouse.y - EDITOR_MAP_OFFSET_Y) / TILE_SIZE);

    if (grid_x >= 0 && grid_x < ed->map.width && grid_y >= 0 && grid_y < ed->map.height &&
        mouse.x >= EDITOR_MAP_OFFSET_X) {
        DrawRectangleLinesEx(
            (Rectangle){EDITOR_MAP_OFFSET_X + grid_x * TILE_SIZE,
                        EDITOR_MAP_OFFSET_Y + grid_y * TILE_SIZE,
                        TILE_SIZE, TILE_SIZE}, 2, YELLOW);
    }

    DrawPalette(ed);
    DrawToolBar(ed);
    DrawStatusBar(ed);
}
