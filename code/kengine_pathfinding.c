
typedef struct path_node
{
    b32 Obstacle;
    b32 Visited;
    
    f32 GlobalGoal;
    f32 LocalGoal;
    
    v2i P;
    
    struct path_node_link NeighbourSentinal;
    struct path_node *Parent;
    
    struct path_node *Next;
} path_node;

global path_node *EndNode;
global path_node *StartNode;
global path_node *Nodes;
global s32 Columns;
global s32 Rows;

#define PADDING 10
#define CEL_DIM 40

inline v2
GetCellP(v2 StartingPoint, s32 Column, s32 Row)
{
    v2 Result = StartingPoint;
    Result.X += (PADDING + CEL_DIM) * Column;
    Result.Y += (PADDING + CEL_DIM) * Row;
    
    return Result;
}

internal b32
NodeLinkPredicate(path_node_link *A, path_node_link *B)
{
    b32 Result = A->Node->GlobalGoal < B->Node->GlobalGoal;
    return Result;
    
}

internal void
DrawPathfinding(app_state *AppState, ui_state *UIState, render_group *RenderGroup, memory_arena *PermArena, memory_arena *TempArena, app_input *Input, rectangle2 Bounds)
{
    v2 MouseP = V2(Input->MouseX, Input->MouseY);
    
    // NOTE(kstandbridge): Calculate number of nodes we have
    v2 TotalDim = V2Subtract(Bounds.Max, Bounds.Min);
    
    // NOTE(kstandbridge): Generate nodes
    if(Nodes == 0)
    {
        Columns = CeilF32ToS32(TotalDim.X / (PADDING+CEL_DIM)) - 1;
        Rows = CeilF32ToS32(TotalDim.Y / (PADDING+CEL_DIM)) - 1;
        Nodes = PushArray(PermArena, Rows * Columns, path_node);
        for(s32 Row = 0;
            Row < Rows;
            ++Row)
        {
            for(s32 Column = 0;
                Column < Columns;
                ++Column)
            {
                path_node *Node = Nodes + (Row*Columns + Column);
                Node->Obstacle = false;
                Node->Visited = false;
                Node->P = V2i(Column, Row);
                Node->Parent = 0;
                PathNodeLinkInit(&Node->NeighbourSentinal);
                Node->Next = 0;
            }
        }
        
        for(s32 Row = 0;
            Row < Rows;
            ++Row)
        {
            for(s32 Column = 0;
                Column < Columns;
                ++Column)
            {
                node *Node = Nodes + (Row*Columns + Column);
                if(Row > 0)
                {
                    path_node_link *Link = PushStruct(PermArena, path_node_link);
                    Link->Node = Nodes + ((Row - 1)*Columns + (Column + 0));;
                    NodeLinkInsertAtLast(&Node->NeighbourSentinal, Link);
                }
                if(Row < Rows - 1)
                {
                    path_node_link *Link = PushStruct(PermArena, path_node_link);
                    Link->Node = Nodes + ((Row + 1)*Columns + (Column + 0));
                    NodeLinkInsertAtLast(&Node->NeighbourSentinal, Link);
                    
                }
                
                if(Column > 0)
                {
                    path_node_link *Link = PushStruct(PermArena, path_node_link);
                    Link->Node = Nodes + ((Row + 0)*Columns + (Column - 1));
                    NodeLinkInsertAtLast(&Node->NeighbourSentinal, Link);
                }
                if(Column < Columns -1)
                {
                    path_node_link *Link = PushStruct(PermArena, path_node_link);
                    Link->Node = Nodes + ((Row + 0)*Columns + (Column + 1));
                    NodeLinkInsertAtLast(&Node->NeighbourSentinal, Link);
                }
            }
        }
        
        StartNode = Nodes;
        EndNode = Nodes + (Rows * Columns) - 1;
    }
    
    v2 TotalUsedDim = V2((f32)Columns * (PADDING+CEL_DIM), (f32)Rows * (PADDING+CEL_DIM));
    v2 RemainingDim = V2Subtract(TotalDim, TotalUsedDim);
    v2 StartingAt = V2Add(Bounds.Min, V2Multiply(V2Set1(0.5f), RemainingDim));
    
    b32 NeedsUpdating = false;
    
    BEGIN_BLOCK("DrawNodes");
    u32 TotalNodes = Rows * Columns;
    {
        u32 Index = 0;
        for(node *Node = Nodes;
            Index < TotalNodes;
            ++Index, ++Node)
        {
            v2 CellP = GetCellP(StartingAt, Node->P.X, Node->P.Y);
            v2 CellDim = V2Add(CellP, V2Set1(CEL_DIM));
            rectangle2 CellBounds = Rectangle2(CellP, CellDim);
            v4 Color = V4(1.0f, 0.5f, 0.0f, 1.0f);
            
            
            // NOTE(kstandbridge): Draw paths between nodes
            {        
                for(path_node_link *NeighbourLink = Node->NeighbourSentinal.Next;
                    NeighbourLink != &Node->NeighbourSentinal;
                    NeighbourLink = NeighbourLink->Next)
                {
                    node *NeighbourNode = NeighbourLink->Node;
                    v2 PathP = GetCellP(StartingAt, NeighbourNode->P.X, NeighbourNode->P.Y);
                    rectangle2 PathBounds = Rectangle2(V2Add(CellP, V2Set1(0.5f*CEL_DIM)),
                                                       V2Add(PathP, V2Set1(0.5f*CEL_DIM)));
                    PathBounds = Rectangle2AddRadiusTo(PathBounds, 1.0f);
                    PushRenderCommandRectangle(RenderGroup, Color, PathBounds, 1.0f);
                }
            }
            
            // NOTE(kstandbridge): Draw Node
            {   
                if(Rectangle2IsIn(CellBounds, MouseP))
                {
                    if(WasPressed(Input->MouseButtons[MouseButton_Left]))
                    {
                        Node->Obstacle = !Node->Obstacle;
                        NeedsUpdating = true;
                    }
                    if(WasPressed(Input->MouseButtons[MouseButton_Middle]))
                    {
                        EndNode = Node;
                        NeedsUpdating = true;
                    }
                    if(WasPressed(Input->MouseButtons[MouseButton_Right]))
                    {
                        StartNode = Node;
                        NeedsUpdating = true;
                    }
                    Color = V4(0.0f, 1.0f, 0.0f, 1.0f);
                }
                
                if(Node->Visited)
                {
                    Color = V4(0.0f, 0.5f, 0.0f, 1.0f);
                }
                
                if(Node->Obstacle)
                {
                    Color = V4(0.3f, 0.3f, 0.3f, 1.0f);
                }
                
                if(Node == StartNode)
                {
                    Color = V4(0.0f, 1.0f, 0.0f, 1.0f);
                }
                if(Node == EndNode)
                {
                    Color = V4(1.0f, 0.0f, 0.0f, 1.0f);
                }
                
                PushRenderCommandRectangle(RenderGroup, Color, CellBounds, 1.5f);
            }
        }
    }
    END_BLOCK();
    
    // NOTE(kstandbridge): Solve A*
    //if(NeedsUpdating)
    BEGIN_BLOCK("SolveAStar");
    {
        // NOTE(kstandbridge): Reset nodes
        {        
            u32 Index = 0;
            for(node *Node = Nodes;
                Index < TotalNodes;
                ++Index, ++Node)
            {
                Node->Visited = false;
                Node->GlobalGoal = F32Max;
                Node->LocalGoal = F32Max;
                Node->Parent = 0;
            }
        }
        
        // NOTE(kstandbridge): Starting node
        StartNode->LocalGoal = 0.0f;
        StartNode->GlobalGoal = V2iDistanceBetween(StartNode->P, EndNode->P);
        
        path_node_link NotTestedSentinel;
        NodeLinkInit(&NotTestedSentinel);
        
        path_node_link *CurrentLink = PushStruct(TempArena, path_node_link);
        CurrentLink->Node = StartNode;
        NodeLinkInsertAtLast(&NotTestedSentinel, CurrentLink); 
        
        while(!NodeLinkIsEmpty(&NotTestedSentinel) &&
              (CurrentLink->Node != EndNode))
        {
            NodeLinkMergeSort(&NotTestedSentinel, NodeLinkPredicate);
            
            while(!NodeLinkIsEmpty(&NotTestedSentinel) && 
                  NotTestedSentinel.Next->Node->Visited)
            {
                NodeLinkRemove(NotTestedSentinel.Next);
            }
            
            if(!NodeLinkIsEmpty(&NotTestedSentinel))
            {            
                CurrentLink = NotTestedSentinel.Next;
                node *CurrentNode = CurrentLink->Node;
                CurrentNode->Visited = true;
                
                // NOTE(kstandbridge): Check each Neighbour
                for(path_node_link *NeighbourLink = CurrentNode->NeighbourSentinal.Next;
                    NeighbourLink != &CurrentNode->NeighbourSentinal;
                    NeighbourLink = NeighbourLink->Next)
                {
                    node *NeighbourNode = NeighbourLink->Node;
                    
                    if(!NeighbourNode->Visited && !NeighbourNode->Obstacle)
                    {
                        path_node_link *LinkCopy = PushStruct(TempArena, path_node_link);
                        LinkCopy->Node = NeighbourNode;
                        NodeLinkInsertAtLast(&NotTestedSentinel, LinkCopy);
                    }
                    
                    f32 NewDistance = V2iDistanceBetween(CurrentNode->P, NeighbourNode->P);
                    f32 PossibleLowerGoal = CurrentNode->LocalGoal + NewDistance;
                    
                    if(PossibleLowerGoal < NeighbourNode->LocalGoal)
                    {
                        NeighbourNode->Parent = CurrentNode;
                        NeighbourNode->LocalGoal = PossibleLowerGoal;
                        
                        NeighbourNode->GlobalGoal = NeighbourNode->LocalGoal + V2iDistanceBetween(NeighbourNode->P, EndNode->P);
                    }
                }
                
            }
        }
    }
    END_BLOCK();
    
    // NOTE(kstandbridge): Draw path
    BEGIN_BLOCK("DrawPath");
    {
        node *Node = EndNode;
        while(Node->Parent)
        {
            node *Parent = Node->Parent;
            
            v2 NodeP = GetCellP(StartingAt, Node->P.X, Node->P.Y);
            v2 ParentP = GetCellP(StartingAt, Parent->P.X, Parent->P.Y);
            v2 Min = V2(Minimum(NodeP.X, ParentP.X), Minimum(NodeP.Y, ParentP.Y));
            v2 Max = V2(Maximum(NodeP.X, ParentP.X), Maximum(NodeP.Y, ParentP.Y));
            
            rectangle2 PathBounds = Rectangle2(V2Add(Min, V2Set1(0.5f*CEL_DIM)),
                                               V2Add(Max, V2Set1(0.5f*CEL_DIM)));
            
            PathBounds = Rectangle2AddRadiusTo(PathBounds, 3.0f);
            v4 Color = V4(1.0f, 1.0f, 0.0f, 1.0f);
            PushRenderCommandRectangle(RenderGroup, Color, PathBounds, 3.0f);
            
            Node = Parent;
        }
    }
    END_BLOCK();
}
