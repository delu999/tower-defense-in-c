# AGENTS.md

Coding agent instructions for the Tower Defense C/Raylib project.

## Build Commands

```bash
# Build the game
make

# Build and run the game
make run

# Build the level editor
make editor

# Run the level editor
make run-editor

# Clean build artifacts
make clean

# Rebuild from scratch
make rebuild

# Debug build (extensive warnings enabled)
make DEBUG=1
```

## Project Structure

```
src/
  main.c           - Entry point, game loop, screen management
  game.h/c         - Core game state and update logic
  map.h/c          - Tilemap rendering and tile queries
  pathfinding.h/c  - A* algorithm and flood fill validation
  enemy.h/c        - Enemy spawning, movement, health
  tower.h/c        - Tower placement, targeting, shooting
  bullet.h/c       - Projectile movement and collision
  wave.h/c         - Wave definitions and spawning logic
  ui.h/c           - Tower shop, placement preview, alerts
  config.h         - Constants and balance values
  base_defs.h      - Primitive type aliases (i32, f32, etc.)

tools/editor/
  main.c           - Editor entry point
  editor.h/c       - Level editor implementation

assets/
  sprites/         - Game sprites (Kenney tower defense tileset)
  fonts/           - Poppins font

levels/            - Custom level JSON files
```

## Testing

No automated test framework is currently implemented. Manual testing via running the game:

```bash
make run
```

## Code Style Guidelines

### Type System

Use the custom type aliases from `base_defs.h`:

```c
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;
```

Use `i32` for integers, `f32` for floats, `u32` for unsigned. Use `bool` from `<stdbool.h>`.

### Naming Conventions

- **Functions**: `snake_case` - e.g., `SpawnEnemy`, `UpdateTowers`, `FindNearestEnemy`
- **Variables**: `snake_case` - e.g., `tower_count`, `grid_x`, `fire_countdown`
- **Constants/Macros**: `UPPER_SNAKE_CASE` - e.g., `MAX_TOWERS`, `TILE_SIZE`, `SCREEN_WIDTH`
- **Enum values**: `UPPER_SNAKE_CASE` with prefix - e.g., `TOWER_VULCAN`, `ENEMY_FAST`, `TILE_BUILDABLE`
- **Structs/Typedefs**: `PascalCase` - e.g., `GameState`, `TowerStats`, `WaveEntry`
- **Static functions**: Mark internal functions as `static`

### Header Files

Use include guards:

```c
#ifndef MODULE_NAME_H
#define MODULE_NAME_H

// includes
// declarations

#endif // MODULE_NAME_H
```

### Import Order

```c
#include "module.h"        // Own header first (if .c file)
#include "raylib.h"        // External library headers
#include "config.h"        // Project headers
#include <stdbool.h>       // Standard library headers
#include <stdio.h>
#include <string.h>
```

### Formatting

- Indent: 4 spaces (no tabs)
- Braces: Opening brace on same line
- Line length: Keep reasonable, no strict limit

```c
void UpdateTowers(GameState *state, f32 dt) {
    for (i32 i = 0; i < state->tower_count; i++) {
        Tower *tower = &state->towers[i];
        if (!tower->active) continue;
        // ...
    }
}
```

### Comments

Do not add comments unless requested. The codebase uses self-documenting code with clear naming. Avoid:

```c
// Bad: This function spawns an enemy
i32 SpawnEnemy(GameState *state, EnemyType type, Vector2 spawn_pos, f32 difficulty);
```

### Error Handling

- Return `-1` or `false` for failures
- Use `printf` for error/debug messages
- Check bounds before array access

```c
i32 PlaceTower(GameState *state, TowerType type, i32 grid_x, i32 grid_y) {
    if (state->currency < TOWER_STATS[type].cost) {
        printf("Not enough currency to place tower\n");
        return -1;
    }
    // ...
}
```

### Memory Management

- Use static arrays with MAX_* constants for game entities
- Use `memset` to clear structures before initialization
- Use `malloc`/`free` only for truly dynamic structures (e.g., pathfinding queues)
- Entity removal: Swap with last element for O(1) removal

```c
void RemoveEnemy(GameState *state, i32 index) {
    if (index < 0 || index >= state->enemy_count) return;
    state->enemies[index] = state->enemies[state->enemy_count - 1];
    state->enemy_count--;
}
```

### Enums

Use `typedef enum` with explicit values when needed:

```c
typedef enum {
    TOWER_VULCAN = 0,
    TOWER_DCA = 1,
    TOWER_FREEZE = 2,
    // ...
} TowerType;
```

### Structs

Use `typedef struct` with PascalCase:

```c
typedef struct {
    TowerType type;
    Vector2 position;
    i32 grid_x, grid_y;
    f32 fire_countdown;
    bool active;
} Tower;
```

### Constants

Define constants in `config.h` using macros:

```c
#define MAX_TOWERS 128
#define MAX_ENEMIES 512
#define TILE_SIZE 54
```

Use static const arrays for lookup tables:

```c
static const TowerStats TOWER_STATS[] = {
    [0] = { .damage = 10, .fire_rate = 1.0f, .range = 2.0f, .cost = 5 },
    // ...
};
```

### Switch Statements

Always include `default` case:

```c
switch (tower->type) {
    case TOWER_VULCAN:
        // ...
        break;
    default:
        // Handle unknown type
        break;
}
```

### raylib Usage

- Use `Vector2` for positions
- Use `Rectangle` for UI elements and sprite regions
- Use `Color` from raylib (e.g., `WHITE`, `RED`, `ColorAlpha(WHITE, 0.5f)`)
- Texture loading: Check `texture.id == 0` for failure

## Platform Notes

- macOS: Requires Homebrew raylib installation (`brew install raylib`)
- Linux: Requires libraylib-dev package
- Compiler: clang with C23 support
