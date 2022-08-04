# kengine

This is mainly for educational and fun purposes. The main goal was to create an immediate mode GUI library from scratch.

Some additional challanges that I would like to include are as follows:

### No libraries
We can use Win32 APIs or OpenGL, but no CRT. Other than that everything has to be build from scratch.

### No linking
Idearly we should dynamically load everything using LoadLibrary and GetProcAddress.

We could even not link to kernal32 and write our own GetProcAddress to get started.

### Meta-programming
Try to avoid using Macros where possible and instead have a preprocessor that will parse and generate code.
Any time we are writing code think can this be automatically generated?

### No includes
No <math.h> or similar is a good start.

We should try to not include <Windows.h> and perhaps even <gl/gl.h>

# Coding style
This is not about right and wrong, it doesn't matter what your preference is when it comes to what you decide to capitalize or not. The only thing that really matters is consistency! Don't mix coding styles, when you work on a project just adopt the current standards and let your code blend in seamlessly. With that being said, heres the guidelines to follow.

### Never use static
For readability, introspection and such we need be more clearer on what we mean by static, so use the pound defines.
```C
#define internal static
#define local_persist static
#define global static
```
* internal is a function that is only used inside this translation unit
* local_persist is a persistant variable that exists within the scope of the function
* global is a variable that exists throughout the translation unit

### Types
Make use of the typedefs, never use int for example.
```C
typedef signed char s8;
typedef short int s16;
typedef int s32;
typedef long int s64;
typedef int b32;

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long int u64;

typedef float f32;
typedef double f64;

#define true 1
#define false 0
```

### Naming
Types should always be snake_case, variables and function names UpperCamelCase regardless of scope. Try to be descriptive, and avoid using single letters, use Width instead of W. Can short form when its more obvious, Dim instead of Dimensions

```C
typedef struct window_dimensions
{
    s32 Width;
    s32 Height
} window_dimensions;
```
This does not apply to math operations: x y P dt. As the code needs to be concise for arithmetic.

Enums should UpperCamelCase the type name then UpperCamelCase the value split by an underscore

```C
typedef enum element_type
{
    ElementType_Row,
    ElementType_Spacer,
    ElementType_Label,
    ElementType_Checkbox,
    ElementType_Textbox,
    ElementType_Button,
    ElementType_VerticleSlider,
} element_type;
```

### Function formatting
Everything should be internal, as we will only have a single translation unit. This helps out the linker and speeds up compile time.
inline can be used also, but the compiler will decide what to do anyway. Use inline more as a note to yourself and other developers that this isn't an overly complex operation and is being seperated out for simplicity

Always name the returning variable as Result. This helps debugging in the watch window and also makes the code more clear in longer methods what we are working towards.

One return statement per function, no exceptions, never early return.

The function name should be on a new line, this is just a habit that stems from C++ and its enormasly long return types but still holds value with long explicit type names.

```C
inline window_dimensions
GetWindowDimensions()
{
    window_dimensions Result;
    return Result;
}
```

### Function parameters
The first parameter should be the global variables/state, the second parameter should be local variables/state, the third parameter should be the object you are working on followed by any other parameters you need.

Any of them are optional, just if they need to be included do them in the specified order.

```C
internal void
DrawElements(interface_state *InterfaceState, layout *Layout, render_group *RenderGroup, element *FirstChild, v2 P);
```
* interface_state would be the persistant state kept throughout the appliction lifecycle, think if you wanted to encapsulate global variables, this is where they would go
* layout is a none persistent state, think of something you declare and share around for a frame then dispose
* RenderGroup is what we are outputting to, using the passed states we can manipulate the RenderGroup
* FirstChild/P are any other things we need that the state of doesn't need to be shared, otherwise they would be in layout

### Initializer List
Just don't use them, we will use introspection to generate ctors which will show us anyway a variable was not declared
```C
window_dimensions Dim = WindowDimensions(1920, 1080);
```

### if statements
Always create a scope even for single line operations.
```C
if(Element->Type == ElementType_Row)
{
    // Work
}
```

### for loops
Each part should be on its own line for readability, starting variable, condition to contiue, iteration
```C
for(element *Element = Sentinal->Next;
    Element != &Sentinal;
    Element = Element->Next)
{
}
```

### switch statements
Always create a scope for each case, break on the same line as closing the scope.
```C
switch(Tokenizer->At[0])
{
    case '"':
    {
        // Work
    } break;
}
```
If we are doing lots of similar operations with minor changes these can be collapsed to single line, but still create a scope!
```C
switch(Tokenizer->At[0])
{
    case '"':
    {
        case '(': {Result.Type = CTokenType_OpenParen;} break;
        case ')': {Result.Type = CTokenType_CloseParen;} break;
        case ':': {Result.Type = CTokenType_Colon;} break;
        case ';': {Result.Type = CTokenType_Semicolon;} break;
        case '*': {Result.Type = CTokenType_Asterisk;} break;
        case '[': {Result.Type = CTokenType_OpenBracket;} break;
        case ']': {Result.Type = CTokenType_CloseBracket;} break;
        case '{': {Result.Type = CTokenType_OpenBrace;} break;
        case '}': {Result.Type = CTokenType_CloseBrace;} break;
    } break;
}
```

# Milestone goals
- [ ] Preprocessor framework
- [ ] Unit test framework
- [ ] Win32 platform layer
- [ ] DLL hot loading
- [ ] Software rendering
- [ ] OpenGL rendering
- [ ] Immediate mode GUI
- [ ] Profiling and debugging system
- [ ] DirectX rendering
- [ ] Client side socket framework
- [ ] Server side socket framework using I/O Completion Ports

# TODOs
- [ ] Output to console/file
- [ ] Unit test macros
- [ ] String utilities, equals, uppercase, camelcase, etc
- [ ] Parse source file and tokenize
- [ ] Introspection framework
- [ ] Win32 API function pointer generation
- [ ] Struct ctor generation
- [ ] Struct math operation generation
- [ ] Memory management, arena, temporary memory
- [ ] Dynamically growing memory arenas
