#include "editor.h"
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDITOR_SCREEN_WIDTH 1280
#define EDITOR_SCREEN_HEIGHT 720
#define EDITOR_MAP_OFFSET_X EDITOR_PANEL_WIDTH
#define EDITOR_MAP_OFFSET_Y 40

static const char *TILE_TYPE_NAMES[] = {
    [TILE_GROUND] = "Ground",
    [TILE_BUILDABLE] = "Buildable",
    [TILE_BLOCKED] = "Blocked",
    [TILE_SPAWN] = "Spawn",
    [TILE_BASE] = "Base"
};

#define PAL_TILE_SIZE 36
#define PAL_MARGIN 2
#define PAL_START_Y 112
#define PAL_VISIBLE_ROWS 10

#define TAB_W 130
#define TAB_H 28

static Rectangle GetLayerToggleRect(void) {
    return (Rectangle){4, 76, EDITOR_PANEL_WIDTH - 8, 14};
}

static Rectangle GetEraseToggleRect(void) {
    return (Rectangle){4, 94, EDITOR_PANEL_WIDTH - 8, 14};
}

static void StripLineEnding(char *line) {
    i32 len = (i32)strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }
}

static char *Trim(char *text) {
    while (*text && isspace((unsigned char)*text)) {
        text++;
    }

    i32 len = (i32)strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[--len] = '\0';
    }

    return text;
}

static const char *SectionName(EditorSection section) {
    switch (section) {
        case EDITOR_SECTION_LEVELS:
            return "Levels";
        case EDITOR_SECTION_ENEMIES:
            return "Enemies";
        case EDITOR_SECTION_TURRETS:
            return "Turrets";
        default:
            return "Editor";
    }
}

static const char *SectionDirectory(EditorSection section) {
    switch (section) {
        case EDITOR_SECTION_LEVELS:
            return "levels";
        case EDITOR_SECTION_ENEMIES:
            return "enemies";
        case EDITOR_SECTION_TURRETS:
            return "turrets";
        default:
            return "levels";
    }
}

static Rectangle GetTabRect(i32 index) {
    return (Rectangle){
        EDITOR_PANEL_WIDTH + 12 + index * (TAB_W + 8),
        6,
        TAB_W,
        TAB_H
    };
}

