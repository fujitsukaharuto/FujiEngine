#pragma once
#include "MyWindow.h"

namespace {
	MyWin* mywin_ = nullptr;
}

class Fuji
{
public:
	Fuji()=default;
	~Fuji()=default;

	static int ProcessMessage();
	static void CreatWind();

private:

	

};
