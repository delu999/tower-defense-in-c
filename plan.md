# Tower Defense — C + Raylib Rewrite Plan

Rewrite of the Unity tower defense game in pure C using raylib. Reuses all sprite/font assets from the original Unity project.

---

## 1. Project Setup

- Create project directory structure:
  ```
  src/
    main.c
    game.h / game.c          (game state, main loop)
    tower.h / tower.c        (tower types, targeting, shooting)
    enemy.h / enemy.c        (enemy types, movement, health)
    bullet.h / bullet.c      (projectile types, movement, collision)
    pathfinding.h / pathfinding.c  (A*, flood fill validation)
    wave.h / wave.c          (wave definitions, spawning)
    map.h / map.c            (tilemap loading, tile queries)
    economy.h / economy.c    (currency manager)
    ui.h / ui.c              (HUD, shop, menus, alerts)
    config.h                 (constants, balance values)
  assets/
    sprites/                 (copied from TowerDefense/Assets/Sprites/)
    fonts/                   (copied from TowerDefense/Assets/Fonts/)
  Makefile
  ```
- Dependencies: raylib (link against it; no other deps)
- Compiler: clang with `-std=c23`
- Build system: simple Makefile (clang + c23 + raylib flags)
- Target: single executable

---

## 2. Asset Migration

### Sprites
- Copy all `towerDefense_tile*.png` files from `TowerDefense/Assets/Sprites/` into `assets/sprites/`
- Copy `towerDefense_tilesheet@2.png` (the main spritesheet) — this is the primary atlas
- Copy `towerDefense_range.png` and `towerDefense_freeze_turret.png`
- The original sprites are from the Kenney tower defense tileset (individual PNGs + one big spritesheet). Decide on one approach:
  - **Recommended:** Use the single spritesheet and define source rectangles in code (fewer file loads, simpler atlas management)
  - Alternative: Load individual tile PNGs if needed

### Fonts
- Copy `Poppins-Regular.ttf` into `assets/fonts/`
- Load with `LoadFontEx()` at desired sizes

### Audio
- The original project had no audio files. Audio can be added later as a stretch goal.

---

## 3. Core Architecture

### Game State (`game.h`)
```c
typedef struct {
    GameScreen screen;       // MENU, PLAYING, GAME_OVER, VICTORY
    Map map;
    Tower towers[MAX_TOWERS];
    int tower_count;
    Enemy enemies[MAX_ENEMIES];
    int enemy_count;
    Bullet bullets[MAX_BULLETS];
    int bullet_count;
    WaveManager wave_mgr;
    int currency;
    int base_life;
    int current_level;       // 0, 1, 2
    float wave_timer;
    bool wave_active;
} GameState;
```

- Use flat arrays with counts (no malloc per entity). Define generous MAX constants (e.g. MAX_ENEMIES=512, MAX_TOWERS=128, MAX_BULLETS=1024).
- Entity "deletion" via swap-with-last (O(1) removal).
- Single `GameState` struct passed everywhere by pointer.

### Main Loop (`main.c`)
```
Init raylib window (e.g. 1280x720)
Load assets (spritesheet, font)
while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    switch (state.screen) {
        case MENU:    UpdateMenu();    break;
        case PLAYING: UpdateGame(dt);  break;
        case GAME_OVER: ...            break;
        case VICTORY:  ...             break;
    }
    BeginDrawing();
    // draw current screen
    EndDrawing();
}
Unload assets
CloseWindow
```

---

## 4. Map / Tilemap System

### Map Representation
- Each level is a 2D grid of tile IDs (e.g. `int tiles[MAP_H][MAP_W]`)
- Each cell has a type: `TILE_GROUND` (walkable), `TILE_BUILDABLE` (can place tower), `TILE_BLOCKED` (decoration/obstacle), `TILE_SPAWN`, `TILE_BASE`
- Store tile type info in a parallel `TileType cell_types[MAP_H][MAP_W]`
- When a tower is placed, mark that cell as `TILE_BLOCKED`; when removed, restore to `TILE_BUILDABLE`

