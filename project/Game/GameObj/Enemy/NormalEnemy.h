#pragma once
#include <list>
#include <memory>
//Function
#include "Easing.h"
// class
#include "BaseEnemy.h"

/// <summary>
/// 敵クラス
/// </summary>
class NormalEnemy : public BaseEnemy {
private:
	

private: 
	
public: 

	///========================================================================================
	///  public method
	///========================================================================================

	// 初期化、更新、描画
	 void Initialize()override;
	 void Update()override;
	 void Draw(Material* mate = nullptr)override;
	
};