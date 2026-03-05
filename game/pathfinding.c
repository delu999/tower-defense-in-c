#include "pathfinding.h"
#include "map.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Helper structures for A*
typedef struct {
    i32 x, y;
    f32 g, h, f;  // g = cost from start, h = heuristic, f = g + h
    i32 parent_x, parent_y;
    bool in_open, in_closed;
} Node;

typedef struct {
    Node **nodes;
    i32 count;
    i32 capacity;
} PriorityQueue;

// Chebyshev distance (for 8-directional movement)
static f32 Heuristic(i32 x1, i32 y1, i32 x2, i32 y2) {
    i32 dx = abs(x1 - x2);
    i32 dy = abs(y1 - y2);
    return (f32)fmax(dx, dy);  // Chebyshev distance
}

// Priority queue operations
static void PQ_Init(PriorityQueue *pq, i32 capacity) {
    pq->nodes = malloc(sizeof(Node*) * capacity);
    pq->count = 0;
    pq->capacity = capacity;
}

static void PQ_Free(PriorityQueue *pq) {
    free(pq->nodes);
}

static void PQ_Push(PriorityQueue *pq, Node *node) {
    if (pq->count >= pq->capacity) return;  // Full

    // Insert at end and bubble up
    i32 i = pq->count++;
    pq->nodes[i] = node;

    while (i > 0) {
        i32 parent = (i - 1) / 2;
        if (pq->nodes[i]->f >= pq->nodes[parent]->f) break;
        // Swap
        Node *temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[parent];
        pq->nodes[parent] = temp;
        i = parent;
    }
}

static Node* PQ_Pop(PriorityQueue *pq) {
    if (pq->count == 0) return NULL;

    Node *result = pq->nodes[0];
    pq->nodes[0] = pq->nodes[--pq->count];

    // Bubble down
    i32 i = 0;
    while (true) {
        i32 left = 2 * i + 1;
        i32 right = 2 * i + 2;
        i32 smallest = i;

        if (left < pq->count && pq->nodes[left]->f < pq->nodes[smallest]->f) {
            smallest = left;
        }
        if (right < pq->count && pq->nodes[right]->f < pq->nodes[smallest]->f) {
            smallest = right;
        }

        if (smallest == i) break;

        // Swap
        Node *temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[smallest];
        pq->nodes[smallest] = temp;
        i = smallest;
    }

    return result;
}

static bool PQ_IsEmpty(const PriorityQueue *pq) {
    return pq->count == 0;
}

// Check if diagonal movement is valid (both adjacent orthogonal cells must be clear)
static bool CanMoveDiagonal(const Map *map, i32 from_x, i32 from_y, i32 to_x, i32 to_y) {
    i32 dx = to_x - from_x;
    i32 dy = to_y - from_y;

    // Not a diagonal move
    if (abs(dx) != 1 || abs(dy) != 1) return true;

    // Check both adjacent orthogonal cells
    bool orth1 = IsWalkable(map, from_x + dx, from_y);
    bool orth2 = IsWalkable(map, from_x, from_y + dy);

    return orth1 && orth2;
}

