
#include "OriginTower.h"

static platform_read_file *PlatformReadFile;

int64
Lerp(int64 a, int64 b, real32 t)
{
	return (int64)(a + t * (b - a));
}

real64
RandomRangeFloat(real32 Bottom, real32 Top, game_state *GameState)
{
	Assert(Bottom < Top);
	real64 Result = 0;

	uint32 RandomMax = 1000;
	uint32 RandomInt = (10 * GameState->RandomGenState % RandomMax);
	real64 RandomScalar = (real32)RandomInt / (real32)RandomMax;

	real64 ScaledNum = (real64)((Top - Bottom) * RandomScalar);
	Result = ScaledNum + Bottom;

	GameState->RandomGenState += GameState->RandomGenState;

	return (Result);
}

int64
RandomRangeInt(int32 Bottom, int32 Top, game_state *GameState)
{
	real64 Result = RandomRangeFloat((real32)Bottom, (real32)Top, GameState);
	return ((int64)Result);
}

// NOTE this debug line only outputs after the end of the GameLoop.
void
DebugLine(char *Output, game_state *GameState)
{
	GameState->DebugOutput = Output;
}

inline riff_iterator
ParseChunkAt(void *At, void *Stop)
{
	riff_iterator Iter;

	Iter.At = (uint8 *)At;
	Iter.Stop = (uint8 *)Stop;

	return (Iter);
}

inline riff_iterator
NextChunk(riff_iterator Iter)
{
	wave_chunk *Chunk = (wave_chunk *)Iter.At;
	uint32 Size = (Chunk->Size + 1) & ~1;
	Iter.At += sizeof(wave_chunk) + Size;

	return (Iter);
}

inline bool32
IsValid(riff_iterator Iter)
{
	bool32 Result = (Iter.At < Iter.Stop);
	return (Result);
}

inline void *
GetChunkData(riff_iterator Iter)
{
	void *Result = (Iter.At + sizeof(wave_chunk));
	return (Result);
}

inline uint32
GetType(riff_iterator Iter)
{
	wave_chunk *Chunk = (wave_chunk *)Iter.At;
	uint32 Result = Chunk->ID;

	return (Result);
}

uint32
GetChunkDataSize(riff_iterator Iter)
{
	wave_chunk *Chunk = (wave_chunk *)Iter.At;
	uint32 Result = Chunk->Size;

	return (Result);
}

loaded_sound
LoadWave(char *FilePath)
{
	loaded_sound Result = {};

	//NOTE this is the loading the wave file code. Maybe all this should be pulled into the game layer
	read_file_result WaveResult = PlatformReadFile(FilePath);
	wave_header *WaveHeader = (wave_header *)WaveResult.Contents;
	Assert(WaveHeader->RiffID == WAVE_ChunkID_RIFF);
	Assert(WaveHeader->WaveID == WAVE_ChunkID_WAVE);

	uint32 ChannelCount = 0;
	uint32 SampleDataSize = 0;
	int16 *SampleData = 0;
	for (riff_iterator Iter = ParseChunkAt(WaveHeader + 1, (uint8 *)(WaveHeader + 1) + WaveHeader->Size - 4);
	     IsValid(Iter);
	     Iter = NextChunk(Iter))
	{
		switch (GetType(Iter))
		{
			case WAVE_ChunkID_fmt:
			{
				wave_fmt *fmt = (wave_fmt *)GetChunkData(Iter);

				// NOTE Assert that this file is in a supported format
				// Using PCM format;
				Assert(fmt->Format == 1);
				Assert(fmt->NumSamplesPerSecond == 48000);
				Assert(fmt->BitsPerSample == 16);
				Assert(fmt->BlockAlign == (sizeof(int16) * fmt->NumberOfChannels));
				ChannelCount = fmt->NumberOfChannels;
			} break;
			case WAVE_ChunkID_data:
			{
				SampleData = (int16 *)GetChunkData(Iter);
				SampleDataSize = GetChunkDataSize(Iter);
			} break;
		}
	}

	Result.ChannelCount = ChannelCount;
	Result.SampleCount = SampleDataSize / (ChannelCount * sizeof(int16));
	if (ChannelCount == 1)
	{
		Result.Samples[0] = SampleData;
		Result.Samples[1] = 0;
	}
	else if (ChannelCount == 2)
	{

		Result.Samples[0] = SampleData;
		Result.Samples[1] = SampleData + Result.SampleCount;

		for (uint32 SampleIndex = 0;
		     SampleIndex < Result.SampleCount;
		     ++SampleIndex)
		{
			int16 Source = SampleData[2 * SampleIndex];
			SampleData[2 * SampleIndex] = SampleData[SampleIndex];
			SampleData[SampleIndex] = Source;
		}
	}
	else
	{
		Assert(!"Invalid Channel Count");
	}

	// TODO this only loads the left channel. MAYBE load the right channel too
	Result.ChannelCount = 1;

	return (Result);
}


