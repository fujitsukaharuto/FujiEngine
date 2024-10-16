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
	bullet->Create("ICO.obj");

}

void Player::Update() {

	ReticleCal();
	Camera* camera = CameraManager::GetInstance()->GetCamera();

	if (Input::GetInstance()->IsTriggerMouse(0)) {
		const float kBulletSpeed = 1.0f;
		Vector3 worldPosReticle;

		worldPosReticle = target->transform.translate;

		Vector3 worldPosPlayer = camera->transform.translate;
		Vector3 velocity = worldPosReticle - worldPosPlayer;
		velocity = velocity.Normalize();
		velocity = velocity * kBulletSpeed;
		velo_ = velocity;

		Vector3 worldPos = camera->transform.translate;
		bullet->transform.translate = worldPos;
	}

	bullet->transform.translate += velo_;

}

void Player::Draw() {
	target->Draw();
	bullet->Draw();
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
