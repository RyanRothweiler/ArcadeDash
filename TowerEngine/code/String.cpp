#ifndef STRING_CPP
#define STRING_CPP

int32
CharArrayLength(char *String)
{
	int Count = 0;
	while (*String++)
	{
		++Count;
	}
	return (Count);
}


#endif