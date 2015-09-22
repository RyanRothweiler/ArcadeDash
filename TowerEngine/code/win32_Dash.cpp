

static bool PRINTFPS = false;

#include "Dash.h"

bool GlobalRunning = true;
window_info ScreenBuffer;
int64 ElapsedFrameCount;
int64 PerfCountFrequency;
LPDIRECTSOUNDBUFFER SoundSecondaryBuffer;

char *GameDllFileName = "Dash";

struct win32_game_code
{
	HMODULE GameCodeDLL;
	game_update_and_render *GameLoop;
	game_load_assets *GameLoadAssets;

	bool32 IsValid;
};

int32
StringLength(char *String)
{
	int Count = 0;
	while (*String++)
	{
		++Count;
	}
	return (Count);
}

int32
DigitCount(int64 *Input)
{
	char NumChar[MAX_PATH] = {};
	char *NumCharPointer = NumChar;

	int32 Count = 0;
	int64 tmp = *Input;
	while (tmp != 0)
	{
		tmp = (tmp - *NumCharPointer) / 10;
		Count++;
	}

	return (Count);
}

void
IntToCharArray(int64 *Input, char *Output)
{
	int64 tmp = *Input;
	char *NumCharPointer = Output + DigitCount(&tmp) - 1;

	while (tmp != 0)
	{
		*NumCharPointer-- = '0' + (tmp % 10);
		tmp = (tmp - *NumCharPointer) / 10;
	}
}


void
ConcatCharArrays(char *SourceA, char *SourceB, char *Destination)
{
	int32 SourceALength = StringLength(SourceA);
	int32 SourceBLength = StringLength(SourceB);

	for (int32 Index = 0;
	     Index < SourceALength;
	     Index++)
	{
		*Destination++ = *SourceA++;
	}

	for (int32 Index = 0;
	     Index < SourceBLength;
	     Index++)
	{
		*Destination++ = *SourceB++;
	}

	*Destination++ = 0;
}

void
DebugLine(int64 *Output)
{
	char NumChar[MAX_PATH] = {};
	IntToCharArray(Output, NumChar);

	char FinalOutput[MAX_PATH] = {};
	ConcatCharArrays(NumChar, "\n", FinalOutput);
	OutputDebugString(FinalOutput);
}

void
DebugLine(int64 Output)
{
	DebugLine(&Output);
}

void
DebugLine(char *Output)
{
	char FinalOutput[MAX_PATH] = {};
	ConcatCharArrays(Output, "\n", FinalOutput);
	OutputDebugString(FinalOutput);
}

void
ConcatIntChar(int64 IntInput, char *CharInput,
              char *CharOutput)
{
	char IntInputAsChar[MAX_PATH] = {};
	IntToCharArray(&IntInput, IntInputAsChar);
	ConcatCharArrays(IntInputAsChar, CharInput, CharOutput);
}

void
ConcatIntChar(char *CharInput, int64 IntInput,
              char *CharOutput)
{
	char IntInputAsChar[MAX_PATH] = {};
	IntToCharArray(&IntInput, IntInputAsChar);
	ConcatCharArrays(IntInputAsChar, CharInput, CharOutput);
}

real64
CheckStickDeadzone(short Value, SHORT DeadZoneThreshold)
{
	real64 Result = 0;

	if (Value < -DeadZoneThreshold)
	{
		Result = (real64)(Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold);
	}
	else if (Value > DeadZoneThreshold)
	{
		// this explicit number is pulled from my ass.
		Result = (real64)(Value  + DeadZoneThreshold) / (47467.0f - DeadZoneThreshold);
	}

	return (Result);
}

void
ProcessButtonInput(input_button *ButtonProcessing, bool32 NewState)
{
	if (NewState)
	{
		if (ButtonProcessing->IsDown)
		{
			ButtonProcessing->OnDown = false;
		}
		else
		{
			ButtonProcessing->IsDown = true;
			ButtonProcessing->OnDown = true;
			ButtonProcessing->IsUp = false;
			ButtonProcessing->OnUp = false;
		}

	}
	else
	{
		if (ButtonProcessing->IsUp)
		{
			ButtonProcessing->OnUp = false;
		}
		else
		{
			ButtonProcessing->IsUp = true;
			ButtonProcessing->OnUp = true;
			ButtonProcessing->IsDown = false;
			ButtonProcessing->OnDown = false;
		}
	}
}