static void DrawSpriteAtGrid(const EditorState *ed, i32 sprite_id, i32 grid_x, i32 grid_y) {
    if (sprite_id < 0) {
        return;
    }
    i32 src_x = (sprite_id % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
    i32 src_y = (sprite_id / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
    Rectangle src = {(f32)src_x, (f32)src_y, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
    Rectangle dest = {
        EDITOR_MAP_OFFSET_X + grid_x * TILE_SIZE,
        EDITOR_MAP_OFFSET_Y + grid_y * TILE_SIZE,
        TILE_SIZE,
        TILE_SIZE
    };
    DrawTexturePro(ed->spritesheet, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
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
    i32 panel_w = EDITOR_PANEL_WIDTH;

    DrawRectangle(0, 0, panel_w, EDITOR_SCREEN_HEIGHT, (Color){40, 40, 40, 255});

    DrawTextEx(ed->font, "Name:", (Vector2){4, 4}, 14, 1, LIGHTGRAY);
    Rectangle name_box = {4, 20, panel_w - 8, 20};
    DrawRectangleRec(name_box, ed->editing_name ? (Color){70, 70, 70, 255} : (Color){55, 55, 55, 255});
    DrawRectangleLinesEx(name_box, 1, ed->editing_name ? YELLOW : GRAY);
    DrawTextEx(ed->font, ed->level_name, (Vector2){8, 22}, 14, 1, WHITE);

    DrawTextEx(ed->font, "Type:", (Vector2){4, 46}, 14, 1, LIGHTGRAY);
    const char *type_labels[] = {"Gnd", "Bld", "Blk", "Spn", "Bas"};
    Color type_colors[] = {GRAY, GREEN, RED, (Color){0, 200, 255, 255}, (Color){255, 100, 0, 255}};
    for (i32 i = 0; i < 5; i++) {
        i32 bx = 4 + i * 42;
        i32 by = 58;
        Rectangle r = {(f32)bx, (f32)by, 40, 14};
        bool selected = (ed->selected_type == (TileType)i);
        DrawRectangleRec(r, selected ? type_colors[i] : (Color){60, 60, 60, 255});
        DrawRectangleLinesEx(r, 1, selected ? WHITE : GRAY);
        DrawTextEx(ed->font, type_labels[i], (Vector2){(f32)bx + 2, (f32)by + 1}, 12, 0, selected ? BLACK : LIGHTGRAY);
    }

    Rectangle layer_btn = GetLayerToggleRect();
    Rectangle erase_btn = GetEraseToggleRect();
    DrawRectangleRec(layer_btn, ed->paint_background ? (Color){60, 120, 180, 255} : (Color){65, 65, 65, 255});
    DrawRectangleLinesEx(layer_btn, 1, ed->paint_background ? SKYBLUE : GRAY);
    DrawTextEx(ed->font,
               TextFormat("Layer: %s", ed->paint_background ? "BACKGROUND" : "FOREGROUND"),
               (Vector2){layer_btn.x + 4, layer_btn.y + 1},
               12, 1, WHITE);

    DrawRectangleRec(erase_btn, ed->erase_mode ? (Color){140, 95, 40, 255} : (Color){65, 65, 65, 255});
    DrawRectangleLinesEx(erase_btn, 1, ed->erase_mode ? ORANGE : GRAY);
    DrawTextEx(ed->font,
               TextFormat("Erase: %s", ed->erase_mode ? "ON" : "OFF"),
               (Vector2){erase_btn.x + 4, erase_btn.y + 1},
               12, 1, WHITE);

    i32 cols = EDITOR_PALETTE_COLS;
    i32 cell = PAL_TILE_SIZE + PAL_MARGIN;
    i32 total_rows = (SPRITE_TOTAL_TILES + cols - 1) / cols;
    i32 scroll = ed->palette_scroll;
    if (scroll > total_rows - PAL_VISIBLE_ROWS) scroll = total_rows - PAL_VISIBLE_ROWS;
    if (scroll < 0) scroll = 0;

    i32 grid_y0 = PAL_START_Y;
    i32 grid_h = PAL_VISIBLE_ROWS * cell;

    BeginScissorMode(0, grid_y0, panel_w, grid_h);
    for (i32 row = scroll; row < scroll + PAL_VISIBLE_ROWS + 1 && row < total_rows; row++) {
        for (i32 col = 0; col < cols; col++) {
            i32 idx = row * cols + col;
            if (idx >= SPRITE_TOTAL_TILES) break;

            i32 tx = 4 + col * cell;
            i32 ty = grid_y0 + (row - scroll) * cell;

            i32 src_x = (idx % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            i32 src_y = (idx / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
            Rectangle src = {(f32)src_x, (f32)src_y, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
            Rectangle dest = {(f32)tx, (f32)ty, PAL_TILE_SIZE, PAL_TILE_SIZE};
            DrawTexturePro(ed->spritesheet, src, dest, (Vector2){0, 0}, 0.0f, WHITE);

            if (idx == ed->selected_sprite) {
                DrawRectangleLinesEx(dest, 2, YELLOW);
            }
        }
    }
    EndScissorMode();

    if (total_rows > PAL_VISIBLE_ROWS) {
        i32 bar_x = panel_w - 8;
        i32 bar_h = grid_h;
        f32 thumb_h = (f32)PAL_VISIBLE_ROWS / (f32)total_rows * (f32)bar_h;
        f32 thumb_y = (f32)grid_y0 + ((f32)scroll / (f32)total_rows) * (f32)bar_h;
        DrawRectangle(bar_x, grid_y0, 6, bar_h, (Color){60, 60, 60, 255});
        DrawRectangle(bar_x, (i32)thumb_y, 6, (i32)thumb_h, LIGHTGRAY);
    }

    i32 info_y = grid_y0 + grid_h + 8;
    DrawTextEx(ed->font, TextFormat("Sprite: %d", ed->selected_sprite), (Vector2){4, (f32)info_y}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, TextFormat("Type: %s", TILE_TYPE_NAMES[ed->selected_type]), (Vector2){4, (f32)(info_y + 16)}, 14, 1, LIGHTGRAY);
    DrawTextEx(ed->font, TextFormat("Enemy cfg: %s", ed->enemies_file), (Vector2){4, (f32)(info_y + 34)}, 11, 1, SKYBLUE);
    DrawTextEx(ed->font, TextFormat("Turret cfg: %s", ed->turrets_file), (Vector2){4, (f32)(info_y + 48)}, 11, 1, ORANGE);

    i32 help_y = info_y + 66;
    DrawTextEx(ed->font, "Controls:", (Vector2){4, (f32)help_y}, 14, 1, YELLOW);
    const char *help[] = {
        "F1/F2/F3: switch section",
        "Click/Drag: paint tiles",
        "Right-click: flood fill",
        "L: toggle FG/BG layer",
        "E: toggle erase mode",
        "Ctrl+S: save",
        "Ctrl+B: browse files",
        "Ctrl+N: new file",
        "Ctrl+Z/Y: undo/redo",
        "G: toggle grid"
    };
    for (i32 i = 0; i < 10; i++) {
        DrawTextEx(ed->font, help[i], (Vector2){4, (f32)(help_y + 16 + i * 14)}, 12, 1, LIGHTGRAY);
    }
}

static void DrawToolBar(const EditorState *ed) {
    DrawRectangle(EDITOR_PANEL_WIDTH, 0, EDITOR_SCREEN_WIDTH - EDITOR_PANEL_WIDTH, EDITOR_MAP_OFFSET_Y, (Color){50, 50, 50, 255});

    DrawTextEx(ed->font, "Game Editor", (Vector2){EDITOR_PANEL_WIDTH + 10, 10}, 20, 1, WHITE);

    const EditorSection sections[] = {EDITOR_SECTION_LEVELS, EDITOR_SECTION_ENEMIES, EDITOR_SECTION_TURRETS};
    for (i32 i = 0; i < 3; i++) {
        Rectangle tab = GetTabRect(i);
        bool selected = (ed->section == sections[i]);
        DrawRectangleRec(tab, selected ? (Color){80, 120, 170, 255} : (Color){70, 70, 70, 255});
        DrawRectangleLinesEx(tab, 1, selected ? YELLOW : GRAY);
        DrawTextEx(ed->font,
                   SectionName(sections[i]),
                   (Vector2){tab.x + 14, tab.y + 6},
                   16,
                   1,
                   WHITE);
    }

    i32 right_x = EDITOR_SCREEN_WIDTH - 390;
    DrawTextEx(ed->font,
               TextFormat("Section: %s | Waves: %d", SectionName(ed->section), ed->wave_count),
               (Vector2){(f32)right_x, 10},
               16,
               1,
               LIGHTGRAY);
}

static void DrawStatusBar(const EditorState *ed) {
    i32 bar_y = EDITOR_SCREEN_HEIGHT - 30;
    DrawRectangle(0, bar_y, EDITOR_SCREEN_WIDTH, 30, (Color){30, 30, 30, 255});

    const char *file = "(unsaved)";
    if (ed->section == EDITOR_SECTION_LEVELS && ed->filename[0]) file = ed->filename;
    if (ed->section == EDITOR_SECTION_ENEMIES && ed->enemy_filename[0]) file = ed->enemy_filename;
    if (ed->section == EDITOR_SECTION_TURRETS && ed->turret_filename[0]) file = ed->turret_filename;

    DrawTextEx(ed->font,
               TextFormat("%s | File: %s %s", SectionName(ed->section), file, ed->modified ? "[Modified]" : ""),
               (Vector2){10, (f32)bar_y + 6},
               14,
               1,
               LIGHTGRAY);

    if (ed->section == EDITOR_SECTION_LEVELS) {
        DrawTextEx(ed->font,
                   TextFormat("Undo: %d | Redo: %d", ed->undo_head, ed->undo_count - ed->undo_head),
                   (Vector2){EDITOR_SCREEN_WIDTH - 180, (f32)bar_y + 6},
                   14,
                   1,
                   LIGHTGRAY);
    }
}

static void DrawBrowser(const EditorState *ed) {
    DrawRectangle(0, 0, EDITOR_SCREEN_WIDTH, EDITOR_SCREEN_HEIGHT, (Color){30, 30, 30, 255});

    DrawTextEx(ed->font,
               TextFormat("%s Browser", SectionName(ed->section)),
               (Vector2){20, 20},
               28,
               1,
               WHITE);
    DrawTextEx(ed->font,
               "Click to load | Del to delete | Esc to close",
               (Vector2){20, 55},
               14,
               1,
               LIGHTGRAY);

    i32 y = 95;
    i32 item_h = 36;
    for (i32 i = 0; i < ed->file_count && y < EDITOR_SCREEN_HEIGHT - 40; i++) {
        Rectangle row = {20, (f32)y, EDITOR_SCREEN_WIDTH - 40.0f, item_h - 2.0f};
        bool hover = CheckCollisionPointRec(GetMousePosition(), row);
        DrawRectangleRec(row, hover ? (Color){80, 80, 80, 255} : (Color){50, 50, 50, 255});
        DrawRectangleLinesEx(row, 1, hover ? YELLOW : GRAY);

        DrawTextEx(ed->font, ed->file_list[i].name, (Vector2){30, (f32)y + 6}, 18, 1, WHITE);
        DrawTextEx(ed->font, ed->file_list[i].filename, (Vector2){390, (f32)y + 8}, 14, 1, LIGHTGRAY);
        DrawTextEx(ed->font, "[Del]", (Vector2){EDITOR_SCREEN_WIDTH - 75, (f32)y + 8}, 12, 1, RED);

        y += item_h;
    }

    if (ed->file_count == 0) {
        DrawTextEx(ed->font,
                   TextFormat("No .conf files found in %s/", SectionDirectory(ed->section)),
                   (Vector2){20, 120},
                   18,
                   1,
                   LIGHTGRAY);
    }
}

void EditorNewMap(EditorState *ed) {
    memset(&ed->map, 0, sizeof(Map));
    ed->map.width = MAP_WIDTH;
    ed->map.height = MAP_HEIGHT;

    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            ed->map.background_tiles[y][x] = -1;
            ed->map.tiles[y][x] = -1;
            ed->map.cell_types[y][x] = TILE_BLOCKED;
        }
    }

    ed->wave_count = 1;
    strncpy(ed->wave_lines[0], "0:10:1.0", sizeof(ed->wave_lines[0]) - 1);
    strncpy(ed->enemies_file, "enemies/default.conf", sizeof(ed->enemies_file) - 1);
    strncpy(ed->turrets_file, "turrets/default.conf", sizeof(ed->turrets_file) - 1);

    ed->undo_count = 0;
    ed->undo_head = 0;
    ed->filename[0] = '\0';
    ed->modified = true;
    strncpy(ed->level_name, "Untitled", EDITOR_LEVEL_NAME_MAX - 1);
}

void EditorNewEnemies(EditorState *ed) {
    memset(ed->enemy_entries, 0, sizeof(ed->enemy_entries));
    strncpy(ed->enemy_config_name, "Default Enemies", sizeof(ed->enemy_config_name) - 1);

    ed->enemy_entries[ENEMY_SIMPLE] = (EditorEnemyEntry){"Simple", 20, 1.0f, 1, 1, 245, -1};
    ed->enemy_entries[ENEMY_FAST] = (EditorEnemyEntry){"Fast", 35, 2.0f, 1, 1, 247, -1};
    ed->enemy_entries[ENEMY_HEAVY] = (EditorEnemyEntry){"Heavy", 100, 0.5f, 1, 1, 246, -1};
    ed->enemy_entries[ENEMY_SHIELDED] = (EditorEnemyEntry){"Shielded", 40, 1.0f, 1, 1, 248, -1};
    ed->enemy_entries[ENEMY_FLYING] = (EditorEnemyEntry){"Flying", 100, 0.9f, 1, 1, 271, 294};
    ed->enemy_entries[ENEMY_BOSS] = (EditorEnemyEntry){"Boss", 5000, 0.75f, 10, 5, 269, 292};

    ed->enemy_filename[0] = '\0';
    ed->enemy_selected_row = 0;
    ed->enemy_selected_col = 0;
    ed->modified = true;
}

void EditorNewTurrets(EditorState *ed) {
    memset(ed->turret_entries, 0, sizeof(ed->turret_entries));
    strncpy(ed->turret_config_name, "Default Turrets", sizeof(ed->turret_config_name) - 1);

    ed->turret_entries[TOWER_VULCAN] = (EditorTurretEntry){"Vulcan", 10, 1.0f, 2.0f, 5, 180, 203, 1};
    ed->turret_entries[TOWER_DCA] = (EditorTurretEntry){"DCA", 20, 1.0f, 2.0f, 25, 181, 205, 4};
    ed->turret_entries[TOWER_FREEZE] = (EditorTurretEntry){"Freeze", 0, 0.5f, 1.5f, 30, 181, 22, 5};
    ed->turret_entries[TOWER_MISSILE] = (EditorTurretEntry){"Missile", 7, 1.0f, 4.0f, 20, 182, 226, 3};
    ed->turret_entries[TOWER_PLASMA] = (EditorTurretEntry){"Plasma", 5, 4.0f, 2.5f, 15, 183, 206, 2};
    ed->turret_entries[TOWER_WALL] = (EditorTurretEntry){"Wall", 0, 0.0f, 0.0f, 2, 180, -1, 0};

    ed->turret_filename[0] = '\0';
    ed->turret_selected_row = 0;
    ed->turret_selected_col = 0;
    ed->modified = true;
}

void InitEditor(EditorState *ed) {
    memset(ed, 0, sizeof(EditorState));

    ed->show_grid = true;
    ed->selected_sprite = SPRITE_GROUND;
    ed->selected_type = TILE_BUILDABLE;
    ed->mode = MODE_EDIT;
    ed->section = EDITOR_SECTION_LEVELS;
    ed->paint_background = false;
    ed->erase_mode = false;
    ed->last_paint_x = -1;
    ed->last_paint_y = -1;

    EditorNewMap(ed);
    EditorNewEnemies(ed);
    EditorNewTurrets(ed);
    ed->modified = false;

    ed->spritesheet = LoadTexture("assets/sprites/towerDefense_tilesheet@2.png");
    if (ed->spritesheet.id == 0) {
        printf("Failed to load spritesheet!\n");
    }
    SetTextureFilter(ed->spritesheet, TEXTURE_FILTER_POINT);

    ed->font = LoadFontEx("assets/fonts/Poppins-Regular.ttf", 96, 0, 0);
    if (ed->font.texture.id == 0) {
        printf("Failed to load font!\n");
    }
    SetTextureFilter(ed->font.texture, TEXTURE_FILTER_BILINEAR);

    printf("Game editor initialized.\n");
}

void CleanupEditor(EditorState *ed) {
    if (ed->spritesheet.id != 0) {
        UnloadTexture(ed->spritesheet);
    }
    if (ed->font.texture.id != 0) {
        UnloadFont(ed->font);
    }
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
    if (ed->undo_head <= 0) return;

    ed->undo_head--;
    EditorAction *a = &ed->undo_stack[ed->undo_head];
    if (a->is_background) {
        ed->map.background_tiles[a->y][a->x] = a->old_tile;
    } else {
        ed->map.tiles[a->y][a->x] = a->old_tile;
        ed->map.cell_types[a->y][a->x] = (TileType)a->old_type;
    }
    ed->modified = true;

    if (!a->is_background &&
        (a->old_type == TILE_SPAWN || a->new_type == TILE_SPAWN ||
         a->old_type == TILE_BASE || a->new_type == TILE_BASE)) {
        EditorRebuildSpawnBase(ed);
    }
}

void EditorRedo(EditorState *ed) {
    if (ed->undo_head >= ed->undo_count) return;

    EditorAction *a = &ed->undo_stack[ed->undo_head];
    if (a->is_background) {
        ed->map.background_tiles[a->y][a->x] = a->new_tile;
    } else {
        ed->map.tiles[a->y][a->x] = a->new_tile;
        ed->map.cell_types[a->y][a->x] = (TileType)a->new_type;
    }
    ed->undo_head++;
    ed->modified = true;

    if (!a->is_background && (a->new_type == TILE_SPAWN || a->new_type == TILE_BASE)) {
        EditorRebuildSpawnBase(ed);
    }
}

void EditorRebuildSpawnBase(EditorState *ed) {
    ed->map.spawn_count = 0;
    ed->map.base_count = 0;

    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            if (ed->map.cell_types[y][x] == TILE_SPAWN && ed->map.spawn_count < 16) {
                ed->map.spawn_points[ed->map.spawn_count++] = (Vector2){(f32)x, (f32)y};
            } else if (ed->map.cell_types[y][x] == TILE_BASE && ed->map.base_count < 16) {
                ed->map.base_points[ed->map.base_count++] = (Vector2){(f32)x, (f32)y};
            }
        }
    }
}

void EditorPaintTile(EditorState *ed, i32 grid_x, i32 grid_y) {
    if (grid_x < 0 || grid_x >= ed->map.width || grid_y < 0 || grid_y >= ed->map.height) return;
    if (grid_x == ed->last_paint_x && grid_y == ed->last_paint_y) return;

    i32 new_tile = ed->erase_mode ? -1 : ed->selected_sprite;

    if (ed->paint_background) {
        if (ed->map.background_tiles[grid_y][grid_x] == new_tile) return;

        EditorAction action = {
            .x = grid_x,
            .y = grid_y,
            .is_background = true,
            .old_tile = ed->map.background_tiles[grid_y][grid_x],
            .old_type = 0,
            .new_tile = new_tile,
            .new_type = 0
        };

        ed->map.background_tiles[grid_y][grid_x] = new_tile;
        EditorPushAction(ed, action);
        ed->last_paint_x = grid_x;
        ed->last_paint_y = grid_y;
        return;
    }

    TileType new_type = ed->erase_mode ? TILE_BLOCKED : ed->selected_type;

    if (ed->map.tiles[grid_y][grid_x] == new_tile && ed->map.cell_types[grid_y][grid_x] == new_type) return;

    EditorAction action = {
        .x = grid_x,
        .y = grid_y,
        .is_background = false,
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
        .x = x,
        .y = y,
        .is_background = false,
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

static void FloodFillBackground(EditorState *ed, i32 x, i32 y, i32 target_tile, i32 replace_tile) {
    if (x < 0 || x >= ed->map.width || y < 0 || y >= ed->map.height) return;
    if (ed->map.background_tiles[y][x] != target_tile) return;
    if (target_tile == replace_tile) return;

    EditorAction action = {
        .x = x,
        .y = y,
        .is_background = true,
        .old_tile = target_tile,
        .old_type = 0,
        .new_tile = replace_tile,
        .new_type = 0
    };
    EditorPushAction(ed, action);

    ed->map.background_tiles[y][x] = replace_tile;

    FloodFillBackground(ed, x + 1, y, target_tile, replace_tile);
    FloodFillBackground(ed, x - 1, y, target_tile, replace_tile);
    FloodFillBackground(ed, x, y + 1, target_tile, replace_tile);
    FloodFillBackground(ed, x, y - 1, target_tile, replace_tile);
}

void EditorFloodFill(EditorState *ed, i32 grid_x, i32 grid_y) {
    if (grid_x < 0 || grid_x >= ed->map.width || grid_y < 0 || grid_y >= ed->map.height) return;

    if (ed->paint_background) {
        i32 target_bg = ed->map.background_tiles[grid_y][grid_x];
        i32 replace_bg = ed->erase_mode ? -1 : ed->selected_sprite;
        FloodFillBackground(ed, grid_x, grid_y, target_bg, replace_bg);
        return;
    }

    i32 target_tile = ed->map.tiles[grid_y][grid_x];
    TileType target_type = ed->map.cell_types[grid_y][grid_x];
    i32 replace_tile = ed->erase_mode ? -1 : ed->selected_sprite;
    TileType replace_type = ed->erase_mode ? TILE_BLOCKED : ed->selected_type;
    FloodFillRecursive(ed, grid_x, grid_y, target_tile, target_type, replace_tile, replace_type);
    EditorRebuildSpawnBase(ed);
}

static bool FindFirstEmptyTile(const EditorState *ed, i32 *out_x, i32 *out_y) {
    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            if (ed->map.background_tiles[y][x] < 0 && ed->map.tiles[y][x] < 0) {
                *out_x = x;
                *out_y = y;
                return true;
            }
        }
    }
    return false;
}

bool EditorSaveMap(EditorState *ed, const char *filename) {
    i32 empty_x = -1;
    i32 empty_y = -1;
    if (FindFirstEmptyTile(ed, &empty_x, &empty_y)) {
        printf("Cannot save level: map contains a cell with no foreground and no background tile at (%d, %d)\n",
               empty_x, empty_y);
        return false;
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Failed to open file for writing: %s\n", filename);
        return false;
    }

    fprintf(f, "name=%s\n", ed->level_name);
    fprintf(f, "enemies_file=%s\n", ed->enemies_file);
    fprintf(f, "turrets_file=%s\n", ed->turrets_file);
    for (i32 i = 0; i < ed->wave_count; i++) {
        fprintf(f, "wave=%s\n", ed->wave_lines[i]);
    }
    fprintf(f, "width=%d\n", ed->map.width);
    fprintf(f, "height=%d\n", ed->map.height);

    fprintf(f, "background\n");
    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            if (x > 0) fprintf(f, " ");
            fprintf(f, "%d", ed->map.background_tiles[y][x]);
        }
        fprintf(f, "\n");
    }

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

    EditorRebuildSpawnBase(ed);
    strncpy(ed->filename, filename, sizeof(ed->filename) - 1);
    ed->filename[sizeof(ed->filename) - 1] = '\0';
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
    strncpy(ed->level_name, "Untitled", sizeof(ed->level_name) - 1);
    strncpy(ed->enemies_file, "enemies/default.conf", sizeof(ed->enemies_file) - 1);
    strncpy(ed->turrets_file, "turrets/default.conf", sizeof(ed->turrets_file) - 1);
    ed->wave_count = 0;

    for (i32 y = 0; y < MAP_HEIGHT; y++) {
        for (i32 x = 0; x < MAP_WIDTH; x++) {
            ed->map.background_tiles[y][x] = -1;
            ed->map.tiles[y][x] = -1;
            ed->map.cell_types[y][x] = TILE_BLOCKED;
        }
    }

    char line[1024];
    enum { SECTION_HEADER, SECTION_BACKGROUND, SECTION_TILES, SECTION_TYPES } section = SECTION_HEADER;
    i32 row = 0;

    while (fgets(line, sizeof(line), f)) {
        StripLineEnding(line);
        char *trimmed = Trim(line);

        if (section == SECTION_HEADER) {
            if (strncmp(trimmed, "name=", 5) == 0) {
                strncpy(ed->level_name, trimmed + 5, sizeof(ed->level_name) - 1);
            } else if (strncmp(trimmed, "enemies_file=", 13) == 0) {
                strncpy(ed->enemies_file, trimmed + 13, sizeof(ed->enemies_file) - 1);
            } else if (strncmp(trimmed, "turrets_file=", 13) == 0) {
                strncpy(ed->turrets_file, trimmed + 13, sizeof(ed->turrets_file) - 1);
            } else if (strncmp(trimmed, "wave=", 5) == 0 && ed->wave_count < MAX_LEVEL_WAVES) {
                strncpy(ed->wave_lines[ed->wave_count], trimmed + 5, sizeof(ed->wave_lines[0]) - 1);
                ed->wave_count++;
            } else if (strncmp(trimmed, "width=", 6) == 0) {
                ed->map.width = atoi(trimmed + 6);
            } else if (strncmp(trimmed, "height=", 7) == 0) {
                ed->map.height = atoi(trimmed + 7);
            } else if (strcmp(trimmed, "background") == 0) {
                section = SECTION_BACKGROUND;
                row = 0;
            } else if (strcmp(trimmed, "tiles") == 0) {
                section = SECTION_TILES;
                row = 0;
            }
        } else if (section == SECTION_BACKGROUND) {
            if (strcmp(trimmed, "tiles") == 0) {
                section = SECTION_TILES;
                row = 0;
                continue;
            }
            if (row < ed->map.height) {
                char *p = trimmed;
                for (i32 x = 0; x < ed->map.width && *p; x++) {
                    ed->map.background_tiles[row][x] = (i32)strtol(p, &p, 10);
                }
                row++;
            }
        } else if (section == SECTION_TILES) {
            if (strcmp(trimmed, "types") == 0) {
                section = SECTION_TYPES;
                row = 0;
                continue;
            }
            if (row < ed->map.height) {
                char *p = trimmed;
                for (i32 x = 0; x < ed->map.width && *p; x++) {
                    ed->map.tiles[row][x] = (i32)strtol(p, &p, 10);
                }
                row++;
            }
        } else if (section == SECTION_TYPES) {
            if (row < ed->map.height) {
                char *p = trimmed;
                for (i32 x = 0; x < ed->map.width && *p; x++) {
                    ed->map.cell_types[row][x] = (TileType)strtol(p, &p, 10);
                }
                row++;
            }
        }
    }

    fclose(f);

    if (ed->wave_count <= 0) {
        ed->wave_count = 1;
        strncpy(ed->wave_lines[0], "0:10:1.0", sizeof(ed->wave_lines[0]) - 1);
    }

    EditorRebuildSpawnBase(ed);

    strncpy(ed->filename, filename, sizeof(ed->filename) - 1);
    ed->filename[sizeof(ed->filename) - 1] = '\0';
    ed->undo_count = 0;
    ed->undo_head = 0;
    ed->modified = false;
    printf("Map loaded from: %s\n", filename);
    return true;
}

static bool ParseEnemyLine(EditorState *ed, const char *line, bool seen[ENEMY_TYPE_COUNT]) {
    char work[512];
    strncpy(work, line, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    char *save = NULL;
    char *token = strtok_r(work, ",", &save);
    if (!token) return false;
    i32 idx = atoi(Trim(token));
    if (idx < 0 || idx >= ENEMY_TYPE_COUNT) return false;

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    strncpy(ed->enemy_entries[idx].name, Trim(token), sizeof(ed->enemy_entries[idx].name) - 1);

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->enemy_entries[idx].health = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->enemy_entries[idx].speed = strtof(Trim(token), NULL);

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->enemy_entries[idx].reward = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->enemy_entries[idx].damage_to_base = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->enemy_entries[idx].sprite_id = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->enemy_entries[idx].overlay_sprite_id = atoi(Trim(token));

    seen[idx] = true;
    return true;
}

bool EditorSaveEnemies(EditorState *ed, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Failed to open enemy file for writing: %s\n", filename);
        return false;
    }

    fprintf(f, "name=%s\n", ed->enemy_config_name);
    for (i32 i = 0; i < ENEMY_TYPE_COUNT; i++) {
        const EditorEnemyEntry *e = &ed->enemy_entries[i];
        fprintf(f,
                "enemy=%d,%s,%d,%.2f,%d,%d,%d,%d\n",
                i,
                e->name,
                e->health,
                e->speed,
                e->reward,
                e->damage_to_base,
                e->sprite_id,
                e->overlay_sprite_id);
    }

    fclose(f);
    strncpy(ed->enemy_filename, filename, sizeof(ed->enemy_filename) - 1);
    ed->enemy_filename[sizeof(ed->enemy_filename) - 1] = '\0';
    ed->modified = false;
    printf("Enemy config saved: %s\n", filename);
    return true;
}

bool EditorLoadEnemies(EditorState *ed, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Failed to open enemy file: %s\n", filename);
        return false;
    }

    bool seen[ENEMY_TYPE_COUNT] = {0};
    char line[1024];
    memset(ed->enemy_entries, 0, sizeof(ed->enemy_entries));

    while (fgets(line, sizeof(line), f)) {
        StripLineEnding(line);
        char *trimmed = Trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

        if (strncmp(trimmed, "name=", 5) == 0) {
            strncpy(ed->enemy_config_name, trimmed + 5, sizeof(ed->enemy_config_name) - 1);
        } else if (strncmp(trimmed, "enemy=", 6) == 0) {
            if (!ParseEnemyLine(ed, trimmed + 6, seen)) {
                fclose(f);
                printf("Invalid enemy line: %s\n", trimmed);
                return false;
            }
        }
    }

    fclose(f);

    for (i32 i = 0; i < ENEMY_TYPE_COUNT; i++) {
        if (!seen[i]) {
            printf("Missing enemy entry %d in %s\n", i, filename);
            return false;
        }
    }

    strncpy(ed->enemy_filename, filename, sizeof(ed->enemy_filename) - 1);
    ed->enemy_filename[sizeof(ed->enemy_filename) - 1] = '\0';
    ed->modified = false;
    printf("Enemy config loaded: %s\n", filename);
    return true;
}

