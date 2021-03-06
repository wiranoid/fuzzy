// todo: to make rapidjson work
#undef internal
#include "rapidjson/document.h"
#define internal static

#include "fuzzy_tiled.h"
#include "fuzzy.h"

using namespace rapidjson;

#define INVERT_Y(y) -(y)

global const char *TILE_LAYER = "tilelayer";
global const char *OBJECT_LAYER = "objectgroup";

typedef GenericDocument<UTF8<>, MemoryPoolAllocator<>, MemoryPoolAllocator<>> DocumentType;

// todo: not scalable
internal entity_type
GetEntityTypeFromString(const char *String)
{
    entity_type Result;

    if (String)
    {
        if (StringEquals(String, "player"))
        {
            Result = ENTITY_PLAYER;
        }
        else if (StringEquals(String, "siren"))
        {
            Result = ENTITY_SIREN;
        }
        else
        {
            Result = ENTITY_UNKNOWN;
        }
    }
    else
    {
        Result = ENTITY_UNKNOWN;
    }

    return Result;
}

inline b32
TileMetaInfoKeyComparator(tile_meta_info *TileMetaInfo, u32 Key)
{
    b32 Result = TileMetaInfo->Id == Key;
    return Result;
}

inline b32
TileMetaInfoKeyExists(tile_meta_info *TileMetaInfo)
{
    b32 Result = (b32)TileMetaInfo->Id;
    return Result;
}

inline void
TileMetaInfoKeySetter(tile_meta_info *TileMetaInfo, u32 Key)
{
    TileMetaInfo->Id = Key;
}

inline tile_meta_info *
GetTileMetaInfo(tileset *Tileset, u32 ID)
{
    tile_meta_info *Result = Get<tile_meta_info, u32>(&Tileset->Tiles, ID, TileMetaInfoKeyComparator);
    return Result;
}

inline tile_meta_info *
CreateTileMetaInfo(tileset *Tileset, u32 ID, memory_arena *Arena)
{
    tile_meta_info *Result = Create<tile_meta_info, u32>(
        &Tileset->Tiles, ID, TileMetaInfoKeyExists, TileMetaInfoKeySetter, Arena);
    return Result;
}

internal DocumentType
ParseJSON(const char *Json, u64 ValueBufferSize, u64 ParseBufferSize, memory_arena *Region) 
{
    // todo: temporary memory for document storage
    void *ValueBuffer = PushSize(Region, ValueBufferSize);
    void *ParseBuffer = PushSize(Region, ParseBufferSize);

    MemoryPoolAllocator<> ValueAllocator(ValueBuffer, ValueBufferSize);
    MemoryPoolAllocator<> ParseAllocator(ParseBuffer, ParseBufferSize);
    DocumentType Document(&ValueAllocator, ParseBufferSize, &ParseAllocator);
    // todo: error handling
    Document.Parse(Json);

    return Document;
}

