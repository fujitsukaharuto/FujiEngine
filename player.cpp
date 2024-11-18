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
	bullet->transform.scale = { 0.5f,0.5f,82.0f };

	left_.reset(new Object3d);
	left_->Create("razer.obj");
	left_->transform.scale = { 0.5f,0.5f,80.0f };
	left_->transform.translate = { -0.5f,-0.5f,0.0f };
	left_->SetCameraParent(true);

	right_.reset(new Object3d);
	right_->Create("razer.obj");
	right_->transform.scale = { 0.5f,0.5f,80.0f };
	right_->transform.translate = { 0.5f,-0.5f,0.0f };
	right_->SetCameraParent(true);



	reticle.reset(new Sprite);
	reticle->Load("Sprite/reticle.png");
	reticle->SetSize({ 100.0f,100.0f });

	energyCase.reset(new Sprite);
	energyCase->Load("Sprite/energyCase.png");
	energyCase->SetPos({ 120.0f,360.0f,0.0f });



	energyQuantity.reset(new Sprite);
	energyQuantity->Load("Sprite/energy.png");
	energyQuantity->SetAnchor({ 0.5f,1.0f });
	energyQuantity->SetPos({ 120.0f,510.0f,0.0f });


}

void Player::Update() {

	Camera* camera = CameraManager::GetInstance()->GetCamera();
	camera->UpdateMatrix();

	ReticleCal();
	

	if (Input::GetInstance()->IsPressMouse(0)) {
		const float kBulletSpeed = 82.0f;
		Vector3 worldPosReticle;

		if (0.0f < energy_) {
			energy_ -= FPSKeeper::DeltaTime();
		}
		else {
			energy_ = 0.0f;
		}
		energyQuantity->SetSize({ 60.0f,energy_ });

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
		if (energy_ > 0.0f) {
			isLive = true;


			worldPosReticle = target->transform.translate;
			worldPosReticle -= camera->transform.translate;
			worldPosPlayer = left_->transform.translate;
			velocity = worldPosReticle - worldPosPlayer;
			velocity = velocity.Normalize();
			velocity = velocity * kBulletSpeed;
			left_->transform.rotate.y = std::atan2(velocity.x, velocity.z);
			yrota = MakeRotateYMatrix(-left_->transform.rotate.y);
			velocityZ = TransformNormal(velocity, yrota);
			left_->transform.rotate.x = std::atan2(-velocityZ.y, velocityZ.z);
			left_->transform.rotate.z = 0.0f;
			left_->transform.rotate-= camera->transform.rotate;
			left_->UpdateMatrix();

			worldPosReticle = target->transform.translate;
			worldPosReticle -= camera->transform.translate;
			worldPosPlayer = right_->transform.translate;
			velocity = worldPosReticle - worldPosPlayer;
			velocity = velocity.Normalize();
			velocity = velocity * kBulletSpeed;
			right_->transform.rotate.y = std::atan2(velocity.x, velocity.z);
			yrota = MakeRotateYMatrix(-right_->transform.rotate.y);
			velocityZ = TransformNormal(velocity, yrota);
			right_->transform.rotate.x = std::atan2(-velocityZ.y, velocityZ.z);
			right_->transform.rotate.z = 0.0f;
			right_->transform.rotate -= camera->transform.rotate;
			right_->UpdateMatrix();

		}
		else {
			isLive = false;
		}

	}
	else {
		isLive = false;
		if (300.0f > energy_) {
			energy_ += FPSKeeper::DeltaTime() * 2.0f;
		}
		else {
			energy_ = 300.0f;
		}
		energyQuantity->SetSize({ 60.0f,energy_ });

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

	if (isLive) {

		left_->Draw();
		right_->Draw();
	}
}

void Player::SpriteDraw() {

	energyQuantity->Draw();
	energyCase->Draw();
	reticle->Draw();

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

	Vector3 reticlePos = Vector3(float(Input::GetInstance()->GetMousePosition().x), float(Input::GetInstance()->GetMousePosition().y), 0);
	Vector3 posNear = reticlePos;
	Vector3 posFar = Vector3(float(Input::GetInstance()->GetMousePosition().x), float(Input::GetInstance()->GetMousePosition().y), 1);

	posNear = Transform(posNear, matInverseVPV);
	posFar = Transform(posFar, matInverseVPV);

	Vector3 mouseDirection = posFar - posNear;
	mouseDirection = mouseDirection.Normalize();

	const float kDistanceTestObject = 80.0f;
	target->transform.translate = posNear + (mouseDirection * kDistanceTestObject);

	reticle->SetPos(reticlePos);

}


//void Player::Initialize() {
//
//
//	target = new Object3d();
//	target->Create("ICO.obj");
//
//	bullet = new Object3d();
//	bullet->Create("razer.obj");
//	bullet->transform.scale.z = 82.0f;
//	bullet->SetCameraParent(true);
//	Vector3 worldPos = { 0.0f,-0.5f,0.0f };
//	bullet->transform.translate = worldPos;
//}
//
//void Player::Update() {
//
//	ReticleCal();
//	Camera* camera = CameraManager::GetInstance()->GetCamera();
//
//	if (Input::GetInstance()->IsPressMouse(0)) {
//		const float kBulletSpeed = 82.0f;
//		Vector3 worldPosReticle;
//
//		worldPosReticle = target->transform.translate;
//
//		Vector3 worldPosPlayer = camera->transform.translate;
//		Vector3 offset = { 0.0f,0.5f,0.0f };
//		worldPosPlayer = worldPosPlayer - offset;
//		Vector3 velocity = worldPosReticle - worldPosPlayer;
//		velocity = velocity.Normalize();
//		velocity = velocity * kBulletSpeed;
//		velo_ = velocity;
//
//		bullet->transform.rotate.y = std::atan2(velocity.x, velocity.z);
//		Matrix4x4 yrota = MakeRotateYMatrix(-bullet->transform.rotate.y);
//		Vector3 velocityZ = TransformNormal(velocity, yrota);
//		bullet->transform.rotate.x = std::atan2(-velocityZ.y, velocityZ.z);
//		bullet->transform.rotate.z = 0.0f;
//
//
//		bullet->transform.rotate -= camera->transform.rotate;
//		startTime = 0.0f;
//
//		/*bullet->transform.scale = { 0.0f,0.0f,0.0f };*/
//		isLive = true;
//	}
//	else {
//		isLive = false;
//	}