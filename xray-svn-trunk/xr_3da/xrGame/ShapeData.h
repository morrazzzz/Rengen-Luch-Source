#ifndef ShapeDataH
#define ShapeDataH

struct CShapeData
{
	enum{
    	cfSphere=0,
        cfBox
    };
	union shape_data
	{
		Fsphere		sphere;
		Fmatrix		box;

		shape_data() {};
	};
	struct shape_def
	{
		u8			type;
		shape_data	data;

		shape_def() {};
	};
    DEFINE_VECTOR					(shape_def,ShapeVec,ShapeIt);
	ShapeVec						shapes;
};

#endif