static bool ParseTurretLine(EditorState *ed, const char *line, bool seen[TOWER_TYPE_COUNT]) {
    char work[512];
    strncpy(work, line, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    char *save = NULL;
    char *token = strtok_r(work, ",", &save);
    if (!token) return false;
    i32 idx = atoi(Trim(token));
    if (idx < 0 || idx >= TOWER_TYPE_COUNT) return false;

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    strncpy(ed->turret_entries[idx].name, Trim(token), sizeof(ed->turret_entries[idx].name) - 1);

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->turret_entries[idx].damage = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->turret_entries[idx].fire_rate = strtof(Trim(token), NULL);

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->turret_entries[idx].range = strtof(Trim(token), NULL);

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->turret_entries[idx].cost = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->turret_entries[idx].base_sprite_id = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->turret_entries[idx].gun_sprite_id = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    ed->turret_entries[idx].shop_slot = atoi(Trim(token));

    seen[idx] = true;
    return true;
}

bool EditorSaveTurrets(EditorState *ed, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Failed to open turret file for writing: %s\n", filename);
        return false;
    }

    fprintf(f, "name=%s\n", ed->turret_config_name);
    for (i32 i = 0; i < TOWER_TYPE_COUNT; i++) {
        const EditorTurretEntry *t = &ed->turret_entries[i];
        fprintf(f,
                "turret=%d,%s,%d,%.2f,%.2f,%d,%d,%d,%d\n",
                i,
                t->name,
                t->damage,
                t->fire_rate,
                t->range,
                t->cost,
                t->base_sprite_id,
                t->gun_sprite_id,
                t->shop_slot);
    }

    fclose(f);
    strncpy(ed->turret_filename, filename, sizeof(ed->turret_filename) - 1);
    ed->turret_filename[sizeof(ed->turret_filename) - 1] = '\0';
    ed->modified = false;
    printf("Turret config saved: %s\n", filename);
    return true;
}

