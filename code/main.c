// NOTE(kstandbridge): This is the consumer code, this would not be part of kengine at all and something implemented by the user of the framework

#include "kengine_pathfinding.c"

// TODO(kstandbridge): Perhaps this should be a dll export?

internal void
DrawAppGrid(app_state *AppState, ui_state *UIState, render_group *RenderGroup, memory_arena *PermArena, memory_arena *TempArena, app_input *Input, rectangle2 Bounds)
{
    
#if 1
    DrawPathfinding(AppState, UIState, RenderGroup, PermArena, TempArena, Input, Bounds);
#else
    ui_grid Grid = BeginGrid(UIState, TempArena, Bounds, 3, 3);
    {
        SetColumnWidth(&Grid, 0, 0, 128.0f);
        SetColumnWidth(&Grid, 2, 2, 256.0f);
        
        SetRowHeight(&Grid, 1, SIZE_AUTO);
        
        if(Button(&Grid, RenderGroup, 0, 0, GenerateInteractionId(AppState), AppState, String("Top Left"))) { __debugbreak(); }
        if(Button(&Grid, RenderGroup, 1, 0, GenerateInteractionId(AppState), AppState, String("Top Middle"))) { __debugbreak(); }
        if(Button(&Grid, RenderGroup, 2, 0, GenerateInteractionId(AppState), AppState, String("Top Right"))) { __debugbreak(); }
        
        if(Button(&Grid, RenderGroup, 0, 1, GenerateInteractionId(AppState), AppState, String("Middle Left"))) { __debugbreak(); }
        
        ui_grid InnerGrid = BeginGrid(UIState, TempArena, GetCellBounds(&Grid, 1, 1), 3, 2);
        {
            SetRowHeight(&InnerGrid, 1, SIZE_AUTO);
            if(Button(&InnerGrid, RenderGroup, 0, 0, GenerateInteractionId(AppState), AppState, String("Inner NW"))) { __debugbreak(); }
            if(Button(&InnerGrid, RenderGroup, 1, 0, GenerateInteractionId(AppState), AppState, String("Inner NE"))) { __debugbreak(); }
            if(Button(&InnerGrid, RenderGroup, 0, 1, GenerateInteractionId(AppState), AppState, String("Inner W"))) { __debugbreak(); }
            if(Button(&InnerGrid, RenderGroup, 1, 1, GenerateInteractionId(AppState), AppState, String("Inner E"))) { __debugbreak(); }
            if(Button(&InnerGrid, RenderGroup, 0, 2, GenerateInteractionId(AppState), AppState, String("Inner SW"))) { __debugbreak(); }
            if(Button(&InnerGrid, RenderGroup, 1, 2, GenerateInteractionId(AppState), AppState, String("Inner SE"))) { __debugbreak(); }
        }
        EndGrid(&InnerGrid);
        
        if(Button(&Grid, RenderGroup, 2, 1, GenerateInteractionId(AppState), AppState, String("Middle Right"))) { __debugbreak(); }
        
        if(Button(&Grid, RenderGroup, 0, 2, GenerateInteractionId(AppState), AppState, String("Bottom Left"))) { __debugbreak(); }
        if(Button(&Grid, RenderGroup, 1, 2, GenerateInteractionId(AppState), AppState, String("Bottom Middle"))) { __debugbreak(); }
        if(Button(&Grid, RenderGroup, 2, 2, GenerateInteractionId(AppState), AppState, String("Bottom Right"))) { __debugbreak(); }
    }
    EndGrid(&Grid);
#endif
    
}
