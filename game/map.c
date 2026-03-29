#include "map.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *SPAWN_FRAME_FILES[SPAWN_ANIMATION_FRAME_COUNT] = {
    "assets/sprites/spawn/f00.png",
    "assets/sprites/spawn/f01.png",
    "assets/sprites/spawn/f02.png",
    "assets/sprites/spawn/f03.png",
    "assets/sprites/spawn/f04.png",
    "assets/sprites/spawn/f05.png",
    "assets/sprites/spawn/f06.png",
    "assets/sprites/spawn/f07.png"
};

static const char *BASE_FRAME_FILES[SPAWN_ANIMATION_FRAME_COUNT] = {
    "assets/sprites/base/f00.png",
    "assets/sprites/base/f01.png",
    "assets/sprites/base/f02.png",
    "assets/sprites/base/f03.png",
    "assets/sprites/base/f04.png",
    "assets/sprites/base/f05.png",
    "assets/sprites/base/f06.png",
    "assets/sprites/base/f07.png"
};

static Texture2D spawn_frames[SPAWN_ANIMATION_FRAME_COUNT] = {0};
static Texture2D base_frames[SPAWN_ANIMATION_FRAME_COUNT] = {0};
static bool spawn_frames_attempted = false;
static bool spawn_frames_ready = false;
static bool base_frames_attempted = false;
static bool base_frames_ready = false;

static Texture2D extra_tileset = {0};
static bool extra_tileset_attempted = false;
static bool extra_tileset_ready = false;