bool EditorLoadTurrets(EditorState *ed, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Failed to open turret file: %s\n", filename);
        return false;
    }

    bool seen[TOWER_TYPE_COUNT] = {0};
    char line[1024];
    memset(ed->turret_entries, 0, sizeof(ed->turret_entries));

    while (fgets(line, sizeof(line), f)) {
        StripLineEnding(line);
        char *trimmed = Trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

        if (strncmp(trimmed, "name=", 5) == 0) {
            strncpy(ed->turret_config_name, trimmed + 5, sizeof(ed->turret_config_name) - 1);
        } else if (strncmp(trimmed, "turret=", 7) == 0) {
            if (!ParseTurretLine(ed, trimmed + 7, seen)) {
                fclose(f);
                printf("Invalid turret line: %s\n", trimmed);
                return false;
            }
        }
    }

    fclose(f);

    for (i32 i = 0; i < TOWER_TYPE_COUNT; i++) {
        if (!seen[i]) {
            printf("Missing turret entry %d in %s\n", i, filename);
            return false;
        }
    }

    strncpy(ed->turret_filename, filename, sizeof(ed->turret_filename) - 1);
    ed->turret_filename[sizeof(ed->turret_filename) - 1] = '\0';
    ed->modified = false;
    printf("Turret config loaded: %s\n", filename);
    return true;
}

