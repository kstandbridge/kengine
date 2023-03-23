
// kengine_math.h

// kengine_types.h

inline u32
GetStringListCount(string_list *Head)
{
    u32 Result = 0;

    while(Head != 0)
    {
        Head = Head->Next;
        ++Result;
    }

    return Result;
}

inline string_list *
GetStringListByIndex(string_list *Head, s32 Index)
{
        string_list *Result = Head;

        if(Result != 0)
        {
            while(Result && Index)
            {
                Result = Result->Next;
            --Index;
            }
        }

    return Result;
}

inline s32
GetIndexOfStringList(string_list *Head, string_list *StringList)
{
    s32 Result = -1;

    for(s32 Index = 0;
        Head;
        ++Index, Head = Head->Next)
        {
        if(Head == StringList)
        {
            Result = Index;
            break;
        }
    }

    return Result;
}

typedef b32 string_list_predicate(void *Context, string_list *A, string_list *B);

inline string_list *
GetStringList(string_list *Head, string_list_predicate *Predicate, void *Context, string_list *Match)
{
    string_list *Result = 0;

    while(Head)
    {
        if(Predicate(Context, Head, Match))
        {
            Result = Head;
            break;
        }
        else
        {
            Head = Head->Next;
        }
    }

    return Result;
}

inline string_list *
GetStringListTail(string_list *Head)
{
    string_list *Result = Head;

    if(Result != 0)
    {
        while(Result->Next != 0)
        {
            Result = Result->Next;
        }
    }

    return Result;
}

inline string_list *
PushStringList(string_list **HeadRef, memory_arena *Arena)
{
    string_list *Result = PushStruct(Arena, string_list);

    Result->Next = *HeadRef;
    *HeadRef = Result;

    return Result;
}

inline string_list *
PushbackStringList(string_list **HeadRef, memory_arena *Arena)
{
    string_list *Result = PushStruct(Arena, string_list);

    Result->Next = 0;
    if(*HeadRef == 0)
    {
        *HeadRef = Result;
    }
    else
    {
        string_list *Tail = GetStringListTail(*HeadRef);
        Tail->Next = Result;
    }

    return Result;
}

inline string_list *
StringListMergeSort_(string_list *Front, string_list *Back, string_list_predicate *Predicate, void *Context, sort_type SortType)
{
    string_list *Result = 0;
    if(Front == 0)
    {
        Result = Back;
    }
    else if(Back == 0)
    {
        Result = Front;
    }
    else
    {
        b32 PredicateResult = Predicate(Context, Front, Back);
        if(SortType == Sort_Descending)
        {
            PredicateResult = !PredicateResult;
        }
        else
        {
            Assert(SortType == Sort_Ascending);
        }
        if(PredicateResult)
        {
            Result = Front;
            Result->Next = StringListMergeSort_(Front->Next, Back, Predicate, Context, SortType);
        }
        else
        {
            Result = Back;
            Back->Next = StringListMergeSort_(Front, Back->Next, Predicate, Context, SortType);
        }
    }

    return Result;
}

inline void
StringListFrontBackSplit(string_list *Head, string_list **FrontRef, string_list **BackRef)
{
    string_list *Fast;
    string_list *Slow;
    Slow = Head;
    Fast = Head->Next;

    while(Fast != 0)
    {
        Fast = Fast->Next;
        if(Fast != 0)
        {
            Slow = Slow->Next;
            Fast = Fast->Next;
        }
    }

    *FrontRef = Head;
    *BackRef = Slow->Next;
    Slow->Next = 0;
}

inline void
StringListMergeSort(string_list **HeadRef, string_list_predicate *Predicate, void *Context, sort_type SortType)
{
    string_list *Head = *HeadRef;

    if((Head!= 0) &&
       (Head->Next != 0))
    {
        string_list *Front;
        string_list *Back;
        StringListFrontBackSplit(Head, &Front, &Back);

        StringListMergeSort(&Front, Predicate, Context, SortType);
        StringListMergeSort(&Back, Predicate, Context, SortType);

        *HeadRef = StringListMergeSort_(Front, Back, Predicate, Context, SortType);
    }
}


#define V2(X, Y) (v2){(f32)(X), (f32)(Y)}
#define V2Set1(Value) (v2){(f32)(Value), (f32)(Value)}
#define V2Add(A, B) (v2){(A.X + B.X), (A.Y + B.Y)}
#define V2Subtract(A, B) (v2){(A.X - B.X), (A.Y - B.Y)}
#define V2Multiply(A, B) (v2){(A.X * B.X), (A.Y * B.Y)}

#define V2i(X, Y) (v2i){(X), (Y)}
#define V2iSet1(Value) (v2i){(s32)(Value), (s32)(Value)}
#define V2iAdd(A, B) (v2i){(A.X + B.X), (A.Y + B.Y)}
#define V2iSubtract(A, B) (v2i){(A.X - B.X), (A.Y - B.Y)}
#define V2iMultiply(A, B) (v2i){(A.X * B.X), (A.Y * B.Y)}

#define V3(X, Y, Z) (v3){(f32)(X), (f32)(Y), (f32)(Z)}
#define V3Set1(Value) (v3){(f32)(Value), (f32)(Value), (f32)(Value)}
#define V3Add(A, B) (v3){(A.X + B.X), (A.Y + B.Y), (A.Z + B.Z)}
#define V3Subtract(A, B) (v3){(A.X - B.X), (A.Y - B.Y), (A.Z - B.Z)}
#define V3Multiply(A, B) (v3){(A.X * B.X), (A.Y * B.Y), (A.Z * B.Z)}

#define V4(R, G, B, A) (v4){(f32)(R), (f32)(G), (f32)(B), (f32)(A)}
#define V4Set1(Value) (v4){(f32)(Value), (f32)(Value), (f32)(Value), (f32)(Value)}
#define V4Add(A, B) (v4){(A.R + B.R), (A.G + B.G), (A.B + B.B), (A.A + B.A)}
#define V4Subtract(A, B) (v4){(A.R - B.R), (A.G - B.G), (A.B - B.B), (A.A - B.A)}
#define V4Multiply(A, B) (v4){(A.R * B.R), (A.G * B.G), (A.B * B.B), (A.A * B.A)}

#define Rectangle2(Min, Max) (rectangle2){(Min), (Max)}

#define Rectangle2i(MinX, MaxX, MinY, MaxY) (rectangle2i){(MinX), (MaxX), (MinY), (MaxY)}