### Level Data
- Define 3 levels as hardcoded C arrays or load from simple text/binary files
- Each level specifies:
  - Grid dimensions and tile IDs (for rendering)
  - Cell types (for gameplay logic)
  - Spawn point positions (list of grid coords)
  - Base/goal positions (list of grid coords)
- **Extracting level data:** The Unity `.unity` scene files are YAML. Parse the tilemap grid data from these files (or manually recreate the 3 maps). Since there are only 3 maps, manual recreation or a one-time extraction script is acceptable.

### Rendering
- Each tile drawn as a `Rectangle` source from the spritesheet, mapped to screen position
- Camera/viewport: fixed orthographic view fitting the map (no scrolling needed if map fits screen, otherwise add simple panning)

---

## 5. Pathfinding

### A* Implementation
- Grid-based A* on the cell_types grid
- 8-directional movement (diagonal allowed only if both adjacent orthogonal cells are clear — same as original)
- Heuristic: Chebyshev distance (since diagonals are allowed)
- Multiple goal cells supported (return path to nearest goal)
- Output: array of `Vector2i` waypoints (grid coords converted to world positions)

### Flood Fill Validation
- Before placing a tower, run BFS from all spawn points to all base points
- If any spawn point cannot reach any base point, reject placement
- Show alert: "Can't place turret, enemies might be stuck!"

### Path Recalculation
- When a tower is placed or destroyed, recalculate paths for all living enemies
- Each enemy stores its own path (array of waypoints + current index)

---

## 6. Towers

### Tower Types (6 total)

| ID | Name     | Damage | Fire Rate (shots/s) | Range (tiles) | Cost | Special |
|----|----------|--------|---------------------|---------------|------|---------|
| 0  | Vulcan   | 10     | 2.0                 | 3.0           | 10   | Standard single-target |
| 1  | DCA      | 12     | 1.5                 | 3.5           | 15   | 4-bullet burst (each 1/4 dmg) |
| 2  | Freeze   | 0      | 0.5                 | 2.5           | 15   | AoE slow (50% speed, 2s) |
| 3  | Missile  | 20     | 0.8                 | 4.0           | 25   | Splash damage (0.5 tile radius) |
| 4  | Plasma   | 15     | 1.2                 | 3.5           | 20   | Standard single-target |
| 5  | Wall     | 0      | 0                   | 0             | 5    | Blocks path, no attack |

*(Balance values are approximate — tune during testing.)*

### Tower Data
```c
typedef struct {
    TowerType type;
    Vector2 position;        // world position (center of tile)
    int grid_x, grid_y;     // grid coords
    float fire_countdown;
    int target_enemy_id;     // index into enemies array, or -1
    float rotation;          // facing angle
    bool active;
} Tower;
```

### Tower Logic (per frame)
1. Find nearest enemy within range (iterate enemies, check distance)
2. Rotate toward target (lerp angle, 400 deg/s)
3. Decrement fire_countdown by dt
4. When countdown <= 0: fire (spawn bullet), reset countdown to 1/fire_rate
5. Special cases:
   - **DCA:** Spawn 4 bullets with staggered delays (0.1s apart), each at 1/4 damage
   - **Freeze:** No bullet; apply slow to all enemies in range via circle check
   - **Missile:** Spawn missile bullet that deals splash on impact
   - **Wall:** Skip all targeting/shooting logic

---

## 7. Enemies

### Enemy Types (6 total)

| ID | Name     | Health | Speed | Reward | Damage to Base | Special |
|----|----------|--------|-------|--------|----------------|---------|
| 0  | Simple   | 50     | 1.0   | 5      | 1              | None |
| 1  | Fast     | 30     | 2.0   | 3      | 1              | High speed |
| 2  | Heavy    | 150    | 0.8   | 10     | 1              | High HP |
| 3  | Shielded | 80     | 1.0   | 8      | 1              | Shield absorbs 50% dmg, then 90% reduction |
| 4  | Flying   | 60     | 1.2   | 7      | 1              | Ignores obstacles (direct path) |
| 5  | Boss     | 500    | 0.75  | 50     | 5              | Spawns minions on death |

