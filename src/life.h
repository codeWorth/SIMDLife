#pragma once
#include "constants.h"

class Life {
public:
	virtual void setup() = 0;
	virtual void tick() = 0;
	virtual void draw(BYTE* pixelBuffer, int& px0, int& py0) = 0;
};