void
ProcessTriggerInput(input_button *Trigger, int32 TriggerValue)
{
	if (TriggerValue > 200)
	{
		ProcessButtonInput(Trigger, true);
	}
	else
	{
		ProcessButtonInput(Trigger, false);
	}
}

win32_game_code
LoadGameCode()
{
	win32_game_code Result = {};

	char DllName[MAX_PATH] = {};
	ConcatCharArrays(GameDllFileName, ".dll", DllName);
	char DllTempName[MAX_PATH] = {};
	ConcatCharArrays(GameDllFileName, "_temp.dll", DllTempName);


	CopyFile(DllName, DllTempName, FALSE);
	Result.GameCodeDLL = LoadLibraryA(DllTempName);
	if (Result.GameCodeDLL)
	{
		Result.GameLoop = (game_update_and_render *)GetProcAddress(Result.GameCodeDLL, "GameLoop");
		Result.GameLoadAssets = (game_load_assets *)GetProcAddress(Result.GameCodeDLL, "GameLoadAssets");
	}

	// NOTE this is the wrong way to set is valid. we don't actually know it is valid.
	Result.IsValid = true;

	return (Result);
}

void
UnloadGameCode(win32_game_code *GameCode)
{
	if (GameCode->GameCodeDLL)
	{
		FreeLibrary(GameCode->GameCodeDLL);
	}

	GameCode->IsValid = false;
	GameCode->GameLoop = GameLoopStub;
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

void
LoadDirectSound(HWND WindowHandle, win32_audio_output *SoundOutput)
{
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			// NOTE channels maybe should be 2.
			WaveFormat.nChannels = 1;
			WaveFormat.nSamplesPerSec = SoundOutput->SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(WindowHandle, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						// NOTE now the format has been set
					}
					else
					{
						// diagnostics
					}
				}
				else
				{
					// diagnostics
				}
			}
			else
			{
				// diagnostics
			}

			//secondary buffer stuff

			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = SoundOutput->SecondaryBufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &SoundSecondaryBuffer, 0)))
			{

			}
			else
			{
				// diagnostics
			}
		}
	}
	else
	{
		// diagnostics
	}
}

void
FillSoundOutput(game_audio_output_buffer *GameAudio, win32_audio_output *SoundOutput,
                DWORD ByteToLock, DWORD BytesToWrite)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;

	if (SUCCEEDED(SoundSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
	              &Region1, &Region1Size,
	              &Region2,  &Region2Size,
	              0)))
	{
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		int16 *DestSample = (int16 *)Region1;
		int16 *SourceSamples = GameAudio->Samples;
		for (DWORD SampleIndex = 0;
		     SampleIndex < Region1SampleCount;
		     ++SampleIndex)
		{
			*DestSample++ = *SourceSamples++;
			*DestSample++ = *SourceSamples++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		DestSample = (int16 *)Region2;
		for (DWORD SampleIndex = 0;
		     SampleIndex < Region2SampleCount;
		     ++SampleIndex)
		{
			*DestSample++ = *SourceSamples++;
			*DestSample++ = *SourceSamples++;
			++SoundOutput->RunningSampleIndex;
		}

		SoundSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

inline LARGE_INTEGER
GetWallClock()
{
	LARGE_INTEGER Count;
	QueryPerformanceCounter(&Count);
	return (Count);
}

inline real32
GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)PerfCountFrequency);
	return (Result);
}

void
SaveSate(char *FileName, game_memory *GameMemory)
{
	DebugLine("Saving State");

	HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// NOTE if totalsize if greater than 4gb then we must write a for loop to loop over the write file.
	DWORD BytesWritten = {};
	bool32 Success = WriteFile(FileHandle, GameMemory->GameMemoryBlock, (DWORD)GameMemory->TotalSize, &BytesWritten, 0);

	Assert(Success)
	CloseHandle(FileHandle);

	DebugLine("Save Complete");
}

