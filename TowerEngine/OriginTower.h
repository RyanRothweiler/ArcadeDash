

#define GLFW_DLL
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#include <windows.h>
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include <xinput.h>
#include <dsound.h>

#define internal static

#define Kilobytes(value) ((value) * 1024)
#define Megabytes(value) (Kilobytes(value) * 1024)
#define Gigabytes(value) (Megabytes(value) * 1024)
#define Terrabytes(value) (Megabytes(value) * 1024)



/*
	SLOW:
		0 - No slow code allowed
		1 - Slow code allowed

	INTERNAL:
		0 - Build on dev machine
		1 - Build on release machine
*/


#if SLOW
	#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}
#else
	#define Assert()
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

#include "Math.cpp"
#include "vector2.cpp"
#include "Color.h"


struct wave_header
{
	uint32 RiffID;
	uint32 Size;
	uint32 WaveID;
};

#define WAVE_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))
enum
{
	WAVE_ChunkID_fmt = WAVE_CODE('f', 'm', 't', ' '),
	WAVE_ChunkID_data = WAVE_CODE('d', 'a', 't', 'a'),
	WAVE_ChunkID_RIFF = WAVE_CODE('R', 'I', 'F', 'F'),
	WAVE_ChunkID_WAVE = WAVE_CODE('W', 'A', 'V', 'E'),
};

struct wave_chunk
{
	uint32 ID;
	uint32 Size;
};

struct wave_fmt
{
	uint16 Format;
	uint16 NumberOfChannels;
	uint32 NumSamplesPerSecond;
	uint32 AverageBytesPerSec;
	uint16 BlockAlign;
	uint16 BitsPerSample;
	uint16 CBSize;
	uint16 ValidBitsPerSample;
	uint32 ChannelMask;
	uint8 SubFormat[16];
};

struct riff_iterator
{
	uint8 *At;
	uint8 *Stop;
};

// NOTE only the platform touches this
struct win32_audio_output
{
	int SamplesPerSecond;
	int BytesPerSample;
	int SecondaryBufferSize;
	real32 ToneVolume;
	uint32 RunningSampleIndex;
	DWORD SafetyBytes;
};

struct loaded_sound
{
	uint32 SampleCount;
	uint32 ChannelCount;
	int16 *Samples[2];
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

struct loaded_image
{
	uint32 Width;
	uint32 Height;

	GLuint GLTexture;
};

struct game_audio_output_buffer
{
	// NOTE this running sample index is only for creating the sinwave. We don't need this otherwise. Remove this eventually.
	uint32 RunningSampleIndex;

	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
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
};

struct gl_line
{
	vector2 Start;
	vector2 End;
	real32 Width;
	color Color;
};

struct active_entity
{
	vector2 ForceOn;
	vector2 Position;
	vector2 Velocity;

	real32 Weight;
	real32 MovementSpeed;

	color Color;

	uint16 ColliderWidth;
	bool32 OnCollide;
	bool32 IsColliding;
	active_entity *CollidingWith;
	vector2 CollideDirection;

	uint16 ImageWidth;
	loaded_image *Image;

	vector2 FacingDirection;

	bool32 Alive;
};

struct enemy
{
	vector2 TargetCenter;
	vector2 TargetPos;
	active_entity Entity;

	real32 MaxHealth;
	real32 CurrHealth;
};

struct player
{
	active_entity Entity;

	int64 CurrHealth;

	vector2 ForceWhenSlow;

	real32 CurrMovementSpeed;
	real32 MaxMovementSpeed;

	bool32 CanGrapple;
	bool32 IsGrappled;
	enemy *GrappledEnemy;
	vector2 RelativeGrapplePoint;

	color RedHitFlash;
};

struct game_state
{
	int32 EnemyCount;
	enemy Enemies[50];

	int32 WorldEntityCount;
	active_entity *WorldEntities[50];

	player Player;

	uint32 PlayerHealthCount;
	gl_square *PlayerHealthSquares[50];

	loaded_sound TestNote;
	uint32 TestNoteSampleIndex;

	int16 BackgroundPositionsCount;
	vector2 BackgroundPositions[200];

	uint32 RenderTexturesCount;
	gl_texture RenderTextures[300];
	uint32 RenderSquaresCount;
	gl_square RenderSquares[50];
	uint32 RenderLinesCount;
	gl_line RenderLines[50];

	loaded_image BackgroundImage;
	loaded_image GrappleRadiusImage;
	loaded_image GrappleArrowImage;
	loaded_image GrappleLineImage;
	loaded_image PlayerImage;
	loaded_image EnemyImage;
	loaded_image EnemyHealthBar;

	vector2 WorldCenter;
	vector2 CamCenter;

	int64 RandomGenState;

	real64 TimeRate;

	bool PrintFPS;
	char *DebugOutput = "";
};

struct read_file_result
{
	uint32 ContentsSize;
	void *Contents;
};

#define PLATFORM_READ_FILE(name) read_file_result name(char *Path)
typedef PLATFORM_READ_FILE(platform_read_file);

struct game_memory
{
	bool32 IsInitialized;

	uint64 PermanentStorageSize;
	void *PermanentStorage; // NOTE Required to be cleared to 0 on startup / allocation
	uint64 TransientStorageSize;
	void *TransientStorage;

	uint64 TotalSize;
	void *GameMemoryBlock;

	int64 ElapsedCycles;

	platform_read_file *PlatformReadFile;
};

#define GAME_LOOP(name) void name(game_memory *Memory, game_input *GameInput, window_info *WindowInfo, game_audio_output_buffer *AudioBuffer)
typedef GAME_LOOP(game_update_and_render);
GAME_LOOP(GameLoopStub)
{ }

#define GAME_LOAD_ASSETS(name) void name(game_memory *Memory)
typedef GAME_LOAD_ASSETS(game_load_assets);
GAME_LOAD_ASSETS(GameLoadAssetsStub)
{ }