// Helper to draw a sprite from the spritesheet at a grid position
static void DrawSpriteAtGrid(Texture2D spritesheet, i32 sprite_id, i32 grid_x, i32 grid_y) {
    if (sprite_id < 0) {
        return;
    }
    Rectangle dest = {
        MAP_OFFSET_X + grid_x * TILE_SIZE,
        MAP_OFFSET_Y + grid_y * TILE_SIZE,
        TILE_SIZE, TILE_SIZE
    };
    if (sprite_id >= SPRITE_TOTAL_TILES && extra_tileset_ready) {
        i32 idx = sprite_id - SPRITE_TOTAL_TILES;
        i32 src_x = (idx % EXTRA_TILE_COLS) * EXTRA_TILE_SIZE;
        i32 src_y = (idx / EXTRA_TILE_COLS) * EXTRA_TILE_SIZE;
        Rectangle src = {src_x, src_y, EXTRA_TILE_SIZE, EXTRA_TILE_SIZE};
        DrawTexturePro(extra_tileset, src, dest, (Vector2){0, 0}, 0, WHITE);
    } else {
        i32 src_x = (sprite_id % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
        i32 src_y = (sprite_id / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
        Rectangle src = {src_x, src_y, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
        DrawTexturePro(spritesheet, src, dest, (Vector2){0, 0}, 0, WHITE);
    }
}

static void ReleaseFrames(Texture2D *frames) {
    for (i32 i = 0; i < SPAWN_ANIMATION_FRAME_COUNT; i++) {
        if (frames[i].id != 0) {
            UnloadTexture(frames[i]);
            frames[i] = (Texture2D){0};
        }
    }
}

static bool LoadFrames(Texture2D *frames, const char *const *files, const char *label) {
    for (i32 i = 0; i < SPAWN_ANIMATION_FRAME_COUNT; i++) {
        frames[i] = LoadTexture(files[i]);
        if (frames[i].id == 0) {
            printf("Failed to load %s animation frame: %s\n", label, files[i]);
            ReleaseFrames(frames);
            return false;
        }
        SetTextureFilter(frames[i], TEXTURE_FILTER_POINT);
    }

    return true;
}

bool LoadMapAssets(void) {
    if (!spawn_frames_attempted) {
        spawn_frames_attempted = true;
        spawn_frames_ready = LoadFrames(spawn_frames, SPAWN_FRAME_FILES, "spawn");
    }

    if (!base_frames_attempted) {
        base_frames_attempted = true;
        base_frames_ready = LoadFrames(base_frames, BASE_FRAME_FILES, "base");
    }

    if (!extra_tileset_attempted) {
        extra_tileset_attempted = true;
        extra_tileset = LoadTexture("assets/sprites/Sprite-0002.png");
        if (extra_tileset.id == 0) {
            printf("Failed to load extra tileset\n");
        } else {
            SetTextureFilter(extra_tileset, TEXTURE_FILTER_POINT);
            extra_tileset_ready = true;
        }
    }

    return spawn_frames_ready && base_frames_ready;
}

void UnloadMapAssets(void) {
    ReleaseFrames(spawn_frames);
    ReleaseFrames(base_frames);
    spawn_frames_ready = false;
    spawn_frames_attempted = false;
    base_frames_ready = false;
    base_frames_attempted = false;
    if (extra_tileset.id != 0) {
        UnloadTexture(extra_tileset);
        extra_tileset = (Texture2D){0};
    }
    extra_tileset_ready = false;
    extra_tileset_attempted = false;
}

Texture2D GetExtraTileset(void) {
    return extra_tileset;
}

static void DrawSpawnFallbackMarker(i32 grid_x, i32 grid_y) {
    f32 px = MAP_OFFSET_X + grid_x * TILE_SIZE;
    f32 py = MAP_OFFSET_Y + grid_y * TILE_SIZE;
    Rectangle cell = {px, py, TILE_SIZE, TILE_SIZE};
    Vector2 center = GridToWorld(grid_x, grid_y);
    f32 r = TILE_SIZE * 0.18f;
    f32 pad = TILE_SIZE * 0.08f;

    DrawRectangleRec(cell, ColorAlpha(SKYBLUE, 0.28f));
    DrawRectangleLinesEx((Rectangle){px + pad, py + pad, TILE_SIZE - pad * 2.0f, TILE_SIZE - pad * 2.0f}, 2.0f, BLUE);
    DrawCircleV(center, r, BLUE);
    DrawCircleLines((i32)center.x, (i32)center.y, r, WHITE);
}

static void DrawSpawnMarker(i32 grid_x, i32 grid_y) {
    f32 px = MAP_OFFSET_X + grid_x * TILE_SIZE;
    f32 py = MAP_OFFSET_Y + grid_y * TILE_SIZE;
    Rectangle cell = {px, py, TILE_SIZE, TILE_SIZE};
    f32 pad = TILE_SIZE * 0.05f;

    if (!LoadMapAssets()) {
        DrawSpawnFallbackMarker(grid_x, grid_y);
        return;
    }

    i32 frame_index = (i32)(GetTime() * SPAWN_ANIMATION_FPS) % SPAWN_ANIMATION_FRAME_COUNT;
    Texture2D frame = spawn_frames[frame_index];
    Rectangle src = {0.0f, 0.0f, (f32)frame.width, (f32)frame.height};
    Rectangle dest = {
        px + pad,
        py + pad,
        TILE_SIZE - pad * 2.0f,
        TILE_SIZE - pad * 2.0f
    };

    DrawRectangleRec(cell, ColorAlpha(SKYBLUE, 0.12f));
    DrawTexturePro(frame, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
    DrawRectangleLinesEx((Rectangle){px + pad, py + pad, TILE_SIZE - pad * 2.0f, TILE_SIZE - pad * 2.0f},
                         1.5f,
                         ColorAlpha(BLUE, 0.55f));
}

static void DrawBaseMarker(i32 grid_x, i32 grid_y) {
    f32 px = MAP_OFFSET_X + grid_x * TILE_SIZE;
    f32 py = MAP_OFFSET_Y + grid_y * TILE_SIZE;
    Rectangle cell = {px, py, TILE_SIZE, TILE_SIZE};
    f32 pad = TILE_SIZE * 0.05f;

    if (!LoadMapAssets()) {
        Vector2 center = GridToWorld(grid_x, grid_y);
        f32 half = TILE_SIZE * 0.18f;

        DrawRectangleRec(cell, ColorAlpha(RED, 0.26f));
        DrawRectangleLinesEx((Rectangle){px + pad, py + pad, TILE_SIZE - pad * 2.0f, TILE_SIZE - pad * 2.0f}, 2.0f, RED);
        DrawRectangleV((Vector2){center.x - half, center.y - half}, (Vector2){half * 2.0f, half * 2.0f}, GOLD);
        DrawRectangleLines((i32)(center.x - half), (i32)(center.y - half), (i32)(half * 2.0f), (i32)(half * 2.0f), WHITE);
        return;
    }

    i32 frame_index = (i32)(GetTime() * SPAWN_ANIMATION_FPS) % SPAWN_ANIMATION_FRAME_COUNT;
    Texture2D frame = base_frames[frame_index];
    Rectangle src = {0.0f, 0.0f, (f32)frame.width, (f32)frame.height};
    Rectangle dest = {
        px + pad,
        py + pad,
        TILE_SIZE - pad * 2.0f,
        TILE_SIZE - pad * 2.0f
    };

    DrawRectangleRec(cell, ColorAlpha(RED, 0.12f));
    DrawTexturePro(frame, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
    DrawRectangleLinesEx((Rectangle){px + pad, py + pad, TILE_SIZE - pad * 2.0f, TILE_SIZE - pad * 2.0f},
                         1.5f,
                         ColorAlpha(RED, 0.7f));
}

void DrawMap(const Map *map, Texture2D spritesheet) {
    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            DrawSpriteAtGrid(spritesheet, map->background_tiles[y][x], x, y);
            DrawSpriteAtGrid(spritesheet, map->tiles[y][x], x, y);
        }
    }

    // Highlight spawn and base cells so they remain easy to identify on every level.
    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            TileType t = map->cell_types[y][x];
            if (t == TILE_SPAWN) {
                DrawSpawnMarker(x, y);
            } else if (t == TILE_BASE) {
                DrawBaseMarker(x, y);
            }
        }
    }
}

bool IsBuildable(const Map *map, i32 grid_x, i32 grid_y) {
    if (grid_x < 0 || grid_x >= map->width || grid_y < 0 || grid_y >= map->height) {
        return false;
    }
    return map->cell_types[grid_y][grid_x] == TILE_BUILDABLE;
}

bool IsWalkable(const Map *map, i32 grid_x, i32 grid_y) {
    if (grid_x < 0 || grid_x >= map->width || grid_y < 0 || grid_y >= map->height) {
        return false;
    }
    TileType type = map->cell_types[grid_y][grid_x];
    return (type == TILE_GROUND || type == TILE_BUILDABLE ||
            type == TILE_SPAWN || type == TILE_BASE);
}

TileType GetTileType(const Map *map, i32 grid_x, i32 grid_y) {
    if (grid_x < 0 || grid_x >= map->width || grid_y < 0 || grid_y >= map->height) {
        return TILE_BLOCKED;
    }
    return map->cell_types[grid_y][grid_x];
}

void SetTileType(Map *map, i32 grid_x, i32 grid_y, TileType type) {
    if (grid_x >= 0 && grid_x < map->width && grid_y >= 0 && grid_y < map->height) {
        map->cell_types[grid_y][grid_x] = type;
    }
}

Vector2 GridToWorld(i32 grid_x, i32 grid_y) {
    return (Vector2){
        MAP_OFFSET_X + grid_x * TILE_SIZE + TILE_SIZE / 2.0f,
        MAP_OFFSET_Y + grid_y * TILE_SIZE + TILE_SIZE / 2.0f
    };
}

void WorldToGrid(Vector2 world_pos, i32 *grid_x, i32 *grid_y) {
    *grid_x = (i32)((world_pos.x - MAP_OFFSET_X) / TILE_SIZE);
    *grid_y = (i32)((world_pos.y - MAP_OFFSET_Y) / TILE_SIZE);
}

// Load map from .conf file
bool LoadMapFromConf(Map *map, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Failed to open level file: %s\n", filename);
        return false;
    }

    memset(map, 0, sizeof(Map));

    char line[1024];
    enum { SECTION_HEADER, SECTION_BACKGROUND, SECTION_TILES, SECTION_TYPES } section = SECTION_HEADER;
    i32 row = 0;
    char level_name[64] = "Untitled";

    for (i32 y = 0; y < MAP_HEIGHT; y++) {
        for (i32 x = 0; x < MAP_WIDTH; x++) {
            map->background_tiles[y][x] = SPRITE_SAND;
        }
    }

    while (fgets(line, sizeof(line), f)) {
        // Strip newline
        i32 len = (i32)strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';

        if (section == SECTION_HEADER) {
            if (strncmp(line, "name=", 5) == 0) {
                strncpy(level_name, line + 5, sizeof(level_name) - 1);
            } else if (strncmp(line, "width=", 6) == 0) {
                map->width = atoi(line + 6);
            } else if (strncmp(line, "height=", 7) == 0) {
                map->height = atoi(line + 7);
            } else if (strcmp(line, "background") == 0) {
                section = SECTION_BACKGROUND;
                row = 0;
            } else if (strcmp(line, "tiles") == 0) {
                section = SECTION_TILES;
                row = 0;
            }
        } else if (section == SECTION_BACKGROUND) {
            if (strcmp(line, "tiles") == 0) {
                section = SECTION_TILES;
                row = 0;
                continue;
            }
            if (row < map->height) {
                char *p = line;
                for (i32 x = 0; x < map->width && *p; x++) {
                    map->background_tiles[row][x] = (i32)strtol(p, &p, 10);
                }
                row++;
            }
        } else if (section == SECTION_TILES) {
            if (strcmp(line, "types") == 0) {
                section = SECTION_TYPES;
                row = 0;
                continue;
            }
            if (row < map->height) {
                char *p = line;
                for (i32 x = 0; x < map->width && *p; x++) {
                    map->tiles[row][x] = (i32)strtol(p, &p, 10);
                }
                row++;
            }
        } else if (section == SECTION_TYPES) {
            if (row < map->height) {
                char *p = line;
                for (i32 x = 0; x < map->width && *p; x++) {
                    map->cell_types[row][x] = (TileType)strtol(p, &p, 10);
                }
                row++;
            }
        }
    }

    fclose(f);

    // Rebuild spawn/base from types grid
    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            if (map->cell_types[y][x] == TILE_SPAWN && map->spawn_count < 16) {
                map->spawn_points[map->spawn_count++] = (Vector2){x, y};
            } else if (map->cell_types[y][x] == TILE_BASE && map->base_count < 16) {
                map->base_points[map->base_count++] = (Vector2){x, y};
            }
        }
    }

    if (map->spawn_count <= 0 || map->base_count <= 0) {
        printf("Invalid map in %s: requires at least 1 spawn and 1 base (got %d spawns, %d bases)\n",
               filename, map->spawn_count, map->base_count);
        return false;
    }

    printf("Map loaded from .conf: %s (%s: %dx%d, %d spawns, %d bases)\n",
           filename, level_name, map->width, map->height, map->spawn_count, map->base_count);
    return true;
}