loaded_image
GLLoadBMP(char *FilePath)
{
	loaded_image Result = {};

	read_file_result FileResult = PlatformReadFile(FilePath);
	uint32 *BitmapPixels = {};
	if (FileResult.ContentsSize != 0)
	{
		bmp_header *Header = (bmp_header *)FileResult.Contents;
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		// NOTE this number offset here is pulled from my ass. The offset in the image doesn't seem to work.
		BitmapPixels = ((uint32 *)FileResult.Contents + 35);
	}
	else
	{
		// Something went wrong with loading the bmp
		// NOTE will eventually want to just show a warning. But don't crash the game.
		Assert(0);
	}

	uint32 *Source = (uint32 *)BitmapPixels;
	for (uint32 PixelIndex = 0;
	     PixelIndex < (Result.Width * Result.Height);
	     ++PixelIndex)
	{
		uint8 *Pixel = (uint8 *)Source;

		uint8 Bit2 = *Pixel++; // A
		uint8 Bit3 = *Pixel++; // R
		uint8 Bit0 = *Pixel++; // G
		uint8 Bit1 = *Pixel++; // B

		*Source++ = (Bit0 << 24) | (Bit1 << 16) | (Bit2 << 8) | (Bit3 << 0);
	}


	glGenTextures(1, &Result.GLTexture);
	glBindTexture(GL_TEXTURE_2D, Result.GLTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	             Result.Width, Result.Height,
	             0, GL_RGBA, GL_UNSIGNED_BYTE, BitmapPixels);

	return (Result);
}

void
LoadAssets(game_state *GameState)
{
	// NOTE currently sound is forced at...
	// Mono, 16 bit, 48000.
	// anything else won't work. need to implement stereo sound at some point.
	GameState->TestNote = LoadWave("../assets/testNote.wav");
	GameState->TestNoteSampleIndex = 0;

	GameState->BackgroundImage = GLLoadBMP("../assets/Background.bmp");
	GameState->PlayerImage = GLLoadBMP("../assets/Player.bmp");
	GameState->GrappleRadiusImage = GLLoadBMP("../assets/LatchRadius.bmp");
	GameState->GrappleLineImage = GLLoadBMP("../assets/GrappleLine.bmp");
	GameState->GrappleArrowImage = GLLoadBMP("../assets/GrappleArrow.bmp");
	GameState->EnemyImage = GLLoadBMP("../assets/Enemy.bmp");
	GameState->EnemyHealthBar = GLLoadBMP("../assets/HealthBarBackground.bmp");

	// NOTE this line is necessary to initialize the DebugOuput var of GameState. It must be initialized to something.
	DebugLine("Loaded", GameState);
}

gl_square
MakeSquare(vector2 Pos, int32 SideLength, color Color)
{
	gl_square Result = {};

	Result.Color = Color;

	int32 HalfSide = SideLength / 2;
	Result.TopLeft = vector2{Pos.X - HalfSide, Pos.Y - HalfSide};
	Result.TopRight = vector2{Pos.X + HalfSide, Pos.Y - HalfSide};
	Result.BottomLeft = vector2{Pos.X - HalfSide, Pos.Y + HalfSide};
	Result.BottomRight = vector2{Pos.X + HalfSide, Pos.Y + HalfSide};

	return (Result);
}

void
PushRenderSquare(game_state *GameState, gl_square Square)
{
	Assert(_countof(GameState->RenderSquares) > GameState->RenderSquaresCount);
	GameState->RenderSquares[GameState->RenderSquaresCount].TopLeft = Square.TopLeft;
	GameState->RenderSquares[GameState->RenderSquaresCount].TopRight = Square.TopRight;
	GameState->RenderSquares[GameState->RenderSquaresCount].BottomLeft = Square.BottomLeft;
	GameState->RenderSquares[GameState->RenderSquaresCount].BottomRight = Square.BottomRight;
	GameState->RenderSquares[GameState->RenderSquaresCount].Color = Square.Color;
	GameState->RenderSquaresCount++;
}

