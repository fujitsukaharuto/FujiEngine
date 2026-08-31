#pragma once
//========================================================================*/
//* FujiEngine 公開API
//* ここに無いEngineヘッダは内部実装。Game から include しないこと
//* 要るヘッダが少ないファイルは個別にincludeした方がコンパイルが速い
//========================================================================*/

//========================================================================*/
//* 基盤
#include "Engine/Core/App/Framework.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Engine/Core/Serialize/GlobalVariables.h"

//========================================================================*/
//* 数学
#include "Engine/Math/Constants.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Math/Quaternion/Quaternion.h"
#include "Engine/Math/Random/Random.h"
#include "Engine/Math/Easing/Easing.h"

//========================================================================*/
//* シーン
#include "Engine/Scene/BaseScene.h"
#include "Engine/Scene/SceneManager.h"
#include "Engine/Scene/AbstractSceneFactory.h"

//========================================================================*/
//* 描画
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/FollowCamera.h"
#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Graphics/Object/AnimationModel.h"
#include "Engine/Graphics/SkyBox/SkyBox.h"
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Graphics/Sprite/Sprite.h"
#include "Engine/Graphics/Sprite/SpriteRenderer.h"

//========================================================================*/
//* ライト
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Light/PointLight.h"

//========================================================================*/
//* パーティクル
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Graphics/Particle/ParticleEmitter.h"

//========================================================================*/
//* シリアライズ
#include "Engine/Core/Serialize/JsonSerializer.h"

//========================================================================*/
//* デバッグ
#include "Engine/Core/Debug/ImGuiManager.h"

//========================================================================*/
//* 非公開
//* Engine/Editor/ 以下はエディタ機能。編集UI付きのJson入出力が要るときも
//* Editor::JsonEditorUI ではなく Core::JsonSerializer を使う
//========================================================================*/
