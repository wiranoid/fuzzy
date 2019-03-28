#include <windows.h>
#include <cstdlib>
#include <string>
#include <cassert>
#include <iostream>

#include "..\..\generated\glad\src\glad.c"
#include <GLFW/glfw3.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "fuzzy_types.h"
#include "fuzzy_memory.h"
#include "fuzzy_platform.h"
#include "win32_fuzzy.h"

#pragma warning(disable:4302)
#pragma warning(disable:4311)

global_variable game_params GameParams = {};

internal_function void
Win32GetFullPathToEXEDirectory(win32_state *State) 
{
    char EXEFullPath[WIN32_FILE_PATH];
    GetModuleFileNameA(0, EXEFullPath, sizeof(EXEFullPath));

    char* OnePastLastEXEFullPathSlash = GetLastAfterDelimiter(EXEFullPath, '\\');

    for (char Index = 0; Index < OnePastLastEXEFullPathSlash - EXEFullPath; ++Index) 
    {
        State->EXEDirectoryFullPath[Index] = EXEFullPath[Index];
    }
}

internal_function FILETIME
Win32GetLastWriteTime(char *FileName) 
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA FileInformation;
    if (GetFileAttributesExA(FileName, GetFileExInfoStandard, &FileInformation)) 
    {
        LastWriteTime = FileInformation.ftLastWriteTime;
    }

    return LastWriteTime;
}

internal_function b32
Win32FileExists(char *FileName) 
{
    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    b32 Exists = GetFileAttributesExA(FileName, GetFileExInfoStandard, &Ignored);

    return Exists;
}

internal_function win32_game_code
Win32LoadGameCode(char *SourceDLLName, char *TempDLLName, char *LockFileName) 
{
    win32_game_code Result = {};

    if (!Win32FileExists(LockFileName)) 
    {
        Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

        CopyFile(SourceDLLName, TempDLLName, false);

        Result.GameCodeDLL = LoadLibraryA(TempDLLName);

        if (Result.GameCodeDLL) 
        {
            Result.UpdateAndRender = (game_update_and_render*)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

            Result.IsValid = (b32)Result.UpdateAndRender;
        }
    }

    return Result;
}

internal_function void
Win32UnloadGameCode(win32_game_code *GameCode) 
{
    if (GameCode->GameCodeDLL) 
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }

    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
}

