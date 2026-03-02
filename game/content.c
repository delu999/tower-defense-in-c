#include "content.h"
#include "map.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static bool ParseEnemyEntry(GameState *state, const char *line, bool seen[ENEMY_TYPE_COUNT]) {
    char work[512];
    strncpy(work, line, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    char *save = NULL;
    char *token = strtok_r(work, ",", &save);
    if (!token) return false;
    i32 index = atoi(Trim(token));
    if (index < 0 || index >= ENEMY_TYPE_COUNT) return false;

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    strncpy(state->enemy_config[index].name, Trim(token), CONFIG_NAME_LEN - 1);
    state->enemy_config[index].name[CONFIG_NAME_LEN - 1] = '\0';

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->enemy_config[index].stats.health = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->enemy_config[index].stats.speed = strtof(Trim(token), NULL);

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->enemy_config[index].stats.reward = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->enemy_config[index].stats.damage_to_base = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->enemy_config[index].sprite_id = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->enemy_config[index].overlay_sprite_id = atoi(Trim(token));

    seen[index] = true;
    return true;
}

static bool ParseTurretEntry(GameState *state, const char *line, bool seen[TOWER_TYPE_COUNT], bool shop_used[SHOP_TOWER_COUNT]) {
    char work[512];
    strncpy(work, line, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    char *save = NULL;
    char *token = strtok_r(work, ",", &save);
    if (!token) return false;
    i32 index = atoi(Trim(token));
    if (index < 0 || index >= TOWER_TYPE_COUNT) return false;

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    strncpy(state->tower_config[index].name, Trim(token), CONFIG_NAME_LEN - 1);
    state->tower_config[index].name[CONFIG_NAME_LEN - 1] = '\0';

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->tower_config[index].stats.damage = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->tower_config[index].stats.fire_rate = strtof(Trim(token), NULL);

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->tower_config[index].stats.range = strtof(Trim(token), NULL);

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->tower_config[index].stats.cost = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->tower_config[index].base_sprite_id = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    state->tower_config[index].gun_sprite_id = atoi(Trim(token));

    token = strtok_r(NULL, ",", &save);
    if (!token) return false;
    i32 shop_slot = atoi(Trim(token));
    if (shop_slot < 0 || shop_slot >= SHOP_TOWER_COUNT || shop_used[shop_slot]) {
        return false;
    }

    state->shop_tower_order[shop_slot] = index;
    shop_used[shop_slot] = true;
    seen[index] = true;
    return true;
}

static bool ParseWaveEntryList(GameState *state, const char *line) {
    if (state->level_total_waves >= MAX_LEVEL_WAVES) {
        return false;
    }

    char work[1024];
    strncpy(work, line, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    i32 wave_size = 0;
    char *save = NULL;
    char *token = strtok_r(work, ",", &save);

    while (token) {
        i32 type = 0;
        i32 quantity = 0;
        f32 difficulty = 1.0f;
        char *entry = Trim(token);
        if (sscanf(entry, "%d:%d:%f", &type, &quantity, &difficulty) != 3) {
            return false;
        }
        if (type < 0 || type >= ENEMY_TYPE_COUNT || quantity <= 0 || difficulty <= 0.0f) {
            return false;
        }
        if (state->level_wave_entry_count >= MAX_LEVEL_WAVE_ENTRIES) {
            return false;
        }

        state->level_wave_entries[state->level_wave_entry_count++] = (WaveEntry){
            .enemy_type = (EnemyType)type,
            .quantity = quantity,
            .difficulty = difficulty
        };
        wave_size++;
        token = strtok_r(NULL, ",", &save);
    }

    if (wave_size <= 0) {
        return false;
    }

    state->level_wave_sizes[state->level_total_waves++] = wave_size;
    return true;
}

bool LoadEnemyConfig(GameState *state, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open enemy config: %s\n", filename);
        return false;
    }

    bool seen[ENEMY_TYPE_COUNT] = {0};
    memset(state->enemy_config, 0, sizeof(state->enemy_config));

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        StripLineEnding(line);
        char *trimmed = Trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }

        if (strncmp(trimmed, "name=", 5) == 0) {
            continue;
        }

        if (strncmp(trimmed, "enemy=", 6) == 0) {
            if (!ParseEnemyEntry(state, trimmed + 6, seen)) {
                fclose(file);
                printf("Invalid enemy entry in %s: %s\n", filename, trimmed);
                return false;
            }
        }
    }

    fclose(file);

    for (i32 i = 0; i < ENEMY_TYPE_COUNT; i++) {
        if (!seen[i]) {
            printf("Missing enemy entry %d in %s\n", i, filename);
            return false;
        }
    }

    strncpy(state->enemy_config_file, filename, sizeof(state->enemy_config_file) - 1);
    state->enemy_config_file[sizeof(state->enemy_config_file) - 1] = '\0';
    printf("Enemy config loaded: %s\n", filename);
    return true;
}

