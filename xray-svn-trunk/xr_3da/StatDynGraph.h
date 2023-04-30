#pragma once

enum eGraphMoveDirection
{
	LEFT_TO_RIGHT,
	RIGHT_TO_LEFT
};

template <class T>
class StatDynGraph
{
private:
	xr_vector<T> graphValues;

	u32 maxIndex;
	int indexToBegin;
	int indexToWrite;
	int curInd;

	T maxRegistered;

	eGraphMoveDirection flowDirection;

	Fvector2 position1;
	Fvector2 position2;

	Fvector2 screenSize;

	Fvector2 screenScale;

	u32 lineColor;
public:
	 StatDynGraph				(u32 max_index, Fvector2 pos1, Fvector2 pos2, Fvector2 scale = {1.f, 1.f}, u32 color = 0xFFFF0000, eGraphMoveDirection flow_direction = RIGHT_TO_LEFT);
	 virtual ~StatDynGraph		();

	 void PushBack				(T value);

	 void Draw					();
};


#include "../Include/xrRender/UIShader.h"
#include "../Include/xrRender/UIRender.h"

extern FactoryPtr<IUIShader>* hShader;

template <class T>
StatDynGraph<T>::StatDynGraph(u32 max_index, Fvector2 pos1, Fvector2 pos2, Fvector2 scale, u32 color, eGraphMoveDirection flow_direction)
{
	maxIndex = max_index;
	indexToBegin = maxIndex - 1;
	indexToWrite = 0;
	curInd = 0;

	flowDirection = flow_direction;

	position1 = pos1;
	position2 = pos2;

	screenSize.x = pos2.x - pos1.x;
	screenSize.y = pos2.y - pos1.y;

	screenScale = scale;

	maxRegistered = (T)0;

	lineColor = color;

	graphValues.reserve(maxIndex);

	for (u32 i = 0; i < maxIndex; ++i)
	{
		graphValues.push_back(T(i));
	}
}

template <class T>
StatDynGraph<T>::~StatDynGraph()
{
	xr_delete(hShader);
}

template <class T>
void StatDynGraph<T>::PushBack(T value)
{
	graphValues[indexToWrite] = value;

	indexToWrite--;

	if (indexToWrite < 0)
		indexToWrite = maxIndex - 1;
}

template <class T>
void StatDynGraph<T>::Draw()
{
	if (!hShader) //init shader, if not already
	{
		hShader = xr_new <FactoryPtr<IUIShader> >();
		(*hShader)->create("hud\\default", "ui\\ui_console");
	}

	UIRender->StartPrimitive(maxIndex, IUIRender::ptLineStrip, IUIRender::pttTL);

	// Find the biggest value
	T max = 0.f;
	for (u32 i = 0; i < graphValues.size(); ++i)
	{
		if (graphValues[i] > max)
			max = graphValues[i];
	}

	maxRegistered = max;
	float y_value_bound = float(maxRegistered * 1.30f); // Set the upper bound a little more then max value

	curInd = indexToBegin;
	Fvector2 cur_vertex;

	for (u32 i = 0; i < graphValues.size(); ++i)
	{
		float x_k = screenSize.x / maxIndex; // calc the X depending on graphValues elements cnt and screenSize.x
		float y_k = graphValues[curInd] / y_value_bound; // calc the spike y

		cur_vertex.x = position1.x + float(i) * x_k * screenScale.x;
		cur_vertex.y = position2.y - (screenSize.y * y_k * screenScale.y);

		UIRender->PushPoint(cur_vertex.x, cur_vertex.y, 0, lineColor, 0, 0);

		// Draw from left to right, but going in opposite direction, since graph flow is from right to left
		if (flowDirection == RIGHT_TO_LEFT) // <<<<<<< //
		{
			curInd--;

			if (curInd < 0)
				curInd = maxIndex - 1;
		}
		// Draw from left to right in 1 2 3 order
		else if (flowDirection == LEFT_TO_RIGHT) // >>>>>>> //
		{
			curInd++;

			if (curInd == (T)maxIndex)
				curInd = 0;
		}
	}

	indexToBegin = curInd - 1;

	// reset back to last index, if reached 0
	if (indexToBegin < 0)
		indexToBegin = maxIndex - 1;

	// Where to write value nexttime
	indexToWrite = curInd;

	// render	
	UIRender->SetShader(**hShader);
	UIRender->FlushPrimitive();
}