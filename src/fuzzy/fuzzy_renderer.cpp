#include "fuzzy_platform.h"
#include "fuzzy_renderer.h"
#include "fuzzy.h"
#include <glm/gtc/type_ptr.hpp>
#include "stdio.h"

#pragma region Shaders

inline b32
UniformKeyComparator(shader_uniform *Uniform, char *Key)
{
    b32 Result = StringEquals(Uniform->Name, Key);
    return Result;
}

inline b32
UniformKeyExists(shader_uniform *Uniform)
{
    b32 Result = (b32)Uniform->Name;
    return Result;
}

inline void
UniformKeySetter(shader_uniform *Uniform, char *Key)
{
    Uniform->Name = Key;
}

shader_uniform *
GetUniform(shader_program *ShaderProgram, char *Name)
{
    shader_uniform *Result = Get<shader_uniform, char *>(&ShaderProgram->Uniforms, Name, UniformKeyComparator);
    return Result;
}

shader_uniform *
CreateUniform(shader_program *ShaderProgram, char *Name, memory_arena *Arena)
{
    shader_uniform *Result = Create<shader_uniform, char *>(
        &ShaderProgram->Uniforms, Name, UniformKeyExists, UniformKeySetter, Arena);
    return Result;
}

u32
CreateShader(game_memory *Memory, game_state *GameState, GLenum Type, char *Source)
{
    u32 Shader = Memory->Renderer.glCreateShader(Type);
    Memory->Renderer.glShaderSource(Shader, 1, &Source, NULL);
    Memory->Renderer.glCompileShader(Shader);

    s32 IsShaderCompiled;
    Memory->Renderer.glGetShaderiv(Shader, GL_COMPILE_STATUS, &IsShaderCompiled);
    if (!IsShaderCompiled)
    {
        temporary_memory ErrorLogMemory = BeginTemporaryMemory(&GameState->WorldArena);
        
        s32 LogLength;
        Memory->Renderer.glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &LogLength);

        char *ErrorLog = PushString(&GameState->WorldArena, LogLength);

        Memory->Renderer.glGetShaderInfoLog(Shader, LogLength, NULL, ErrorLog);

        char Output[1024];
        FormatString(Output, sizeof(Output), "%s%s%s", "Shader compilation failed:\n", ErrorLog, "\n");
        Memory->Platform.PrintOutput(Output);

        Memory->Renderer.glDeleteShader(Shader);

        EndTemporaryMemory(ErrorLogMemory);
    }
    assert(IsShaderCompiled);

    return Shader;
}

u32
CreateProgram(game_memory *Memory, game_state *GameState, u32 VertexShader, u32 FragmentShader)
{
    u32 Program = Memory->Renderer.glCreateProgram();
    Memory->Renderer.glAttachShader(Program, VertexShader);
    Memory->Renderer.glAttachShader(Program, FragmentShader);
    Memory->Renderer.glLinkProgram(Program);
    Memory->Renderer.glDeleteShader(VertexShader);
    Memory->Renderer.glDeleteShader(FragmentShader);

    s32 IsProgramLinked;
    Memory->Renderer.glGetProgramiv(Program, GL_LINK_STATUS, &IsProgramLinked);

    if (!IsProgramLinked)
    {
        temporary_memory ErrorLogMemory = BeginTemporaryMemory(&GameState->WorldArena);
        
        s32 LogLength;
        Memory->Renderer.glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &LogLength);

        char *ErrorLog = PushString(&GameState->WorldArena, LogLength);

        Memory->Renderer.glGetProgramInfoLog(Program, LogLength, nullptr, ErrorLog);

        char Output[1024];
        FormatString(Output, sizeof(Output), "%s%s%s", "Shader program linkage failed:\n", ErrorLog, "\n");
        Memory->Platform.PrintOutput(Output);
        
        EndTemporaryMemory(ErrorLogMemory);
    }
    assert(IsProgramLinked);

    return Program;
}