bool LoadTurretConfig(GameState *state, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open turret config: %s\n", filename);
        return false;
    }

    bool seen[TOWER_TYPE_COUNT] = {0};
    bool shop_used[SHOP_TOWER_COUNT] = {0};
    memset(state->tower_config, 0, sizeof(state->tower_config));
    memset(state->shop_tower_order, 0, sizeof(state->shop_tower_order));

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        StripLineEnding(line);
        char *trimmed = Trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }

        if (strncmp(trimmed, "name=", 5) == 0) {
            continue;
        }

        if (strncmp(trimmed, "turret=", 7) == 0) {
            if (!ParseTurretEntry(state, trimmed + 7, seen, shop_used)) {
                fclose(file);
                printf("Invalid turret entry in %s: %s\n", filename, trimmed);
                return false;
            }
        }
    }

    fclose(file);

    for (i32 i = 0; i < TOWER_TYPE_COUNT; i++) {
        if (!seen[i]) {
            printf("Missing turret entry %d in %s\n", i, filename);
            return false;
        }
    }
    for (i32 i = 0; i < SHOP_TOWER_COUNT; i++) {
        if (!shop_used[i]) {
            printf("Missing shop slot %d in %s\n", i, filename);
            return false;
        }
    }

    strncpy(state->turret_config_file, filename, sizeof(state->turret_config_file) - 1);
    state->turret_config_file[sizeof(state->turret_config_file) - 1] = '\0';
    printf("Turret config loaded: %s\n", filename);
    return true;
}

bool LoadLevelConfig(GameState *state, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open level file: %s\n", filename);
        return false;
    }

    state->level_total_waves = 0;
    state->level_wave_entry_count = 0;
    state->level_name[0] = '\0';
    state->enemy_config_file[0] = '\0';
    state->turret_config_file[0] = '\0';

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        StripLineEnding(line);
        char *trimmed = Trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }

        if (strcmp(trimmed, "background") == 0 || strcmp(trimmed, "tiles") == 0) {
            break;
        }

        if (strncmp(trimmed, "name=", 5) == 0) {
            strncpy(state->level_name, trimmed + 5, sizeof(state->level_name) - 1);
            state->level_name[sizeof(state->level_name) - 1] = '\0';
            continue;
        }

        if (strncmp(trimmed, "enemies_file=", 13) == 0) {
            strncpy(state->enemy_config_file, trimmed + 13, sizeof(state->enemy_config_file) - 1);
            state->enemy_config_file[sizeof(state->enemy_config_file) - 1] = '\0';
            continue;
        }

        if (strncmp(trimmed, "turrets_file=", 13) == 0) {
            strncpy(state->turret_config_file, trimmed + 13, sizeof(state->turret_config_file) - 1);
            state->turret_config_file[sizeof(state->turret_config_file) - 1] = '\0';
            continue;
        }

        if (strncmp(trimmed, "wave=", 5) == 0) {
            if (!ParseWaveEntryList(state, trimmed + 5)) {
                fclose(file);
                printf("Invalid wave line in %s: %s\n", filename, trimmed);
                return false;
            }
        }
    }

    fclose(file);

    if (state->enemy_config_file[0] == '\0' || state->turret_config_file[0] == '\0') {
        printf("Level %s must define enemies_file= and turrets_file=\n", filename);
        return false;
    }

    if (state->level_total_waves <= 0 || state->level_wave_entry_count <= 0) {
        printf("Level %s must define at least one wave\n", filename);
        return false;
    }

    if (!LoadMapFromConf(&state->map, filename)) {
        return false;
    }

    if (!LoadEnemyConfig(state, state->enemy_config_file)) {
        return false;
    }

    if (!LoadTurretConfig(state, state->turret_config_file)) {
        return false;
    }

    strncpy(state->level_filename, filename, sizeof(state->level_filename) - 1);
    state->level_filename[sizeof(state->level_filename) - 1] = '\0';

    printf("Level config loaded: %s (%d waves, %d entries)\n",
           filename, state->level_total_waves, state->level_wave_entry_count);
    return true;
}

const char *GetEnemyName(const GameState *state, EnemyType type) {
    if (type < ENEMY_SIMPLE || type > ENEMY_BOSS) {
        return "Enemy";
    }
    if (state->enemy_config[type].name[0] == '\0') {
        return "Enemy";
    }
    return state->enemy_config[type].name;
}

const char *GetTowerName(const GameState *state, TowerType type) {
    if (type < TOWER_VULCAN || type > TOWER_WALL) {
        return "Tower";
    }
    if (state->tower_config[type].name[0] == '\0') {
        return "Tower";
    }
    return state->tower_config[type].name;
}
