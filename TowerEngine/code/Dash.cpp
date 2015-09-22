#include "Dash.h"

static platform_read_file *PlatformReadFile;

void
LoadAssets(game_state *GameState)
{

}

extern "C" GAME_LOOP(GameLoop)
{
	PlatformReadFile = Memory->PlatformReadFile;

	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	Assert(GameState);
	if (!Memory->IsInitialized)
	{

	}

	GameState->RenderSquaresCount = 0;
	GameState->RenderTexturesCount = 0;
	GameState->RenderLinesCount = 0;
}


// NOTE we shouldn't need this. Should get rid of it at some point
extern "C" GAME_LOAD_ASSETS(GameLoadAssets)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	LoadAssets(GameState);
}