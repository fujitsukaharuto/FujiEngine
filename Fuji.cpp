#include "Fuji.h"

int Fuji::ProcessMessage()
{
	return mywin_->ProcessMessage() ? 1 : 0;
}

void Fuji::CreatWind()
{
	mywin_ = MyWin::GetInstance();
	mywin_->CreatWind();
}