internal_function void
Win32InitOpenGLRenderer(game_memory *GameMemory) 
{
    // todo: in future this will be different
    GameMemory->Renderer = {};
    GameMemory->Renderer.glDrawArrays = glDrawArrays;
    GameMemory->Renderer.glPolygonMode = glPolygonMode;
    GameMemory->Renderer.glCreateShader = glCreateShader;
    GameMemory->Renderer.glShaderSource = glShaderSource;
    GameMemory->Renderer.glCompileShader = glCompileShader;
    GameMemory->Renderer.glGetShaderiv = glGetShaderiv;
    GameMemory->Renderer.glGetShaderInfoLog = glGetShaderInfoLog;
    GameMemory->Renderer.glDeleteShader = glDeleteShader;
    GameMemory->Renderer.glGetUniformLocation = glGetUniformLocation;
    GameMemory->Renderer.glUniform1i = glUniform1i;
    GameMemory->Renderer.glUniform1f = glUniform1f;
    GameMemory->Renderer.glUniform2f = glUniform2f;
    GameMemory->Renderer.glUniformMatrix4fv = glUniformMatrix4fv;
    GameMemory->Renderer.glGenTextures = glGenTextures;
    GameMemory->Renderer.glBindTexture = glBindTexture;
    GameMemory->Renderer.glTexParameteri = glTexParameteri;
    GameMemory->Renderer.glTexImage2D = glTexImage2D;
    GameMemory->Renderer.glCreateProgram = glCreateProgram;
    GameMemory->Renderer.glAttachShader = glAttachShader;
    GameMemory->Renderer.glLinkProgram = glLinkProgram;
    GameMemory->Renderer.glGetProgramiv = glGetProgramiv;
    GameMemory->Renderer.glGetProgramInfoLog = glGetProgramInfoLog;
    GameMemory->Renderer.glUseProgram = glUseProgram;
    GameMemory->Renderer.glGenVertexArrays = glGenVertexArrays;
    GameMemory->Renderer.glBindVertexArray = glBindVertexArray;
    GameMemory->Renderer.glGenBuffers = glGenBuffers;
    GameMemory->Renderer.glBindBuffer = glBindBuffer;
    GameMemory->Renderer.glBufferData = glBufferData;
    GameMemory->Renderer.glBufferSubData = glBufferSubData;
    GameMemory->Renderer.glBindBufferRange = glBindBufferRange;
    GameMemory->Renderer.glBindBufferBase = glBindBufferBase;
    GameMemory->Renderer.glVertexAttribPointer = glVertexAttribPointer;
    GameMemory->Renderer.glVertexAttribIPointer = glVertexAttribIPointer;
    GameMemory->Renderer.glEnableVertexAttribArray = glEnableVertexAttribArray;
    GameMemory->Renderer.glVertexAttribDivisor = glVertexAttribDivisor;
    GameMemory->Renderer.glBlendFunc = glBlendFunc;
    GameMemory->Renderer.glClear = glClear;
    GameMemory->Renderer.glClearColor = glClearColor;
    GameMemory->Renderer.glDrawArraysInstanced = glDrawArraysInstanced;
    GameMemory->Renderer.glGetActiveUniform = glGetActiveUniform;
    GameMemory->Renderer.glGetUniformBlockIndex = glGetUniformBlockIndex;
    GameMemory->Renderer.glUniformBlockBinding = glUniformBlockBinding;
    GameMemory->Renderer.glGetString = glGetString;
    GameMemory->Renderer.glViewport = glViewport;
    GameMemory->Renderer.glEnable = glEnable;
}

internal_function void
CalculateFrameStats(GLFWwindow *Window, char *WindowTitle, f64 TotalTime)
{
    local_persist s32 FrameCount = 0;
    local_persist f32 TimeElapsed = 0.f;

    ++FrameCount;

    // Compute averages over one second period.
    if ((TotalTime - TimeElapsed) >= 1.f) {
        f32 FPS = (f32)FrameCount;
        f32 MsPerFrame = 1000.f / FPS;

        char WindowText[64];
        snprintf(WindowText, sizeof(WindowText), "%s    fps: %f    ms: %f", WindowTitle, FPS, MsPerFrame);
        glfwSetWindowTitle(Window, WindowText);

        FrameCount = 0;
        TimeElapsed += 1.f;
    }
}