void EditorScanFiles(EditorState *ed) {
    ed->file_count = 0;

    const char *dir_path = SectionDirectory(ed->section);
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir)) && ed->file_count < EDITOR_MAX_FILES) {
        i32 len = (i32)strlen(ent->d_name);
        if (len < 6 || strcmp(ent->d_name + len - 5, ".conf") != 0) continue;

        EditorFileEntry *entry = &ed->file_list[ed->file_count];
        snprintf(entry->filename, sizeof(entry->filename), "%s/%s", dir_path, ent->d_name);
        strncpy(entry->name, ent->d_name, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';

        FILE *f = fopen(entry->filename, "r");
        if (f) {
            char line[256];
            if (fgets(line, sizeof(line), f)) {
                StripLineEnding(line);
                if (strncmp(line, "name=", 5) == 0) {
                    strncpy(entry->name, line + 5, sizeof(entry->name) - 1);
                    entry->name[sizeof(entry->name) - 1] = '\0';
                }
            }
            fclose(f);
        }

        ed->file_count++;
    }

    closedir(dir);
}

static bool FindNextFilename(const char *dir, const char *prefix, char *out, size_t out_size) {
    for (i32 n = 1; n < 1000; n++) {
        snprintf(out, out_size, "%s/%s%d.conf", dir, prefix, n);
        FILE *test = fopen(out, "r");
        if (!test) {
            return true;
        }
        fclose(test);
    }
    return false;
}

