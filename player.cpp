#include "Player.h"
#include "MatrixCalculation.h"
#include "MyWindow.h"
#include "CameraManager.h"
#include "Input.h"

Player::Player() {
}

Player::~Player() {
	delete target;
	delete bullet;
}

void Player::Initialize() {


	target = new Object3d();
	target->Create("ICO.obj");

	bullet = new Object3d();
	bullet->Create("razer.obj");
	bullet->transform.scale.z = 82.0f;

}

void Player::Update() {

	ReticleCal();
	Camera* camera = CameraManager::GetInstance()->GetCamera();

	if (Input::GetInstance()->IsPressMouse(0)) {
		const float kBulletSpeed = 82.0f;
		Vector3 worldPosReticle;

		worldPosReticle = target->transform.translate;

		Vector3 worldPosPlayer = camera->transform.translate;
		Vector3 offset = { 0.0f,0.5f,0.0f };
		worldPosPlayer = worldPosPlayer - offset;
		Vector3 velocity = worldPosReticle - worldPosPlayer;
		velocity = velocity.Normalize();
		velocity = velocity * kBulletSpeed;
		velo_ = velocity;

		bullet->transform.rotate.y = std::atan2(velocity.x, velocity.z);
		Matrix4x4 yrota = MakeRotateYMatrix(-bullet->transform.rotate.y);
		Vector3 velocityZ = TransformNormal(velocity, yrota);
		bullet->transform.rotate.x = std::atan2(-velocityZ.y, velocityZ.z);
		bullet->transform.rotate.z = 0.0f;

		startTime = 0.0f;
		Vector3 worldPos = worldPosPlayer;
		bullet->transform.translate = worldPos;
		/*bullet->transform.scale = { 0.0f,0.0f,0.0f };*/
		isLive = true;
	}
	else {
		isLive = false;
	}

	if (isLive) {
		if (startTime <= kStartTime) {
			startTime += FPSKeeper::DeltaTime();
			//bullet->transform.scale.x += 0.05f * FPSKeeper::DeltaTime();
			//bullet->transform.scale.y += 0.05f * FPSKeeper::DeltaTime();
			//bullet->transform.scale.z += 0.05f * FPSKeeper::DeltaTime();
		}
	}

}

void Player::Draw() {
	target->Draw();
	if (isLive) {
		bullet->Draw();
	}
}


Vector3 Player::GetCenterBullet() const {
	return bullet->transform.translate;
}

float Player::GetBulletScale() const {
	return bullet->transform.scale.x;
}

void Player::ReticleCal() {

	Camera* camera = CameraManager::GetInstance()->GetCamera();

	Matrix4x4 matViewport = MakeViewportMat(0, 0, MyWin::kWindowWidth, MyWin::kWindowHeight, 0, 1);
	Matrix4x4 matVPV = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());
	matVPV = Multiply(matVPV, matViewport);

	Matrix4x4 matInverseVPV = Inverse(matVPV);

	Vector3 posNear = Vector3(float(Input::GetInstance()->GetMousePosition().x), float(Input::GetInstance()->GetMousePosition().y), 0);
	Vector3 posFar = Vector3(float(Input::GetInstance()->GetMousePosition().x), float(Input::GetInstance()->GetMousePosition().y), 1);

	posNear = Transform(posNear, matInverseVPV);
	posFar = Transform(posFar, matInverseVPV);

	Vector3 mouseDirection = posFar - posNear;
	mouseDirection = mouseDirection.Normalize();

	const float kDistanceTestObject = 80.0f;
	target->transform.translate = posNear + (mouseDirection * kDistanceTestObject);

}