i32 FindPath(const Map *map, Vector2 start_grid, const Vector2 *goals, i32 goal_count,
             Vector2 *out_path, i32 max_path_len) {
    if (goal_count == 0) return 0;

    i32 start_x = (i32)start_grid.x;
    i32 start_y = (i32)start_grid.y;

    // Find closest goal using Chebyshev distance
    i32 closest_goal = 0;
    f32 min_dist = Heuristic(start_x, start_y, (i32)goals[0].x, (i32)goals[0].y);
    for (i32 i = 1; i < goal_count; i++) {
        f32 dist = Heuristic(start_x, start_y, (i32)goals[i].x, (i32)goals[i].y);
        if (dist < min_dist) {
            min_dist = dist;
            closest_goal = i;
        }
    }

    i32 goal_x = (i32)goals[closest_goal].x;
    i32 goal_y = (i32)goals[closest_goal].y;

    // Initialize node grid
    Node nodes[MAP_HEIGHT][MAP_WIDTH];
    memset(nodes, 0, sizeof(nodes));

    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            nodes[y][x].x = x;
            nodes[y][x].y = y;
            nodes[y][x].g = INFINITY;
            nodes[y][x].h = Heuristic(x, y, goal_x, goal_y);
            nodes[y][x].f = INFINITY;
            nodes[y][x].parent_x = -1;
            nodes[y][x].parent_y = -1;
            nodes[y][x].in_open = false;
            nodes[y][x].in_closed = false;
        }
    }

    // Initialize start node
    Node *start_node = &nodes[start_y][start_x];
    start_node->g = 0;
    start_node->f = start_node->h;

    // Priority queue
    PriorityQueue open_set;
    PQ_Init(&open_set, MAP_WIDTH * MAP_HEIGHT);
    PQ_Push(&open_set, start_node);
    start_node->in_open = true;

    // 8 directions: N, NE, E, SE, S, SW, W, NW
    const i32 dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
    const i32 dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};
    const f32 cost[] = {1.0f, 1.414f, 1.0f, 1.414f, 1.0f, 1.414f, 1.0f, 1.414f};  // sqrt(2) for diagonals

    bool found = false;
    Node *goal_node = NULL;

    // A* main loop
    while (!PQ_IsEmpty(&open_set)) {
        Node *current = PQ_Pop(&open_set);
        current->in_open = false;
        current->in_closed = true;

        // Check if reached any goal
        for (i32 i = 0; i < goal_count; i++) {
            if (current->x == (i32)goals[i].x && current->y == (i32)goals[i].y) {
                found = true;
                goal_node = current;
                break;
            }
        }

        if (found) break;

        // Explore neighbors
        for (i32 dir = 0; dir < 8; dir++) {
            i32 nx = current->x + dx[dir];
            i32 ny = current->y + dy[dir];

            // Out of bounds
            if (nx < 0 || nx >= map->width || ny < 0 || ny >= map->height) continue;

            Node *neighbor = &nodes[ny][nx];

            // Not walkable or already closed
            if (!IsWalkable(map, nx, ny) || neighbor->in_closed) continue;

            // Check diagonal blocking
            if (!CanMoveDiagonal(map, current->x, current->y, nx, ny)) continue;

            f32 tentative_g = current->g + cost[dir];

            if (tentative_g < neighbor->g) {
                neighbor->parent_x = current->x;
                neighbor->parent_y = current->y;
                neighbor->g = tentative_g;
                neighbor->f = neighbor->g + neighbor->h;

                if (!neighbor->in_open) {
                    PQ_Push(&open_set, neighbor);
                    neighbor->in_open = true;
                }
            }
        }
    }

    PQ_Free(&open_set);

    // Reconstruct path
    if (!found) {
        return 0;  // No path found
    }

    // Build path backwards
    i32 path_len = 0;
    Node *current = goal_node;
    while (current->parent_x != -1 && path_len < max_path_len) {
        out_path[path_len++] = (Vector2){current->x, current->y};
        current = &nodes[current->parent_y][current->parent_x];
    }

    // Add start node
    if (path_len < max_path_len) {
        out_path[path_len++] = (Vector2){start_x, start_y};
    }

    // Reverse path (currently backwards)
    for (i32 i = 0; i < path_len / 2; i++) {
        Vector2 temp = out_path[i];
        out_path[i] = out_path[path_len - 1 - i];
        out_path[path_len - 1 - i] = temp;
    }

    return path_len;
}