### Enemy Data
```c
typedef struct {
    EnemyType type;
    Vector2 position;
    float health, max_health;
    float base_speed;
    float speed_factor;        // 1.0 normal, 0.5 when frozen
    float freeze_timer;        // seconds remaining of slow
    int reward;
    int damage_to_base;
    float difficulty;          // health multiplier applied on spawn
    Vector2 path[MAX_PATH_LEN];
    int path_len;
    int path_index;            // current waypoint target
    float shield_hp;           // shielded enemy only
    bool active;
} Enemy;
```

### Enemy Logic (per frame)
1. If `freeze_timer > 0`: decrement by dt, apply speed_factor = 0.5; else speed_factor = 1.0
2. Move toward `path[path_index]` at `base_speed * speed_factor * dt`
3. When within threshold of waypoint: advance `path_index`
4. When path exhausted (reached base): deal `damage_to_base` to base life, deactivate enemy
5. **Flying enemies:** path is a straight line from spawn to base (2 waypoints)
6. **Boss on death:** enqueue a mini-wave of minions to spawn at boss's death position
7. **Shielded:** Override TakeDamage — damage goes to shield first (50% absorbed), remaining health takes 90% reduction

---

## 8. Bullets / Projectiles

```c
typedef struct {
    BulletType type;
    Vector2 position;
    Vector2 direction;
    float speed;               // 10.0 default
    float damage;
    float max_range;           // destroy if traveled beyond
    float distance_traveled;
    int target_enemy_id;
    bool active;
} Bullet;
```

### Bullet Logic (per frame)
1. Move toward target enemy position (homing)
2. Track distance traveled; destroy if exceeds max_range
3. If target enemy is dead/inactive: destroy bullet
4. On collision (distance < hit_radius):
   - **Standard/Plasma/DCA:** Deal damage to target, destroy bullet
   - **Missile:** Deal damage to all enemies within splash radius (0.5 tiles), destroy bullet

---

## 9. Wave System

### Wave Definitions
- Define waves as static C data (arrays of structs)
- Each wave = list of `{ enemy_type, quantity, difficulty_multiplier }`
- 3 level configs, each with 10-11 waves (matching original)

### Wave Data (from original)
**Level 1:**
| Wave | Enemies |
|------|---------|
| 1    | 3 Simple (1.0) + 3 Fast (1.0) |
| 2    | 10 Fast (1.0) |
| 3    | 5 Heavy (1.0) + 10 Simple (1.0) |
| 4    | 5 Heavy (1.5) + 10 Simple (1.5) |
| 5    | 20 Flying (5.0) |
| 6    | 5 Shielded (2.0) |
| 7    | 20 Fast (5.0) |
| 8    | 90 Simple (6.0) + 5 Shielded (2.0) |
| 9    | 40 Flying (6.0) + 40 Heavy (10.0) |
| 10   | 1 Boss (1.0) |

*(Level 2 and 3 data to be transcribed similarly from the config assets.)*

### Spawning Logic
- 20-second countdown between waves (shown on HUD)
- Player can start wave early via button
- Enemies spawn one at a time with 0.05s delay
- Random spawn point selection per enemy
- Difficulty multiplier applied to health on spawn: `health = base_health * difficulty`

---

## 10. Economy

- Starting currency: 50
- Gain currency from enemy kills (enemy.reward)
- Spend currency to place towers (tower.cost)
- Check affordability before placement
- No upgrade system (matching original)

---

## 11. UI / HUD

### Game Screens

1. **Main Menu**
   - Title text
   - Level select buttons (Level 1, 2, 3)
   - Draw with raylib text/rectangle primitives

2. **Game HUD** (drawn over gameplay)
   - Top bar: Currency display, Wave counter, Wave timer/countdown, Base HP
   - Bottom/side panel: Tower shop (6 tower icons with costs)
   - Tower placement preview: semi-transparent sprite following mouse, range circle (green=valid, red=invalid)
   - Selected tower info: name, description
   - Alert text (3-second fade): placement errors, insufficient funds

3. **Game Over Screen**
   - "Game Over" text
   - Retry / Main Menu buttons

4. **Victory Screen**
   - "Victory" text
   - Next Level / Main Menu buttons

### Tower Shop UI
- Display tower icons from spritesheet
- Show cost below each icon
- Click to select; click again to deselect
- Highlight selected tower
- When hovering over map with tower selected: show placement preview + range circle

