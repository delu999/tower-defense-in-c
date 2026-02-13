#include "pathfinding.h"
#include "map.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Helper structures for A*
typedef struct {
    int x, y;
    float g, h, f;  // g = cost from start, h = heuristic, f = g + h
    int parent_x, parent_y;
    bool in_open, in_closed;
} Node;

typedef struct {
    Node **nodes;
    int count;
    int capacity;
} PriorityQueue;

// Chebyshev distance (for 8-directional movement)
static float Heuristic(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    return (float)fmax(dx, dy);  // Chebyshev distance
}

// Priority queue operations
static void PQ_Init(PriorityQueue *pq, int capacity) {
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
    int i = pq->count++;
    pq->nodes[i] = node;

    while (i > 0) {
        int parent = (i - 1) / 2;
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
    int i = 0;
    while (true) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

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
static bool CanMoveDiagonal(const Map *map, int from_x, int from_y, int to_x, int to_y) {
    int dx = to_x - from_x;
    int dy = to_y - from_y;

    // Not a diagonal move
    if (abs(dx) != 1 || abs(dy) != 1) return true;

    // Check both adjacent orthogonal cells
    bool orth1 = IsWalkable(map, from_x + dx, from_y);
    bool orth2 = IsWalkable(map, from_x, from_y + dy);

    return orth1 && orth2;
}

int FindPath(const Map *map, Vector2 start_grid, const Vector2 *goals, int goal_count,
             Vector2 *out_path, int max_path_len) {
    if (goal_count == 0) return 0;

    int start_x = (int)start_grid.x;
    int start_y = (int)start_grid.y;

    // Find closest goal using Chebyshev distance
    int closest_goal = 0;
    float min_dist = Heuristic(start_x, start_y, (int)goals[0].x, (int)goals[0].y);
    for (int i = 1; i < goal_count; i++) {
        float dist = Heuristic(start_x, start_y, (int)goals[i].x, (int)goals[i].y);
        if (dist < min_dist) {
            min_dist = dist;
            closest_goal = i;
        }
    }

    int goal_x = (int)goals[closest_goal].x;
    int goal_y = (int)goals[closest_goal].y;

    // Initialize node grid
    Node nodes[MAP_HEIGHT][MAP_WIDTH];
    memset(nodes, 0, sizeof(nodes));

    for (int y = 0; y < map->height; y++) {
        for (int x = 0; x < map->width; x++) {
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
    const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
    const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};
    const float cost[] = {1.0f, 1.414f, 1.0f, 1.414f, 1.0f, 1.414f, 1.0f, 1.414f};  // sqrt(2) for diagonals

    bool found = false;
    Node *goal_node = NULL;

    // A* main loop
    while (!PQ_IsEmpty(&open_set)) {
        Node *current = PQ_Pop(&open_set);
        current->in_open = false;
        current->in_closed = true;

        // Check if reached any goal
        for (int i = 0; i < goal_count; i++) {
            if (current->x == (int)goals[i].x && current->y == (int)goals[i].y) {
                found = true;
                goal_node = current;
                break;
            }
        }

        if (found) break;

        // Explore neighbors
        for (int dir = 0; dir < 8; dir++) {
            int nx = current->x + dx[dir];
            int ny = current->y + dy[dir];

            // Out of bounds
            if (nx < 0 || nx >= map->width || ny < 0 || ny >= map->height) continue;

            Node *neighbor = &nodes[ny][nx];

            // Not walkable or already closed
            if (!IsWalkable(map, nx, ny) || neighbor->in_closed) continue;

            // Check diagonal blocking
            if (!CanMoveDiagonal(map, current->x, current->y, nx, ny)) continue;

            float tentative_g = current->g + cost[dir];

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
    int path_len = 0;
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
    for (int i = 0; i < path_len / 2; i++) {
        Vector2 temp = out_path[i];
        out_path[i] = out_path[path_len - 1 - i];
        out_path[path_len - 1 - i] = temp;
    }

    return path_len;
}

bool ValidatePaths(const Map *map) {
    // BFS from each spawn to check if any base is reachable

    for (int s = 0; s < map->spawn_count; s++) {
        int start_x = (int)map->spawn_points[s].x;
        int start_y = (int)map->spawn_points[s].y;

        bool visited[MAP_HEIGHT][MAP_WIDTH] = {0};
        int queue_x[MAP_WIDTH * MAP_HEIGHT];
        int queue_y[MAP_WIDTH * MAP_HEIGHT];
        int head = 0, tail = 0;

        queue_x[tail] = start_x;
        queue_y[tail] = start_y;
        tail++;
        visited[start_y][start_x] = true;

        bool found_base = false;

        // BFS
        while (head < tail) {
            int x = queue_x[head];
            int y = queue_y[head];
            head++;

            // Check if reached a base
            for (int b = 0; b < map->base_count; b++) {
                if (x == (int)map->base_points[b].x && y == (int)map->base_points[b].y) {
                    found_base = true;
                    break;
                }
            }

            if (found_base) break;

            // Explore 8 directions
            const int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
            const int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};

            for (int dir = 0; dir < 8; dir++) {
                int nx = x + dx[dir];
                int ny = y + dy[dir];

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

void DrawPath(const Vector2 *path, int path_len, Color color) {
    for (int i = 0; i < path_len - 1; i++) {
        Vector2 start = GridToWorld((int)path[i].x, (int)path[i].y);
        Vector2 end = GridToWorld((int)path[i + 1].x, (int)path[i + 1].y);
        DrawLineEx(start, end, 4.0f, color);
    }

    // Draw waypoint circles
    for (int i = 0; i < path_len; i++) {
        Vector2 pos = GridToWorld((int)path[i].x, (int)path[i].y);
        DrawCircle((int)pos.x, (int)pos.y, 6, color);
    }
}