bool ValidatePaths(const Map *map) {
    // BFS from each spawn to check if any base is reachable
    if (map->spawn_count <= 0 || map->base_count <= 0) {
        printf("Validation failed: map requires at least 1 spawn and 1 base\n");
        return false;
    }

    for (i32 s = 0; s < map->spawn_count; s++) {
        i32 start_x = (i32)map->spawn_points[s].x;
        i32 start_y = (i32)map->spawn_points[s].y;

        bool visited[MAP_HEIGHT][MAP_WIDTH] = {0};
        i32 queue_x[MAP_WIDTH * MAP_HEIGHT];
        i32 queue_y[MAP_WIDTH * MAP_HEIGHT];
        i32 head = 0, tail = 0;

        queue_x[tail] = start_x;
        queue_y[tail] = start_y;
        tail++;
        visited[start_y][start_x] = true;

        bool found_base = false;

        // BFS
        while (head < tail) {
            i32 x = queue_x[head];
            i32 y = queue_y[head];
            head++;

            // Check if reached a base
            for (i32 b = 0; b < map->base_count; b++) {
                if (x == (i32)map->base_points[b].x && y == (i32)map->base_points[b].y) {
                    found_base = true;
                    break;
                }
            }

            if (found_base) break;

            // Explore 8 directions
            const i32 dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
            const i32 dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

            for (i32 dir = 0; dir < 8; dir++) {
                i32 nx = x + dx[dir];
                i32 ny = y + dy[dir];

                if (nx < 0 || nx >= map->width || ny < 0 || ny >= map->height) continue;
                if (visited[ny][nx]) continue;
                if (!IsWalkable(map, nx, ny)) continue;
                if (!CanMoveDiagonal(map, x, y, nx, ny)) continue;

                visited[ny][nx] = true;
                queue_x[tail] = nx;
                queue_y[tail] = ny;
                tail++;
            }
        }

        if (!found_base) {
            printf("Validation failed: spawn %d cannot reach any base\n", s);
            return false;
        }
    }

    printf("Path validation passed: all spawns can reach a base\n");
    return true;
}

void DrawPath(const Vector2 *path, i32 path_len, Color color) {
    for (i32 i = 0; i < path_len - 1; i++) {
        Vector2 start = GridToWorld((i32)path[i].x, (i32)path[i].y);
        Vector2 end = GridToWorld((i32)path[i + 1].x, (i32)path[i + 1].y);
        DrawLineEx(start, end, 4.0f, color);
    }

    // Draw waypoint circles
    for (i32 i = 0; i < path_len; i++) {
        Vector2 pos = GridToWorld((i32)path[i].x, (i32)path[i].y);
        DrawCircle((i32)pos.x, (i32)pos.y, 6, color);
    }
}

// FLOW FIELD

static bool CanMoveDiagonalFlow(const u8 *cost_field, u32 width, u32 from, u32 to) {
    u32 fx = from % width;
    u32 fy = from / width;
    u32 tx = to % width;
    u32 ty = to / width;

    i32 dx = (i32)tx - (i32)fx;
    i32 dy = (i32)ty - (i32)fy;

    if (abs(dx) != 1 || abs(dy) != 1) return true;

    u32 orth1 = fy * width + (u32)((i32)fx + dx);
    u32 orth2 = (u32)((i32)fy + dy) * width + fx;
    return cost_field[orth1] != 255 && cost_field[orth2] != 255;
}

static u8 Get_Neighbors(const u8 *cost_field, u32 width, u32 height, u32 current, u32 *neighbors) {
    u32 x = current % width;
    u32 y = current / width;
    u8 size = 0;

    bool left  = x > 0;
    bool right = x < width - 1;
    bool up    = y > 0;
    bool down  = y < height - 1;

    // cardinal
    if (left)  neighbors[size++] = y * width + (x - 1);
    if (right) neighbors[size++] = y * width + (x + 1);
    if (up)    neighbors[size++] = (y - 1) * width + x;
    if (down)  neighbors[size++] = (y + 1) * width + x;

    // diagonal (prevent corner-cutting through blocked orthogonal cells)
    if (up && left) {
        u32 n = (y - 1) * width + (x - 1);
        if (CanMoveDiagonalFlow(cost_field, width, current, n)) neighbors[size++] = n;
    }
    if (up && right) {
        u32 n = (y - 1) * width + (x + 1);
        if (CanMoveDiagonalFlow(cost_field, width, current, n)) neighbors[size++] = n;
    }
    if (down && left) {
        u32 n = (y + 1) * width + (x - 1);
        if (CanMoveDiagonalFlow(cost_field, width, current, n)) neighbors[size++] = n;
    }
    if (down && right) {
        u32 n = (y + 1) * width + (x + 1);
        if (CanMoveDiagonalFlow(cost_field, width, current, n)) neighbors[size++] = n;
    }

    return size;
}

