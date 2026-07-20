#pragma once
//========================================================================*/
//* FujiEngine 公開API
//*
//* Engineがゲーム側へ公開するヘッダの一覧。ここに無いEngineヘッダは内部実装であり、
//* Game から include してはいけない。
//*
//* このヘッダ自体をincludeしてもよいが、必須ではない。
//* 必要なヘッダが少ないファイルは個別にincludeした方がコンパイルが速い
//* (プリコンパイル済みヘッダを導入したら、その中身をこのヘッダにするとよい)。
//========================================================================*/

//========================================================================*/
//* 基盤
#include "Engine/DX/Framework.h"
#include "Engine/DX/FPSKeeper.h"
#include "Engine/WinApp/MyWindow.h"
#include "Engine/Input/Input.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Engine/GlobalVariables/GlobalVariables.h"

//========================================================================*/
//* 数学
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Math/Quaternion/Quaternion.h"
#include "Engine/Math/Random/Random.h"

//========================================================================*/
//* シーン
#include "Engine/Scene/BaseScene.h"
#include "Engine/Scene/SceneManager.h"
#include "Engine/Scene/AbstractSceneFactory.h"

//========================================================================*/
//* 描画
#include "Engine/Camera/CameraManager.h"
#include "Engine/Model/Object3d.h"
#include "Engine/Model/ObjectRenderer.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Model/AnimationData/AnimationModel.h"
#include "Engine/Model/SkyBox/SkyBox.h"
#include "Engine/Model/Line/Line3dDrawer.h"
#include "Engine/Model/Sprite/Sprite.h"
#include "Engine/Model/Sprite/SpriteRenderer.h"

//========================================================================*/
//* ライト
#include "Engine/Light/LightManager.h"
#include "Engine/Light/PointLight.h"

//========================================================================*/
//* パーティクル
#include "Engine/Particle/ParticleManager.h"
#include "Engine/Particle/ParticleEmitter.h"

//========================================================================*/
//* シリアライズ
#include "Engine/Serialize/JsonSerializer.h"

//========================================================================*/
//* デバッグ
#include "Engine/ImGuiManager/ImGuiManager.h"

//========================================================================*/
//* 公開しないもの
//*
//* Engine/Editor/ 以下はすべてエディタ機能であり公開APIではない。
//* 編集UI付きのJson入出力が要るときも Editor::JsonEditorUI ではなく
//* Core::JsonSerializer を使うこと。
//* 上記以外のEngineヘッダ(DXCom.h、各Pipe、TextureManager等)はすべて内部実装。
//========================================================================*/
