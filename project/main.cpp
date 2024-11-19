#include "GameRun.h"

// やること
// オフスクリーンのかんり
// モデルマネージャー、スプライトマネージャー
// wave対応
// クラス分け

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	Framework* game = new GameRun;

	game->Run();

	delete game;

	return 0;
}

