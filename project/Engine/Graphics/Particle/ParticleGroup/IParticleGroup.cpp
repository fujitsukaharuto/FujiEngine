#include "IParticleGroup.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Math/Random/Random.h"
#include "Engine/Graphics/Camera/CameraManager.h"

using namespace Core;
using namespace Math;
using namespace Graphics;


bool IParticleGroup::LifeUpdate(Particle& particle) {
	if (particle.lifeTime_ <= 0) {
		particle.isLive_ = false;
		return true;
	}
	if (!particle.isLive_) {
		return true;
	}

	particle.lifeTime_ -= FPSKeeper::DeltaTimeForEffect();
	return false;
}

void IParticleGroup::ParticleSizeUpdate(Particle& particle) {
	SizeType sizeType = SizeType(particle.type_);
	float t = (1.0f - float(float(particle.lifeTime_) / float(particle.startLifeTime_)));

	if (particle.isColorFade_) {
		particle.color_.w = Lerp(particle.startAlpha_, 0.0f, t * t);
	}
	if (particle.isAutoUVMove_) {
		particle.uvTrans_ += particle.autoUVSpeed_ * FPSKeeper::DeltaTimeForEffect();
	}

	switch (sizeType) {
	case SizeType::kNormal:
		break;
	case SizeType::kShift:

		particle.scale.x = Lerp(particle.startSize_.x, particle.endSize_.x, t);
		particle.scale.y = Lerp(particle.startSize_.y, particle.endSize_.y, t);
		if (particle.isZandX_) {
			particle.scale.z = Lerp(particle.startSize_.x, particle.endSize_.x, t);
		}

		break;
	case SizeType::kSin:

		Vector2 minSize = particle.startSize_; // 最小値
		Vector2 maxSize = particle.endSize_; // 最大値

		if (minSize.x > maxSize.x) {
			std::swap(minSize.x, maxSize.x); // minとmaxを交換
		}
		if (minSize.y > maxSize.y) {
			std::swap(minSize.y, maxSize.y); // minとmaxを交換
		}

		Vector2 sizeSin = minSize + (maxSize - minSize) * 0.5f * (1.0f + sin(particle.lifeTime_));

		particle.scale.x = sizeSin.x;
		particle.scale.y = sizeSin.y;

		break;
	}
}

void IParticleGroup::SRTUpdate(Particle& particle) {
	if (particle.rotateType_ == static_cast<int>(RotateType::kRandomR)) {
		if (particle.isContinuouslyRotate_) {
			particle.rotate += particle.continuouslyR_ * FPSKeeper::DeltaTimeForEffect();
		}
	}

	particle.speed_ += particle.accele_ * FPSKeeper::DeltaTimeForEffect();
	particle.translate += particle.speed_ * FPSKeeper::DeltaTimeForEffect();
}

void IParticleGroup::Billboard(Particle& particle, Matrix4x4& worldMatrix, const Matrix4x4& billboardMatrix, const Matrix4x4& rotate, Camera* camera) {
	if (!particle.isBillBoard_) {
		worldMatrix = MakeAffineMatrix(particle.scale, particle.rotate, particle.translate);
	}
	if (particle.isBillBoard_) {
		switch (particle.pattern_) {
		case BillBoardPattern::kXYZBillBoard: {
			Vector3 possition = particle.translate;
			if (particle.isParentRotate_) {
				possition = TransformNormal(possition, rotate);
			}
			worldMatrix = Multiply(MakeScaleMatrix(particle.scale), billboardMatrix);
			worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(possition));
			break;
		}
		case BillBoardPattern::kXBillBoard: {

			Matrix4x4 xBillboardMatrix;
			xBillboardMatrix = billboardMatrix;
			xBillboardMatrix.m[1][0] = 0.0f; // Y軸成分をゼロにする
			xBillboardMatrix.m[2][0] = 0.0f; // Z軸成分をゼロにする

			worldMatrix = Multiply(MakeScaleMatrix(particle.scale), MakeRotateXYZMatrix({ 0.0f,particle.rotate.y,particle.rotate.z }));
			worldMatrix = Multiply(worldMatrix, xBillboardMatrix);
			worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.translate));
			break;
		}
		case BillBoardPattern::kYBillBoard: {
			if (camera) {
				Matrix4x4 yBillboardMatrix = billboardMatrix;
				yBillboardMatrix.m[0][1] = 0.0f; // X軸成分をゼロにする
				yBillboardMatrix.m[2][1] = 0.0f; // Z軸成分をゼロにする

				worldMatrix = Multiply(MakeScaleMatrix(particle.scale), MakeRotateXYZMatrix({ particle.rotate.x,0.0f,particle.rotate.z }));
				worldMatrix = Multiply(worldMatrix, yBillboardMatrix);
				worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.translate));
			}
			break;
		}
		case BillBoardPattern::kZBillBoard: {

			Matrix4x4 zBillboardMatrix = billboardMatrix;
			zBillboardMatrix.m[0][2] = 0.0f; // X軸成分をゼロにする
			zBillboardMatrix.m[1][2] = 0.0f; // Y軸成分をゼロにする

			worldMatrix = Multiply(MakeScaleMatrix(particle.scale), MakeRotateXYZMatrix({ particle.rotate.x,particle.rotate.y,0.0f }));
			worldMatrix = Multiply(worldMatrix, zBillboardMatrix);
			worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.translate));

			break;
		}
		case BillBoardPattern::kXYBillBoard: {
			Matrix4x4 xyBillboardMatrix = billboardMatrix;
			xyBillboardMatrix.m[0][1] = 0.0f; // X軸成分をゼロにする
			xyBillboardMatrix.m[2][1] = 0.0f; // Z軸成分をゼロにする
			xyBillboardMatrix.m[1][0] = 0.0f; // Y軸成分をゼロにする
			xyBillboardMatrix.m[2][0] = 0.0f; // Z軸成分をゼロにする

			worldMatrix = Multiply(MakeScaleMatrix(particle.scale), MakeRotateXYZMatrix({ particle.rotate.x,0.0f,particle.rotate.z }));
			worldMatrix = Multiply(worldMatrix, xyBillboardMatrix);
			worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(particle.translate));

			break;
		}
		default:
			break;
		}
	}
}

