#pragma once

template <class T>
struct _trapezoid {
public:
	typedef T			TYPE;
	typedef _trapezoid<T>Self;
	typedef Self&		SelfRef;
	typedef const Self&	SelfCRef;
	typedef _vector2<T>	Tvector;

private:
	IC bool check_triangle(const Tvector& corner_A, const Tvector& corner_B, const Tvector& corner_C, const Tvector& Point) const
	{
		float a = (corner_A.x - Point.x) * (corner_B.y - corner_A.y) - (corner_B.x - corner_A.x) * (corner_A.y - Point.y);
		float b = (corner_B.x - Point.x) * (corner_C.y - corner_B.y) - (corner_C.x - corner_B.x) * (corner_B.y - Point.y);
		float c = (corner_C.x - Point.x) * (corner_A.y - corner_C.y) - (corner_A.x - corner_C.x) * (corner_C.y - Point.y);

		if ((a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0))
			return true;
		else
			return false;
	}

public:

	Tvector corner1, corner2, corner3, corner4;

	// create with two x and two y coordinates, representing min and max limit
	IC SelfRef set(const T _x1, const T _y1, const T _x2, const T _y2, const T _x3, const T _y3, const T _x4, const T _y4)
	{
		corner1.set(_x1, _y1); corner2.set(_x2, _y2); corner3.set(_x3, _y3); corner4.set(_x4, _y4); return *this;
	};

	IC SelfRef set(const Tvector& A, const Tvector& B, const Tvector& C, Tvector& D)
	{
		corner1 = A; corner2 = B; corner3 = C; corner4 = D;  return *this;
	};

	IC bool point_belongs(T x, T  y) const
	{
		return (check_triangle(corner1, corner2, corner3, Tvector().set(x, y))) || (check_triangle(corner1, corner3, corner4, Tvector().set(x, y))) || (check_triangle(corner1, corner2, corner4, Tvector().set(x, y)));
	};

	IC bool point_belongs(const Tvector& point) const
	{
		return (check_triangle(corner1, corner2, corner3, point)) || (check_triangle(corner1, corner3, corner4, point)) || (check_triangle(corner1, corner2, corner4, point));
	};
};

typedef _trapezoid<float> Ftrapezoid;