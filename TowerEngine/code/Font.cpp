#ifndef FONT_CPP
#define FONT_CPP

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

//TODO there are some weird artifacts when rendering the text, can be seen in the top left of the fps text

//NOTE there are some problems with vertical spacing that I had to manually fix... that sucks.

//NOTE Not entirely sure what this does...
global_variable real64 GlobalFontRenderSize = 200.0f;

#if DEBUG_PATH
	global_variable bool32 ShowDebugInfo = false;
#endif

void
MakeBitmapCodepoint(char Letter, platform_read_file *ReadFile, game_state *GameState)
{
	read_file_result FontFile = PlatformReadFile("../Assets/Gotham-Medium.ttf");
	Assert(FontFile.Contents > 0);

	int Width, Height, XOffset, YOffset;
	stbtt_fontinfo FontInfo;
	stbtt_InitFont(&FontInfo, (uint8 *)FontFile.Contents, stbtt_GetFontOffsetForIndex((uint8 *)FontFile.Contents, 0));
	uint8 *MonoBitmap = stbtt_GetCodepointBitmap(&FontInfo, 0, stbtt_ScaleForPixelHeight(&FontInfo, (float)GlobalFontRenderSize),
	                    Letter, &Width, &Height, &XOffset, &YOffset);

	font_codepoint *NextCodepoint = &GameState->AlphabetBitmaps[GameState->AlphabetBitmapsCount];
	GameState->AlphabetBitmapsCount++;

	NextCodepoint->BaselineFactor = YOffset;
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

void
FontRenderLetter(char Letter, vector2 TopLeft, real64 ScaleModifier, color Color, game_state *GameState,
                 list_head *RenderObjectsList, memory_arena *Memory)
{
	//NOTE there are half multipliers in here (0.5f) because I'm drawing the bitmaps a bit weird. It's really janky and I'm a little to lazy to fix it since it works.
	font_codepoint *CodepointUsing = &GameState->AlphabetBitmaps[(uint32)Letter];

	gl_texture Texture = {};
	Texture.Image = &CodepointUsing->Bitmap;

	char *Adjusters = "acemnosruvwxz";
	uint8 Index = 0;
	int8 VerticalModifier = 0;
	while (Adjusters[Index])
	{
		if (Adjusters[Index] == Letter)
		{
			VerticalModifier = 33;
		}
		Index++;
	}
	vector2 LetterCenterOffset = vector2{0, (real64)(CodepointUsing->BaselineFactor + VerticalModifier)} +
	                             vector2{(real64)CodepointUsing->Bitmap.Width * 0.5f, (real64)CodepointUsing->Bitmap.Height * 1.5f};
	Texture.Center = TopLeft + (LetterCenterOffset * ScaleModifier);
	Texture.Color = Color;
	Texture.Scale = vector2{(real64)CodepointUsing->Bitmap.Width, (real64)(CodepointUsing->Bitmap.Height)} * 0.5f * ScaleModifier;
	PushRenderTexture(RenderObjectsList, &Texture, Memory);

	#if DEBUG_PATH
	if (ShowDebugInfo)
	{
		PushRenderSquare(RenderObjectsList,
		                 MakeRectangle(Texture.Center, (int32)(10 * ScaleModifier), (int32)(10 * ScaleModifier), color{0.0, 1.0, 0.0, 0.5}),
		                 Memory);
		PushRenderSquare(RenderObjectsList,
		                 MakeRectangle(Texture.Center,
		                               (int32)(CodepointUsing->Bitmap.Width * ScaleModifier), (int32)(CodepointUsing->Bitmap.Height * ScaleModifier),
		                               color{1.0, 0.0, 0.0, 0.5}),
		                 Memory);
	}
	#endif
}

void
FontRenderWord(char *Word, vector2 TopLeft, real64 ScaleModifier, color Color, game_state * GameState,
               list_head *SquareListHead, memory_arena *Memory)
{
	vector2 PosAt = TopLeft;

	uint32 WordLength = CharArrayLength(Word);
	for (uint32 LetterIndex = 0;
	     LetterIndex < WordLength;
	     LetterIndex++)
	{
		#if DEBUG_PATH
		if (ShowDebugInfo)
		{
			PushRenderSquare(SquareListHead,
			                 MakeRectangle(PosAt, (int32)(10 * ScaleModifier), (int32)(10 * ScaleModifier), color{0.0, 0.0, 1.0, 0.5}),
			                 Memory);
		}
		#endif

		char *Letter = &Word[LetterIndex];
		FontRenderLetter(*Letter, PosAt, ScaleModifier, Color, GameState, SquareListHead, Memory);

		real64 AmountToAdvance = 0;
		if (LetterIndex + 1 != WordLength)
		{
			AmountToAdvance = (GameState->AlphabetBitmaps[(uint32)Word[LetterIndex]].Bitmap.Width);
			if ((int)Word[LetterIndex] == (int)' ')
			{
				AmountToAdvance = 100 * ScaleModifier;
			}
		}
		real64 LetterSpacing = 5;
		PosAt.X += (AmountToAdvance + LetterSpacing) * ScaleModifier;
	}
}

#endif