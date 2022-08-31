// NOTE(kstandbridge): This is the consumer code, this would not be part of kengine at all and something implemented by the user of the framework

// TODO(kstandbridge): Perhaps this should be a dll export?

typedef struct app_state
{
    b32 IsInitialized;
    
    b32 ShowEmptyWorlds;
    b32 ShowLocal;
    b32 ShowAvailable;
    
} app_state;


#include "kengine_pathfinding.c"


internal void
DrawAppGrid(app_state *AppState, ui_state *UIState, render_group *RenderGroup, memory_arena *PermArena, memory_arena *TempArena, app_input *Input, rectangle2 Bounds)
{
    if(!AppState->IsInitialized)
    {
        AppState->IsInitialized = true;
    }
    
#if 0
    DrawPathfinding(AppState, UIState, RenderGroup, PermArena, TempArena, Input, Bounds);
#else
    
    
    
#if 0
    ui_grid Grid = BeginGrid(UIState, TempArena, Bounds, 3, 3);
    {
        StaticLabel(&Grid, RenderGroup, 0, 0, String("TopLeft"), TextPosition_TopLeft);
        StaticLabel(&Grid, RenderGroup, 1, 0, String("TopMiddle"), TextPosition_TopMiddle);
        StaticLabel(&Grid, RenderGroup, 2, 0, String("TopRight"), TextPosition_TopRight);
        
        StaticLabel(&Grid, RenderGroup, 0, 1, String("MiddleLeft"), TextPosition_MiddleLeft);
        StaticLabel(&Grid, RenderGroup, 1, 1, String("MiddleMiddle"), TextPosition_MiddleMiddle);
        StaticLabel(&Grid, RenderGroup, 2, 1, String("MiddleRight"), TextPosition_MiddleRight);
        
        StaticLabel(&Grid, RenderGroup, 0, 2, String("BottomLeft"), TextPosition_BottomLeft);
        StaticLabel(&Grid, RenderGroup, 1, 2, String("BottomMiddle"), TextPosition_BottomMiddle);
        StaticLabel(&Grid, RenderGroup, 2, 2, String("BottomRight"), TextPosition_BottomRight);
    }
    EndGrid(&Grid);
#else
    ui_grid Grid = BeginGrid(UIState, TempArena, Bounds, 4, 1);
    {
        SetRowHeight(&Grid, 0, 32.0f);
        SetRowHeight(&Grid, 3, 32.0f);
        
        ui_grid TopBarGrid = BeginGrid(UIState, TempArena, GetCellBounds(&Grid, 0, 0), 1, 7);
        {        
            SetColumnWidth(&TopBarGrid, 0, 0, 64.0f);
            SetColumnWidth(&TopBarGrid, 0, 1, 152.0f);
            SetColumnWidth(&TopBarGrid, 0, 2, 84.0f);
            SetColumnWidth(&TopBarGrid, 0, 3, 116.0f);
            SetColumnWidth(&TopBarGrid, 0, 5, 64.0f);
            SetColumnWidth(&TopBarGrid, 0, 6, 128.0f);
            StaticLabel(&TopBarGrid, RenderGroup, 0, 0, String("Show:"), TextPosition_MiddleLeft);
            Checkbox(&TopBarGrid, RenderGroup, 1, 0, &AppState->ShowEmptyWorlds, String("Empty Worlds"));
            Checkbox(&TopBarGrid, RenderGroup, 2, 0, &AppState->ShowLocal, String("Local"));
            Checkbox(&TopBarGrid, RenderGroup, 3, 0, &AppState->ShowAvailable, String("Available"));
            
            StaticLabel(&TopBarGrid, RenderGroup, 5, 0, String("Filter:"), TextPosition_MiddleLeft);
            Button(&TopBarGrid, RenderGroup, 6, 0, GenerateInteractionId(AppState), AppState, String("<search>"));
        }
        EndGrid(&TopBarGrid);
        
        Button(&Grid, RenderGroup, 0, 1, GenerateInteractionId(AppState), AppState, String("Worlds"));
        Button(&Grid, RenderGroup, 0, 2, GenerateInteractionId(AppState), AppState, String("Logs"));
        Button(&Grid, RenderGroup, 0, 3, GenerateInteractionId(AppState), AppState, String("BottomBar"));
        
    }
    EndGrid(&Grid);
#endif
    
#endif
}