inline s32
GetUniformLocation(game_memory *Memory, u32 ShaderProgram, const char *Name)
{
    s32 UniformLocation = Memory->Renderer.glGetUniformLocation(ShaderProgram, Name);
    //assert(uniformLocation != -1);
    return UniformLocation;
}

inline void
SetShaderUniform(game_memory *Memory, s32 Location, s32 Value)
{
    Memory->Renderer.glUniform1i(Location, Value);
}

inline void
SetShaderUniform(game_memory *Memory, s32 Location, f32 Value)
{
    Memory->Renderer.glUniform1f(Location, Value);
}

inline void
SetShaderUniform(game_memory *Memory, s32 Location, vec2 Value)
{
    Memory->Renderer.glUniform2f(Location, Value.x, Value.y);
}

inline void
SetShaderUniform(game_memory *Memory, s32 Location, vec3 Value)
{
    Memory->Renderer.glUniform3f(Location, Value.x, Value.y, Value.z);
}

inline void
SetShaderUniform(game_memory *Memory, s32 Location, vec4 Value)
{
    Memory->Renderer.glUniform4f(Location, Value.x, Value.y, Value.z, Value.w);
}

inline void
SetShaderUniform(game_memory *Memory, s32 Location, const mat4& Value)
{
    Memory->Renderer.glUniformMatrix4fv(Location, 1, GL_FALSE, glm::value_ptr(Value));
}

inline void
SetShaderUniform(game_memory *Memory, shader_uniform *Uniform, s32 Value)
{
    if (Uniform)
    {
        Memory->Renderer.glUniform1i(Uniform->Location, Value);
    }
}

inline void
SetShaderUniform(game_memory *Memory, shader_uniform *Uniform, f32 Value)
{
    if (Uniform)
    {
        Memory->Renderer.glUniform1f(Uniform->Location, Value);
    }
}

inline void
SetShaderUniform(game_memory *Memory, shader_uniform *Uniform, vec2 Value)
{
    if (Uniform)
    {
        Memory->Renderer.glUniform2f(Uniform->Location, Value.x, Value.y);
    }
}

inline void
SetShaderUniform(game_memory *Memory, shader_uniform *Uniform, vec3 Value)
{
    if (Uniform)
    {
        Memory->Renderer.glUniform3f(Uniform->Location, Value.x, Value.y, Value.z);
    }
}

inline void
SetShaderUniform(game_memory *Memory, shader_uniform *Uniform, vec4 Value)
{
    if (Uniform)
    {
        Memory->Renderer.glUniform4f(Uniform->Location, Value.x, Value.y, Value.z, Value.w);
    }
}

inline void
SetShaderUniform(game_memory *Memory, shader_uniform *Uniform, const mat4& Value)
{
    if (Uniform)
    {
        Memory->Renderer.glUniformMatrix4fv(Uniform->Location, 1, GL_FALSE, glm::value_ptr(Value));
    }
}

shader_program
CreateShaderProgram(game_memory *Memory, game_state *GameState, char *VertexShaderFileName, char *FragmentShaderFileName)
{
    shader_program Result = {};

    char *VertexShaderSource = (char *)Memory->Platform.ReadFile(VertexShaderFileName).Contents;
    char *FragmentShaderSource = (char *)Memory->Platform.ReadFile(FragmentShaderFileName).Contents;
    u32 VertexShader = CreateShader(Memory, GameState, GL_VERTEX_SHADER, &VertexShaderSource[0]);
    u32 FragmentShader = CreateShader(Memory, GameState, GL_FRAGMENT_SHADER, &FragmentShaderSource[0]);

    Result.ProgramHandle = CreateProgram(Memory, GameState, VertexShader, FragmentShader);

    s32 UniformCount;
    Memory->Renderer.glGetProgramiv(Result.ProgramHandle, GL_ACTIVE_UNIFORMS, &UniformCount);

    Result.Uniforms.Count = (u32)UniformCount;
    Result.Uniforms.Values = PushArray<shader_uniform>(&GameState->WorldArena, Result.Uniforms.Count);

    s32 MaxUniformLength;
    Memory->Renderer.glGetProgramiv(Result.ProgramHandle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &MaxUniformLength);

    for (s32 UniformIndex = 0; UniformIndex < UniformCount; ++UniformIndex)
    {
        GLsizei Length;
        s32 Size;
        GLenum Type;
        char *Name = PushString(&GameState->WorldArena, MaxUniformLength);
        Memory->Renderer.glGetActiveUniform(Result.ProgramHandle, UniformIndex, MaxUniformLength, &Length, &Size, &Type, Name);
        
        shader_uniform *Uniform = CreateUniform(&Result, Name, &GameState->WorldArena);
        Uniform->Location = GetUniformLocation(Memory, Result.ProgramHandle, Uniform->Name);
    }

    return Result;
}