internal void
LoadTileset(tileset *Tileset, const char *Json, memory_arena *Arena, platform_api *Platform) 
{
    // todo: think more about the sizes
    constexpr u64 ValueBufferSize = Kilobytes(512);
    constexpr u64 ParseBufferSize = Kilobytes(32);
    DocumentType Document = ParseJSON(Json, ValueBufferSize, ParseBufferSize, Arena);

    const char *ImagePath = Document["image"].GetString();

    char FullImagePath[256];
    FormatString(FullImagePath, sizeof(FullImagePath), "%s%s", "tilesets/", ImagePath);

    Tileset->Image.Memory = Platform->ReadImageFile(FullImagePath, 
        &Tileset->Image.Width, &Tileset->Image.Height, &Tileset->Image.Channels, 0);
    if (!Tileset->Image.Memory) 
    {
        Platform->PrintOutput("Image loading failed:\n");
    }
    Assert(Tileset->Image.Memory);

    Tileset->TileWidthInPixels = Document["tilewidth"].GetInt();
    Tileset->TileHeightInPixels = Document["tileheight"].GetInt();
    Tileset->Columns = Document["columns"].GetUint();
    Tileset->Margin = Document["margin"].GetUint();
    Tileset->Spacing = Document["spacing"].GetUint();

    Assert(Document.HasMember("properties"));

    const Value& CustomProperties = Document["properties"];
    Assert(CustomProperties.IsArray());
    
    for (SizeType CustomPropertyIndex = 0; CustomPropertyIndex < CustomProperties.Size(); ++CustomPropertyIndex) 
    {
        const Value& CustomProperty = CustomProperties[CustomPropertyIndex];
        const char* PropertyName = CustomProperty["name"].GetString();

        if (StringEquals(PropertyName, "TileWidthInWorldUnits"))
        {
            Tileset->TileWidthInWorldUnits = CustomProperty["value"].GetFloat();
        }
        else if (StringEquals(PropertyName, "TileHeightInWorldUnits"))
        {
            Tileset->TileHeightInWorldUnits = CustomProperty["value"].GetFloat();
        }
        else 
        {
            InvalidCodePath;
        }
    }

    Tileset->TilesetWidthPixelsToWorldUnits = Tileset->TileWidthInWorldUnits / Tileset->TileWidthInPixels;
    Tileset->TilesetHeightPixelsToWorldUnits = Tileset->TileHeightInWorldUnits / Tileset->TileHeightInPixels;

    Tileset->Tiles.Count = Document["tilecount"].GetUint();
    Tileset->Tiles.Values = PushArray<tile_meta_info>(Arena, Tileset->Tiles.Count);

    if (Document.HasMember("tiles")) 
    {
        const Value& Tiles = Document["tiles"];
        Assert(Tiles.IsArray());

        for (SizeType TileIndex = 0; TileIndex < Tiles.Size(); ++TileIndex) 
        {
            const Value& Tile = Tiles[TileIndex];

            u32 TileId = Tile["id"].GetUint();

            tile_meta_info *TileInfo = CreateTileMetaInfo(Tileset, TileId, Arena);
            TileInfo->BoxCount = 0;
            TileInfo->AnimationFrameCount = 0;

            if (Tile.HasMember("type"))
            {
                TileInfo->Type = const_cast<char*>(Tile["type"].GetString());
            }

            if (Tile.HasMember("objectgroup")) 
            {
                const Value& TileObjects = Tile["objectgroup"]["objects"];

                Assert(TileObjects.IsArray());

                TileInfo->BoxCount = TileObjects.Size();
                TileInfo->Boxes = PushArray<aabb>(Arena, TileInfo->BoxCount);

                for (SizeType ObjectIndex = 0; ObjectIndex < TileObjects.Size(); ++ObjectIndex) 
                {
                    const Value& Object = TileObjects[ObjectIndex];

                    aabb *Box = TileInfo->Boxes + ObjectIndex;
                    Box->Position = {
                        Object["x"].GetFloat(), Object["y"].GetFloat()
                    };
                    Box->Size = {
                        Object["width"].GetFloat(), Object["height"].GetFloat()
                    };
                }
            }

            if (Tile.HasMember("animation")) 
            {
                const Value& Animation = Tile["animation"];

                Assert(Animation.IsArray());

                TileInfo->AnimationFrameCount = Animation.Size();
                TileInfo->AnimationFrames = PushArray<tile_animation_frame>(Arena, TileInfo->AnimationFrameCount);

                for (SizeType AnimationFrameIndex = 0; AnimationFrameIndex < Animation.Size(); ++AnimationFrameIndex) 
                {
                    const Value& Frame = Animation[AnimationFrameIndex];

                    tile_animation_frame *AnimationFrame = TileInfo->AnimationFrames + AnimationFrameIndex;
                    AnimationFrame->Duration = Frame["duration"].GetUint();
                    AnimationFrame->TileId = Frame["tileid"].GetUint();
                }
            }

            if (Tile.HasMember("properties"))
            {
                const Value& CustomTileProperties = Tile["properties"];

                Assert(CustomTileProperties.IsArray());

                TileInfo->CustomPropertiesCount = CustomTileProperties.Size();
                TileInfo->CustomProperties = PushArray<tile_custom_property>(Arena, TileInfo->CustomPropertiesCount);

                if (CustomTileProperties.IsArray())
                {
                    for (SizeType CustomTilePropertyIndex = 0; CustomTilePropertyIndex < CustomTileProperties.Size(); ++CustomTilePropertyIndex)
                    {
                        const Value& CustomTilePropertyValue = CustomTileProperties[CustomTilePropertyIndex];

                        tile_custom_property *CustomTileProperty = TileInfo->CustomProperties + CustomTilePropertyIndex;

                        *CustomTileProperty = {};
                        CustomTileProperty->Name = const_cast<char*>(CustomTilePropertyValue["name"].GetString());
                        CustomTileProperty->Type = const_cast<char*>(CustomTilePropertyValue["type"].GetString());

                        if (StringEquals(CustomTileProperty->Type, "string"))
                        {
                            u32 StringLength = CustomTilePropertyValue["value"].GetStringLength() + 1;
                            CustomTileProperty->Value = PushString(Arena, StringLength);
                            CopyString(CustomTilePropertyValue["value"].GetString(), (char *)CustomTileProperty->Value, StringLength);
                        }
                        else if (StringEquals(CustomTileProperty->Type, "float"))
                        {
                            CustomTileProperty->Value = PushSize(Arena, sizeof(f32));
                            *(f32 *)CustomTileProperty->Value = CustomTilePropertyValue["value"].GetFloat();
                        }
                        else if (StringEquals(CustomTileProperty->Type, "int"))
                        {
                            CustomTileProperty->Value = PushSize(Arena, sizeof(i32));
                            *(i32 *)CustomTileProperty->Value = CustomTilePropertyValue["value"].GetInt();
                        }
                        else if (StringEquals(CustomTileProperty->Type, "bool"))
                        {
                            CustomTileProperty->Value = PushSize(Arena, sizeof(b32));
                            *(b32 *)CustomTileProperty->Value = CustomTilePropertyValue["value"].GetBool();
                        }
                        else
                        {
                            // not supported property type
                            InvalidCodePath;
                        }
                    }
                }
            }
        }
    }
}