// Map a coordinate delta (dx, dy) to a Direction enum value
static Direction DeltaToDir(i32 dx, i32 dy) {
    if (dx ==  0 && dy == -1) return DIR_NORTH;
    if (dx ==  0 && dy ==  1) return DIR_SOUTH;
    if (dx ==  1 && dy ==  0) return DIR_EAST;
    if (dx == -1 && dy ==  0) return DIR_WEST;
    if (dx ==  1 && dy == -1) return DIR_NORTH_EAST;
    if (dx == -1 && dy == -1) return DIR_NORTH_WEST;
    if (dx ==  1 && dy ==  1) return DIR_SOUTH_EAST;
    if (dx == -1 && dy ==  1) return DIR_SOUTH_WEST;
    return DIR_NONE;
}

static bool IsValidDirection(Direction dir) {
    return dir >= DIR_NONE && dir <= DIR_SOUTH_WEST;
}

// Lightweight min-heap for Dijkstra integration field
typedef struct { u32 cost; u32 idx; } FlowNode;

typedef struct {
    FlowNode *data;
    u32 count;
    u32 capacity;
} FlowPQ;

static void FlowPQ_Init(FlowPQ *pq, u32 capacity) {
    pq->data = malloc(capacity * sizeof(FlowNode));
    pq->count = 0;
    pq->capacity = capacity;
}

static void FlowPQ_Free(FlowPQ *pq) {
    free(pq->data);
}

static void FlowPQ_Push(FlowPQ *pq, u32 cost, u32 idx) {
    if (pq->count >= pq->capacity) return;
    u32 i = pq->count++;
    pq->data[i] = (FlowNode){cost, idx};
    while (i > 0) {
        u32 parent = (i - 1) / 2;
        if (pq->data[i].cost >= pq->data[parent].cost) break;
        FlowNode tmp = pq->data[i]; pq->data[i] = pq->data[parent]; pq->data[parent] = tmp;
        i = parent;
    }
}

static FlowNode FlowPQ_Pop(FlowPQ *pq) {
    FlowNode result = pq->data[0];
    pq->data[0] = pq->data[--pq->count];
    u32 i = 0;
    while (true) {
        u32 l = 2*i+1, r = 2*i+2, s = i;
        if (l < pq->count && pq->data[l].cost < pq->data[s].cost) s = l;
        if (r < pq->count && pq->data[r].cost < pq->data[s].cost) s = r;
        if (s == i) break;
        FlowNode tmp = pq->data[i]; pq->data[i] = pq->data[s]; pq->data[s] = tmp;
        i = s;
    }
    return result;
}