static void HandleGlobalSectionSwitch(EditorState *ed) {
    if (IsKeyPressed(KEY_F1)) ed->section = EDITOR_SECTION_LEVELS;
    if (IsKeyPressed(KEY_F2)) ed->section = EDITOR_SECTION_ENEMIES;
    if (IsKeyPressed(KEY_F3)) ed->section = EDITOR_SECTION_TURRETS;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        for (i32 i = 0; i < 3; i++) {
            Rectangle tab = GetTabRect(i);
            if (CheckCollisionPointRec(mouse, tab)) {
                ed->section = (EditorSection)i;
                ed->mode = MODE_EDIT;
                return;
            }
        }
    }
}

static void HandleBrowseMode(EditorState *ed) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        ed->mode = MODE_EDIT;
        return;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse = GetMousePosition();
        i32 y = 95;
        i32 item_h = 36;
        for (i32 i = 0; i < ed->file_count; i++) {
            Rectangle row = {20, (f32)y, EDITOR_SCREEN_WIDTH - 40.0f, item_h - 2.0f};
            if (CheckCollisionPointRec(mouse, row)) {
                bool loaded = false;
                if (ed->section == EDITOR_SECTION_LEVELS) loaded = EditorLoadMap(ed, ed->file_list[i].filename);
                if (ed->section == EDITOR_SECTION_ENEMIES) loaded = EditorLoadEnemies(ed, ed->file_list[i].filename);
                if (ed->section == EDITOR_SECTION_TURRETS) loaded = EditorLoadTurrets(ed, ed->file_list[i].filename);
                if (loaded) {
                    ed->mode = MODE_EDIT;
                }
                return;
            }
            y += item_h;
        }
    }

    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
        Vector2 mouse = GetMousePosition();
        i32 y = 95;
        i32 item_h = 36;
        for (i32 i = 0; i < ed->file_count; i++) {
            Rectangle row = {20, (f32)y, EDITOR_SCREEN_WIDTH - 40.0f, item_h - 2.0f};
            if (CheckCollisionPointRec(mouse, row)) {
                remove(ed->file_list[i].filename);
                printf("Deleted: %s\n", ed->file_list[i].filename);
                EditorScanFiles(ed);
                return;
            }
            y += item_h;
        }
    }
}

static void UpdateLevelSection(EditorState *ed) {
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
        return;
    }

    Vector2 mouse = GetMousePosition();
    i32 grid_x = (i32)((mouse.x - EDITOR_MAP_OFFSET_X) / TILE_SIZE);
    i32 grid_y = (i32)((mouse.y - EDITOR_MAP_OFFSET_Y) / TILE_SIZE);

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

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Rectangle name_box = {4, 20, EDITOR_PANEL_WIDTH - 8.0f, 20};
        if (CheckCollisionPointRec(mouse, name_box)) {
            ed->editing_name = true;
            return;
        }

        Rectangle layer_btn = GetLayerToggleRect();
        if (CheckCollisionPointRec(mouse, layer_btn)) {
            ed->paint_background = !ed->paint_background;
            return;
        }

        Rectangle erase_btn = GetEraseToggleRect();
        if (CheckCollisionPointRec(mouse, erase_btn)) {
            ed->erase_mode = !ed->erase_mode;
            return;
        }

        if (mouse.x < EDITOR_PANEL_WIDTH) {
            for (i32 i = 0; i < 5; i++) {
                Rectangle r = {4 + i * 42.0f, 58, 40, 14};
                if (CheckCollisionPointRec(mouse, r)) {
                    ed->selected_type = (TileType)i;
                    return;
                }
            }

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
                        ed->erase_mode = false;
                    }
                }
            }
        }
    }

    if (mouse.x < EDITOR_PANEL_WIDTH) {
        i32 wheel = (i32)GetMouseWheelMove();
        if (wheel != 0) {
            ed->palette_scroll -= wheel * 2;
            i32 total_rows = (SPRITE_TOTAL_TILES + EDITOR_PALETTE_COLS - 1) / EDITOR_PALETTE_COLS;
            if (ed->palette_scroll > total_rows - PAL_VISIBLE_ROWS) ed->palette_scroll = total_rows - PAL_VISIBLE_ROWS;
            if (ed->palette_scroll < 0) ed->palette_scroll = 0;
        }
    }

    if (IsKeyPressed(KEY_G)) {
        ed->show_grid = !ed->show_grid;
    }

    if (IsKeyPressed(KEY_L)) {
        ed->paint_background = !ed->paint_background;
    }

    if (IsKeyPressed(KEY_E)) {
        ed->erase_mode = !ed->erase_mode;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyPressed(KEY_S)) {
            if (!ed->filename[0]) {
                char path[256];
                if (FindNextFilename("levels", "level", path, sizeof(path))) {
                    EditorSaveMap(ed, path);
                }
            } else {
                EditorSaveMap(ed, ed->filename);
            }
        }

        if (IsKeyPressed(KEY_B)) {
            EditorScanFiles(ed);
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
}

static void AdjustEnemyCell(EditorState *ed, i32 direction) {
    EditorEnemyEntry *entry = &ed->enemy_entries[ed->enemy_selected_row];
    bool large = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    switch (ed->enemy_selected_col) {
        case 0:
            entry->health += direction * (large ? 10 : 1);
            if (entry->health < 1) entry->health = 1;
            break;
        case 1:
            entry->speed += (f32)direction * (large ? 0.5f : 0.1f);
            if (entry->speed < 0.1f) entry->speed = 0.1f;
            break;
        case 2:
            entry->reward += direction * (large ? 5 : 1);
            if (entry->reward < 0) entry->reward = 0;
            break;
        case 3:
            entry->damage_to_base += direction * (large ? 5 : 1);
            if (entry->damage_to_base < 0) entry->damage_to_base = 0;
            break;
        case 4:
            entry->sprite_id += direction * (large ? 10 : 1);
            if (entry->sprite_id < 0) entry->sprite_id = 0;
            break;
        case 5:
            entry->overlay_sprite_id += direction * (large ? 10 : 1);
            if (entry->overlay_sprite_id < -1) entry->overlay_sprite_id = -1;
            break;
        default:
            break;
    }

    ed->modified = true;
}

