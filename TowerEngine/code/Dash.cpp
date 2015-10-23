#include "Dash.h"

global_variable platform_read_file *PlatformReadFile;
global_variable platform_save_state *PlatformSaveState;
global_variable platform_load_state *PlatformLoadState;


global_variable debug_settings GlobalDebugSettings;

//TODO Currently entity rendering uses transient memory.
// Would be better to allocate only once instead of creating a new render list every frame.


void
PushRenderTexture(list_head *ListHead, gl_texture *Texture, memory_arena *Memory)
{
	list_link *NewLink = CreateLink(ListHead, LINKTYPE_GLTEXTURE, Memory);
	gl_texture *TextureLinkData = (gl_texture *)NewLink->Data;
	TextureLinkData->Image = Texture->Image;
	TextureLinkData->Center = Texture->Center;
	TextureLinkData->Scale = Texture->Scale;
	TextureLinkData->Color = Texture->Color;
	TextureLinkData->RadiansAngle = Texture->RadiansAngle;
}

void
PushRenderSquare(list_head *ListHead, gl_square Square, memory_arena *Memory)
{
	list_link *NewLink = CreateLink(ListHead, LINKTYPE_GLSQUARE, Memory);
	gl_square *SquareLinkData = (gl_square *)NewLink->Data;
	SquareLinkData->TopLeft = Square.TopLeft;
	SquareLinkData->TopRight = Square.TopRight;
	SquareLinkData->BottomLeft = Square.BottomLeft;
	SquareLinkData->BottomRight = Square.BottomRight;
	SquareLinkData->Color = Square.Color;
}

void
PushRenderLine(list_head *ListHead, gl_line Line, memory_arena *Memory)
{
	list_link *NewLink = CreateLink(ListHead, LINKTYPE_GLLINE, Memory);
	gl_line *LineLinkData = (gl_line *)NewLink->Data;

	LineLinkData->Start = Line.Start;
	LineLinkData->End = Line.End;
	LineLinkData->Color = Line.Color;
	LineLinkData->Width = Line.Width;
}

void
PushRenderSquareOutline(list_head *ListHead, gl_square_outline SquareOutline, memory_arena *Memory)
{
	PushRenderLine(ListHead, SquareOutline.LeftLine, Memory);
	PushRenderLine(ListHead, SquareOutline.RightLine, Memory);
	PushRenderLine(ListHead, SquareOutline.TopLine, Memory);
	PushRenderLine(ListHead, SquareOutline.BottomLine, Memory);
}

gl_line
MakeGLLine(vector2 Start, vector2 End, color Color, real32 LineWidth)
{
	gl_line Line = {};

	Line.Start = Start;
	Line.End = End;
	Line.Width = LineWidth;
	Line.Color = Color;

	return (Line);
}

gl_square_outline
MakeSquareOutline(vector2 Pos, int32 Width, int32 Height, color Color, real32 LineWidth)
{
	gl_square_outline Outline = {};

	//NOTE this is common, if we use this a again pull it out
	int32 HalfWidth = Width / 2;
	int32 HalfHeight = Height / 2;
	vector2 TopLeft = vector2{Pos.X - HalfWidth, Pos.Y - HalfHeight};
	vector2 TopRight = vector2{Pos.X + HalfWidth, Pos.Y - HalfHeight};
	vector2 BottomLeft = vector2{Pos.X - HalfWidth, Pos.Y + HalfHeight};
	vector2 BottomRight = vector2{Pos.X + HalfWidth, Pos.Y + HalfHeight};

	Outline.LeftLine = MakeGLLine(TopLeft, BottomLeft, Color, LineWidth);
	Outline.RightLine = MakeGLLine(TopRight, BottomRight, Color, LineWidth);
	Outline.BottomLine = MakeGLLine(BottomLeft, BottomRight, Color, LineWidth);
	Outline.TopLine = MakeGLLine(TopLeft, TopRight, Color, LineWidth);

	return (Outline);
}


