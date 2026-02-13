# Tower Defense - C + Raylib

A complete tower defense game rewritten in C using raylib, ported from the original Unity project.

## Features

### 6 Tower Types
- **Vulcan**: Standard single-target tower (10 dmg, 2.0 fire rate, 3.0 range, $10)
- **DCA**: 4-bullet burst fire (12 dmg total, 1.5 fire rate, 3.5 range, $15)
- **Freeze**: AoE slow tower (50% speed reduction, 2s duration, 2.5 range, $15)
- **Missile**: Splash damage on impact (20 dmg, 0.8 fire rate, 4.0 range, $25)
- **Plasma**: Strong single-target (15 dmg, 1.2 fire rate, 3.5 range, $20)
- **Wall**: Blocks enemy paths, no attack ($5)

### 6 Enemy Types
- **Simple**: Basic enemy (50 HP, 1.0 speed, $5 reward)
- **Fast**: Quick but weak (30 HP, 2.0 speed, $3 reward)
- **Heavy**: Slow but tanky (150 HP, 0.8 speed, $10 reward)
- **Shielded**: Shield absorbs damage (80 HP + shield, $8 reward)
- **Flying**: Flies directly to base, ignores obstacles (60 HP, 1.2 speed, $7 reward)
- **Boss**: Spawns minions on death (500 HP, 0.75 speed, $50 reward)

### 3 Levels
- **Level 1**: 10 waves, horizontal path layout
- **Level 2**: 10 waves, L-shaped path layout
- **Level 3**: 10 waves, S-shaped path layout

### Game Systems
- **A* Pathfinding**: 8-directional movement with diagonal blocking
- **Path Validation**: Ensures enemies can always reach the base
- **Wave System**: 30 total waves across 3 levels with increasing difficulty
- **Economy**: Earn currency by defeating enemies, spend to place towers
- **Interactive UI**: Click-to-place towers with range preview
- **Tower Inspection**: Click towers to view range, delete with DEL key

## Building

### Requirements
- clang compiler with C23 support
- raylib 5.5 (install via Homebrew on macOS: `brew install raylib`)
- Make

### Build Commands
```bash
# Build the game
make

# Run the game
make run

# Clean build artifacts
make clean

# Rebuild from scratch
make rebuild
```

## Controls

### Menu
- Click level buttons or press **1**, **2**, **3** to select level

### Gameplay
- **Mouse**: Click tower in shop to select, click map to place
- **Right Click / ESC**: Cancel tower placement
- **Click Tower**: Select existing tower to view range
- **DELETE / BACKSPACE**: Remove selected tower
- **SPACE**: Start next wave early
- **Mouse Hover**: See placement preview with range circle

### Game Over / Victory
- **R**: Retry current level
- **M**: Return to main menu
- **N**: Next level (victory screen only)

## File Structure
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

assets/
  sprites/         - Game sprites from Unity project
  fonts/           - Poppins font

Makefile           - Build configuration
plan.md            - Original implementation plan
```

## Game Balance

- Starting currency: $50
- Starting base life: 20
- Wave countdown: 20 seconds
- Enemy spawn delay: 0.05s
- Bullet speed: 10 tiles/s
- Tower rotation speed: 400°/s
- Freeze duration: 2s (50% slow)
- Missile splash radius: 0.5 tiles

## Credits

- Original Unity project assets (Kenney tower defense tileset)
- Poppins font
- Built with raylib game framework
- Rewritten in C with modern C23 features

## License

Assets from the original Unity project. Code implementation is original work.