void
LoadState(char *FileName, game_memory *GameMemory, win32_game_code *GameCode)
{
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ,  0, 0, OPEN_EXISTING, 0, 0);
	DWORD BytesRead;
	bool32 Success = ReadFile(FileHandle, GameMemory->GameMemoryBlock, (DWORD)GameMemory->TotalSize, &BytesRead, 0);

	GameCode->GameLoadAssets(GameMemory);

	if (!Success)
	{
		DebugLine("Loading State File Failed");
	}
	CloseHandle(FileHandle);
}

read_file_result
LoadFileData(char *FileName)
{
	read_file_result Result = {};

	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32 FileSize32 = (uint32)FileSize.QuadPart;
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
				    (FileSize32 == BytesRead))
				{
					// File read successfully
					Result.ContentsSize = FileSize32;
				}
				else
				{
					VirtualFree(Result.Contents, 0, MEM_RELEASE);
					Result.Contents = 0;
				}
			}
			else
			{

			}
		}
		else
		{

		}

		CloseHandle(FileHandle);
	}
	else
	{

	}

	return (Result);
}

PLATFORM_READ_FILE(PlatformReadFile)
{
	read_file_result Result = LoadFileData(Path);
	return (Result);
}

inline FILETIME
GetGameCodeLastWriteTime()
{
	FILETIME LastWriteTime = {};

	WIN32_FILE_ATTRIBUTE_DATA Data;
	if (GetFileAttributesEx("OriginTower.dll", GetFileExInfoStandard, &Data))
	{
		LastWriteTime = Data.ftLastWriteTime;
	}

	return (LastWriteTime);
}

void
CheckSaveState(char *FilePath, input_button *ButtonChecking, bool32 SelectIsDown,
               game_memory *GameMemory, win32_game_code *GameCode)
{
	if (ButtonChecking->OnDown && SelectIsDown)
	{
		SaveSate(FilePath, GameMemory);
	}
	if (ButtonChecking->OnDown && !SelectIsDown)
	{
		LoadState(FilePath, GameMemory, GameCode);
	}
}