Direction *CreateFlowFieldWithReachability(const Map *map, bool *out_reachable, i32 out_reachable_len) {
    u32 grid_size = map->width * map->height;

    // cost field
    u8 *cost_field = malloc(grid_size * sizeof(u8));
    if (!cost_field) {
        printf("Failed to allocate memory for cost field\n");
        return NULL;
    }

    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            u32 idx = (u32)y * (u32)map->width + (u32)x;
            cost_field[idx] = map->cell_types[y][x] == TILE_BLOCKED ? 255 : 1;
        }
    }

    for (i32 y = 0; y < map->height; y++) {
        for (i32 x = 0; x < map->width; x++) {
            u32 idx = (u32)y * (u32)map->width + (u32)x;
            if (cost_field[idx] == 255) continue;

            u32 neighbors[8];
            u8 neighbors_size = Get_Neighbors(cost_field, (u32)map->width, (u32)map->height, idx, neighbors);
            for (u8 i = 0; i < neighbors_size; i++) {
                if (cost_field[neighbors[i]] == 255) {
                    cost_field[idx] = 2; // penalty for being next to a wall
                    break;
                }
            }
        }
    }

    // integration field — Dijkstra from all bases
    u32 *integration_field = malloc(grid_size * sizeof(u32));
    if (!integration_field) {
        printf("Failed to allocate memory for integration field\n");
        free(cost_field);
        return NULL;
    }
    for (u32 i = 0; i < grid_size; i++) integration_field[i] = UINT32_MAX;

    // Each cell can be relaxed multiple times (lazy Dijkstra), worst case 8× per cell
    // Helps with avoiding congestions on some routes
    FlowPQ pq;
    FlowPQ_Init(&pq, grid_size * 8);
    if (!pq.data) {
        printf("Failed to allocate memory for flow field priority queue\n");
        free(cost_field);
        free(integration_field);
        return NULL;
    }

    for (u32 y = 0; y < (u32)map->height; y++) {
        for (u32 x = 0; x < (u32)map->width; x++) {
            u32 idx = y * (u32)map->width + x;
            if (map->cell_types[y][x] == TILE_BASE) {
                integration_field[idx] = 0;
                FlowPQ_Push(&pq, 0, idx);
            }
        }
    }

    while (pq.count > 0) {
        FlowNode cur = FlowPQ_Pop(&pq);

        if (cur.cost > integration_field[cur.idx]) continue;

        u32 cx = cur.idx % (u32)map->width;
        u32 cy = cur.idx / (u32)map->width;

        u32 neighbors[8];
        u8 neighbors_size = Get_Neighbors(cost_field, (u32)map->width, (u32)map->height, cur.idx, neighbors);

        for (u8 i = 0; i < neighbors_size; i++) {
            u32 neighbor = neighbors[i];
            if (cost_field[neighbor] == 255) continue;  // impassable

            u32 nx = neighbor % (u32)map->width;
            u32 ny = neighbor / (u32)map->width;
            u32 move_cost = ((nx != cx) && (ny != cy)) ? 14 : 10;
            u32 new_cost = integration_field[cur.idx] + (u32)cost_field[neighbor] * move_cost;

            if (new_cost < integration_field[neighbor]) {
                integration_field[neighbor] = new_cost;
                FlowPQ_Push(&pq, new_cost, neighbor);
            }
        }
    }

    FlowPQ_Free(&pq);

    if (out_reachable && out_reachable_len > 0) {
        i32 reachable_count = (i32)grid_size;
        if (out_reachable_len < reachable_count) {
            reachable_count = out_reachable_len;
        }

        for (i32 i = 0; i < reachable_count; i++) {
            out_reachable[i] = integration_field[i] != UINT32_MAX;
        }

        for (i32 i = reachable_count; i < out_reachable_len; i++) {
            out_reachable[i] = false;
        }
    }

    // flow field — for each cell, point toward the neighbor with lowest integration cost
    Direction *flow_field = malloc(grid_size * sizeof(Direction));
    if (!flow_field) {
        printf("Failed to allocate memory for flow field\n");
        free(cost_field);
        free(integration_field);
        return NULL;
    }
    for (u32 i = 0; i < grid_size; i++) {
        flow_field[i] = DIR_NONE;
    }

    for (u32 y = 0; y < (u32)map->height; y++) {
        for (u32 x = 0; x < (u32)map->width; x++) {
            u32 idx = y * (u32)map->width + x;

            if (cost_field[idx] == 255) {
                flow_field[idx] = DIR_NONE;
                continue;
            }

            u32 neighbors[8];
            u8 neighbors_size = Get_Neighbors(cost_field, (u32)map->width, (u32)map->height, idx, neighbors);

            u32 best_cost = integration_field[idx];
            Direction best_dir = DIR_NONE;
            bool best_is_diagonal = false;
            for (u8 i = 0; i < neighbors_size; i++) {
                u32 neighbor = neighbors[i];
                u32 neighbor_cost = integration_field[neighbor];
                if (neighbor_cost == UINT32_MAX) continue;

                i32 nx = (i32)(neighbor % (u32)map->width);
                i32 ny = (i32)(neighbor / (u32)map->width);
                bool candidate_is_diagonal = (nx != (i32)x) && (ny != (i32)y);

                bool better_cost = neighbor_cost < best_cost;
                bool tie_break_diagonal = (neighbor_cost == best_cost) &&
                                          (best_dir != DIR_NONE) &&
                                          candidate_is_diagonal &&
                                          !best_is_diagonal;

                if (better_cost || tie_break_diagonal) {
                    best_cost = neighbor_cost;
                    best_dir = DeltaToDir(nx - (i32)x, ny - (i32)y);
                    best_is_diagonal = candidate_is_diagonal;
                }
            }
            flow_field[idx] = best_dir;
        }
    }

    free(cost_field);
    free(integration_field);
    return flow_field;
}