internal void
LoadMap(tilemap *Map, const char *Json, memory_arena *Arena, platform_api *Platform) 
{
    // todo: think more about the sizes
    constexpr u64 ValueBufferSize = Megabytes(32);
    constexpr u64 ParseBufferSize = Megabytes(1);
    DocumentType Document = ParseJSON(Json, ValueBufferSize, ParseBufferSize, Arena);

    Assert(Document.HasMember("tilesets"));

    // todo: add support for multiple tilesets
    const Value& Tileset = Document["tilesets"].GetArray()[0];

    Map->TilesetCount = 1;
    Map->Tilesets = PushArray<tileset_source>(Arena, Map->TilesetCount);
    Map->Tilesets[0].FirstGID = Tileset["firstgid"].GetUint();

    char *TilesetSource = const_cast<char*>(Tileset["source"].GetString());
    char *TilesetPath = GetLastAfterDelimiter(TilesetSource, '/');

    char FullTilesetPath[256];
    FormatString(FullTilesetPath, sizeof(FullTilesetPath), "%s%s", "tilesets/", TilesetPath);

    char *TilesetJson = (char*)Platform->ReadFile(FullTilesetPath).Contents;

    Map->Tilesets[0].Source = {};
    LoadTileset(&Map->Tilesets[0].Source, TilesetJson, Arena, Platform);

    const Value& Layers = Document["layers"];
    Assert(Layers.IsArray());

    u32 TileLayerCount = 0;
    u32 ObjectGroupCount = 0;

    for (SizeType LayerIndex = 0; LayerIndex < Layers.Size(); ++LayerIndex) 
    {
        const Value& Layer = Layers[LayerIndex];
        const char *LayerType = Layer["type"].GetString();

        if (StringEquals(LayerType, TILE_LAYER)) 
        {
            ++TileLayerCount;
        }
        else if (StringEquals(LayerType, OBJECT_LAYER)) 
        {
            ++ObjectGroupCount;
        }
        else 
        {
            InvalidCodePath;
        }
    }

    Map->TileLayerCount = TileLayerCount;
    Map->TileLayers = PushArray<tile_layer>(Arena, Map->TileLayerCount);

    Map->ObjectLayerCount = ObjectGroupCount;
    Map->ObjectLayers = PushArray<object_layer>(Arena, Map->ObjectLayerCount);

    u32 TileLayerIndex = 0;
    u32 ObjectLayerIndex = 0;

    for (SizeType LayerIndex = 0; LayerIndex < Layers.Size(); ++LayerIndex)
    {
        const Value& Layer = Layers[LayerIndex];
        const char *LayerType = Layer["type"].GetString();

        if (StringEquals(LayerType, TILE_LAYER)) 
        {
            tile_layer *TileLayer = Map->TileLayers + TileLayerIndex;

            TileLayer->StartX = Layer["startx"].GetInt();
            TileLayer->StartY = INVERT_Y(Layer["starty"].GetInt());
            
            TileLayer->Width = Layer["width"].GetUint();
            TileLayer->Height = Layer["height"].GetUint();

            TileLayer->Visible = Layer["visible"].GetBool();

            const Value& Chunks = Layer["chunks"];
            Assert(Chunks.IsArray());

            TileLayer->ChunkCount = Chunks.Size();
            TileLayer->Chunks = PushArray<map_chunk>(Arena, TileLayer->ChunkCount);

            for (SizeType ChunkIndex = 0; ChunkIndex < Chunks.Size(); ++ChunkIndex)
            {
                const Value& Chunk = Chunks[ChunkIndex];

                map_chunk *MapChunk = TileLayer->Chunks + ChunkIndex;

                MapChunk->X = Chunk["x"].GetInt();
                MapChunk->Y = INVERT_Y(Chunk["y"].GetInt());

                MapChunk->Width = Chunk["width"].GetUint();
                MapChunk->Height = Chunk["height"].GetUint();

                const Value& ChunkData = Chunk["data"];
                Assert(ChunkData.IsArray());

                MapChunk->GIDCount = ChunkData.Size();
                MapChunk->GIDs = PushArray<u32>(Arena, MapChunk->GIDCount);

                for (SizeType ChunkDataIndex = 0; ChunkDataIndex < ChunkData.Size(); ++ChunkDataIndex)
                {
                    MapChunk->GIDs[ChunkDataIndex] = ChunkData[ChunkDataIndex].GetUint();
                }
            }

            ++TileLayerIndex;
        }
        else if (StringEquals(LayerType, OBJECT_LAYER)) 
        {
            object_layer *ObjectLayer = Map->ObjectLayers + ObjectLayerIndex;

            ObjectLayer->Visible = Layer["visible"].GetBool();

            const Value& Objects = Layer["objects"];
            Assert(Objects.IsArray());

            ObjectLayer->ObjectCount = Objects.Size();
            ObjectLayer->Objects = PushArray<map_object>(Arena, ObjectLayer->ObjectCount);

            for (SizeType ObjectIndex = 0; ObjectIndex < Objects.Size(); ++ObjectIndex) 
            {
                const Value& Object = Objects[ObjectIndex];

                map_object *MapObject = ObjectLayer->Objects + ObjectIndex;

                MapObject->X = Object["x"].GetFloat();
                MapObject->Y = INVERT_Y(Object["y"].GetFloat());

                MapObject->Width = Object["width"].GetFloat();
                MapObject->Height = Object["height"].GetFloat();

                MapObject->GID = Object["gid"].GetUint();

                entity_type ObjectType = GetEntityTypeFromString(Object["type"].GetString());
                if (ObjectType != ENTITY_UNKNOWN)
                {
                    MapObject->Type = ObjectType;
                }
                else 
                {
                    u32 TileID = MapObject->GID - Map->Tilesets[0].FirstGID;
                    tile_meta_info *TileInfo = GetTileMetaInfo(&Map->Tilesets[0].Source, TileID);

                    if (TileInfo)
                    {
                        MapObject->Type = GetEntityTypeFromString(TileInfo->Type);
                    }
                }

                MapObject->Rotation = Object["rotation"].GetFloat();
                MapObject->ID = Object["id"].GetUint();
            }

            ++ObjectLayerIndex;
        }
        else
        {
            InvalidCodePath;
        }
    }
}
