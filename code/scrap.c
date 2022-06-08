#if 0

typedef struct element
{
    u32 Id;
    
    element *Parent;
    element *LastChild;
    
    element *Next;
} element;

internal void
UpdateAndRender()
{
    BeginFrame();
    
    BeginRow();
    {
        Label("Show: ");
        Checkbox("Empty Worlds", &AppState->ShowEmptyWorlds); // Editable bool
        Checkbox("Local", &AppState->ShowLocal); // Editable bool
        Checkbox("Available", &AppState->ShowAvailable); // Editable bool
        Spacer();
        Label("Filter: ");
        Textbox(&AppState->FilterText);
    }
    EndRow();
    
    BeginRow();
    {
        BeginListView();
        {
            if(AddListHeaderButton("World Name"))
            {
                SortWorldsByName(AppState);
            }
            if(AddListHeaderButton("World Age"))
            {
                SortWorldsByAge(AppState);
            }
            if(AddListHeaderButton("Synced Builds"))
            {
                SortWorldsBySycnedBuilds(AppState);
            }
            for(world *World = AppState->Worlds;
                World != 0;
                World = World->Next)
            {
                BeginListRow(World, &AppState->SelectedWorld); // Row click sets target
                {
                    AddListTextColumn(World->Name);
                    AddListTextColumn(World->Age);
                    AddListTextColumn(World->SyncedBuilds);
                }
                EndListRow();
            }
        }
        EndListView();
    }
    EndRow();
    
    BeginRow();
    {
        BeginRow();
        {
            SetRowHeader("Builds :"); // Adds a header and border to this row, like a groupbox
            BeginListView();
            {
                if(AddListHeaderButton("wbn"))
                {
                    SortWorldsByName(AppState);
                }
                if(AddListHeaderButton("Build Age"))
                {
                    SortWorldsByAge(AppState);
                }
                if(AddListHeaderButton("Status"))
                {
                    SortWorldsBySycnedBuilds(AppState);
                }
                for(build *Build = AppState->SelectedWorld->Builds;
                    Build != 0;
                    Build = Build->Next)
                {
                    BeginListRow(Build, &AppState->SelectedBuild); // Row click sets target
                    {
                        AddListTextColumn(Build->Wbn);
                        AddListTextColumn(Build->Age);
                        AddListTextColumn(Build->Status);
                    }
                    EndListRow();
                }
            }
            EndListView();
            
            
        }
        EndRow();
        
        Splitter(&UserSettings->BuildRunSplitSize);
        
        BeginRow();
        {   
            BeginRow();
            Checkbox("Edit run params", &UserSettings->EditRunParams);
            EndRow();
            
            BeginRow();
            DropDown("default");
            EndRow();
            
            BeginRow();
            Button("Foo");
            EndRow();
            
            BeginRow();
            Button("Bar");
            EndRow();
            
            BeginRow();
            Button("Bas");
            EndRow();
            
            BeginRow();
            Button("Quz");
            EndRow();
            
            BeginRow();
            {
                Textbox("thing.exe");
                Button(copy);
            }
            EndRow();
            
            BeginRow();
            {
                Label("Sync command");
                Button(copy);
            }
            EndRow();
            
            BeginRow();
            {
                if(Button(Run)) 
                { 
                    RunBuild(&AppState->SelectedBuild); 
                }
                if(Button(Sync))
                {
                    Call
                }
                Button(Cancel);
                Button(Clean);
                Button(Open Folder);
            }
            EndRow();
            
            
            BeginRow();
            {
                Label("DownloadPath");
                Button(copy);
            }
            EndRow();
        }
        EndRow();
        
    }
    EndRow();
    
    BeginRow();
    MultiLineTextBox(&AppState->LogMessages, &AppSate->SelectedMessage);
    EndRow();
    
    BeginRow();
    Button("Settings");
    Button("Settings");
    Button("Settings");
    Spacer();
    Button("Open Remote Config");
    Button("Open Log");
    EndRow();
    
    
    
    
    EndFrame();
}

#endif