void
PushRenderTexture(game_state *GameState, gl_texture *Texture)
{
	Assert(_countof(GameState->RenderTextures) > GameState->RenderTexturesCount);
	GameState->RenderTextures[GameState->RenderTexturesCount].Image = Texture->Image;
	GameState->RenderTextures[GameState->RenderTexturesCount].Center = Texture->Center;
	GameState->RenderTextures[GameState->RenderTexturesCount].Scale = Texture->Scale;
	GameState->RenderTextures[GameState->RenderTexturesCount].RadiansAngle = Texture->RadiansAngle;
	GameState->RenderTexturesCount++;
}

void
PushRenderLine(game_state *GameState, gl_line *Line)
{
	Assert(_countof(GameState->RenderLines) > GameState->RenderLinesCount);
	GameState->RenderLines[GameState->RenderLinesCount].Start = Line->Start;
	GameState->RenderLines[GameState->RenderLinesCount].End = Line->End;
	GameState->RenderLines[GameState->RenderLinesCount].Color = Line->Color;
	GameState->RenderLines[GameState->RenderLinesCount].Width = Line->Width;
	GameState->RenderLinesCount++;
}

void
AddWorldEntity(game_state *GameState, active_entity *Entity)
{
	GameState->WorldEntities[GameState->WorldEntityCount] = Entity;
	GameState->WorldEntityCount++;
}

enemy *
GetNewEnemy(game_state *GameState)
{
	Assert(_countof(GameState->Enemies) > GameState->EnemyCount);
	AddWorldEntity(GameState, &GameState->Enemies[GameState->EnemyCount].Entity);
	enemy *Result = &GameState->Enemies[GameState->EnemyCount];
	GameState->EnemyCount++;
	return (Result);
}

void
ControllerAssert(input_button *Button)
{
	if (Button->IsDown)
	{
		Assert(0);
	}
}

void
KillEntity(active_entity *Entity)
{
	Entity->Alive = false;
	Entity->Position = vector2{90000, 90000};
}

void
EnemyGetRandomTarget(enemy *Enemy, game_state *GameState)
{
	Enemy->TargetPos.X = Enemy->TargetCenter.X + RandomRangeInt(-200, 200, GameState);
	Enemy->TargetPos.Y = Enemy->TargetCenter.X + RandomRangeInt(-200, 200, GameState);
}

void
TimePause(game_state *GameState)
{
	GameState->TimeRate = 0.0f;
}

bool32
ClipLine(uint8 Dimension, vector2 Start, vector2 End, gl_square *Square, real64 *FLow, real64 *FHigh)
{

	real64 Low = (Vector2GetDimension(Dimension, Square->TopLeft) - Vector2GetDimension(Dimension, Start)) /
	             (Vector2GetDimension(Dimension, End) - Vector2GetDimension(Dimension, Start));
	real64 High = (Vector2GetDimension(Dimension, Square->BottomRight) - Vector2GetDimension(Dimension, Start)) /
	              (Vector2GetDimension(Dimension, End) - Vector2GetDimension(Dimension, Start));

	if (High < Low)
	{
		real64 Temp = Low;
		Low = High;
		High = Temp;
	}

	if (High < *FLow)
	{
		return (false);
	}

	if (Low > *FHigh)
	{
		return (false);
	}

	*FLow = Max(Low, *FLow);
	*FHigh = Min(High, *FHigh);

	if (*FLow > *FHigh)
	{
		return (false);
	}

	return (true);

}

real64
FacingDirectionToRotationAngle(vector2 FacingDirection)
{
	real64 Angle = Vector2AngleBetween(FacingDirection, vector2{1, 0});
	if (FacingDirection.Y > 0)
	{
		Angle = Angle * -1;
	}
	return (Angle);
}



