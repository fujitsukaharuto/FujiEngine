#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#endif // _DEBUG

class MyWin;
class DXCom;

class ImGuiManager
{
public:
	static ImGuiManager* GetInstance();
	/// <summary>
	/// 初期化
	/// </summary>
	void Init(MyWin* myWin, DXCom* dxComon);
	
	/// <summary>
	/// 終了
	/// </summary>
	void Fin();
	
	/// <summary>
	/// ImGui受付
	/// </summary>
	void Begin();

	/// <summary>
	/// ImGui締切
	/// </summary>
	void End();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	
private:
	DXCom* dxCommon_ = nullptr;
	uint32_t srvIndex;
private:
	ImGuiManager() = default;
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	const ImGuiManager& operator=(const ImGuiManager&) = delete;
};