int32 main (int32 argc, char **argv)
{
	if (!glfwInit())
	{
		Assert(0);
		exit(EXIT_FAILURE);
	}

	ScreenBuffer = {};
	ScreenBuffer.Width = 1366;
	ScreenBuffer.Height = 768;

	GLFWwindow* OpenGLWindow = glfwCreateWindow(ScreenBuffer.Width, ScreenBuffer.Height, "Origin Tower", NULL, NULL);
	if (!OpenGLWindow)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(OpenGLWindow);
	glClearColor(1.0f, 0.2f, 1.0f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glOrtho(0, ScreenBuffer.Width, ScreenBuffer.Height, 0, -10, 10);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);

	LARGE_INTEGER FrequencyLong;
	QueryPerformanceFrequency(&FrequencyLong);
	PerfCountFrequency = FrequencyLong.QuadPart;

	LARGE_INTEGER FlipWallClock = GetWallClock();

	// Probably need to get this from hardware instead of pulling a number out of my ass
	int32 MonitorUpdateHz = 60;
	int32 GameUpdateHz = 60;
	real64 TargetSecondsElapsedPerFrame = 1.0f / (real64)GameUpdateHz;

	game_input GameInput = {};


	// #if INTERNAL
	// LPVOID BaseAddress = (LPVOID)Megabytes((uint64)2);
	// #else
	// LPVOID BaseAddress = 0;
	// #endif
	game_memory GameMemory = {};
	GameMemory.PermanentStorageSize = Megabytes(64);
	GameMemory.TransientStorageSize = Megabytes((uint64)1);
	GameMemory.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

	GameMemory.GameMemoryBlock = VirtualAlloc(NULL, (SIZE_T)GameMemory.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	DWORD error = GetLastError();
	GameMemory.PermanentStorage = GameMemory.GameMemoryBlock;
	GameMemory.TransientStorage = (uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;
	GameMemory.PlatformReadFile = PlatformReadFile;

	LARGE_INTEGER PreviousFrameCount = GetWallClock();

	win32_audio_output SoundOutput = {};
	SoundOutput.SamplesPerSecond = 48000;
	SoundOutput.RunningSampleIndex = 0;
	SoundOutput.BytesPerSample = sizeof(int16) * 2;
	SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
	SoundOutput.ToneVolume = 0.1f;
	bool SoundIsValid = false;

	int16 *AudioSamplesMemory = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	HWND WindowHandle = glfwGetWin32Window(OpenGLWindow);
	LoadDirectSound(WindowHandle, &SoundOutput);
	SoundSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

	win32_game_code GameCode = LoadGameCode();
	FILETIME GameCodeLastWriteTime = GetGameCodeLastWriteTime();

	while (!glfwWindowShouldClose(OpenGLWindow) && GlobalRunning)
	{
		if (glfwWindowShouldClose(OpenGLWindow))
		{
			GlobalRunning = false;
		}

		FILETIME NewDLLWriteTime = GetGameCodeLastWriteTime();
		if (CompareFileTime(&NewDLLWriteTime, &GameCodeLastWriteTime) != 0)
		{
			UnloadGameCode(&GameCode);
			GameCode = LoadGameCode();
			GameCodeLastWriteTime = NewDLLWriteTime;
		}

		DWORD dwResult;
		for (DWORD ControllerIndex = 0;
		     ControllerIndex < XUSER_MAX_COUNT;
		     ControllerIndex++)
		{
			XINPUT_STATE ControllerState;
			ZeroMemory(&ControllerState, sizeof(XINPUT_STATE));
			dwResult = XInputGetState(ControllerIndex, &ControllerState);

			if (dwResult == ERROR_SUCCESS)
			{
				XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

				ProcessButtonInput(&GameInput.AButton, Pad->wButtons & XINPUT_GAMEPAD_A);
				ProcessButtonInput(&GameInput.BButton, Pad->wButtons & XINPUT_GAMEPAD_B);
				ProcessButtonInput(&GameInput.XButton, Pad->wButtons & XINPUT_GAMEPAD_X);
				ProcessButtonInput(&GameInput.YButton, Pad->wButtons & XINPUT_GAMEPAD_Y);

				ProcessButtonInput(&GameInput.DUp, Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
				ProcessButtonInput(&GameInput.DDown, Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
				ProcessButtonInput(&GameInput.DLeft, Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
				ProcessButtonInput(&GameInput.DRight, Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

				ProcessButtonInput(&GameInput.R1, Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
				ProcessButtonInput(&GameInput.L1, Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
				ProcessTriggerInput(&GameInput.R2, Pad->bRightTrigger);
				ProcessTriggerInput(&GameInput.L2, Pad->bLeftTrigger);

				ProcessButtonInput(&GameInput.Start, Pad->wButtons & XINPUT_GAMEPAD_START);
				ProcessButtonInput(&GameInput.Select, Pad->wButtons & XINPUT_GAMEPAD_BACK);

				GameInput.LeftStick.X = ClampValue(-0.9f, 0.9f, CheckStickDeadzone(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
				GameInput.LeftStick.Y = ClampValue(-0.9f, 0.9f, CheckStickDeadzone(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)) * -1;

			}
			else
			{
				// Controller is not connected
			}
		}

		// CheckSaveState("SateSlot1.ts", &GameInput.R1, GameInput.Select.IsDown, &GameMemory, &GameCode);
		CheckSaveState("SateSlot2.ts", &GameInput.L1, GameInput.Select.IsDown, &GameMemory, &GameCode);
		CheckSaveState("SateSlot3.ts", &GameInput.R2, GameInput.Select.IsDown, &GameMemory, &GameCode);
		CheckSaveState("SateSlot4.ts", &GameInput.L2, GameInput.Select.IsDown, &GameMemory, &GameCode);

		LARGE_INTEGER AudioWallClock = GetWallClock();
		real32 FromBeginToAudioSeconds = GetSecondsElapsed(FlipWallClock, AudioWallClock);

		DWORD PlayCursor;
		DWORD WriteCursor;
		game_audio_output_buffer GameAudio = {};
		DWORD BytesToWrite = 0;
		DWORD ByteToLock = 0;
		if (SoundSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
		{
			if (!SoundIsValid)
			{
				SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
				SoundIsValid = true;
			}

			ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
			             SoundOutput.SecondaryBufferSize;


			DWORD ExpectedSoundBytesPerFrame = (int)(((real32)(SoundOutput.SamplesPerSecond *
			                                   SoundOutput.BytesPerSample)) /  GameUpdateHz);
			real64 SecondsLeftUntilFlip = (TargetSecondsElapsedPerFrame - FromBeginToAudioSeconds);
			DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsElapsedPerFrame
			                                       ) * (real32)ExpectedSoundBytesPerFrame);

			DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

			DWORD SafeWriteCursor = WriteCursor;
			if (SafeWriteCursor < PlayCursor)
			{
				SafeWriteCursor += SoundOutput.SecondaryBufferSize;
			}
			Assert(SafeWriteCursor >= PlayCursor);
			SafeWriteCursor += SoundOutput.SafetyBytes;

			bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

			DWORD TargetCursor = 0;
			if (AudioCardIsLowLatency)
			{
				TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
			}
			else
			{
				TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
			}
			TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;

			if (ByteToLock > TargetCursor)
			{
				BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock) + TargetCursor;
			}
			else
			{
				BytesToWrite = TargetCursor - ByteToLock;
			}

			GameAudio.SamplesPerSecond = SoundOutput.SamplesPerSecond;
			GameAudio.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
			GameAudio.Samples = AudioSamplesMemory;
		}

		GameCode.GameLoop(&GameMemory, &GameInput, &ScreenBuffer, &GameAudio);
		FillSoundOutput(&GameAudio, &SoundOutput, ByteToLock, BytesToWrite);

		game_state *GameStateFromMemory = (game_state *)GameMemory.PermanentStorage;
		char *EmptyChar = "";
		if (GameStateFromMemory->DebugOutput &&
		    GameStateFromMemory->DebugOutput != EmptyChar)
		{
			DebugLine(GameStateFromMemory->DebugOutput);
			GameStateFromMemory->DebugOutput = EmptyChar;
		}

		SYSTEMTIME SystemTime = {};
		GetSystemTime(&SystemTime);
		GameStateFromMemory->RandomGenState += SystemTime.wMilliseconds + SystemTime.wSecond + SystemTime.wMinute +
		                                       SystemTime.wDay + SystemTime.wMonth + SystemTime.wYear;
		if (GameStateFromMemory->RandomGenState > 100000)
		{
			GameStateFromMemory->RandomGenState = 0;
		}

		glClear(GL_COLOR_BUFFER_BIT);

		for (uint32 PosCount = 0;
		     PosCount < (uint32)GameStateFromMemory->RenderTexturesCount;
		     PosCount++)
		{
			glPushMatrix();


			vector2 Center = GameStateFromMemory->RenderTextures[PosCount].Center;
			vector2 Scale = GameStateFromMemory->RenderTextures[PosCount].Scale;
			glEnable(GL_TEXTURE_2D);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glBindTexture(GL_TEXTURE_2D, GameStateFromMemory->RenderTextures[PosCount].Image->GLTexture);
			glBegin(GL_QUADS);
			{
				glColor3f(1.0f, 1.0f, 1.0f);

				real64 Radians = GameStateFromMemory->RenderTextures[PosCount].RadiansAngle;

				vector2 RotatedPoint = {};
				vector2 OrigPoint = {};

				OrigPoint = {Center.X - Scale.X, Center.Y - Scale.Y};
				RotatedPoint = Vector2RotatePoint(OrigPoint, Center, Radians);
				glTexCoord2f(0, 1);
				glVertex2f((GLfloat)RotatedPoint.X, (GLfloat)RotatedPoint.Y);

				OrigPoint = {Center.X + Scale.X, Center.Y - Scale.Y};
				RotatedPoint = Vector2RotatePoint(OrigPoint, Center, Radians);
				glTexCoord2f(1, 1);
				glVertex2f((GLfloat)RotatedPoint.X, (GLfloat)RotatedPoint.Y);

				OrigPoint = {Center.X + Scale.X, Center.Y + Scale.Y};
				RotatedPoint = Vector2RotatePoint(OrigPoint, Center, Radians);
				glTexCoord2f(1, 0);
				glVertex2f((GLfloat)RotatedPoint.X, (GLfloat)RotatedPoint.Y);

				OrigPoint = {Center.X - Scale.X, Center.Y + Scale.Y};
				RotatedPoint = Vector2RotatePoint(OrigPoint, Center, Radians);
				glTexCoord2f(0, 0);
				glVertex2f((GLfloat)RotatedPoint.X, (GLfloat)RotatedPoint.Y);
			}
			glEnd();

			glPopMatrix();
		}

		glBindTexture(GL_TEXTURE_2D, 0);
		for (uint32 SquareIndex = 0;
		     SquareIndex < GameStateFromMemory->RenderSquaresCount;
		     SquareIndex++)
		{
			glBegin(GL_QUADS);
			{
				gl_square Square = GameStateFromMemory->RenderSquares[SquareIndex];
				glColor4f((GLfloat)Square.Color.R, (GLfloat)Square.Color.G, (GLfloat)Square.Color.B, (GLfloat)Square.Color.A);
				// NOTE the order of this can't be changed. Though I can't find any documentation on why or what the correct order is, but this works.
				glVertex2d(Square.TopRight.X, Square.TopRight.Y);
				glVertex2d(Square.TopLeft.X, Square.TopLeft.Y);
				glVertex2d(Square.BottomLeft.X, Square.BottomLeft.Y);
				glVertex2d(Square.BottomRight.X, Square.BottomRight.Y);
			}
			glEnd();
		}

		for (uint32 LineIndex = 0;
		     LineIndex < GameStateFromMemory->RenderLinesCount;
		     LineIndex++)
		{
			gl_line Line = GameStateFromMemory->RenderLines[LineIndex];
			glColor4f((GLfloat)Line.Color.R, (GLfloat)Line.Color.G, (GLfloat)Line.Color.B, (GLfloat)Line.Color.A);
			glBegin(GL_LINES);
			{
				glVertex2d(Line.Start.X, Line.Start.Y);
				glVertex2d(Line.End.X, Line.End.Y);
			}
			glEnd();
		}


		glfwSwapBuffers(OpenGLWindow);

		LARGE_INTEGER WorkFrameCount = GetWallClock();
		ElapsedFrameCount = WorkFrameCount.QuadPart - PreviousFrameCount.QuadPart;

		real64 SecondsElapsedForWork = (real64)ElapsedFrameCount / (real64)PerfCountFrequency;
		real64 SecondsElapsedForFrame = SecondsElapsedForWork;
		while (SecondsElapsedForFrame < TargetSecondsElapsedPerFrame)
		{
			LARGE_INTEGER NewWorkFrameCount = GetWallClock();
			SecondsElapsedForFrame = (((real64)NewWorkFrameCount.QuadPart - PreviousFrameCount.QuadPart) /
			                          (real64)PerfCountFrequency);
		}

		WorkFrameCount = GetWallClock();

		ElapsedFrameCount = WorkFrameCount.QuadPart - PreviousFrameCount.QuadPart;
		int64 MSThisFrame = (1000 * ElapsedFrameCount) / PerfCountFrequency;

		// NOTE game is forced at 60 fps. Anything smaller doesn't work.
		int64 FPS = PerfCountFrequency / ElapsedFrameCount;
		char charFPS[MAX_PATH] = {};
		ConcatIntChar(FPS, " FPS", charFPS);
		if (GameStateFromMemory->PrintFPS)
		{
			DebugLine(charFPS);
		}

		PreviousFrameCount = WorkFrameCount;
		GameMemory.ElapsedCycles = PreviousFrameCount.QuadPart;

		FlipWallClock = GetWallClock();

		glfwPollEvents();
	}

	glfwDestroyWindow(OpenGLWindow);
	glfwTerminate();
}