#include "map.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Helper to draw a sprite from the spritesheet at a grid position
static void DrawSpriteAtGrid(Texture2D spritesheet, i32 sprite_id, i32 grid_x, i32 grid_y) {
    i32 src_x = (sprite_id % SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
    i32 src_y = (sprite_id / SPRITE_SHEET_COLS) * SPRITE_TILE_SIZE;
    Rectangle src = {src_x, src_y, SPRITE_TILE_SIZE, SPRITE_TILE_SIZE};
    Rectangle dest = {
        MAP_OFFSET_X + grid_x * TILE_SIZE,
        MAP_OFFSET_Y + grid_y * TILE_SIZE,
        TILE_SIZE, TILE_SIZE
    };
    DrawTexturePro(spritesheet, src, dest, (Vector2){0, 0}, 0, WHITE);
}

// Level 1 layout: Open field with borders (free-pathing maze TD)
static void InitLevel1(Map *map) {
    map->width = 18;
    map->height = 12;

    // Fill everything with gray ground (buildable + walkable)
    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            map->tiles[y][x] = SPRITE_GROUND;
            map->cell_types[y][x] = TILE_BUILDABLE;
        }
    }

    // Top row: trees with rock corners
    for (i32 x = 0; x < map->width; x++) {
        map->tiles[0][x] = SPRITE_TREE;
        map->cell_types[0][x] = TILE_BLOCKED;
    }
    map->tiles[0][0] = SPRITE_ROCK;
    map->tiles[0][map->width - 1] = SPRITE_ROCK;

    // Bottom row: trees with rock corners
    for (i32 x = 0; x < map->width; x++) {
        map->tiles[map->height - 1][x] = SPRITE_TREE;
        map->cell_types[map->height - 1][x] = TILE_BLOCKED;
    }
    map->tiles[map->height - 1][0] = SPRITE_ROCK;
    map->tiles[map->height - 1][map->width - 1] = SPRITE_ROCK;

    // Second row from top: green grass border
    for (i32 x = 1; x < map->width - 1; x++) {
        map->tiles[1][x] = SPRITE_GRASS;
        map->cell_types[1][x] = TILE_BLOCKED;
    }

    // Second row from bottom: green grass border
    for (i32 x = 1; x < map->width - 1; x++) {
        map->tiles[map->height - 2][x] = SPRITE_GRASS;
        map->cell_types[map->height - 2][x] = TILE_BLOCKED;
    }

    // Left and right columns: rocks
    for (i32 y = 1; y < map->height - 1; y++) {
        map->tiles[y][0] = SPRITE_ROCK;
        map->cell_types[y][0] = TILE_BLOCKED;
        map->tiles[y][map->width - 1] = SPRITE_ROCK;
        map->cell_types[y][map->width - 1] = TILE_BLOCKED;
    }

    // Spawn points: left rock border (col 0, rows 2 to height-3)
    // Enemies enter from the left edge, entire inner area stays buildable
    i32 inner_top = 2;
    i32 inner_bottom = map->height - 3;
    map->spawn_count = inner_bottom - inner_top + 1;
    for (i32 i = 0; i < map->spawn_count; i++) {
        i32 y = inner_top + i;
        map->spawn_points[i] = (Vector2){0, y};
        map->cell_types[y][0] = TILE_SPAWN;
    }

    // Base points: right rock border (col width-1, rows 2 to height-3)
    map->base_count = inner_bottom - inner_top + 1;
    for (i32 i = 0; i < map->base_count; i++) {
        i32 y = inner_top + i;
        map->base_points[i] = (Vector2){map->width - 1, y};
        map->cell_types[y][map->width - 1] = TILE_BASE;
    }
}

// Level 2 layout: Open field (different spawn/base config)
static void InitLevel2(Map *map) {
    map->width = 18;
    map->height = 12;

    // Fill with gray ground
    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            map->tiles[y][x] = SPRITE_GROUND;
            map->cell_types[y][x] = TILE_BUILDABLE;
        }
    }

    // Borders (same as Level 1)
    for (i32 x = 0; x < map->width; x++) {
        map->tiles[0][x] = SPRITE_TREE;
        map->cell_types[0][x] = TILE_BLOCKED;
        map->tiles[map->height - 1][x] = SPRITE_TREE;
        map->cell_types[map->height - 1][x] = TILE_BLOCKED;
    }
    map->tiles[0][0] = SPRITE_ROCK;
    map->tiles[0][map->width - 1] = SPRITE_ROCK;
    map->tiles[map->height - 1][0] = SPRITE_ROCK;
    map->tiles[map->height - 1][map->width - 1] = SPRITE_ROCK;

    for (i32 x = 1; x < map->width - 1; x++) {
        map->tiles[1][x] = SPRITE_GRASS;
        map->cell_types[1][x] = TILE_BLOCKED;
        map->tiles[map->height - 2][x] = SPRITE_GRASS;
        map->cell_types[map->height - 2][x] = TILE_BLOCKED;
    }

    for (i32 y = 1; y < map->height - 1; y++) {
        map->tiles[y][0] = SPRITE_ROCK;
        map->cell_types[y][0] = TILE_BLOCKED;
        map->tiles[y][map->width - 1] = SPRITE_ROCK;
        map->cell_types[y][map->width - 1] = TILE_BLOCKED;
    }

    // Spawn: top-left area
    map->spawn_count = 2;
    map->spawn_points[0] = (Vector2){1, 3};
    map->spawn_points[1] = (Vector2){1, 4};
    map->cell_types[3][1] = TILE_SPAWN;
    map->cell_types[4][1] = TILE_SPAWN;

    // Base: bottom-right area
    map->base_count = 2;
    map->base_points[0] = (Vector2){map->width - 2, 7};
    map->base_points[1] = (Vector2){map->width - 2, 8};
    map->cell_types[7][map->width - 2] = TILE_BASE;
    map->cell_types[8][map->width - 2] = TILE_BASE;
}

