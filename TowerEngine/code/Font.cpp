#ifndef FONT_CPP
#define FONT_CPP


#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

void
PushLetter(char Letter, game_state *GameState)
{
	GameState->FontUtility.Letters[GameState->FontUtility.LetterConverterCount] = Letter;
	GameState->FontUtility.LetterConverterCount++;
}

void
InitializeAlphabetVars(game_state *GameState)
{
	GameState->FontUtility.LetterConverterCount = 0;
	PushLetter('A', GameState);
	PushLetter('B', GameState);
	PushLetter('C', GameState);
	PushLetter('D', GameState);
	PushLetter('E', GameState);
	PushLetter('F', GameState);
	PushLetter('G', GameState);
	PushLetter('H', GameState);
	PushLetter('I', GameState);
	PushLetter('J', GameState);
	PushLetter('K', GameState);
	PushLetter('L', GameState);
	PushLetter('M', GameState);
	PushLetter('N', GameState);
	PushLetter('O', GameState);
	PushLetter('P', GameState);
	PushLetter('Q', GameState);
	PushLetter('R', GameState);
	PushLetter('S', GameState);
	PushLetter('T', GameState);
	PushLetter('U', GameState);
	PushLetter('V', GameState);
	PushLetter('W', GameState);
	PushLetter('X', GameState);
	PushLetter('Y', GameState);
	PushLetter('Z', GameState);
}

void
MakeBitmapCodepoint(char Letter, platform_read_file *ReadFile, game_state *GameState)
{
	//TODO add away to get fonts. Try to load one but default to ariel if can't find a font we want.
	read_file_result FontFile = PlatformReadFile("C:/Windows/Fonts/arial.ttf");
	Assert(FontFile.Contents > 0);

	stbtt_fontinfo FontInfo;
	int Width, Height, XOffset, YOffset;
	stbtt_InitFont(&FontInfo, (uint8 *)FontFile.Contents, stbtt_GetFontOffsetForIndex((uint8 *)FontFile.Contents, 0));
	uint8 *MonoBitmap = stbtt_GetCodepointBitmap(&FontInfo, 0, stbtt_ScaleForPixelHeight(&FontInfo, 128.0f),
	                    Letter, &Width, &Height, &XOffset, &YOffset);

	font_codepoint *NextCodepoint = &GameState->AlphabetBitmaps[GameState->AlphabetBitmapsCount];
	GameState->AlphabetBitmapsCount++;

	NextCodepoint->Bitmap.Width = Width;
	NextCodepoint->Bitmap.Height = Height;
	NextCodepoint->Bitmap.GLTexture = (GLuint)VirtualAlloc(0, sizeof(*MonoBitmap), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	SIZE_T BitmapSize = sizeof(uint32) * Width * Height;
	void *ConvertedBitmap = malloc(BitmapSize);

	uint32 Pitch = Width * sizeof(uint32);
	uint8 *Source = (uint8 *)MonoBitmap;
	uint8 *DestRow = (uint8 *)ConvertedBitmap + ((Height - 1) * Pitch);
	for (uint32 Y = 0;
	     Y < (uint32)Height;
	     ++Y)
	{
		uint32 *Dest = (uint32 *)DestRow;
		for (uint32 X = 0;
		     X < (uint32)Width;
		     ++X)
		{
			uint8 MonoAlpha = *Source++;

			uint8 Bit2 = MonoAlpha; // A
			uint8 Bit3 = MonoAlpha; // R
			uint8 Bit0 = MonoAlpha; // G
			uint8 Bit1 = MonoAlpha; // B

			*Dest++ = ((Bit0 << 24) | (Bit1 << 16) | (Bit2 << 8) | (Bit3 << 0));
			// Dest++;
		}

		DestRow -= Pitch;
	}

	//TODO pull this out into a separate GL function to keep gl code separate from game code.
	glGenTextures(1, &NextCodepoint->Bitmap.GLTexture);
	glBindTexture(GL_TEXTURE_2D, NextCodepoint->Bitmap.GLTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height,
	             0, GL_RGBA, GL_UNSIGNED_BYTE, ConvertedBitmap);

	VirtualFree(ConvertedBitmap, BitmapSize, MEM_RELEASE);
	stbtt_FreeBitmap(MonoBitmap, 0);
}

void
MakeAlphabetBitmaps(game_state *GameState, platform_read_file *ReadFileFunction)
{
	for (uint32 LetterIndex = 0;
	     LetterIndex < GameState->FontUtility.LetterConverterCount;
	     LetterIndex++)
	{
		MakeBitmapCodepoint(GameState->FontUtility.Letters[LetterIndex], ReadFileFunction, GameState);
	}
}

uint32
LetterToIndex(char Letter, game_state *GameState)
{
	for (uint32 Index = 0;
	     Index < GameState->FontUtility.LetterConverterCount;
	     Index++)
	{
		if (GameState->FontUtility.Letters[Index] == Letter)
		{
			return (Index);
		}
	}

	Assert(0);
	return (0);
}

void
FontRenderLetter(char Letter, vector2 TopLeft, game_state *GameState)
{
	gl_texture Texture = {};

	Texture.Image = &GameState->AlphabetBitmaps[LetterToIndex(Letter, GameState)].Bitmap;
	Texture.Center = TopLeft;
	Texture.Scale = vector2{(real64)GameState->AlphabetBitmaps[LetterToIndex(Letter, GameState)].Bitmap.Width,
	                        (real64)GameState->AlphabetBitmaps[LetterToIndex(Letter, GameState)].Bitmap.Height};

	PushRenderTexture(GameState, &Texture);
}

void
FontRenderWord(char *Word, vector2 TopLeft, game_state *GameState)
{
	// uint32 WordLenght = (uint32)CharArrayLength(Word);
	for (uint32 LetterIndex = 0;
	     LetterIndex < (uint32)CharArrayLength(Word);
	     LetterIndex++)
	{
		char *Letter = &Word[LetterIndex];
		FontRenderLetter(*Letter, vector2{LetterIndex * 150.0f, 0} + TopLeft, GameState);
	}
}

#endif