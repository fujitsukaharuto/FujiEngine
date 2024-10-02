#pragma once
#include "MyWindow.h"

MyWin* mywin_ = nullptr;

class My
{
public:
	My() = default;
	~My() = default;

	//static void Init(const char* title, int width = 1280, int height = 720);
	//static void Fin();

	static int ProcessMessage();
	static void CreatWind();

private:

};

void My::CreatWind() {
	mywin_ = MyWin::GetInstance();
	mywin_->CreatWind();
}

int My::ProcessMessage() { return mywin_->ProcessMessage() ? 1 : 0; }