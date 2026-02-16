#include "editor.h"
#include <stdio.h>

i32 main(void) {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(1280, 720, "Tower Defense Level Editor");
    SetTargetFPS(120);
    
    EditorState editor = {0};
    InitEditor(&editor);
    
    printf("Level Editor ready.\n");
    printf("Controls:\n");
    printf("  Click/Drag: Paint tiles\n");
    printf("  Right-click: Flood fill\n");
    printf("  G: Toggle grid\n");
    printf("  Ctrl+S: Save\n");
    printf("  Ctrl+L: Load\n");
    printf("  Ctrl+N: New map\n");
    printf("  Ctrl+Z: Undo\n");
    printf("  Ctrl+Y: Redo\n");
    printf("  Esc: Quit\n");
    
    while (!WindowShouldClose()) {
        UpdateEditor(&editor);
        
        BeginDrawing();
        DrawEditor(&editor);
        EndDrawing();
    }
    
    CleanupEditor(&editor);
    CloseWindow();
    
    return 0;
}