Direction *CreateFlowField(const Map *map) {
    return CreateFlowFieldWithReachability(map, NULL, 0);
}

static const Vector2 dir_arrow_vec[9] = {
    { 0,    0   },  // DIR_NONE
    { 0,   -1   },  // DIR_NORTH
    { 0,    1   },  // DIR_SOUTH
    { 1,    0   },  // DIR_EAST
    {-1,    0   },  // DIR_WEST
    { 0.707f, -0.707f },  // DIR_NORTH_EAST
    {-0.707f, -0.707f },  // DIR_NORTH_WEST
    { 0.707f,  0.707f },  // DIR_SOUTH_EAST
    {-0.707f,  0.707f },  // DIR_SOUTH_WEST
};

void DrawFlowField(const Direction *flow_field, const Map *map) {
    if (!flow_field) return;

    u32 grid_size = (u32)(map->width * map->height);

    u16 *dist = malloc(grid_size * sizeof(u16));
    if (!dist) return;

    u16 max_dist = 0;
    for (u32 y = 0; y < (u32)map->height; y++) {
        for (u32 x = 0; x < (u32)map->width; x++) {
            u32 idx = y * (u32)map->width + x;
            u16 min_d = UINT16_MAX;
            for (i32 b = 0; b < map->base_count; b++) {
                u32 bx = (u32)map->base_points[b].x;
                u32 by = (u32)map->base_points[b].y;
                u32 dx = x > bx ? x - bx : bx - x;
                u32 dy = y > by ? y - by : by - y;
                u16 d = (u16)(dx > dy ? dx : dy);
                if (d < min_d) min_d = d;
            }
            dist[idx] = min_d;
            if (min_d != UINT16_MAX && min_d > max_dist) max_dist = min_d;
        }
    }

    f32 arrow_len = TILE_SIZE * 0.38f;
    f32 head_len  = TILE_SIZE * 0.13f;
    f32 line_thick = 2.0f;

    for (u32 y = 0; y < (u32)map->height; y++) {
        for (u32 x = 0; x < (u32)map->width; x++) {
            u32 idx = y * (u32)map->width + x;
            Direction dir = flow_field[idx];
            if (!IsValidDirection(dir)) {
                dir = DIR_NONE;
            }

            Vector2 center = GridToWorld((i32)x, (i32)y);

            f32 t = (max_dist > 0 && dist[idx] != UINT16_MAX)
                    ? (f32)dist[idx] / (f32)max_dist
                    : 1.0f;
            Color cell_color = {
                (u8)(t * 220),
                (u8)((1.0f - t) * 220),
                0,
                180
            };

            if (dir == DIR_NONE) {
                DrawRectangle(
                    (i32)(center.x - TILE_SIZE / 2),
                    (i32)(center.y - TILE_SIZE / 2),
                    TILE_SIZE, TILE_SIZE,
                    (Color){40, 40, 40, 100}
                );
                continue;
            }

            DrawRectangle(
                (i32)(center.x - TILE_SIZE / 2),
                (i32)(center.y - TILE_SIZE / 2),
                TILE_SIZE, TILE_SIZE,
                (Color){cell_color.r, cell_color.g, cell_color.b, 60}
            );

            Vector2 av = dir_arrow_vec[dir];
            Vector2 tip = {
                center.x + av.x * arrow_len,
                center.y + av.y * arrow_len,
            };
            Vector2 tail = {
                center.x - av.x * arrow_len * 0.5f,
                center.y - av.y * arrow_len * 0.5f,
            };
            DrawLineEx(tail, tip, line_thick, cell_color);

            Vector2 perp = {-av.y, av.x};
            Vector2 head_l = {
                tip.x - av.x * head_len + perp.x * head_len * 0.6f,
                tip.y - av.y * head_len + perp.y * head_len * 0.6f,
            };
            Vector2 head_r = {
                tip.x - av.x * head_len - perp.x * head_len * 0.6f,
                tip.y - av.y * head_len - perp.y * head_len * 0.6f,
            };
            DrawLineEx(tip, head_l, line_thick, cell_color);
            DrawLineEx(tip, head_r, line_thick, cell_color);
        }
    }

    free(dist);
}