static void UpdateEnemiesSection(EditorState *ed) {
    if (IsKeyPressed(KEY_UP)) ed->enemy_selected_row = (ed->enemy_selected_row + ENEMY_TYPE_COUNT - 1) % ENEMY_TYPE_COUNT;
    if (IsKeyPressed(KEY_DOWN)) ed->enemy_selected_row = (ed->enemy_selected_row + 1) % ENEMY_TYPE_COUNT;
    if (IsKeyPressed(KEY_LEFT)) ed->enemy_selected_col = (ed->enemy_selected_col + 6 - 1) % 6;
    if (IsKeyPressed(KEY_RIGHT)) ed->enemy_selected_col = (ed->enemy_selected_col + 1) % 6;

    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
        AdjustEnemyCell(ed, +1);
    }
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        AdjustEnemyCell(ed, -1);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyPressed(KEY_S)) {
            if (!ed->enemy_filename[0]) {
                char path[256];
                if (FindNextFilename("enemies", "enemy", path, sizeof(path))) {
                    EditorSaveEnemies(ed, path);
                }
            } else {
                EditorSaveEnemies(ed, ed->enemy_filename);
            }
        }

        if (IsKeyPressed(KEY_B)) {
            EditorScanFiles(ed);
            ed->mode = MODE_BROWSE;
        }

        if (IsKeyPressed(KEY_N)) {
            EditorNewEnemies(ed);
        }
    }
}

static void AdjustTurretCell(EditorState *ed, i32 direction) {
    EditorTurretEntry *entry = &ed->turret_entries[ed->turret_selected_row];
    bool large = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    switch (ed->turret_selected_col) {
        case 0:
            entry->damage += direction * (large ? 5 : 1);
            if (entry->damage < 0) entry->damage = 0;
            break;
        case 1:
            entry->fire_rate += (f32)direction * (large ? 0.5f : 0.1f);
            if (entry->fire_rate < 0.0f) entry->fire_rate = 0.0f;
            break;
        case 2:
            entry->range += (f32)direction * (large ? 0.5f : 0.1f);
            if (entry->range < 0.0f) entry->range = 0.0f;
            break;
        case 3:
            entry->cost += direction * (large ? 5 : 1);
            if (entry->cost < 0) entry->cost = 0;
            break;
        case 4:
            entry->base_sprite_id += direction * (large ? 10 : 1);
            if (entry->base_sprite_id < 0) entry->base_sprite_id = 0;
            break;
        case 5:
            entry->gun_sprite_id += direction * (large ? 10 : 1);
            if (entry->gun_sprite_id < -1) entry->gun_sprite_id = -1;
            break;
        case 6:
            entry->shop_slot += direction;
            if (entry->shop_slot < 0) entry->shop_slot = 0;
            if (entry->shop_slot >= SHOP_TOWER_COUNT) entry->shop_slot = SHOP_TOWER_COUNT - 1;
            break;
        default:
            break;
    }

    ed->modified = true;
}

static void UpdateTurretsSection(EditorState *ed) {
    if (IsKeyPressed(KEY_UP)) ed->turret_selected_row = (ed->turret_selected_row + TOWER_TYPE_COUNT - 1) % TOWER_TYPE_COUNT;
    if (IsKeyPressed(KEY_DOWN)) ed->turret_selected_row = (ed->turret_selected_row + 1) % TOWER_TYPE_COUNT;
    if (IsKeyPressed(KEY_LEFT)) ed->turret_selected_col = (ed->turret_selected_col + 7 - 1) % 7;
    if (IsKeyPressed(KEY_RIGHT)) ed->turret_selected_col = (ed->turret_selected_col + 1) % 7;

    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
        AdjustTurretCell(ed, +1);
    }
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        AdjustTurretCell(ed, -1);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyPressed(KEY_S)) {
            if (!ed->turret_filename[0]) {
                char path[256];
                if (FindNextFilename("turrets", "turret", path, sizeof(path))) {
                    EditorSaveTurrets(ed, path);
                }
            } else {
                EditorSaveTurrets(ed, ed->turret_filename);
            }
        }

        if (IsKeyPressed(KEY_B)) {
            EditorScanFiles(ed);
            ed->mode = MODE_BROWSE;
        }

        if (IsKeyPressed(KEY_N)) {
            EditorNewTurrets(ed);
        }
    }
}

void UpdateEditor(EditorState *ed) {
    HandleGlobalSectionSwitch(ed);

    if (ed->mode == MODE_BROWSE) {
        HandleBrowseMode(ed);
        return;
    }

    if (ed->section == EDITOR_SECTION_LEVELS) {
        UpdateLevelSection(ed);
    } else if (ed->section == EDITOR_SECTION_ENEMIES) {
        UpdateEnemiesSection(ed);
    } else {
        UpdateTurretsSection(ed);
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        CloseWindow();
    }
}

static void DrawLevelSection(const EditorState *ed) {
    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            Color cell = ((x + y) % 2 == 0) ? (Color){74, 74, 74, 255} : (Color){92, 92, 92, 255};
            DrawRectangle(EDITOR_MAP_OFFSET_X + x * TILE_SIZE,
                          EDITOR_MAP_OFFSET_Y + y * TILE_SIZE,
                          TILE_SIZE, TILE_SIZE,
                          cell);
        }
    }

    for (i32 y = 0; y < ed->map.height; y++) {
        for (i32 x = 0; x < ed->map.width; x++) {
            DrawSpriteAtGrid(ed, ed->map.background_tiles[y][x], x, y);
            DrawSpriteAtGrid(ed, ed->map.tiles[y][x], x, y);

            if (ed->map.cell_types[y][x] == TILE_SPAWN) {
                DrawRectangleLinesEx(
                    (Rectangle){EDITOR_MAP_OFFSET_X + x * TILE_SIZE,
                                EDITOR_MAP_OFFSET_Y + y * TILE_SIZE,
                                TILE_SIZE,
                                TILE_SIZE},
                    2,
                    GREEN);
                DrawTextEx(ed->font,
                           "S",
                           (Vector2){EDITOR_MAP_OFFSET_X + x * TILE_SIZE + 2, EDITOR_MAP_OFFSET_Y + y * TILE_SIZE + 2},
                           16,
                           1,
                           GREEN);
            } else if (ed->map.cell_types[y][x] == TILE_BASE) {
                DrawRectangleLinesEx(
                    (Rectangle){EDITOR_MAP_OFFSET_X + x * TILE_SIZE,
                                EDITOR_MAP_OFFSET_Y + y * TILE_SIZE,
                                TILE_SIZE,
                                TILE_SIZE},
                    2,
                    RED);
                DrawTextEx(ed->font,
                           "B",
                           (Vector2){EDITOR_MAP_OFFSET_X + x * TILE_SIZE + 2, EDITOR_MAP_OFFSET_Y + y * TILE_SIZE + 2},
                           16,
                           1,
                           RED);
            }
        }
    }

    if (ed->show_grid) {
        DrawEditorGrid(ed);
    }

    Vector2 mouse = GetMousePosition();
    i32 grid_x = (i32)((mouse.x - EDITOR_MAP_OFFSET_X) / TILE_SIZE);
    i32 grid_y = (i32)((mouse.y - EDITOR_MAP_OFFSET_Y) / TILE_SIZE);

    if (grid_x >= 0 && grid_x < ed->map.width && grid_y >= 0 && grid_y < ed->map.height && mouse.x >= EDITOR_MAP_OFFSET_X) {
        DrawRectangleLinesEx((Rectangle){EDITOR_MAP_OFFSET_X + grid_x * TILE_SIZE,
                                         EDITOR_MAP_OFFSET_Y + grid_y * TILE_SIZE,
                                         TILE_SIZE,
                                         TILE_SIZE},
                             2,
                             ed->paint_background ? SKYBLUE : YELLOW);
    }

    DrawPalette(ed);

    DrawTextEx(ed->font,
               "Wave lines are preserved from file header (wave=...).",
               (Vector2){EDITOR_PANEL_WIDTH + 12, EDITOR_SCREEN_HEIGHT - 64},
               14,
               1,
               LIGHTGRAY);
}

