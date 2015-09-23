#include "Dash.h"

static platform_read_file *PlatformReadFile;

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

extern "C" GAME_LOOP(GameLoop)
{
	PlatformReadFile = Memory->PlatformReadFile;

	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	Assert(GameState);
	if (!Memory->IsInitialized)
	{
		GameState->WorldCenter = vector2{0, 0};
		GameState->CamCenter = vector2{(real64)(WindowInfo->Width / 2), (real64)(WindowInfo->Height / 2)};

		GameState->PlayerPos.X = WindowInfo->Width / 2;
		GameState->PlayerPos.Y = WindowInfo->Height / 2;

		GameState->BoxPos.X = (WindowInfo->Width / 2) - 50;
		GameState->BoxPos.Y = (WindowInfo->Height / 2) - 100;

		Memory->IsInitialized = true;
	}

	GameState->RenderSquaresCount = 0;
	GameState->RenderTexturesCount = 0;
	GameState->RenderLinesCount = 0;

	real64 PlayerSpeed = 10;
	if (GameInput->DUp.IsDown)
	{
		GameState->PlayerMovingDirection = vector2{0, -1};
	}
	if (GameInput->DDown.IsDown)
	{
		GameState->PlayerMovingDirection = vector2{0, 1};
	}
	if (GameInput->DRight.IsDown)
	{
		GameState->PlayerMovingDirection = vector2{1, 0};
	}
	if (GameInput->DLeft.IsDown)
	{
		GameState->PlayerMovingDirection = vector2{ -1, 0};
	}

	if (GameInput->AButton.IsDown)
	{
		GameState->PlayerMovingDirection = VECTOR2_ZERO;
	}

	GameState->PlayerPos = GameState->PlayerPos + (GameState->PlayerMovingDirection * PlayerSpeed);

	PushRenderSquare(GameState, MakeSquare(GameState->PlayerPos, 10, COLOR_WHITE));
	PushRenderSquare(GameState, MakeSquare(GameState->BoxPos, 50, COLOR_BLACK));
}


// NOTE we shouldn't need this. Should get rid of it at some point
extern "C" GAME_LOAD_ASSETS(GameLoadAssets)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
}