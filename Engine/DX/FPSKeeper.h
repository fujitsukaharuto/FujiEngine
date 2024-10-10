#pragma once
#include <chrono>


class FPSKeeper {
public:
	FPSKeeper();
	~FPSKeeper();

public:

	void Initialize();

	void FixFPS();


private:



private:

	std::chrono::steady_clock::time_point reference_;



};