#pragma endregion

#pragma region Buffers

void
SetupVertexBuffer(renderer_api *Renderer, vertex_buffer *Buffer)
{
    Renderer->glGenVertexArrays(1, &Buffer->VAO);
    Renderer->glBindVertexArray(Buffer->VAO);

    Renderer->glGenBuffers(1, &Buffer->VBO);
    Renderer->glBindBuffer(GL_ARRAY_BUFFER, Buffer->VBO);
    Renderer->glBufferData(GL_ARRAY_BUFFER, Buffer->Size, NULL, Buffer->Usage);

    for (u32 VertexSubBufferIndex = 0; VertexSubBufferIndex < Buffer->DataLayout->SubBufferCount; ++VertexSubBufferIndex)
    {
        vertex_sub_buffer *VertexSubBuffer = Buffer->DataLayout->SubBuffers + VertexSubBufferIndex;

        Renderer->glBufferSubData(GL_ARRAY_BUFFER, VertexSubBuffer->Offset, VertexSubBuffer->Size, VertexSubBuffer->Data);
    }

    for (u32 VertexAttributeIndex = 0; VertexAttributeIndex < Buffer->AttributesLayout->AttributeCount; ++VertexAttributeIndex)
    {
        vertex_buffer_attribute *VertexAttribute = Buffer->AttributesLayout->Attributes + VertexAttributeIndex;

        Renderer->glEnableVertexAttribArray(VertexAttribute->Index);
        if (VertexAttribute->Type == GL_UNSIGNED_INT)
        {
            Renderer->glVertexAttribIPointer(VertexAttribute->Index, VertexAttribute->Size,
                VertexAttribute->Type, VertexAttribute->Stride, VertexAttribute->OffsetPointer);
        }
        else
        {
            Renderer->glVertexAttribPointer(VertexAttribute->Index, VertexAttribute->Size,
                VertexAttribute->Type, VertexAttribute->Normalized, VertexAttribute->Stride, VertexAttribute->OffsetPointer);
        }
        Renderer->glVertexAttribDivisor(VertexAttribute->Index, VertexAttribute->Divisor);
    }
}

#pragma endregion

#pragma region New Renderer API

