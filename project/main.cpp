#include "Game/Scene/GameRun.h"
#include <memory>

// やること
// オフスクリーンのかんり
// モデルマネージャー、スプライトマネージャー
// wave対応
// クラス分け

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	std::unique_ptr<Framework> game = std::make_unique<GameRun>();

	game->Run();

	return 0;
}