extern "C" GAME_LOOP(GameLoop)
{
	PlatformReadFile = Memory->PlatformReadFile;

	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	Assert(GameState);
	if (!Memory->IsInitialized)
	{
		GameState->PrintFPS = true;

		LoadAssets(GameState);

		GameState->WorldEntityCount = 0;
		GameState->PlayerHealthCount = 0;
		GameState->EnemyCount = 0;

		GameState->TimeRate = 1.0f;

		uint16 PosCount = 0;
		uint16 XCount = 10;
		uint16 YCount = 10;
		GameState->BackgroundPositionsCount = XCount * YCount;
		for (int16 BackgroundX = (XCount / 2) * -1;
		     BackgroundX < (XCount / 2);
		     BackgroundX++)
		{
			for (int16 BackgroundY = (YCount / 2) * -1;
			     BackgroundY < YCount / 2;
			     BackgroundY++)
			{
				GameState->BackgroundPositions[PosCount] = vector2{(real64)(BackgroundX * GameState->BackgroundImage.Width),
				                                                   (real64)(BackgroundY * GameState->BackgroundImage.Height)};
				PosCount++;
			}
		}


		GameState->Player.Entity.Position.X = WindowInfo->Width / 2;
		GameState->Player.Entity.Position.Y = WindowInfo->Height / 2;
		GameState->Player.Entity.ColliderWidth = 20;
		GameState->Player.Entity.ImageWidth = 40;
		GameState->Player.Entity.MovementSpeed = 3;
		GameState->Player.CurrMovementSpeed = GameState->Player.Entity.MovementSpeed;
		GameState->Player.MaxMovementSpeed = GameState->Player.Entity.MovementSpeed;
		GameState->Player.Entity.Weight = 0.8f;
		GameState->Player.Entity.Color = COLOR_BLUE;
		GameState->Player.Entity.Alive = true;
		GameState->Player.Entity.Image = &GameState->PlayerImage;
		GameState->Player.CurrHealth = 3;
		GameState->Player.RedHitFlash = color{1.0f, 0.0f, 0.0f, 0.0f};
		GameState->Player.CanGrapple = true;
		GameState->Player.IsGrappled = false;
		AddWorldEntity(GameState, &GameState->Player.Entity);


		enemy *Enemy = {};

		Enemy = GetNewEnemy(GameState);
		Enemy->Entity.Position.X = 700.0f;
		Enemy->Entity.Position.Y = 700.0f;
		Enemy->TargetCenter = Enemy->Entity.Position;
		Enemy->Entity.ImageWidth = 25;
		Enemy->Entity.ColliderWidth = Enemy->Entity.ImageWidth;
		Enemy->Entity.MovementSpeed = 1;
		Enemy->Entity.Color = COLOR_GREEN;
		Enemy->Entity.Alive = true;
		Enemy->Entity.Weight = 0.5f;
		Enemy->Entity.Image = &GameState->EnemyImage;
		Enemy->MaxHealth = 150;
		Enemy->CurrHealth = Enemy->MaxHealth;
		EnemyGetRandomTarget(Enemy, GameState);

		Enemy = GetNewEnemy(GameState);
		Enemy->Entity.Position.X = 300.0f;
		Enemy->Entity.Position.Y = 300.0f;
		Enemy->TargetCenter = Enemy->Entity.Position;
		Enemy->Entity.ImageWidth = 50;
		Enemy->Entity.ColliderWidth = Enemy->Entity.ImageWidth;
		Enemy->Entity.MovementSpeed = 5;
		Enemy->Entity.Color = color{1.0f, 0.2f, 0, 1.0f};
		Enemy->Entity.Alive = true;
		Enemy->Entity.Weight = 7;
		Enemy->Entity.Image = &GameState->EnemyImage;
		Enemy->MaxHealth = 150;
		Enemy->CurrHealth = Enemy->MaxHealth;
		EnemyGetRandomTarget(Enemy, GameState);

		GameState->WorldCenter = vector2{0, 0};
		GameState->CamCenter = vector2{(real64)(WindowInfo->Width / 2), (real64)(WindowInfo->Height / 2)};

		AudioBuffer->RunningSampleIndex = 0;

		Memory->IsInitialized = true;
		// NOTE this line is necessary to initialize the DebugOuput var of GameState. It must be initialized to something.
		DebugLine("Initialized", GameState);
	}

	player *Player = &GameState->Player;

	GameState->RenderSquaresCount = 0;
	GameState->RenderTexturesCount = 0;
	GameState->RenderLinesCount = 0;

	if (GameInput->BButton.OnDown)
	{
		GameState->TestNoteSampleIndex = 0;
	}

	if (GameInput->YButton.IsDown)
	{
		GameState->TimeRate = 0.2f;
	}
	else
	{
		GameState->TimeRate = 1.0f;
	}

	// NOTE Sound doesn't work well at all when fps drops below 60
	int16 *SampleOut = AudioBuffer->Samples;
	for (int SampleIndex = 0;
	     SampleIndex < AudioBuffer->SampleCount;
	     ++SampleIndex)
	{
		int16 SampleValue = 0;

		uint32 SampleValueIndex = (GameState->TestNoteSampleIndex + SampleIndex);
		if (SampleValueIndex < GameState->TestNote.SampleCount)
		{
			SampleValue = GameState->TestNote.Samples[0][SampleValueIndex];
		}
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
	}
	GameState->TestNoteSampleIndex += AudioBuffer->SampleCount;

	for (int8 index = 0;
	     index < GameState->EnemyCount;
	     index++)
	{
		enemy *Enemy = &GameState->Enemies[index];

		if (Enemy->CurrHealth < 0)
		{
			KillEntity(&Enemy->Entity);
		}

		if (Enemy->Entity.Alive)
		{
			real64 Distance = Vector2Distance(Enemy->Entity.Position, Enemy->TargetPos);
			if (Distance < 10.0f)
			{
				EnemyGetRandomTarget(Enemy, GameState);
			}
			vector2 DirectionPos = {};
			DirectionPos = Enemy->Entity.Position - Enemy->TargetPos;
			DirectionPos = -1 * Vector2Normalize(DirectionPos);

			vector2 DirectionDifference = DirectionPos - Enemy->Entity.FacingDirection;
			Enemy->Entity.FacingDirection = Enemy->Entity.FacingDirection + (DirectionDifference * 0.2f);

			Enemy->Entity.ForceOn = Enemy->Entity.ForceOn + (DirectionPos * Enemy->Entity.MovementSpeed);
		}
	}

	if (GameInput->R1.OnDown && Player->IsGrappled)
	{
		Player->IsGrappled = false;
		Player->CanGrapple = false;
	}
	if (GameInput->R1.OnUp)
	{
		Player->CanGrapple = true;
	}

	if (GameInput->R1.IsDown && Player->CanGrapple)
	{
		GameState->TimeRate = 0.15f;
	}
	else
	{
		GameState->TimeRate = 1.0f;
	}

	if (GameInput->R1.OnUp && !Player->IsGrappled && Player->CanGrapple)
	{
		vector2 Start = GameState->Player.Entity.Position;
		vector2 End = GameState->Player.Entity.Position + (GameState->Player.Entity.FacingDirection * 1000.0);

		for (int EntityIndex = 0;
		     EntityIndex < GameState->EnemyCount;
		     EntityIndex++)
		{
			enemy *EnemyAbout = &GameState->Enemies[EntityIndex];
			active_entity *EntityAbout = &EnemyAbout->Entity;
			if (EntityAbout->Alive && EntityAbout != &GameState->Player.Entity)
			{

				bool32 Hit = true;

				gl_square Square = MakeSquare(EntityAbout->Position, EntityAbout->ColliderWidth, COLOR_RED);

				real64 FLow = 0.0f;
				real64 FHigh = 1.0f;

				if (!ClipLine(0, Start, End, &Square, &FLow, &FHigh))
				{
					Hit = false;
				}
				if (!ClipLine(1, Start, End, &Square, &FLow, &FHigh))
				{
					Hit = false;
				}

				if (Hit)
				{
					Player->IsGrappled = true;
					Player->GrappledEnemy = EnemyAbout;

					vector2 Ray = End - Start;
					vector2 IntersectPoint = Start + (Ray * FLow);
					Player->RelativeGrapplePoint = EntityAbout->Position - IntersectPoint;
				}

			}
		}
	}

	if (Player->IsGrappled)
	{
		real64 SpringConstant = 0.1f;
		real64 SpringRestLength = 100.0f;

		vector2 SpringVector = Player->GrappledEnemy->Entity.Position - Player->Entity.Position;
		real64 SpringVectorLength = Vector2Length(SpringVector);
		vector2 SpringForce = (Vector2Normalize(SpringVector) * -1.0 * SpringConstant) * (SpringVectorLength - SpringRestLength) * 0.5f;

		Player->Entity.ForceOn = Player->Entity.ForceOn + (SpringForce * -1.0f);
		Player->GrappledEnemy->Entity.ForceOn = Player->GrappledEnemy->Entity.ForceOn + SpringForce;
	}

	Player->Entity.MovementSpeed = Player->CurrMovementSpeed;
	if (GameInput->R1.IsDown && Player->CanGrapple)
	{
		Player->Entity.ForceOn = Player->Entity.ForceOn + Player->ForceWhenSlow;;
	}
	else
	{
		Player->Entity.ForceOn = Player->Entity.ForceOn + Vector2Normalize(GameInput->LeftStick) * Player->Entity.MovementSpeed;
	}
	if (GameInput->R1.OnDown)
	{
		Player->ForceWhenSlow = Vector2Normalize(GameInput->LeftStick) * Player->Entity.MovementSpeed * 0.2f;
	}

	if (GameInput->LeftStick.X > 0.1f || GameInput->LeftStick.X < -0.1f ||
	    GameInput->LeftStick.Y > 0.1f || GameInput->LeftStick.Y < -0.1f)
	{
		vector2 DirectionDifference = Vector2Normalize(GameInput->LeftStick) - Player->Entity.FacingDirection;
		Player->Entity.FacingDirection = Player->Entity.FacingDirection + (DirectionDifference * 0.2f);
	}

	vector2 PlayerCamDifference = Player->Entity.Position - GameState->WorldCenter;
	GameState->WorldCenter = GameState->WorldCenter + (PlayerCamDifference * 0.08f * GameState->TimeRate);
	vector2 WorldCenter = GameState->WorldCenter - GameState->CamCenter;

	for (uint32 PosCount = 0;
	     PosCount < (uint32)GameState->BackgroundPositionsCount;
	     PosCount++)
	{
		loaded_image Image = GameState->BackgroundImage;
		gl_texture Texture = {};
		Texture.Image = &GameState->BackgroundImage;
		Texture.Center = GameState->BackgroundPositions[PosCount] - WorldCenter;
		Texture.Scale = vector2{1000, 1000};
		PushRenderTexture(GameState, &Texture);
	}

	if (Player->IsGrappled)
	{
		gl_line Line;
		Line.Start = Player->Entity.Position - WorldCenter;
		vector2 WorldPoint = (Player->GrappledEnemy->Entity.Position + Player->RelativeGrapplePoint) - WorldCenter;
		real64 EntityRotation = FacingDirectionToRotationAngle(Player->GrappledEnemy->Entity.FacingDirection);
		Line.End = Vector2RotatePoint(WorldPoint, Player->GrappledEnemy->Entity.Position - WorldCenter, EntityRotation);
		Line.Color = COLOR_WHITE;
		Line.Width = 40;
		PushRenderLine(GameState, &Line);


		Player->GrappledEnemy->CurrHealth -= 0.5f;
		if (Player->GrappledEnemy->CurrHealth < 0)
		{
			KillEntity(&Player->GrappledEnemy->Entity);
			Player->CanGrapple = true;
			Player->IsGrappled = false;
		}
		else
		{
			real64 HealthRatio = Player->GrappledEnemy->CurrHealth / Player->GrappledEnemy->MaxHealth;
			int32 BarWidth = (int32)(100 * HealthRatio);
			vector2 EntityWorldCenter = Player->GrappledEnemy->Entity.Position - WorldCenter;
			vector2 BarCenter = EntityWorldCenter - vector2{0, 25};

			gl_texture Texture = {};
			Texture.Image = &GameState->EnemyHealthBar;
			Texture.Center = BarCenter;
			Texture.Scale = vector2{(real64)(BarWidth / 2), 30.0f};
			Texture.RadiansAngle = 0;
			PushRenderTexture(GameState, &Texture);
		}
	}

	if (Player->Entity.OnCollide)
	{
		DebugLine("Losing Health", GameState);
		Player->CurrHealth--;
		if (Player->CurrHealth == 0)
		{
			DebugLine("DEAD", GameState);
		}

		Player->RedHitFlash.A = 1.0f;
	}

	Player->RedHitFlash.A = Player->RedHitFlash.A * 0.93f;
	PushRenderSquare(GameState, MakeSquare(vector2{(real64)(WindowInfo->Width / 2),
	                                       (real64)(WindowInfo->Height / 2)}, 200, Player->RedHitFlash));

	for (int EntityIndex = 0;
	     EntityIndex < GameState->WorldEntityCount;
	     EntityIndex++)
	{
		active_entity *EntityAbout = GameState->WorldEntities[EntityIndex];
		if (EntityAbout->Alive)
		{
			if (EntityAbout->IsColliding)
			{
				vector2 CollideDirection = EntityAbout->CollideDirection;
				real64 WidthSum = (EntityAbout->ColliderWidth / 2) + (EntityAbout->CollidingWith->ColliderWidth / 2) + 0.2f;

				if (CollideDirection.X > 0  &&
				    (EntityAbout->Position.X > (EntityAbout->CollidingWith->Position.X + WidthSum)) ||
				    (EntityAbout->Position.Y > (EntityAbout->CollidingWith->Position.Y + WidthSum)) ||
				    (EntityAbout->Position.Y < (EntityAbout->CollidingWith->Position.Y - WidthSum)))
				{
					EntityAbout->CollidingWith->IsColliding = false;
					EntityAbout->IsColliding = false;
				}
				if (CollideDirection.X < 0  &&
				    (EntityAbout->Position.X < (EntityAbout->CollidingWith->Position.X - WidthSum)) ||
				    (EntityAbout->Position.Y > (EntityAbout->CollidingWith->Position.Y + WidthSum)) ||
				    (EntityAbout->Position.Y < (EntityAbout->CollidingWith->Position.Y - WidthSum)))
				{
					EntityAbout->CollidingWith->IsColliding = false;
					EntityAbout->IsColliding = false;
				}
				// NOTE I don't need to check the positive and negative y directions. I'm not quite sure why not.
			}

			EntityAbout->OnCollide = false;

			if (!EntityAbout->Image)
			{
				vector2 EntityWorldCenter = EntityAbout->Position - WorldCenter;
				PushRenderSquare(GameState, MakeSquare(EntityWorldCenter, EntityAbout->ImageWidth, EntityAbout->Color));
			}
			else if (EntityAbout != &Player->Entity)
			{
				gl_texture Texture = {};
				Texture.Image = EntityAbout->Image;
				Texture.Center = EntityAbout->Position - WorldCenter;
				Texture.Scale = vector2{(real64)(EntityAbout->ImageWidth / 2), (real64)(EntityAbout->ImageWidth / 2)};
				Texture.RadiansAngle = FacingDirectionToRotationAngle(EntityAbout->FacingDirection);
				PushRenderTexture(GameState, &Texture);
			}
		}
	}

	for (int EntityIndex = 0;
	     EntityIndex < GameState->WorldEntityCount;
	     EntityIndex++)
	{
		active_entity *EntityAbout = GameState->WorldEntities[EntityIndex];
		if (EntityAbout->Alive)
		{
			vector2 Acceleration = (((EntityAbout->ForceOn * GameState->TimeRate) / EntityAbout->Weight) + (-0.25f * EntityAbout->Velocity));
			// NOTE these 0.9f here should actually be the previous elapsed frame time. Maybe do that at some point
			vector2 NewTestPos = (0.5f * Acceleration * SquareInt((int64)(0.9f))) + (EntityAbout->Velocity * 0.9f) + EntityAbout->Position;

			bool32 CollisionDetected = false;
			active_entity *EntityHit = {};

			for (int EntityCheckingCollision = 0;
			     EntityCheckingCollision < GameState->WorldEntityCount;
			     EntityCheckingCollision++)
			{
				if (GameState->WorldEntities[EntityCheckingCollision] != EntityAbout)
				{
					real64 WidthAdding = EntityAbout->ColliderWidth;
					vector2 EntityTopLeft =
					{
						GameState->WorldEntities[EntityCheckingCollision]->Position.X - ((GameState->WorldEntities[EntityCheckingCollision]->ColliderWidth + WidthAdding) / 2),
						GameState->WorldEntities[EntityCheckingCollision]->Position.Y - ((GameState->WorldEntities[EntityCheckingCollision]->ColliderWidth + WidthAdding) / 2)
					};
					vector2 EntityBottomRight =
					{
						GameState->WorldEntities[EntityCheckingCollision]->Position.X + ((GameState->WorldEntities[EntityCheckingCollision]->ColliderWidth + WidthAdding) / 2),
						GameState->WorldEntities[EntityCheckingCollision]->Position.Y + ((GameState->WorldEntities[EntityCheckingCollision]->ColliderWidth + WidthAdding) / 2)
					};

					if (NewTestPos.X > EntityTopLeft.X &&
					    NewTestPos.X < EntityBottomRight.X &&
					    NewTestPos.Y > EntityTopLeft.Y &&
					    NewTestPos.Y < EntityBottomRight.Y)
					{
						CollisionDetected = true;
						EntityHit = GameState->WorldEntities[EntityIndex];
					}
				}
			}

			if (!CollisionDetected)
			{
				EntityAbout->Position = NewTestPos;
				EntityAbout->Velocity = (Acceleration * 0.9f) + EntityAbout->Velocity;
			}
			else
			{
				if (!EntityAbout->IsColliding)
				{
					EntityAbout->OnCollide = true;
				}
				if (!EntityHit->IsColliding)
				{
					EntityHit->OnCollide = true;
				}

				EntityHit->IsColliding = true;
				EntityHit->CollidingWith = EntityAbout;
				EntityAbout->IsColliding = true;
				EntityAbout->CollidingWith = EntityHit;

				EntityHit->ForceOn = EntityHit->ForceOn + EntityAbout->ForceOn;

				real64 WidthSum = (EntityHit->ColliderWidth / 2) + (EntityAbout->ColliderWidth / 2);

				vector2 NewPos = NewTestPos;
				vector2 NewVelocity = (Acceleration * 0.9f) + EntityAbout->Velocity;

				if (EntityAbout->Position.X > (EntityHit->Position.X + WidthSum))
				{
					NewVelocity.X = 0;
					NewPos.X = EntityHit->Position.X + WidthSum + 0.1f;
					EntityAbout->CollideDirection = vector2{1, 0};
				}
				if (EntityAbout->Position.X < (EntityHit->Position.X - WidthSum))
				{
					NewVelocity.X = 0;
					NewPos.X = EntityHit->Position.X - WidthSum - 0.1f;
					EntityAbout->CollideDirection = vector2{ -1, 0};
				}
				if (EntityAbout->Position.Y > (EntityHit->Position.Y + WidthSum))
				{
					NewVelocity.Y = 0;
					NewPos.Y = EntityHit->Position.Y + WidthSum + 0.1f;
					EntityAbout->CollideDirection = vector2{0, 1};
				}
				if (EntityAbout->Position.Y < (EntityHit->Position.Y - WidthSum))
				{
					NewVelocity.Y = 0;
					NewPos.Y = EntityHit->Position.Y - WidthSum - 0.1f;
					EntityAbout->CollideDirection = vector2{0, -1};
				}

				EntityAbout->Position = NewPos;
				EntityAbout->Velocity = NewVelocity;
			}

			EntityAbout->ForceOn = VECTOR2_ZERO;
		}
	}

	if (GameInput->R1.IsDown && Player->CanGrapple)
	{
		gl_texture Texture = {};
		Texture.Image = &GameState->GrappleArrowImage;
		Texture.Center = Player->Entity.Position - WorldCenter;
		Texture.Scale = vector2{100, 100};
		Texture.RadiansAngle = FacingDirectionToRotationAngle(Player->Entity.FacingDirection);
		PushRenderTexture(GameState, &Texture);
	}

	// gl_texture RadiusTexture = {};
	// RadiusTexture.Image = &GameState->GrappleRadiusImage;
	// RadiusTexture.Center = Player->Entity.Position - WorldCenter;
	// RadiusTexture.Scale = vector2{100, 100};
	// RadiusTexture.RadiansAngle = 0;
	// PushRenderTexture(GameState, &RadiusTexture);

	gl_texture PlayerTexture = {};
	PlayerTexture.Image = Player->Entity.Image;
	PlayerTexture.Center = Player->Entity.Position - WorldCenter;
	PlayerTexture.Scale = vector2{(real64)(Player->Entity.ImageWidth / 2), (real64)(Player->Entity.ImageWidth / 2)};
	PlayerTexture.RadiansAngle = FacingDirectionToRotationAngle(Player->Entity.FacingDirection);
	PushRenderTexture(GameState, &PlayerTexture);

	// Visualize the players collision box
	// PushRenderSquare(GameState, MakeSquare(Player->Entity.Position - WorldCenter, Player->Entity.ColliderWidth, COLOR_RED));

	GameState->Player.CurrHealth = ClampValue(0, 1000, GameState->Player.CurrHealth);
	for (int HealthIndex = 1;
	     HealthIndex < GameState->Player.CurrHealth + 1;
	     HealthIndex++)
	{
		uint8 Width = 30;
		vector2 Center = vector2{(real64)(HealthIndex * (25 + (Width / 2))), (real64)(25 + (Width / 2))};
		PushRenderSquare(GameState, MakeSquare(Center, Width, COLOR_RED));
	}
}

extern "C" GAME_LOAD_ASSETS(GameLoadAssets)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	LoadAssets(GameState);
}