void
DrawRectangle(
    game_memory *Memory, 
    game_state *GameState, 
    vec2 Position, 
    vec2 Size, 
    rotation_info *Rotation,
    vec4 Color
)
{
    // todo: move out
    Memory->Renderer.glUseProgram(GameState->RectangleShaderProgram.ProgramHandle);
    Memory->Renderer.glBindVertexArray(GameState->QuadVertexBuffer.VAO);
    Memory->Renderer.glBindBuffer(GL_ARRAY_BUFFER, GameState->QuadVertexBuffer.VBO);

    shader_uniform *ModelUniform = GetUniform(&GameState->RectangleShaderProgram, "u_Model");
    shader_uniform *ColorUniform = GetUniform(&GameState->RectangleShaderProgram, "u_Color");

    mat4 Model = mat4(1.f);

    // translation
    Model = glm::translate(Model, vec3(Position, 0.f));

    // scaling
    Model = glm::scale(Model, vec3(Size, 0.f));

    // rotation
    Model = glm::translate(Model, vec3(Size / 2.f, 0.f));
    Model = glm::rotate(Model, Rotation->AngleInRadians, Rotation->Axis);
    Model = glm::translate(Model, vec3(-Size / 2.f, 0.f));

    SetShaderUniform(Memory, ModelUniform, Model);
    SetShaderUniform(Memory, ColorUniform, Color);

    Memory->Renderer.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void
DrawRectangleOutline(
    game_memory *Memory, 
    game_state *GameState, 
    vec2 Position, 
    vec2 Size, 
    rotation_info *Rotation,
    f32 Thickness,
    vec4 Color
)
{
    // todo: move out
    Memory->Renderer.glUseProgram(GameState->RectangleOutlineShaderProgram.ProgramHandle);
    Memory->Renderer.glBindVertexArray(GameState->QuadVertexBuffer.VAO);
    Memory->Renderer.glBindBuffer(GL_ARRAY_BUFFER, GameState->QuadVertexBuffer.VBO);

    shader_uniform *ModelUniform = GetUniform(&GameState->RectangleOutlineShaderProgram, "u_Model");
    shader_uniform *ColorUniform = GetUniform(&GameState->RectangleOutlineShaderProgram, "u_Color");
    shader_uniform *ThicknessUniform = GetUniform(&GameState->RectangleOutlineShaderProgram, "u_Thickness");
    shader_uniform *WidthOverHeightUniform = GetUniform(&GameState->RectangleOutlineShaderProgram, "u_WidthOverHeight");

    SetShaderUniform(Memory, WidthOverHeightUniform, Size.x / Size.y);

    mat4 Model = mat4(1.f);

    // translation
    Model = glm::translate(Model, vec3(Position, 0.f));

    // scaling
    Model = glm::scale(Model, vec3(Size, 0.f));

    // rotation
    Model = glm::translate(Model, vec3(Size / 2.f, 0.f));
    Model = glm::rotate(Model, Rotation->AngleInRadians, Rotation->Axis);
    Model = glm::translate(Model, vec3(-Size / 2.f, 0.f));

    SetShaderUniform(Memory, ModelUniform, Model);
    SetShaderUniform(Memory, ColorUniform, Color);

    // meters to (0-1) uv-range
    SetShaderUniform(Memory, ThicknessUniform, Thickness / Size.x);

    Memory->Renderer.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void
DrawSprite(
    game_memory *Memory,
    game_state *GameState,
    vec2 Position,
    vec2 Size,
    rotation_info *Rotation,
    vec2 UV,
    vec2 Alignment = vec2(0.f)
)
{
    // todo: move out
    Memory->Renderer.glUseProgram(GameState->SpriteShaderProgram.ProgramHandle);
    Memory->Renderer.glBindVertexArray(GameState->QuadVertexBuffer.VAO);
    Memory->Renderer.glBindBuffer(GL_ARRAY_BUFFER, GameState->QuadVertexBuffer.VBO);

    shader_uniform *ModelUniform = GetUniform(&GameState->SpriteShaderProgram, "u_Model");
    shader_uniform *UVUniform = GetUniform(&GameState->SpriteShaderProgram, "u_UVOffset");

    mat4 Model = mat4(1.f);

    // translation
    Model = glm::translate(Model, vec3(Position + Alignment * Size, 0.f));

    // scaling
    Model = glm::scale(Model, vec3(Size, 0.f));

    // rotation
    Model = glm::translate(Model, vec3(Size / 2.f, 0.f));
    Model = glm::rotate(Model, Rotation->AngleInRadians, Rotation->Axis);
    Model = glm::translate(Model, vec3(-Size / 2.f, 0.f));

    SetShaderUniform(Memory, ModelUniform, Model);
    SetShaderUniform(Memory, UVUniform, UV);

    Memory->Renderer.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

s32 inline
GetHorizontalAdvanceForPair(font_asset *Font, char Character, char NextCharacter)
{
    // todo: add start character or something
    s32 Start = ' ';
    s32 End = '~';
    s32 Count = End - Start + 1;

    u32 RowIndex = Character - Start;
    u32 ColumnIndex = NextCharacter - Start;

    assert((RowIndex * Count + ColumnIndex) < Font->FontInfo.HorizontalAdvanceTableCount);
    
    s32 *Result = Font->FontInfo.HorizontalAdvanceTable + RowIndex * Count + ColumnIndex;

    return *Result;
}

void
DrawTextLine(
    game_memory *Memory,
    game_state *GameState,
    char *String,
    vec2 TextBaselinePosition,
    f32 TextScale,
    rotation_info *Rotation,
    vec4 TextColor
)
{
    TextScale = 1.f;
    Memory->Renderer.glUseProgram(GameState->TextShaderProgram.ProgramHandle);
    Memory->Renderer.glBindVertexArray(GameState->QuadVertexBuffer.VAO);
    Memory->Renderer.glBindBuffer(GL_ARRAY_BUFFER, GameState->QuadVertexBuffer.VBO);

    shader_uniform *TextColorUniform = GetUniform(&GameState->TextShaderProgram, "u_TextColor");
    SetShaderUniform(Memory, TextColorUniform->Location, TextColor);

    vec2 TextureAtlasSize = vec2(GameState->Assets.FontInfo.TextureAtlas.Width, GameState->Assets.FontInfo.TextureAtlas.Height);

    f32 AtX = TextBaselinePosition.x;

    for(char *Character = String; *Character; ++Character)
    {
        vec2 Position = vec2(AtX, TextBaselinePosition.y);

        // todo: 
        u32 GlyphIndex = *Character - ' ';

        glyph_info *GlyphInfo = GameState->Assets.Glyphs + GlyphIndex;
        
        // todo: handle space properly
        //if (*Character == ' ')
        //{
        //    AtX += 42.f * TextScale * GameState->PixelsToMeters;
        //    continue;
        //}

        shader_uniform *SpriteSizeUniform = GetUniform(&GameState->TextShaderProgram, "u_SpriteSize");
        SetShaderUniform(Memory, SpriteSizeUniform->Location, GlyphInfo->SpriteSize / TextureAtlasSize);

        vec2 Size = GlyphInfo->CharacterSize * GameState->PixelsToMeters * TextScale;
        vec2 UV = GlyphInfo->UV;

        shader_uniform *ModelUniform = GetUniform(&GameState->TextShaderProgram, "u_Model");
        shader_uniform *UVUniform = GetUniform(&GameState->TextShaderProgram, "u_UVOffset");

        mat4 Model = mat4(1.f);

        vec2 Alignment = vec2(0.f, GlyphInfo->Alignment.y) * GameState->PixelsToMeters * TextScale;
        Model = glm::translate(Model, vec3(Position + Alignment, 0.f));

        // scaling
        Model = glm::scale(Model, vec3(Size, 0.f));

        // rotation
        Model = glm::translate(Model, vec3(Size / 2.f, 0.f));
        Model = glm::rotate(Model, Rotation->AngleInRadians, Rotation->Axis);
        Model = glm::translate(Model, vec3(-Size / 2.f, 0.f));

        SetShaderUniform(Memory, ModelUniform->Location, Model);
        SetShaderUniform(Memory, UVUniform->Location, UV);

        Memory->Renderer.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        s32 HorizontalAdvance = 0;
        if (*(Character + 1))
        {
            HorizontalAdvance = GetHorizontalAdvanceForPair(&GameState->Assets, *Character, *(Character + 1));
        }

        AtX += HorizontalAdvance * GameState->PixelsToMeters * TextScale;
        //AtX += Size.x + HorizontalAdvance * GameState->PixelsToMeters * TextScale;
    }
}

#pragma endregion