### Tower Interaction
- Click an existing tower: show range circle + delete button
- Click delete: remove tower, refund partial cost (or no refund, matching original), recalculate paths

---

## 12. Implementation Order

Execute these phases in order. Each phase should result in a compilable, testable build.

### Phase 1: Skeleton + Window + Asset Loading
- [ ] Set up project structure, Makefile
- [ ] Copy assets from Unity project
- [ ] Create main.c: init raylib window, load spritesheet + font
- [ ] Draw a test tile from the spritesheet to verify asset loading works
- [ ] Implement game screen state machine (MENU → PLAYING → GAME_OVER/VICTORY)

### Phase 2: Map Rendering
- [ ] Define map data structure and tile types
- [ ] Hardcode Level 1 map (or write a minimal extraction from Unity YAML)
- [ ] Render tilemap from spritesheet using source rectangles
- [ ] Implement cell type queries (is_buildable, is_walkable, etc.)

### Phase 3: Pathfinding
- [ ] Implement A* on the tile grid (8-directional)
- [ ] Implement flood fill validation (BFS reachability check)
- [ ] Test: visualize paths from spawn to base on the rendered map

### Phase 4: Enemies
- [ ] Define enemy types and base Enemy struct
- [ ] Implement enemy spawning at spawn points
- [ ] Implement enemy movement along path waypoints
- [ ] Implement enemy reaching base (damage base life, deactivate)
- [ ] Draw enemies using spritesheet tiles
- [ ] Implement health bars above enemies

### Phase 5: Towers + Bullets
- [ ] Define tower types and Tower struct
- [ ] Implement tower placement on buildable tiles (click to place)
- [ ] Path validation on placement (flood fill check)
- [ ] Path recalculation for all enemies on tower place/remove
- [ ] Implement targeting (nearest enemy in range)
- [ ] Implement tower rotation toward target
- [ ] Implement bullet spawning and movement (homing)
- [ ] Implement bullet-enemy collision and damage
- [ ] Implement special tower behaviors: DCA burst, freeze AoE slow, missile splash
- [ ] Implement wall (blocks path, no shooting)

### Phase 6: Wave System + Economy
- [ ] Define wave data for all 3 levels
- [ ] Implement wave spawning with delays and countdown timer
- [ ] Implement currency system (starting money, earn on kill, spend on place)
- [ ] Wire up affordability checks to tower placement

### Phase 7: UI / HUD
- [ ] Draw HUD: currency, wave counter, timer, base HP
- [ ] Draw tower shop panel with icons and costs
- [ ] Implement tower selection + placement preview (ghost sprite + range circle)
- [ ] Implement alert messages (fade-out text)
- [ ] Implement tower click-to-select and delete flow
- [ ] Main menu screen with level select
- [ ] Game over and victory screens

### Phase 8: Polish + Remaining Content
- [ ] Add Level 2 and Level 3 maps
- [ ] Implement boss enemy (minion spawning on death)
- [ ] Implement shielded enemy damage reduction
- [ ] Implement flying enemy (direct path ignoring obstacles)
- [ ] Tune balance values (damage, costs, health, difficulty multipliers)
- [ ] Add camera adjustments if maps don't fit screen
- [ ] Final testing across all 3 levels

---

## 13. Technical Notes

- **Coordinate system:** Raylib uses screen coordinates (top-left origin, Y down). Convert grid coords to pixel coords: `screen_x = grid_x * TILE_SIZE`, `screen_y = grid_y * TILE_SIZE`. TILE_SIZE likely 64 or 128 depending on chosen resolution.
- **Spritesheet indexing:** The Kenney tileset tiles are numbered 001-299. Determine sprite layout in the spritesheet (columns × rows) and compute source rect: `src.x = (id % cols) * tile_w`, `src.y = (id / cols) * tile_h`.
- **Collision detection:** All distance-based (circle checks). No physics engine needed.
- **Memory:** All static arrays. No dynamic allocation needed for gameplay entities.
- **Frame rate:** Target 60 FPS via `SetTargetFPS(60)`. All movement/timing uses `GetFrameTime()` delta.
