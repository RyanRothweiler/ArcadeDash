#ifndef MATH_ARCADE
#define MATH_ARCADE



#include "Vector2.h"
#include <math.h>


int64
SquareInt(int64 input)
{
	return (input * input);
}

real64
SquareReal(real64 input)
{
	return (input * input);
}

real64
ClampValue(real64 Bottom, real64 Top, real64 Value)
{
	if (Value <= Bottom)
	{
		return (Bottom);
	}
	if (Value >= Top)
	{
		return (Top);
	}
	return (Value);
}

vector2
ClampValue(real64 Bottom, real64 Top, vector2 Value)
{
	vector2 Output;
	Output.X = ClampValue(Bottom, Top, Value.X);
	Output.Y = ClampValue(Bottom, Top, Value.Y);
	return (Output);
}

int64
ClampValue(int64 Bottom, int64 Top, int64 Value)
{
	if (Value <= Bottom)
	{
		return (Bottom);
	}
	if (Value >= Top)
	{
		return (Top);
	}
	return (Value);
}

real64
SquareRoot(real64 num)
{
	real64 i = 0;
	real64 x1, x2;

	while ( (i * i) <= num )
	{
		i += 0.1f;
	}
	x1 = i;
	for (int j = 0; j < 10; j++)
	{
		x2 = num;
		x2 /= x1;
		x2 += x1;
		x2 /= 2;
		x1 = x2;
	}
	return x2;
}

real64
Abs(real64 A)
{
	if (A >= 0)
	{
		return (A);
	}
	else
	{
		return (A * -1);
	}
}

real64
DotProduct(vector2 A, vector2 B)
{
	real64 Result;
	Result = (A.X * B.X) + (A.Y * B.Y);
	return (Result);
}

real64
Cos(real64 Input)
{
	return (cos(Input));
}

real64
ArcCos(real64 Input)
{
	return (acos(Input));
}

real64
Sin(real64 Input)
{
	return (sin(Input));
}

real64
Max(real64 A, real64 B)
{
	if (A > B)
	{
		return (A);
	}
	else
	{
		return (B);
	}
}

real64
Min(real64 A, real64 B)
{
	if (A < B)
	{
		return (A);
	}
	else
	{
		return (B);
	}
}

const real64 PI = 3.14159f;

#endif