//s32 CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, s32 nCmdShow)
s32 main(s32 Argc, char *Argv[])
{
    win32_state Win32State = {};
    game_memory GameMemory = {};

    GameMemory.PermanentStorageSize = Megabytes(64);
    GameMemory.TransientStorageSize = Megabytes(128);

    void *BaseAddress = (void*)Terabytes(2);

    Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
    Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, Win32State.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
    GameMemory.TransientStorage = (u8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;

    GameMemory.Platform = {};
    GameMemory.Platform.ReadFile = PlatformReadFile;
    GameMemory.Platform.FreeFile = PlatformFreeFile;
    GameMemory.Platform.PrintOutput = PlatformPrintOutput;

    // todo: stbi_load loads image into arbitrary location which is not good
    GameMemory.Platform.ReadImageFile = stbi_load;
    GameMemory.Platform.FreeImageFile = stbi_image_free;

    Win32InitOpenGLRenderer(&GameMemory);

    Win32GetFullPathToEXEDirectory(&Win32State);

    char SourceGameCodeDLLFullPath[WIN32_FILE_PATH];
    ConcatenateStrings(Win32State.EXEDirectoryFullPath, "fuzzy.dll", SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFullPath[WIN32_FILE_PATH];
    ConcatenateStrings(Win32State.EXEDirectoryFullPath, "fuzzy_temp.dll", TempGameCodeDLLFullPath);

    char GameCodeLockFullPath[WIN32_FILE_PATH];
    ConcatenateStrings(Win32State.EXEDirectoryFullPath, "lock.tmp", GameCodeLockFullPath);

    win32_game_code GameCode = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
#if 1
    GameParams.ScreenWidth = 1280;
    GameParams.ScreenHeight = 720;
#else
    GameParams.ScreenWidth = 1920;
    GameParams.ScreenHeight = 1080;
#endif
    
    if (!glfwInit()) 
    {
        OutputDebugStringA("Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    srand((u32) glfwGetTimerValue());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWmonitor *Monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* VidMode = glfwGetVideoMode(Monitor);

    char *WindowTitle = "Fuzzy";

    GLFWwindow *Window = glfwCreateWindow(GameParams.ScreenWidth, GameParams.ScreenHeight, WindowTitle, NULL, NULL);
    if (!Window) 
    {
        OutputDebugStringA("Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSetWindowPos(Window, (VidMode->width - GameParams.ScreenWidth) / 2, (VidMode->height - GameParams.ScreenHeight) / 2);

    glfwSetKeyCallback(Window, [](GLFWwindow *Window, s32 Key, s32 Scancode, s32 Action, s32 Mods) 
    {
        if (Key == GLFW_KEY_ESCAPE && Action == GLFW_PRESS) 
        {
            glfwSetWindowShouldClose(Window, GLFW_TRUE);
        }
        if (Action == GLFW_PRESS) 
        {
            switch (Key)
            {
            case GLFW_KEY_LEFT:
                GameParams.Input.Left.isPressed = true;
                break;
            case GLFW_KEY_RIGHT:
                GameParams.Input.Right.isPressed = true;
                break;
            case GLFW_KEY_UP:
                GameParams.Input.Up.isPressed = true;
                break;
            case GLFW_KEY_DOWN:
                GameParams.Input.Down.isPressed = true;
                break;
            case GLFW_KEY_SPACE:
                GameParams.Input.Jump.isPressed = true;
                break;
            default:
                break;
            }

        } 
        else if (Action == GLFW_RELEASE) 
        {
            switch (Key)
            {
            case GLFW_KEY_LEFT:
                GameParams.Input.Left.isPressed = false;
                GameParams.Input.Left.isProcessed = false;
                break;
            case GLFW_KEY_RIGHT:
                GameParams.Input.Right.isPressed = false;
                GameParams.Input.Right.isProcessed = false;
                break;
            case GLFW_KEY_UP:
                GameParams.Input.Up.isPressed = false;
                GameParams.Input.Up.isProcessed = false;
                break;
            case GLFW_KEY_DOWN:
                GameParams.Input.Down.isPressed = false;
                GameParams.Input.Down.isProcessed = false;
                break;
            case GLFW_KEY_SPACE:
                GameParams.Input.Jump.isPressed = false;
                GameParams.Input.Jump.isProcessed = false;
            default:
                break;
            }
        }
    });

    glfwSetFramebufferSizeCallback(Window, [](GLFWwindow *Window, s32 Width, s32 Height) 
    {
        GameParams.ScreenWidth = Width;
        GameParams.ScreenHeight = Height;
    });

    glfwMakeContextCurrent(Window);

#if 1
    glfwSwapInterval(1);
#else
    glfwSwapInterval(0);
#endif

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    {
        OutputDebugStringA("Failed to initialize OpenGL context\n");
        return EXIT_FAILURE;
    }

    glfwSetTime(0.f);

    f64 LastTime = glfwGetTime();
    f64 TotalTime = glfwGetTime();

    while (!glfwWindowShouldClose(Window)) 
    {
        FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
        if (CompareFileTime(&NewDLLWriteTime, &GameCode.DLLLastWriteTime) != 0) 
        {
            Win32UnloadGameCode(&GameCode);
            GameCode = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
        }

        TotalTime = glfwGetTime();
        GameParams.Delta = (f32) (TotalTime - LastTime);
        LastTime = TotalTime;
        
        CalculateFrameStats(Window, WindowTitle, TotalTime);

        if (GameCode.IsValid) 
        {
            GameCode.UpdateAndRender(&GameMemory, &GameParams);
        }

        glfwPollEvents();
        glfwSwapBuffers(Window);
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
