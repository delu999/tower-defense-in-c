#include "editor.h"
#include <stdio.h>

i32 main(void) {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(1280, 720, "Tower Defense Game Editor");
    SetTargetFPS(120);
    
    EditorState editor = {0};
    InitEditor(&editor);
    
    printf("Game Editor ready.\n");
    printf("Controls:\n");
    printf("  F1/F2/F3: Levels/Enemies/Turrets\n");
    printf("  Ctrl+S: Save current section\n");
    printf("  Ctrl+B: Browse files for current section\n");
    printf("  Ctrl+N: New file for current section\n");
    printf("  Arrow keys + +/-: Edit table values (Enemies/Turrets)\n");
    printf("  Click/Drag: Paint tiles (Levels)\n");
    printf("  Right-click: Flood fill (Levels)\n");
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