bool IParticleGroup::InitEmitParticle(Particle& particle, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParameter& para) {
	if (particle.isLive_ == false) {
		particle.translate = grain.translate;
		particle.scale = grain.scale;
		particle.rotate = grain.rotate;
		particle.translate = Random::GetVector3(para.transx, para.transy, para.transz);
		particle.translate += pos;
		particle.scale = { grain.startSize_.x + para.addRandomSize.x,grain.startSize_.y + para.addRandomSize.y,1.0f };
		if (grain.speedType_ == static_cast<int>(SpeedType::kCenter)) {
			particle.speed_ = grain.speed_;
		} else {
			particle.speed_ = Random::GetVector3(para.speedx, para.speedy, para.speedz);
		}
		particle.returnPower_ = grain.returnPower_;

		particle.rotateType_ = grain.rotateType_;
		particle.isContinuouslyRotate_ = grain.isContinuouslyRotate_;
		if (particle.isContinuouslyRotate_) {
			particle.continuouslyR_ = Random::GetVector3({ -1.2f,1.2f }, { -1.2f,1.2f }, { -1.2f,1.2f });
		}
		Vector3 veloSpeed = particle.speed_.Normalize();
		Vector3 cameraR{};
		Vector3 defa = { 0.0f,1.0f,0.0f };
		Vector3 angleDToD{};
		Matrix4x4 rotateCamera;
		Matrix4x4 dToD;

		switch (particle.rotateType_) {
		case static_cast<int>(RotateType::kUsually):
			particle.rotate = rotate;
			break;
		case static_cast<int>(RotateType::kVelocityR):

			veloSpeed = particle.speed_.Normalize();

			// カメラの回転を考慮して速度ベクトルを変換
			cameraR = CameraManager::GetInstance()->GetCamera()->GetTranslate();
			rotateCamera = MakeRotateXYZMatrix(-cameraR);
			veloSpeed = TransformNormal(veloSpeed, rotateCamera);

			defa = TransformNormal(defa, rotateCamera);
			dToD = DirectionToDirection(defa, veloSpeed.Normalize());
			angleDToD = ExtractEulerAngles(dToD);
			particle.rotate = angleDToD;

			break;
		case static_cast<int>(RotateType::kRandomR):
			particle.rotate = Random::GetVector3({ -3.0f,3.0f }, { -3.0f,3.0f }, { -3.0f,3.0f });
			break;
		}

		particle.lifeTime_ = grain.lifeTime_;
		particle.startLifeTime_ = particle.lifeTime_;
		particle.isBillBoard_ = grain.isBillBoard_;
		particle.pattern_ = grain.pattern_;
		particle.colorType_ = grain.colorType_;
		particle.isColorFade_ = grain.isColorFade_;
		particle.isColorFadeIn_ = grain.isColorFadeIn_;
		particle.uvTrans_ = grain.uvTrans_;
		particle.uvScale_ = grain.uvScale_;
		particle.isAutoUVMove_ = grain.isAutoUVMove_;
		particle.autoUVSpeed_ = Vector2((Random::GetFloat(para.autoUVMin.x, para.autoUVMax.x)), (Random::GetFloat(para.autoUVMin.y, para.autoUVMax.y)));
		switch (particle.colorType_) {
		case static_cast<int>(ColorType::kDefault):
			particle.color_ = para.colorMax;
			break;
		case static_cast<int>(ColorType::kRandom):
			particle.color_.x = Random::GetFloat(para.colorMin.x, para.colorMax.x);
			particle.color_.y = Random::GetFloat(para.colorMin.y, para.colorMax.y);
			particle.color_.z = Random::GetFloat(para.colorMin.z, para.colorMax.z);
			particle.color_.w = Random::GetFloat(para.colorMin.w, para.colorMax.w);
			break;
		}
		particle.startAlpha_ = particle.color_.w;

		SpeedType type = SpeedType(grain.speedType_);
		switch (type) {// スピードのタイプで分ける
		case SpeedType::kConstancy:
			particle.accele_ = Vector3{ 0.0f,0.0f,0.0f };
			break;
		case SpeedType::kChange:
			particle.accele_ = grain.accele_;
			break;
		case SpeedType::kReturn:
			particle.accele_ = (particle.speed_) * grain.returnPower_;
			break;
		case SpeedType::kCenter:
			particle.accele_ = Vector3{ 0.0f,0.0f,0.0f };
			break;
		}

		particle.type_ = grain.type_;
		particle.startSize_ = grain.startSize_ + para.addRandomSize;
		particle.endSize_ = grain.endSize_ + para.addRandomSize;
		particle.isZandX_ = grain.isZandX_;
		particle.isParent_ = grain.isParent_;
		particle.isParentRotate_ = grain.isParentRotate_;

		particle.isLive_ = true;
		return true;
	}
	return false;
}