static void DrawEnemiesSection(const EditorState *ed) {
    DrawRectangle(0, 0, EDITOR_SCREEN_WIDTH, EDITOR_SCREEN_HEIGHT, (Color){35, 35, 35, 255});

    DrawTextEx(ed->font,
               TextFormat("Enemy Config: %s", ed->enemy_config_name),
               (Vector2){30, 60},
               24,
               1,
               WHITE);

    DrawTextEx(ed->font,
               "Arrows: select cell | +/-: edit value | Shift: larger step | Ctrl+S/B/N",
               (Vector2){30, 88},
               14,
               1,
               LIGHTGRAY);

    const char *headers[] = {"Health", "Speed", "Reward", "BaseDmg", "Sprite", "Overlay"};
    i32 x0 = 30;
    i32 y0 = 130;
    i32 row_h = 42;
    i32 col_w = 130;

    DrawTextEx(ed->font, "Enemy", (Vector2){(f32)x0, (f32)y0 - 24}, 16, 1, YELLOW);
    for (i32 c = 0; c < 6; c++) {
        DrawTextEx(ed->font, headers[c], (Vector2){(f32)(x0 + 160 + c * col_w), (f32)y0 - 24}, 16, 1, YELLOW);
    }

    for (i32 r = 0; r < ENEMY_TYPE_COUNT; r++) {
        const EditorEnemyEntry *e = &ed->enemy_entries[r];
        i32 y = y0 + r * row_h;

        DrawTextEx(ed->font, e->name, (Vector2){(f32)x0, (f32)y + 10}, 18, 1, WHITE);

        char values[6][32];
        snprintf(values[0], sizeof(values[0]), "%d", e->health);
        snprintf(values[1], sizeof(values[1]), "%.2f", e->speed);
        snprintf(values[2], sizeof(values[2]), "%d", e->reward);
        snprintf(values[3], sizeof(values[3]), "%d", e->damage_to_base);
        snprintf(values[4], sizeof(values[4]), "%d", e->sprite_id);
        snprintf(values[5], sizeof(values[5]), "%d", e->overlay_sprite_id);

        for (i32 c = 0; c < 6; c++) {
            Rectangle cell = {(f32)(x0 + 150 + c * col_w), (f32)y + 4, (f32)(col_w - 10), (f32)(row_h - 8)};
            bool selected = (ed->enemy_selected_row == r && ed->enemy_selected_col == c);
            DrawRectangleRec(cell, selected ? (Color){80, 120, 170, 255} : (Color){60, 60, 60, 255});
            DrawRectangleLinesEx(cell, 1, selected ? YELLOW : GRAY);
            DrawTextEx(ed->font, values[c], (Vector2){cell.x + 8, cell.y + 8}, 16, 1, WHITE);
        }
    }
}

static void DrawTurretsSection(const EditorState *ed) {
    DrawRectangle(0, 0, EDITOR_SCREEN_WIDTH, EDITOR_SCREEN_HEIGHT, (Color){35, 35, 35, 255});

    DrawTextEx(ed->font,
               TextFormat("Turret Config: %s", ed->turret_config_name),
               (Vector2){20, 60},
               24,
               1,
               WHITE);

    DrawTextEx(ed->font,
               "Arrows: select cell | +/-: edit value | Shift: larger step | Ctrl+S/B/N",
               (Vector2){20, 88},
               14,
               1,
               LIGHTGRAY);

    const char *headers[] = {"Damage", "Rate", "Range", "Cost", "Base", "Gun", "Shop"};
    i32 x0 = 20;
    i32 y0 = 130;
    i32 row_h = 42;
    i32 col_w = 115;

    DrawTextEx(ed->font, "Turret", (Vector2){(f32)x0, (f32)y0 - 24}, 16, 1, YELLOW);
    for (i32 c = 0; c < 7; c++) {
        DrawTextEx(ed->font, headers[c], (Vector2){(f32)(x0 + 150 + c * col_w), (f32)y0 - 24}, 16, 1, YELLOW);
    }

    for (i32 r = 0; r < TOWER_TYPE_COUNT; r++) {
        const EditorTurretEntry *t = &ed->turret_entries[r];
        i32 y = y0 + r * row_h;

        DrawTextEx(ed->font, t->name, (Vector2){(f32)x0, (f32)y + 10}, 18, 1, WHITE);

        char values[7][32];
        snprintf(values[0], sizeof(values[0]), "%d", t->damage);
        snprintf(values[1], sizeof(values[1]), "%.2f", t->fire_rate);
        snprintf(values[2], sizeof(values[2]), "%.2f", t->range);
        snprintf(values[3], sizeof(values[3]), "%d", t->cost);
        snprintf(values[4], sizeof(values[4]), "%d", t->base_sprite_id);
        snprintf(values[5], sizeof(values[5]), "%d", t->gun_sprite_id);
        snprintf(values[6], sizeof(values[6]), "%d", t->shop_slot);

        for (i32 c = 0; c < 7; c++) {
            Rectangle cell = {(f32)(x0 + 140 + c * col_w), (f32)y + 4, (f32)(col_w - 10), (f32)(row_h - 8)};
            bool selected = (ed->turret_selected_row == r && ed->turret_selected_col == c);
            DrawRectangleRec(cell, selected ? (Color){80, 120, 170, 255} : (Color){60, 60, 60, 255});
            DrawRectangleLinesEx(cell, 1, selected ? YELLOW : GRAY);
            DrawTextEx(ed->font, values[c], (Vector2){cell.x + 8, cell.y + 8}, 16, 1, WHITE);
        }
    }
}

void DrawEditor(const EditorState *ed) {
    ClearBackground(BG_COLOR);

    if (ed->mode == MODE_BROWSE) {
        DrawBrowser(ed);
        return;
    }

    if (ed->section == EDITOR_SECTION_LEVELS) {
        DrawLevelSection(ed);
    } else if (ed->section == EDITOR_SECTION_ENEMIES) {
        DrawEnemiesSection(ed);
    } else {
        DrawTurretsSection(ed);
    }

    DrawToolBar(ed);
    DrawStatusBar(ed);
}
