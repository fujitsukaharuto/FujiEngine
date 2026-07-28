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
#include "Engine/Core/App/Framework.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Engine/Core/Serialize/GlobalVariables.h"

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
#include "Engine/Graphics/Camera/CameraManager.h"
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
//* 公開しないもの
//*
//* Engine/Editor/ 以下はすべてエディタ機能であり公開APIではない。
//* 編集UI付きのJson入出力が要るときも Editor::JsonEditorUI ではなく
//* Core::JsonSerializer を使うこと。
//* 上記以外のEngineヘッダ(DXCom.h、各Pipe、TextureManager等)はすべて内部実装。
//========================================================================*/
