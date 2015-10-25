#include "Dash.h"

global_variable platform_read_file *PlatformReadFile;
global_variable platform_save_state *PlatformSaveState;
global_variable platform_load_state *PlatformLoadState;

global_variable debug_settings GlobalDebugSettings;



//TODO remove this global
enum level_data
{
	LevelDataEmpty,
	LevelDataWall,
	LevelDataEnemy,
	LevelDataWallCrawler
};
// Entity Types
// 0 - Empty
// 1 - Wall
// 2 - Enemy
// 3 - Wall Crawler
global_variable uint16 GlobalLevelData[10][15] =
{
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 3, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 2, 0, 1},
	{1, 0, 0, 0, 0, 0, 2, 1, 0, 0, 1, 0, 2, 2, 1},
	{1, 0, 0, 1, 1, 0, 0, 1, 0, 2, 1, 0, 2, 0, 1},
	{1, 2, 0, 2, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1},
	{1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 2, 1, 0, 1},
	{1, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 1, 0, 2, 1},
	{1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

//TODO Currently entity rendering uses transient memory.
// Would be better to allocate only once instead of creating a new render list every frame.


inline vector2
GridToWorldPos(vector2 GridPos, real64 CellSize)
{
	return (vector2{GridPos.X * CellSize, GridPos.Y * CellSize});
}

void
PushRenderTexture(list_head *ListHead, gl_texture *Texture, memory_arena *Memory)
{
	list_link *NewLink = CreateLink(ListHead, LinkTypeGLTexture, Memory);
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
	list_link *NewLink = CreateLink(ListHead, LinkTypeGLSquare, Memory);
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
	list_link *NewLink = CreateLink(ListHead, LinkTypeGLLine, Memory);
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
PlayerLose()
{
	PlatformLoadState("GameBeginningState.ts");
}

active_entity *
CreateNewEntity(memory_arena *Arena, list_head *WorldEntities)
{
	active_entity *Entity = (active_entity *)(CreateLink(WorldEntities, LinkTypeEntity, Arena)->Data);
	Entity->Dead = false;
	return (Entity);
}

void
RotateCrawler(wall_crawler *Crawler, real64 RotationRadians)
{
	Crawler->Entity->RotationRadians = RotationRadians;
	Crawler->ForwardDirection = Vector2RotatePoint(Crawler->ForwardDirection, vector2{0, 0}, RotationRadians);
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

inline void
CheckGridPosDirection(vector2 WorldPos, vector2 *GridPos, vector2 Dir, real64 CellSize)
{
	real64 Distance = Vector2Distance(WorldPos, GridToWorldPos(*GridPos + Dir, CellSize));
	if (Distance < CellSize / 2)
	{
		*GridPos = *GridPos + Dir;
	}
}

void
UpdateGridPos(wall_crawler *WallCrawler, real64 CellSize)
{
	CheckGridPosDirection(WallCrawler->Entity->Position, &WallCrawler->GridPos, vector2{ 0, 1}, CellSize);
	CheckGridPosDirection(WallCrawler->Entity->Position, &WallCrawler->GridPos, vector2{ 0, -1}, CellSize);
	CheckGridPosDirection(WallCrawler->Entity->Position, &WallCrawler->GridPos, vector2{ -1, 0}, CellSize);
	CheckGridPosDirection(WallCrawler->Entity->Position, &WallCrawler->GridPos, vector2{ 1, 0}, CellSize);
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
		Memory->TransientMemory.EndOfMemory = (uint8 *)Memory->TransientMemory.Memory + Memory->TransientMemory.Size;

		Memory->PermanentMemory.EndOfMemory = (uint8 *)Memory->PermanentMemory.Memory + Memory->PermanentMemory.Size;
		Memory->PermanentMemory.Head = (uint8 *)Memory->PermanentMemory.Memory + sizeof(game_state);

		GameState->WorldCenter = vector2{0, 0};
		GameState->CamCenter = vector2{(real64)(WindowInfo->Width / 2), (real64)(WindowInfo->Height / 2)};

		GameState->TestImage = GLLoadBMP("../assets/Background.bmp");
		GameState->WallCrawlerImage = GLLoadBMP("../assets/WallCrawler.bmp");

		GameState->RenderLayersCount = 10;

		GameState->WorldEntities = CreateList(&Memory->PermanentMemory);
		GameState->WallCrawlers = CreateList(&Memory->PermanentMemory);

		GameState->Player.SpeedCoeficient = 1.0f;
		GameState->Player.BaseSpeed = 1.5f;
		GameState->Player.BaseColor = COLOR_BLUE;
		GameState->Player.DashColor = COLOR_RED;
		GameState->Player.DashFrameLength = 4;
		GameState->Player.IsDashing = false;
		GameState->Player.Entity = CreateNewEntity(&Memory->PermanentMemory, GameState->WorldEntities);
		GameState->Player.Entity->Position.X = WindowInfo->Width / 2;
		GameState->Player.Entity->Position.Y = WindowInfo->Height / 2;
		GameState->Player.Entity->Collider.Width = 15;
		GameState->Player.Entity->MovementSpeed = GameState->Player.BaseSpeed;
		GameState->Player.Entity->Color = GameState->Player.BaseColor;
		GameState->Player.Entity->Type = EntityTypePlayer;

		GameState->LevelGrid.CellSize = 150;
		GameState->LevelGrid.Width = 15;
		GameState->LevelGrid.Height = 10;

		uint16 CellSize = GameState->LevelGrid.CellSize;
		for (uint16 x = 0; x < GameState->LevelGrid.Width; x++)
		{
			for (uint16 y = 0; y < GameState->LevelGrid.Height; y++)
			{
				switch (GlobalLevelData[y][x])
				{
					case LevelDataWall:
					{
						active_entity *Entity = CreateNewEntity(&Memory->PermanentMemory, GameState->WorldEntities);
						Entity->Color = COLOR_BLACK;
						Entity->Position = vector2{(real64)(x * CellSize), (real64)(y * CellSize)};
						Entity->Collider.Width = CellSize + 1;
						Entity->Type = EntityTypeWall;

						break;
					}

					case LevelDataEnemy:
					{
						active_entity *Entity = CreateNewEntity(&Memory->PermanentMemory, GameState->WorldEntities);
						Entity->Color = COLOR_GREEN;
						Entity->Position = vector2{(real64)(x * CellSize), (real64)(y * CellSize)};
						Entity->Collider.Width = (uint16)RandomRangeInt(10, 50, &GameState->RandomGenState);
						Entity->Type = EntityTypeEnemy;

						break;
					}

					case LevelDataWallCrawler:
					{

						wall_crawler *WallCrawler = (wall_crawler *)CreateLink(GameState->WallCrawlers, LinkTypeWallCrawler,
						                            &Memory->PermanentMemory)->Data;
						active_entity *Entity = CreateNewEntity(&Memory->PermanentMemory, GameState->WorldEntities);
						WallCrawler->Entity = Entity;

						WallCrawler->ForwardDirection = vector2{1, 0};
						WallCrawler->ImageOffset = vector2{10, 10};
						WallCrawler->GridPos = vector2{(real64)x, (real64)y};
						WallCrawler->DirMoving = vector2{0, 1};

						Entity->Color = COLOR_GREEN;
						Entity->Position = vector2{(real64)(x * CellSize), (real64)(y * CellSize)};
						Entity->Collider.Width = 40;
						Entity->Type = EntityTypeEnemy;
						Entity->Image = &GameState->WallCrawlerImage;
						Entity->ImageWidth = 60;

						break;
					}
				}
			}
		}

		for (uint32 CrawlerIndex = 1;
		     CrawlerIndex <= GameState->WallCrawlers->LinkCount;
		     CrawlerIndex++)
		{

			wall_crawler *CrawlerAbout = (wall_crawler *)GetLinkData(GameState->WallCrawlers, CrawlerIndex);

			if (CrawlerAbout != NULL)
			{
				real64 SmallestDist = 1000000;
				active_entity *NearestEntity = {};

				for (uint32 EntityIndex = 1;
				     EntityIndex < GameState->WorldEntities->LinkCount;
				     EntityIndex++)
				{
					active_entity *EntityChecking = (active_entity *)GetLinkData(GameState->WorldEntities, EntityIndex);

					if (EntityChecking != NULL &&
					    EntityChecking != CrawlerAbout->Entity &&
					    EntityChecking->Type == EntityTypeWall)
					{
						real64 NewDistance = Vector2Distance(CrawlerAbout->Entity->Position, EntityChecking->Position);
						if (NewDistance < SmallestDist)
						{
							SmallestDist = NewDistance;
							NearestEntity = EntityChecking;
						}
					}
				}

				real64 WidthSum = (CrawlerAbout->Entity->Collider.Width / 2) + (NearestEntity->Collider.Width / 2);
				vector2 CardinalRelDir = Vector2GetCardinalDirection(CrawlerAbout->Entity->Position, NearestEntity->Position, (uint32)WidthSum);
				if (CardinalRelDir.X > 0)
				{
					CrawlerAbout->Entity->Position = NearestEntity->Position + vector2{WidthSum, 0};
				}
				if (CardinalRelDir.X < 0)
				{
					CrawlerAbout->Entity->Position = NearestEntity->Position + vector2{ -WidthSum, 0};
					RotateCrawler(CrawlerAbout, PI);
				}
				if (CardinalRelDir.Y > 0)
				{
					CrawlerAbout->Entity->Position = NearestEntity->Position + vector2{0, WidthSum};
					RotateCrawler(CrawlerAbout, -PI / 2);
				}
				if (CardinalRelDir.Y < 0)
				{
					CrawlerAbout->Entity->Position = NearestEntity->Position + vector2{0, -WidthSum};
					RotateCrawler(CrawlerAbout, PI / 2);
				}
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

	Player->Entity->MovementSpeed = Player->BaseSpeed + Player->SpeedCoeficient;
	if (GameInput->AButton.IsDown)
	{
		Player->IsDashing = true;
		Player->Entity->MovementSpeed += 5.0f;

		//NOTE this is for timed dashing
		#if 0
		Player->DashStartFrame = GameState->FrameCounter;
		#endif
	}
	else
	{
		Player->IsDashing = false;
	}
	if (Player->IsDashing)
	{
		Player->Entity->Color = Player->DashColor;

		//NOTE this is for timed dashing
		#if 0
		if (Player->DashStartFrame + Player->DashFrameLength < GameState->FrameCounter)
		{
			Player->IsDashing = false;
		}
		#endif
	}
	else
	{
		Player->Entity->Color = Player->BaseColor;
	}
	Player->Entity->ForceOn = Player->MovingDirection * Player->Entity->MovementSpeed;

	if (Player->Entity->Collider.IsColliding)
	{
		if (!Player->IsDashing)
		{
			PlayerLose();
		}
		else
		{
			if (Player->Entity->Collider.CollidingWith->Type == EntityTypeWall)
			{
				PlayerLose();
			}
			else
			{
				Player->Entity->Collider.IsColliding = false;

				Player->Entity->Collider.CollidingWith->Dead = true;
				RemoveLink(GameState->WorldEntities, Player->Entity->Collider.CollidingWith);

				Player->Entity->MovementSpeed -= 3.0f;
				Player->SpeedCoeficient += 0.2f;
			}
		}
	}

	for (uint32 EntityIndex = 1;
	     EntityIndex <= GameState->WorldEntities->LinkCount;
	     EntityIndex++)
	{
		active_entity *EntityAbout = (active_entity *)GetLinkData(GameState->WorldEntities, EntityIndex);
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

	vector2 PlayerCamDifference = Player->Entity->Position - GameState->WorldCenter;
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


	for (uint32 CrawlerIndex = 1;
	     CrawlerIndex <= GameState->WallCrawlers->LinkCount;
	     CrawlerIndex++)
	{
		//TODO don't need this
		// instaed just check if collision happened with a wall



		// wall_crawler *Crawler = (wall_crawler *)GetLinkData(GameState->WallCrawlers, CrawlerIndex);
		// UpdateGridPos(Crawler, GameState->LevelGrid.CellSize);

		// Crawler->Entity->Position = Crawler->Entity->Position + vector2{0, -1};

	}

	for (uint32 EntityIndex = 1;
	     EntityIndex <= GameState->WorldEntities->LinkCount;
	     EntityIndex++)
	{
		active_entity *EntityAbout = (active_entity *)GetLinkData(GameState->WorldEntities, EntityIndex);

		vector2 Acceleration = ((EntityAbout->ForceOn * GameState->TimeRate) + (-0.25f * EntityAbout->Velocity));
		// NOTE these 0.9f here should actually be the previous elapsed frame time. Maybe do that at some point
		vector2 NewTestPos = (0.5f * Acceleration * SquareInt((int64)(0.9f))) + (EntityAbout->Velocity * 0.9f) + EntityAbout->Position;

		if (EntityAbout->Type == EntityTypePlayer)
		{
			NewTestPos = EntityAbout->Position + EntityAbout->ForceOn;
		}

		bool32 CollisionDetected = false;
		active_entity *EntityHit = {};

		for (uint32 EntityCheckingCollision = 1;
		     EntityCheckingCollision <= GameState->WorldEntities->LinkCount;
		     EntityCheckingCollision++)
		{
			active_entity *EntityChecking = (active_entity *)GetLinkData(GameState->WorldEntities, EntityCheckingCollision);
			if (EntityChecking != EntityAbout)
			{
				real64 WidthAdding = EntityAbout->Collider.Width;
				vector2 EntityTopLeft =
				{
					EntityChecking->Position.X - ((EntityChecking->Collider.Width + WidthAdding) / 2),
					EntityChecking->Position.Y - ((EntityChecking->Collider.Width + WidthAdding) / 2)
				};
				vector2 EntityBottomRight =
				{
					EntityChecking->Position.X + ((EntityChecking->Collider.Width + WidthAdding) / 2),
					EntityChecking->Position.Y + ((EntityChecking->Collider.Width + WidthAdding) / 2)
				};

				if (NewTestPos.X > EntityTopLeft.X &&
				    NewTestPos.X < EntityBottomRight.X &&
				    NewTestPos.Y > EntityTopLeft.Y &&
				    NewTestPos.Y < EntityBottomRight.Y)
				{
					CollisionDetected = true;
					EntityHit = EntityChecking;
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

			uint32 WidthSum = (EntityHit->Collider.Width / 2) + (EntityAbout->Collider.Width / 2);

			vector2 NewPos = NewTestPos;
			vector2 NewVelocity = (Acceleration * 0.9f) + EntityAbout->Velocity;

			vector2 CardinalRelDir = Vector2GetCardinalDirection(EntityAbout->Position, EntityHit->Position, WidthSum);
			EntityAbout->Collider.CollideDirection = CardinalRelDir;

			//NOTE this is used for more advanced collision detection, doesn't work but the concept is there
			#if 0
			if (CardinalRelDir.X == 1)
			{
				NewVelocity.X = 0;
				NewPos.X = EntityHit->Position.X + WidthSum + 0.1f;
			}
			if (CardinalRelDir.X == -1)
			{
				NewVelocity.X = 0;
				NewPos.X = EntityHit->Position.X - WidthSum - 0.1f;
				EntityAbout->Collider.CollideDirection = vector2{ -1, 0};
			}
			if (CardinalRelDir.Y == 1)
			{
				NewVelocity.Y = 0;
				NewPos.Y = EntityHit->Position.Y + WidthSum + 0.1f;
				EntityAbout->Collider.CollideDirection = vector2{0, 1};
			}
			if (CardinalRelDir.Y == -1)
			{
				NewVelocity.Y = 0;
				NewPos.Y = EntityHit->Position.Y - WidthSum - 0.1f;
				EntityAbout->Collider.CollideDirection = vector2{0, -1};
			}

			EntityAbout->Position = NewPos;
			EntityAbout->Velocity = NewVelocity;
			#endif
		}

		EntityAbout->ForceOn = VECTOR2_ZERO;

		if (!EntityAbout->Image)
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

	//TODO find a better way to do this
	uint32 CrawlerRemovingIndex = 100000;

	for (uint32 CrawlerIndex = 1;
	     CrawlerIndex <= GameState->WallCrawlers->LinkCount;
	     CrawlerIndex++)
	{
		wall_crawler *Crawler = (wall_crawler *)GetLinkData(GameState->WallCrawlers, CrawlerIndex);

		if (!Crawler->Entity->Dead)
		{

			gl_texture SpriteTexture = {};
			SpriteTexture.Image = Crawler->Entity->Image;
			SpriteTexture.Center = (Crawler->Entity->Position + (Crawler->ImageOffset * Crawler->ForwardDirection)) - WorldCenter;
			SpriteTexture.Color = COLOR_WHITE;
			SpriteTexture.Scale = vector2{(real64)(Crawler->Entity->ImageWidth / 2), (real64)(Crawler->Entity->ImageWidth / 2)};

			SpriteTexture.RadiansAngle = Crawler->Entity->RotationRadians;

			PushRenderTexture(&GameState->RenderObjects[5], &SpriteTexture, &Memory->TransientMemory);

			if (GlobalDebugSettings.DrawColliderBoxes)
			{
				PushRenderSquareOutline(&GameState->RenderObjects[0],
				                        MakeSquareOutline(Crawler->Entity->Position - WorldCenter, Crawler->Entity->Collider.Width,
				                                Crawler->Entity->Collider.Width, GlobalDebugSettings.DrawColor, 10),
				                        &Memory->TransientMemory);

				PushRenderSquare(&GameState->RenderObjects[0],
				                 MakeSquare(GridToWorldPos(Crawler->GridPos, GameState->LevelGrid.CellSize) - WorldCenter, GameState->LevelGrid.CellSize,
				                            color{1.0f, 0.0f, 0.0f, 0.5f}),
				                 &Memory->TransientMemory);
			}
		}
		else
		{
			CrawlerRemovingIndex = CrawlerIndex;
		}
	}

	//TODO find a better way to do this
	if (CrawlerRemovingIndex != 100000)
	{
		RemoveLink(GameState->WallCrawlers, CrawlerRemovingIndex);
	}
}


// NOTE  Should get rid of it at some point
extern "C" GAME_LOAD_ASSETS(GameLoadAssets)
{
	game_state *GameState = (game_state *)Memory->PermanentMemory.Memory;
}