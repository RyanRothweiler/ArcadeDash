#ifndef DASH_H
#define DASH_H

#define GLFW_DLL
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#include <windows.h>
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include <xinput.h>
#include <dsound.h>

#define internal static
#define global_variable static

#define Kilobytes(value) ((value) * 1024)
#define Megabytes(value) (Kilobytes(value) * 1024)
#define Gigabytes(value) (Megabytes(value) * 1024)
#define Terrabytes(value) (Megabytes(value) * 1024)

#if DEBUG_PATH
	#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}
	//NOTE AssertM is the same as Assert it just has a message perameter to say how to fix the assert. 
	// Just so we don't have to comment it every time
	#define AssertM(Expression, Message) if (!(Expression)) {*(int *)0 = 0;}
#else
	#define Assert(Expression)
	#define AssertM(Expression, Message)
#endif


typedef _int8 int8;
typedef _int16 int16;
typedef _int32 int32;
typedef _int64 int64;

typedef unsigned _int8 uint8;
typedef unsigned _int16 uint16;
typedef unsigned _int32 uint32;
typedef unsigned _int64 uint64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

struct read_file_result
{
	uint32 ContentsSize;
	void *Contents;
};

#define PLATFORM_READ_FILE(name) read_file_result name(char *Path)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_SAVE_STATE(name) void name(char *FilePath)
typedef PLATFORM_SAVE_STATE(platform_save_state);

#define PLATFORM_LOAD_STATE(name) void name(char *FilePath)
typedef PLATFORM_LOAD_STATE(platform_load_state);


struct game_memory
{
	bool32 IsInitialized;

	uint64 PermanentStorageSize;
	void *PermanentStorage; // NOTE Required to be cleared to 0 on startup / allocation

	//NOTE this transient memory head is reset to the top of transient storage at the beginning of the game loop
	// Anything that needs to stick around for more tha one game loop should go in PermanentStorage
	uint64 TransientStorageSize;
	void *TransientStorage;
	uint8 *TransientMemoryHead;

	uint64 TotalSize;
	void *GameMemoryBlock;

	int64 ElapsedCycles;

	platform_read_file *PlatformReadFile;
	platform_save_state *PlatformSaveState;
	platform_load_state *PlatformLoadState;
};

#include "Math.cpp"
#include "vector2.cpp"
#include "Color.cpp"
#include "String.cpp"
#include "TransientMemory.cpp"

struct game_audio_output_buffer
{
	// NOTE this running sample index is only for creating the sinwave. We don't need this otherwise. Remove this eventually.
	uint32 RunningSampleIndex;

	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
};

struct win32_audio_output
{
	int SamplesPerSecond;
	int BytesPerSample;
	int SecondaryBufferSize;
	real32 ToneVolume;
	uint32 RunningSampleIndex;
	DWORD SafetyBytes;
};

struct window_info
{
	uint32 Width;
	uint32 Height;
};

struct input_button
{
	bool32 OnDown;
	bool32 OnUp;
	bool32 IsDown;
	bool32 IsUp;
};

struct loaded_image
{
	uint32 Width;
	uint32 Height;

	GLuint GLTexture;
};

#include "Font.h"

struct game_input
{
	input_button AButton;
	input_button BButton;
	input_button XButton;
	input_button YButton;

	input_button DUp;
	input_button DRight;
	input_button DLeft;
	input_button DDown;

	input_button R1;
	input_button R2;
	input_button L1;
	input_button L2;

	input_button Select;
	input_button Start;

	vector2 LeftStick;
	bool32 LeftStickButton;

	bool32 RightStickButton;

};

struct gl_square
{
	color Color;
	vector2 TopLeft;
	vector2 TopRight;
	vector2 BottomLeft;
	vector2 BottomRight;
};

struct gl_texture
{
	loaded_image *Image;

	vector2 Center;
	vector2 Scale;
	real64 RadiansAngle;
	color Color;
};

struct gl_line
{
	vector2 Start;
	vector2 End;
	real32 Width;
	color Color;
};

#include "LinkedList.cpp"

enum entity_type
{
	ENTITY_TYPE_WALL,
	ENTITY_TYPE_ENEMY,
};

struct active_entity
{
	vector2 ForceOn;
	vector2 Position;
	vector2 Velocity;

	real32 MovementSpeed;

	color Color;

	uint16 ColliderWidth;
	bool32 OnCollide;
	bool32 IsColliding;
	active_entity *CollidingWith;
	vector2 CollideDirection;

	bool32 Alive;
	entity_type Type;
};

struct player
{
	vector2 MovingDirection;
	real32 SpeedCoeficient;
	real32 BaseSpeed;
	active_entity Entity;

	uint64 DashStartFrame;
	uint64 DashFrameLength;
	bool32 IsDashing;
	color DashColor;
	color BaseColor;
};

#pragma pack(push, 1)
struct bmp_header
{
	uint16 FileType;
	uint32 FileSize;
	uint16 Reserved1;
	uint16 Reserved2;
	uint32 BitmapOffset;
	uint32 Size;
	int32 Width;
	int32 Height;
	uint16 Planes;
	uint16 BitsPerPixel;
};
#pragma pack(pop)


struct game_state
{
	uint32 RandomGenState;

	player Player;

	vector2 WorldCenter;
	vector2 CamCenter;

	loaded_image TestImage;

	uint32 EntityBucketCount;
	active_entity EntityBucket[200];

	list_head RenderObjects;
	uint8 RenderLayerIndecies;

	//TODO change this to a list, first need to expand list to use given memory. 
	int32 WorldEntityCount;
	active_entity *WorldEntities[300];

	real64 TimeRate;

	uint64 FrameCounter;

	bool PrintFPS;
	char *DebugOutput = "";
	int64 PrevFrameFPS;

	uint32 AlphabetBitmapsCount;
	font_codepoint AlphabetBitmaps[200];
};

#define GAME_LOOP(name) void name(game_memory *Memory, game_input *GameInput, window_info *WindowInfo, game_audio_output_buffer *AudioBuffer)
typedef GAME_LOOP(game_update_and_render);
GAME_LOOP(GameLoopStub)
{ }

#define GAME_LOAD_ASSETS(name) void name(game_memory *Memory)
typedef GAME_LOAD_ASSETS(game_load_assets);
GAME_LOAD_ASSETS(GameLoadAssetsStub)
{ }

#endif