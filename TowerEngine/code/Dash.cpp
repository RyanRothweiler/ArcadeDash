#include "Dash.h"

global_variable platform_read_file *PlatformReadFile;
global_variable platform_save_state *PlatformSaveState;
global_variable platform_load_state *PlatformLoadState;

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


#include "Font.cpp"

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
AddWorldEntity(game_state *GameState, active_entity *Entity)
{
	GameState->WorldEntities[GameState->WorldEntityCount] = Entity;
	GameState->WorldEntityCount++;
}

void
PlayerLose()
{
	PlatformLoadState("GameBeginningState.ts");
}

active_entity *
GetNewSingleEntity(game_state *GameState)
{
	Assert(_countof(GameState->EntityBucket) > GameState->EntityBucketCount);
	AddWorldEntity(GameState, &GameState->EntityBucket[GameState->EntityBucketCount]);
	active_entity *Result = &GameState->EntityBucket[GameState->EntityBucketCount];
	GameState->EntityBucketCount++;
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

extern "C" GAME_LOOP(GameLoop)
{
	PlatformReadFile = Memory->PlatformReadFile;
	PlatformSaveState = Memory->PlatformSaveState;
	PlatformLoadState = Memory->PlatformLoadState;

	bool32 UseFourDirections = true;

	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	Assert(GameState);

	if (!Memory->IsInitialized)
	{
		GameState->PrintFPS = true;

		GameState->WorldCenter = vector2{0, 0};
		GameState->CamCenter = vector2{(real64)(WindowInfo->Width / 2), (real64)(WindowInfo->Height / 2)};

		GameState->TestImage = GLLoadBMP("../assets/Background.bmp");

		GameState->Player.SpeedCoeficient = 1.0f;
		GameState->Player.BaseSpeed = 1.0f;
		GameState->Player.BaseColor = COLOR_BLUE;
		GameState->Player.DashColor = COLOR_RED;
		GameState->Player.DashFrameLength = 4;
		GameState->Player.IsDashing = false;
		GameState->Player.Entity.Position.X = WindowInfo->Width / 2;
		GameState->Player.Entity.Position.Y = WindowInfo->Height / 2;
		GameState->Player.Entity.ColliderWidth = 15;
		GameState->Player.Entity.MovementSpeed = GameState->Player.BaseSpeed;
		GameState->Player.Entity.Color = GameState->Player.BaseColor;
		GameState->Player.Entity.Alive = true;
		AddWorldEntity(GameState, &GameState->Player.Entity);

		// Entity Types
		// 1 - Wall
		// 2 - Enemy

		active_entity *Entity;
		uint16 cellSize = 150;
		const uint16 GridWidth = 15;
		const uint16 GridHeight = 10;
		uint16 levelGrid[GridHeight][GridWidth] =
		{
			{0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
			{0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 2, 0, 1},
			{0, 0, 1, 1, 0, 0, 2, 1, 0, 0, 1, 0, 2, 2, 1},
			{0, 0, 0, 1, 1, 0, 0, 1, 0, 2, 1, 0, 2, 0, 1},
			{0, 2, 0, 2, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1},
			{1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 2, 1, 0, 1},
			{1, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 1, 0, 2, 1},
			{1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		};
		for (uint16 x = 0; x < GridWidth; x++)
		{
			for (uint16 y = 0; y < GridHeight; y++)
			{
				if (levelGrid[y][x] == 1)
				{
					Entity = GetNewSingleEntity(GameState);
					Entity->Color = COLOR_BLACK;
					Entity->Position = vector2{(real64)(x * cellSize), (real64)(y * cellSize)};
					Entity->ColliderWidth = cellSize + 1;
					Entity->Alive = true;
					Entity->Type = ENTITY_TYPE_WALL;
				}

				if (levelGrid[y][x] == 2)
				{
					Entity = GetNewSingleEntity(GameState);
					Entity->Color = COLOR_GREEN;
					Entity->Position = vector2{(real64)(x * cellSize), (real64)(y * cellSize)};
					Entity->ColliderWidth = (uint16)RandomRangeInt(10, 50, GameState);
					Entity->Alive = true;
					Entity->Type = ENTITY_TYPE_ENEMY;
				}
			}
		}

		GameState->AlphabetBitmapsCount = 0;
		MakeAlphabetBitmaps(GameState, PlatformReadFile);


		// Entity = GetNewSingleEntity(GameState);
		// Entity->Color = COLOR_GREEN;
		// Entity->Position = vector2{100, 700};
		// Entity->ColliderWidth = 30;
		// Entity->Alive = true;

		GameState->FrameCounter = 0;

		Memory->IsInitialized = true;
		PlatformSaveState("GameBeginningState.ts");

	}

	GameState->FrameCounter++;

	player *Player = &GameState->Player;

	GameState->RenderSquaresCount = 0;
	GameState->RenderTexturesCount = 0;
	GameState->RenderLinesCount = 0;


	GameState->TimeRate = 1.0f;

	if (UseFourDirections)
	{
		if (GameInput->DUp.IsDown)
		{
			Player->MovingDirection = vector2{0, -1};
		}
		if (GameInput->DDown.IsDown)
		{
			Player->MovingDirection = vector2{0, 1};
		}
		if (GameInput->DRight.IsDown)
		{
			Player->MovingDirection = vector2{1, 0};
		}
		if (GameInput->DLeft.IsDown)
		{
			Player->MovingDirection = vector2{ -1, 0};
		}
		if (GameInput->YButton.IsDown)
		{
			Player->MovingDirection = VECTOR2_ZERO;
		}
	}
	else
	{

	}

	Player->Entity.MovementSpeed = Player->BaseSpeed + Player->SpeedCoeficient;
	if (GameInput->AButton.IsDown)
	{
		Player->IsDashing = true;
		Player->Entity.MovementSpeed += 5.0f;
		Player->DashStartFrame = GameState->FrameCounter;
	}
	if (Player->IsDashing)
	{
		Player->Entity.Color = Player->DashColor;
		if (Player->DashStartFrame + Player->DashFrameLength < GameState->FrameCounter)
		{
			Player->IsDashing = false;
		}
	}
	else
	{
		Player->Entity.Color = Player->BaseColor;
	}
	Player->Entity.ForceOn = Player->MovingDirection * Player->Entity.MovementSpeed;

	if (Player->Entity.IsColliding)
	{
		if (!Player->IsDashing)
		{
			PlayerLose();
		}
		else
		{
			if (Player->Entity.CollidingWith->Type == ENTITY_TYPE_WALL)
			{
				PlayerLose();
			}
			else
			{
				Player->Entity.IsColliding = false;
				Player->Entity.CollidingWith->Alive = false;
				Player->Entity.MovementSpeed -= 3.0f;
				Player->SpeedCoeficient += 0.2f;
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
		}
	}

	vector2 PlayerCamDifference = Player->Entity.Position - GameState->WorldCenter;
	GameState->WorldCenter = GameState->WorldCenter + (PlayerCamDifference * 0.08f * GameState->TimeRate);
	vector2 WorldCenter = GameState->WorldCenter - GameState->CamCenter;


	for (int EntityIndex = 0;
	     EntityIndex < GameState->WorldEntityCount;
	     EntityIndex++)
	{
		active_entity *EntityAbout = GameState->WorldEntities[EntityIndex];
		if (EntityAbout->Alive)
		{
			vector2 Acceleration = ((EntityAbout->ForceOn * GameState->TimeRate) + (-0.25f * EntityAbout->Velocity));
			// NOTE these 0.9f here should actually be the previous elapsed frame time. Maybe do that at some point
			vector2 NewTestPos = (0.5f * Acceleration * SquareInt((int64)(0.9f))) + (EntityAbout->Velocity * 0.9f) + EntityAbout->Position;

			bool32 CollisionDetected = false;
			active_entity *EntityHit = {};

			for (int EntityCheckingCollision = 0;
			     EntityCheckingCollision < GameState->WorldEntityCount;
			     EntityCheckingCollision++)
			{
				if (GameState->WorldEntities[EntityCheckingCollision] != EntityAbout &&
				    GameState->WorldEntities[EntityCheckingCollision]->Alive)
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
						EntityHit = GameState->WorldEntities[EntityCheckingCollision];
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

				// EntityAbout->Position = NewPos;
				// EntityAbout->Velocity = NewVelocity;
			}

			EntityAbout->ForceOn = VECTOR2_ZERO;

			PushRenderSquare(GameState, MakeSquare(EntityAbout->Position - WorldCenter, EntityAbout->ColliderWidth, EntityAbout->Color));
		}
	}
	FontRenderWord("W", vector2{500, 500}, 1.0f, GameState);

}


// NOTE  Should get rid of it at some point
extern "C" GAME_LOAD_ASSETS(GameLoadAssets)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
}