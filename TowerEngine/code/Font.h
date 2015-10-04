#ifndef FONT_H
#define FONT_H


struct font_codepoint
{
	loaded_image Bitmap;
};

struct font_utility
{
	uint32 LetterConverterCount;
	char Letters[100];
};

#endif
