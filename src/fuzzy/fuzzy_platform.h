#pragma once

#include "fuzzy_types.h"

#define EXPORT __declspec(dllexport)

#define GAME_PRINT_OUTPUT(name) void name(const string& Output)
typedef GAME_PRINT_OUTPUT(platform_print_output);

#define GAME_READ_TEXT_FILE(name) string name(const string& Path)
typedef GAME_READ_TEXT_FILE(platform_read_text_file);

#define GAME_READ_JSON_FILE(name) json name(const string& Path)
typedef GAME_READ_JSON_FILE(platform_read_json_file);

struct platform_api {
    platform_print_output* PrintOutput;
    platform_read_text_file* ReadTextFile;
    platform_read_json_file* ReadJsonFile;
};

struct game_memory {
    b32 IsInitalized;

    u64 PermanentStorageSize;
    void* PermanentStorage;

    u64 TransientStorageSize;
    void* TransientStorage;

    platform_api Platform;
};

struct game_input {
    b32 Keys[512];
    b32 ProcessedKeys[512];
};

struct game_params {
    s32 ScreenWidth;
    s32 ScreenHeight;

    f32 Delta;

    game_input Input;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* Memory, game_params* Params)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