// Level 3 layout: Open field (more spawn points)
static void InitLevel3(Map *map) {
    map->width = 18;
    map->height = 12;

    // Fill with gray ground
    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            map->tiles[y][x] = SPRITE_GROUND;
            map->cell_types[y][x] = TILE_BUILDABLE;
        }
    }

    // Borders (same pattern)
    for (i32 x = 0; x < map->width; x++) {
        map->tiles[0][x] = SPRITE_TREE;
        map->cell_types[0][x] = TILE_BLOCKED;
        map->tiles[map->height - 1][x] = SPRITE_TREE;
        map->cell_types[map->height - 1][x] = TILE_BLOCKED;
    }
    map->tiles[0][0] = SPRITE_ROCK;
    map->tiles[0][map->width - 1] = SPRITE_ROCK;
    map->tiles[map->height - 1][0] = SPRITE_ROCK;
    map->tiles[map->height - 1][map->width - 1] = SPRITE_ROCK;

    for (i32 x = 1; x < map->width - 1; x++) {
        map->tiles[1][x] = SPRITE_GRASS;
        map->cell_types[1][x] = TILE_BLOCKED;
        map->tiles[map->height - 2][x] = SPRITE_GRASS;
        map->cell_types[map->height - 2][x] = TILE_BLOCKED;
    }

    for (i32 y = 1; y < map->height - 1; y++) {
        map->tiles[y][0] = SPRITE_ROCK;
        map->cell_types[y][0] = TILE_BLOCKED;
        map->tiles[y][map->width - 1] = SPRITE_ROCK;
        map->cell_types[y][map->width - 1] = TILE_BLOCKED;
    }

    // Spawn: left side, spread across
    map->spawn_count = 6;
    for (i32 i = 0; i < 6; i++) {
        i32 y = 3 + i;
        map->spawn_points[i] = (Vector2){1, y};
        map->cell_types[y][1] = TILE_SPAWN;
    }

    // Base: right side, spread across
    map->base_count = 6;
    for (i32 i = 0; i < 6; i++) {
        i32 y = 3 + i;
        map->base_points[i] = (Vector2){map->width - 2, y};
        map->cell_types[y][map->width - 2] = TILE_BASE;
    }
}

void InitMap(Map *map, i32 level) {
    memset(map, 0, sizeof(Map));

    switch (level) {
        case 0:
            InitLevel1(map);
            break;
        case 1:
            InitLevel2(map);
            break;
        case 2:
            InitLevel3(map);
            break;
        default:
            printf("Unknown level %d, defaulting to Level 1\n", level);
            InitLevel1(map);
            break;
    }

    printf("Map initialized for level %d: %dx%d, %d spawns, %d bases\n",
           level, map->width, map->height, map->spawn_count, map->base_count);
}

void DrawMap(const Map *map, Texture2D spritesheet) {
    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            DrawSpriteAtGrid(spritesheet, map->tiles[y][x], x, y);
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

bool LoadMapFromFile(Map *map, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Failed to open level file: %s\n", filename);
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
    
    memset(map, 0, sizeof(Map));
    
    ParseJsonInt(json, "width", &map->width);
    ParseJsonInt(json, "height", &map->height);
    ParseJsonArray2D(json, "tiles", map->tiles, map->height, map->width);
    ParseJsonArray2D(json, "cell_types", (i32 (*)[MAP_WIDTH])map->cell_types, map->height, map->width);
    
    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            if (map->cell_types[y][x] == TILE_SPAWN && map->spawn_count < 16) {
                map->spawn_points[map->spawn_count++] = (Vector2){x, y};
            } else if (map->cell_types[y][x] == TILE_BASE && map->base_count < 16) {
                map->base_points[map->base_count++] = (Vector2){x, y};
            }
        }
    }
    
    free(json);
    printf("Map loaded from file: %s (%dx%d, %d spawns, %d bases)\n", 
           filename, map->width, map->height, map->spawn_count, map->base_count);
    return true;
}
