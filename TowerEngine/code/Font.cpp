#ifndef FONT_CPP
#define FONT_CPP


#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

//TODO there are some weird modifiers and weird things going on in here that aren't clean. Fix it at some point.
// 1 - Most TopLeft variables are not actually top left, they're bottom middle.
// 2 - I had to half the final rendering size since the bitmap rendering is janky.

//NOTE Not entirely sure what this does...
global_variable real64 GlobalFontRenderSize = 200.0f;
global_variable stbtt_fontinfo GlobalCurrentFontInfo;
//TODO create a debug build path, so this stuff doesn't compile in the final version.
global_variable bool32 ShowDebugInfo = true;

void
MakeBitmapCodepoint(char Letter, platform_read_file *ReadFile, game_state *GameState)
{
	read_file_result FontFile = PlatformReadFile("../Assets/Gotham-Medium.ttf");
	Assert(FontFile.Contents > 0);

	int Width, Height, XOffset, YOffset;
	stbtt_InitFont(&GlobalCurrentFontInfo, (uint8 *)FontFile.Contents, stbtt_GetFontOffsetForIndex((uint8 *)FontFile.Contents, 0));
	uint8 *MonoBitmap = stbtt_GetCodepointBitmap(&GlobalCurrentFontInfo, 0, stbtt_ScaleForPixelHeight(&GlobalCurrentFontInfo, (float)GlobalFontRenderSize),
	                    Letter, &Width, &Height, &XOffset, &YOffset);


	font_codepoint *NextCodepoint = &GameState->AlphabetBitmaps[GameState->AlphabetBitmapsCount];
	GameState->AlphabetBitmapsCount++;

	// int Ascent, Descent, LineGap;
	// stbtt_GetGlyphVMetrics(&GlobalCurrentFontInfo, &Ascent, &Descent, &LineGap);
	NextCodepoint->BaselineFactor = (YOffset + Height) * 2;

	//m - 122
	//n - 

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
	for (uint32 Letter = 0;
	     Letter <= 127;
	     Letter++)
	{
		MakeBitmapCodepoint((char)Letter, ReadFileFunction, GameState);
	}
}

//NOTE TopLeft here is not actually TopLeft. It's more bottom Middle.
void
FontRenderLetter(char Letter, vector2 TopLeft, real64 ScaleModifier, game_state *GameState)
{
	//NOTE there are half multipliers in here (0.5f) because I'm drawing the bitmaps a bit weird. It's really janky and I'm a little to lazy to fix it since it works.
	font_codepoint *CodepointUsing = &GameState->AlphabetBitmaps[(uint32)Letter];

	gl_texture Texture = {};
	Texture.Image = &CodepointUsing->Bitmap;
	Texture.Center = (TopLeft + (vector2{0, (real64)CodepointUsing->BaselineFactor} * 0.5f)) * ScaleModifier;
	Texture.Scale = vector2{(real64)CodepointUsing->Bitmap.Width, (real64)(CodepointUsing->Bitmap.Height)} * 0.5f * ScaleModifier;
	PushRenderTexture(GameState, &Texture);

	if (ShowDebugInfo)
	{
		PushRenderSquare(GameState, MakeRectangle(Texture.Center, (int32)(CodepointUsing->Bitmap.Width * ScaleModifier), (int32)(CodepointUsing->Bitmap.Height * ScaleModifier), color{1.0, 0.0, 0.0, 0.5}));
	}
}

//NOTE TopLeft here is not actually TopLeft. It's more bottom Middle.
void
FontRenderWord(char *Word, vector2 TopLeft, real64 ScaleModifier, game_state * GameState)
{
	vector2 PosAt = TopLeft;

	uint32 WordLength = CharArrayLength(Word);
	for (uint32 LetterIndex = 0;
	     LetterIndex < WordLength;
	     LetterIndex++)
	{
		if (ShowDebugInfo)
		{
			PushRenderSquare(GameState, MakeRectangle(PosAt * ScaleModifier, (int32)(10 * ScaleModifier), (int32)(10 * ScaleModifier), color{0.0, 0.0, 1.0, 0.5}));
		}

		char *Letter = &Word[LetterIndex];
		FontRenderLetter(*Letter, PosAt, ScaleModifier, GameState);

		real64 AmountToAdvance = 0;
		if (LetterIndex + 1 != WordLength)
		{
			AmountToAdvance = (GameState->AlphabetBitmaps[(uint32)Word[LetterIndex]].Bitmap.Width / 2) +
			                  (GameState->AlphabetBitmaps[(uint32)Word[LetterIndex + 1]].Bitmap.Width / 2);
		}
		PosAt.X += AmountToAdvance + (5 * ScaleModifier);
	}

}

#endif