gl_square
MakeRectangle(vector2 Pos, int32 Width, int32 Height, color Color)
{
	gl_square Result = {};

	Result.Color = Color;

	int32 HalfWidth = Width / 2;
	int32 HalfHeight = Height / 2;
	Result.TopLeft = vector2{Pos.X - HalfWidth, Pos.Y - HalfHeight};
	Result.TopRight = vector2{Pos.X + HalfWidth, Pos.Y - HalfHeight};
	Result.BottomLeft = vector2{Pos.X - HalfWidth, Pos.Y + HalfHeight};
	Result.BottomRight = vector2{Pos.X + HalfWidth, Pos.Y + HalfHeight};

	return (Result);
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

#include "Font.cpp"

real64
RandomRangeFloat(real32 Bottom, real32 Top, uint32 *RandomGenState)
{
	Assert(Bottom < Top);
	real64 Result = 0;

	uint32 RandomMax = 1000;
	uint32 RandomInt = (10 * *RandomGenState % RandomMax);
	real64 RandomScalar = (real32)RandomInt / (real32)RandomMax;

	real64 ScaledNum = (real64)((Top - Bottom) * RandomScalar);
	Result = ScaledNum + Bottom;

	*RandomGenState += *RandomGenState;

	return (Result);
}

int64
RandomRangeInt(int32 Bottom, int32 Top, uint32 *RandomGenState)
{
	real64 Result = RandomRangeFloat((real32)Bottom, (real32)Top, RandomGenState);
	return ((int64)Result);
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

//TODO make this save the data in some given memory
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

	GlobalDebugSettings.DrawColor = COLOR_RED;
	GlobalDebugSettings.DrawColliderBoxes = true;
	GlobalDebugSettings.ShowFPS = true;


	Assert(sizeof(game_state) <= Memory->PermanentMemory.Size);
	game_state *GameState = (game_state *)Memory->PermanentMemory.Memory;
	Assert(GameState);

	Memory->TransientMemory.Head = (uint8 *)Memory->TransientMemory.Memory;

	if (!Memory->IsInitialized)
	{
		GameState->WorldCenter = vector2{0, 0};
		GameState->CamCenter = vector2{(real64)(WindowInfo->Width / 2), (real64)(WindowInfo->Height / 2)};

		GameState->TestImage = GLLoadBMP("../assets/Background.bmp");
		GameState->WallCrawler = GLLoadBMP("../assets/WallCrawler.bmp");

		GameState->RenderLayersCount = 10;

		GameState->Player.SpeedCoeficient = 1.0f;
		GameState->Player.BaseSpeed = 1.0f;
		GameState->Player.BaseColor = COLOR_BLUE;
		GameState->Player.DashColor = COLOR_RED;
		GameState->Player.DashFrameLength = 4;
		GameState->Player.IsDashing = false;
		GameState->Player.Entity.Position.X = WindowInfo->Width / 2;
		GameState->Player.Entity.Position.Y = WindowInfo->Height / 2;
		GameState->Player.Entity.Collider.Width = 15;
		GameState->Player.Entity.MovementSpeed = GameState->Player.BaseSpeed;
		GameState->Player.Entity.Color = GameState->Player.BaseColor;
		GameState->Player.Entity.Alive = true;
		AddWorldEntity(GameState, &GameState->Player.Entity);

		// Entity Types
		// 1 - Wall
		// 2 - Enemy
		// 3 - Wall Crawler

		//NOTE this is for level generation. This will definitely need to change.
		uint32 CrawlerLength = 100;
		active_entity *WallCrawlers[100] = {};
		uint32 CrawlerListIndex = 0;

		active_entity *Entity;
		uint16 cellSize = 150;
		const uint16 GridWidth = 15;
		const uint16 GridHeight = 10;
		uint16 levelGrid[GridHeight][GridWidth] =
		{
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
			{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
			{1, 3, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 2, 0, 1},
			{1, 0, 1, 1, 0, 0, 2, 1, 0, 0, 1, 0, 2, 2, 1},
			{1, 0, 0, 1, 1, 0, 0, 1, 0, 2, 1, 0, 2, 0, 1},
			{1, 2, 0, 2, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1},
			{1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 2, 1, 0, 1},
			{1, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 1, 0, 2, 1},
			{1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
			{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		};
		for (uint16 x = 0; x < GridWidth; x++)
		{
			for (uint16 y = 0; y < GridHeight; y++)
			{
				switch (levelGrid[y][x])
				{
					case 1:
					{
						Entity = GetNewSingleEntity(GameState);
						Entity->Color = COLOR_BLACK;
						Entity->Position = vector2{(real64)(x * cellSize), (real64)(y * cellSize)};
						Entity->Collider.Width = cellSize + 1;
						Entity->Alive = true;
						Entity->Type = ENTITY_TYPE_WALL;

						break;
					}

					case 2:
					{
						Entity = GetNewSingleEntity(GameState);
						Entity->Color = COLOR_GREEN;
						Entity->Position = vector2{(real64)(x * cellSize), (real64)(y * cellSize)};
						Entity->Collider.Width = (uint16)RandomRangeInt(10, 50, &GameState->RandomGenState);
						Entity->Alive = true;
						Entity->Type = ENTITY_TYPE_ENEMY;

						break;
					}

					case 3:
					{
						Entity = GetNewSingleEntity(GameState);
						Entity->Color = COLOR_GREEN;
						Entity->Position = vector2{(real64)(x * cellSize), (real64)(y * cellSize)};
						Entity->ImageOffset = vector2{20, 0};
						Entity->Collider.Width = 50;
						Entity->Alive = true;
						Entity->Type = ENTITY_TYPE_ENEMY;
						Entity->Image = &GameState->WallCrawler;
						Entity->ImageWidth = 75;

						WallCrawlers[CrawlerListIndex] = Entity;
						CrawlerListIndex++;

						break;
					}
				}
			}
		}

		for (uint32 CrawlerIndex = 0;
		     CrawlerIndex < CrawlerLength;
		     CrawlerIndex++)
		{

			active_entity *CrawlerAbout = WallCrawlers[CrawlerIndex];

			if (CrawlerAbout != NULL)
			{
				real64 SmallestDist = 1000000;
				active_entity *NearestEntity = {};

				for (uint32 EntityIndex = 0;
				     EntityIndex < GameState->WorldEntityCount;
				     EntityIndex++)
				{
					active_entity *EntityChecking = GameState->WorldEntities[EntityIndex];

					if (EntityChecking != NULL && EntityChecking != CrawlerAbout)
					{
						real64 NewDistance = Vector2Distance(CrawlerAbout->Position, EntityChecking->Position);
						if (NewDistance < SmallestDist)
						{
							SmallestDist = NewDistance;
							NearestEntity = EntityChecking;
						}
					}
				}

				//TODO find the relative direction that the crawler is to the wall, the offset it by half of each in that direction
				CrawlerAbout->Position = NearestEntity->Position;
			}
		}

		GameState->AlphabetBitmapsCount = 0;
		MakeAlphabetBitmaps(GameState, PlatformReadFile);

		GameState->FrameCounter = 0;

		Memory->IsInitialized = true;
		PlatformSaveState("GameBeginningState.ts");
	}

	GameState->FrameCounter++;

	player *Player = &GameState->Player;

	GameState->TimeRate = 1.0f;

	for (uint32 layerIndex = 0;
	     layerIndex < GameState->RenderLayersCount;
	     layerIndex++)
	{
		GameState->RenderObjects[layerIndex] = *CreateList(&Memory->TransientMemory);
	}

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

	Player->Entity.MovementSpeed = Player->BaseSpeed + Player->SpeedCoeficient;
	if (GameInput->AButton.IsDown)
	{
		Player->IsDashing = true;
		Player->Entity.MovementSpeed += 5.0f;
		// Player->DashStartFrame = GameState->FrameCounter;
	}
	else
	{
		Player->IsDashing = false;
	}
	if (Player->IsDashing)
	{
		Player->Entity.Color = Player->DashColor;
		// if (Player->DashStartFrame + Player->DashFrameLength < GameState->FrameCounter)
		// {
		// 	Player->IsDashing = false;
		// }
	}
	else
	{
		Player->Entity.Color = Player->BaseColor;
	}
	Player->Entity.ForceOn = Player->MovingDirection * Player->Entity.MovementSpeed;

	if (Player->Entity.Collider.IsColliding)
	{
		if (!Player->IsDashing)
		{
			PlayerLose();
		}
		else
		{
			if (Player->Entity.Collider.CollidingWith->Type == ENTITY_TYPE_WALL)
			{
				PlayerLose();
			}
			else
			{
				Player->Entity.Collider.IsColliding = false;
				Player->Entity.Collider.CollidingWith->Alive = false;
				Player->Entity.MovementSpeed -= 3.0f;
				Player->SpeedCoeficient += 0.2f;
			}
		}
	}

	for (uint32 EntityIndex = 0;
	     EntityIndex < GameState->WorldEntityCount;
	     EntityIndex++)
	{
		active_entity *EntityAbout = GameState->WorldEntities[EntityIndex];
		if (EntityAbout->Alive)
		{
			if (EntityAbout->Collider.IsColliding)
			{
				vector2 CollideDirection = EntityAbout->Collider.CollideDirection;
				real64 WidthSum = (EntityAbout->Collider.Width / 2) + (EntityAbout->Collider.CollidingWith->Collider.Width / 2) + 0.2f;

				if (CollideDirection.X > 0  &&
				    (EntityAbout->Position.X > (EntityAbout->Collider.CollidingWith->Position.X + WidthSum)) ||
				    (EntityAbout->Position.Y > (EntityAbout->Collider.CollidingWith->Position.Y + WidthSum)) ||
				    (EntityAbout->Position.Y < (EntityAbout->Collider.CollidingWith->Position.Y - WidthSum)))
				{
					EntityAbout->Collider.CollidingWith->Collider.IsColliding = false;
					EntityAbout->Collider.IsColliding = false;
				}
				if (CollideDirection.X < 0  &&
				    (EntityAbout->Position.X < (EntityAbout->Collider.CollidingWith->Position.X - WidthSum)) ||
				    (EntityAbout->Position.Y > (EntityAbout->Collider.CollidingWith->Position.Y + WidthSum)) ||
				    (EntityAbout->Position.Y < (EntityAbout->Collider.CollidingWith->Position.Y - WidthSum)))
				{
					EntityAbout->Collider.CollidingWith->Collider.IsColliding = false;
					EntityAbout->Collider.IsColliding = false;
				}
				// NOTE I don't need to check the positive and negative y directions. I'm not quite sure why not.
			}

			EntityAbout->Collider.OnCollide = false;
		}
	}

	vector2 PlayerCamDifference = Player->Entity.Position - GameState->WorldCenter;
	GameState->WorldCenter = GameState->WorldCenter + (PlayerCamDifference * 0.08f * GameState->TimeRate);
	vector2 WorldCenter = GameState->WorldCenter - GameState->CamCenter;

	char charFPS[MAX_PATH] = {};
	IntToCharArray(&GameState->PrevFrameFPS, charFPS);
	real32 FontSize = 0.1f;
	if (GameState->PrevFrameFPS < 59)
	{
		FontRenderWord(charFPS, vector2{15, 15}, FontSize, color{1.0f, 0.0f, 0.0f, 0.5f}, GameState, &GameState->RenderObjects[0], &Memory->TransientMemory);
	}
	else
	{
		FontRenderWord(charFPS, vector2{15, 15}, FontSize, color{0.0f, 1.0f, 0.0f, 0.5f}, GameState, &GameState->RenderObjects[0], &Memory->TransientMemory);
	}

	for (uint32 EntityIndex = 0;
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

			for (uint32 EntityCheckingCollision = 0;
			     EntityCheckingCollision < GameState->WorldEntityCount;
			     EntityCheckingCollision++)
			{
				if (GameState->WorldEntities[EntityCheckingCollision] != EntityAbout &&
				    GameState->WorldEntities[EntityCheckingCollision]->Alive)
				{
					real64 WidthAdding = EntityAbout->Collider.Width;
					vector2 EntityTopLeft =
					{
						GameState->WorldEntities[EntityCheckingCollision]->Position.X - ((GameState->WorldEntities[EntityCheckingCollision]->Collider.Width + WidthAdding) / 2),
						GameState->WorldEntities[EntityCheckingCollision]->Position.Y - ((GameState->WorldEntities[EntityCheckingCollision]->Collider.Width + WidthAdding) / 2)
					};
					vector2 EntityBottomRight =
					{
						GameState->WorldEntities[EntityCheckingCollision]->Position.X + ((GameState->WorldEntities[EntityCheckingCollision]->Collider.Width + WidthAdding) / 2),
						GameState->WorldEntities[EntityCheckingCollision]->Position.Y + ((GameState->WorldEntities[EntityCheckingCollision]->Collider.Width + WidthAdding) / 2)
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
				if (!EntityAbout->Collider.IsColliding)
				{
					EntityAbout->Collider.OnCollide = true;
				}
				if (!EntityHit->Collider.IsColliding)
				{
					EntityHit->Collider.OnCollide = true;
				}

				EntityHit->Collider.IsColliding = true;
				EntityHit->Collider.CollidingWith = EntityAbout;
				EntityAbout->Collider.IsColliding = true;
				EntityAbout->Collider.CollidingWith = EntityHit;

				EntityHit->ForceOn = EntityHit->ForceOn + EntityAbout->ForceOn;

				real64 WidthSum = (EntityHit->Collider.Width / 2) + (EntityAbout->Collider.Width / 2);

				vector2 NewPos = NewTestPos;
				vector2 NewVelocity = (Acceleration * 0.9f) + EntityAbout->Velocity;

				if (EntityAbout->Position.X > (EntityHit->Position.X + WidthSum))
				{
					NewVelocity.X = 0;
					NewPos.X = EntityHit->Position.X + WidthSum + 0.1f;
					EntityAbout->Collider.CollideDirection = vector2{1, 0};
				}
				if (EntityAbout->Position.X < (EntityHit->Position.X - WidthSum))
				{
					NewVelocity.X = 0;
					NewPos.X = EntityHit->Position.X - WidthSum - 0.1f;
					EntityAbout->Collider.CollideDirection = vector2{ -1, 0};
				}
				if (EntityAbout->Position.Y > (EntityHit->Position.Y + WidthSum))
				{
					NewVelocity.Y = 0;
					NewPos.Y = EntityHit->Position.Y + WidthSum + 0.1f;
					EntityAbout->Collider.CollideDirection = vector2{0, 1};
				}
				if (EntityAbout->Position.Y < (EntityHit->Position.Y - WidthSum))
				{
					NewVelocity.Y = 0;
					NewPos.Y = EntityHit->Position.Y - WidthSum - 0.1f;
					EntityAbout->Collider.CollideDirection = vector2{0, -1};
				}

				// EntityAbout->Position = NewPos;
				// EntityAbout->Velocity = NewVelocity;
			}

			EntityAbout->ForceOn = VECTOR2_ZERO;

			if (EntityAbout->Image)
			{
				gl_texture SpriteTexture = {};
				SpriteTexture.Image = EntityAbout->Image;
				SpriteTexture.Center = (EntityAbout->Position + EntityAbout->ImageOffset) - WorldCenter;
				SpriteTexture.Color = COLOR_WHITE;
				SpriteTexture.Scale = vector2{(real64)(EntityAbout->ImageWidth / 2), (real64)(EntityAbout->ImageWidth / 2)};

				PushRenderTexture(&GameState->RenderObjects[5], &SpriteTexture, &Memory->TransientMemory);

				if (GlobalDebugSettings.DrawColliderBoxes)
				{
					PushRenderSquareOutline(&GameState->RenderObjects[0],
					                        MakeSquareOutline(EntityAbout->Position - WorldCenter, EntityAbout->Collider.Width,
					                                EntityAbout->Collider.Width, GlobalDebugSettings.DrawColor, 10),
					                        &Memory->TransientMemory);
				}

			}
			else
			{
				PushRenderSquare(&GameState->RenderObjects[9],
				                 MakeSquare(EntityAbout->Position - WorldCenter, EntityAbout->Collider.Width, EntityAbout->Color),
				                 &Memory->TransientMemory);

				if (GlobalDebugSettings.DrawColliderBoxes)
				{
					PushRenderSquareOutline(&GameState->RenderObjects[0],
					                        MakeSquareOutline(EntityAbout->Position - WorldCenter, EntityAbout->Collider.Width,
					                                EntityAbout->Collider.Width, GlobalDebugSettings.DrawColor, 10),
					                        &Memory->TransientMemory);
				}
			}
		}
	}
}


// NOTE  Should get rid of it at some point
extern "C" GAME_LOAD_ASSETS(GameLoadAssets)
{
	game_state *GameState = (game_state *)Memory->PermanentMemory.Memory;
}