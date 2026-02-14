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
