#include "Game/Scene/GameRun.h"
#include <memory>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	std::unique_ptr<Core::Framework> game = std::make_unique<GameRun>();

	game->Run();

	return 0;
}

