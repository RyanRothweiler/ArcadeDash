#include "Vector2.h"

vector2 operator+(vector2 A, vector2 B)
{
	vector2 Output;
	Output.X = A.X + B.X;
	Output.Y = A.Y + B.Y;
	return (Output);
}

vector2 operator+(real64 B, vector2 A)
{
	vector2 Output;
	Output.X = A.X + B;
	Output.Y = A.Y + B;
	return (Output);
}

vector2 operator-(vector2 A, vector2 B)
{
	vector2 Output;
	Output.X = A.X - B.X;
	Output.Y = A.Y - B.Y;
	return (Output);
}

vector2 operator/(vector2 A, int32 B)
{
	vector2 Output;
	Output.X = A.X / B;
	Output.Y = A.Y / B;
	return (Output);
}

vector2 operator/(vector2 A, real64 B)
{
	vector2 Output;
	Output.X = A.X / B;
	Output.Y = A.Y / B;
	return (Output);
}

vector2 operator/(real64 B, vector2 A)
{
	vector2 Output;
	Output.X = A.X / B;
	Output.Y = A.Y / B;
	return (Output);
}

vector2 operator*(vector2 A, real64 B)
{
	vector2 Output;
	Output.X = A.X * B;
	Output.Y = A.Y * B;
	return (Output);
}

vector2 operator*(vector2 A, float B)
{
	vector2 Output;
	Output.X = A.X * B;
	Output.Y = A.Y * B;
	return (Output);
}

vector2 operator*(float B, vector2 A)
{
	vector2 Output;
	Output.X = A.X * B;
	Output.Y = A.Y * B;
	return (Output);
}

vector2 operator*(vector2 A, int64 B)
{
	vector2 Output;
	Output.X = A.X * B;
	Output.Y = A.Y * B;
	return (Output);
}

vector2 operator*(vector2 A, vector2 B)
{
	vector2 Output;
	Output.X = A.X * B.X;
	Output.Y = A.Y * B.Y;
	return (Output);
}

bool32 operator>(vector2 A, int32 B)
{
	if (A.X > B &&
	    A.Y > B)
	{
		return (true);
	}
	return (false);
}

real64
Vector2Length(vector2 A)
{
	return (SquareRoot((A.X * A.X) + (A.Y * A.Y)));
}

vector2
Vector2Normalize(vector2 Input)
{
	vector2 Output = {};

	real64 Length = Vector2Length(Input);
	Output.X = Input.X / Length;
	Output.Y = Input.Y / Length;

	return (Output);
}

real64
Vector2Distance(vector2 A, vector2 B)
{
	real64 Output = 0;
	real64 ASideLength = Abs(A.X - B.X);
	real64 BSideLength = Abs(A.Y - B.Y);
	Output = SquareRoot((ASideLength * ASideLength) + (BSideLength * BSideLength));
	return (Abs(Output));
}

real64
Vector2AngleBetween(vector2 A, vector2 B)
{
	real64 Result = 0;
	Result = acos(DotProduct(A, B) / (Vector2Length(A) * Vector2Length(B)));
	return (Result);
}

real64
Vector2GetDimension(uint8 Dimension, vector2 Vector)
{
	if (Dimension == 0)
	{
		return (Vector.X);
	}
	if (Dimension == 1)
	{
		return (Vector.Y);
	}

	// We probably tried to get a dimension which is not 0 or 1

	Assert(0);
	return (0);
}

vector2
Vector2RotatePoint(vector2 OriginalPoint, vector2 Center, real64 Angle)
{
	vector2 Result = {};
	Result.X = Center.X + ((OriginalPoint.X - Center.X) * cos(Angle)) + ((OriginalPoint.Y - Center.Y) * sin(Angle));
	Result.Y = Center.Y - ((OriginalPoint.X - Center.X) * sin(Angle)) + ((OriginalPoint.Y - Center.Y) * cos(Angle));
	return (Result);
}

const vector2 VECTOR2_ZERO = vector2{0, 0};