#include "Engine/Math/Spline/Spline.h"
#include <algorithm>
#include <cmath>

using namespace Math;

void Spline::Set(const std::vector<Point>& points, bool isClosed) {
	points_ = points;
	isClosed_ = isClosed && points_.size() >= 3;
	BuildLengthTable();
}

int Spline::GetSegmentCount() const {
	const int count = static_cast<int>(points_.size());
	if (count < 2) {
		return 0;
	}
	return isClosed_ ? count : count - 1;
}

Vector3 Spline::TangentAt(int index) const {
	const int count = static_cast<int>(points_.size());
	if (isClosed_) {
		const Vector3& previous = points_[(index + count - 1) % count].position;
		const Vector3& next = points_[(index + 1) % count].position;
		return (next - previous) * 0.5f;
	}
	if (index == 0) {
		return points_[1].position - points_[0].position;
	}
	if (index == count - 1) {
		return points_[count - 1].position - points_[count - 2].position;
	}
	return (points_[index + 1].position - points_[index - 1].position) * 0.5f;
}

void Spline::SplitKey(float key, int& segment, float& t) const {
	const int segments = GetSegmentCount();
	key = std::clamp(key, 0.0f, static_cast<float>(segments));
	segment = (std::min)(static_cast<int>(std::floor(key)), segments - 1);
	t = key - static_cast<float>(segment);
}

Vector3 Spline::Evaluate(float key) const {
	if (points_.empty()) {
		return {};
	}
	if (GetSegmentCount() == 0) {
		return points_[0].position;
	}
	int segment;
	float t;
	SplitKey(key, segment, t);
	const int count = static_cast<int>(points_.size());
	const Vector3& p0 = points_[segment].position;
	const Vector3& p1 = points_[(segment + 1) % count].position;
	if (points_[segment].type == PointType::Linear) {
		return p0 + (p1 - p0) * t;
	}
	// Hermite: 両端の位置と接線を通る3次式
	const Vector3 m0 = TangentAt(segment);
	const Vector3 m1 = TangentAt((segment + 1) % count);
	const float t2 = t * t;
	const float t3 = t2 * t;
	return p0 * (2.0f * t3 - 3.0f * t2 + 1.0f) + m0 * (t3 - 2.0f * t2 + t) + p1 * (-2.0f * t3 + 3.0f * t2) + m1 * (t3 - t2);
}

Vector3 Spline::EvaluateDerivative(float key) const {
	if (GetSegmentCount() == 0) {
		return {};
	}
	int segment;
	float t;
	SplitKey(key, segment, t);
	const int count = static_cast<int>(points_.size());
	const Vector3& p0 = points_[segment].position;
	const Vector3& p1 = points_[(segment + 1) % count].position;
	if (points_[segment].type == PointType::Linear) {
		return p1 - p0;
	}
	const Vector3 m0 = TangentAt(segment);
	const Vector3 m1 = TangentAt((segment + 1) % count);
	const float t2 = t * t;
	return p0 * (6.0f * t2 - 6.0f * t) + m0 * (3.0f * t2 - 4.0f * t + 1.0f) + p1 * (-6.0f * t2 + 6.0f * t) + m1 * (3.0f * t2 - 2.0f * t);
}

void Spline::BuildLengthTable() {
	lengths_.clear();
	const int segments = GetSegmentCount();
	if (segments == 0) {
		return;
	}
	const int steps = segments * kStepsPerSegment_;
	lengths_.reserve(static_cast<size_t>(steps) + 1);
	lengths_.push_back(0.0f);
	Vector3 previous = Evaluate(0.0f);
	for (int i = 1; i <= steps; ++i) {
		const Vector3 position = Evaluate(static_cast<float>(i) / kStepsPerSegment_);
		lengths_.push_back(lengths_.back() + (position - previous).Length());
		previous = position;
	}
}

float Spline::GetKeyAtDistance(float distance) const {
	const float length = GetLength();
	if (lengths_.size() < 2 || length <= 0.0f) {
		return 0.0f;
	}
	if (isClosed_) {
		distance = std::fmod(distance, length);
		if (distance < 0.0f) {
			distance += length;
		}
	} else {
		distance = std::clamp(distance, 0.0f, length);
	}
	// 表の中で distance を挟む2つの刻みの間を直線でつなぐ
	const auto upper = std::upper_bound(lengths_.begin(), lengths_.end(), distance);
	const size_t i1 = (std::min)(static_cast<size_t>(upper - lengths_.begin()), lengths_.size() - 1);
	const size_t i0 = (i1 > 0) ? i1 - 1 : 0;
	const float span = lengths_[i1] - lengths_[i0];
	const float f = (span > 1e-6f) ? (distance - lengths_[i0]) / span : 0.0f;
	return (static_cast<float>(i0) + f) / kStepsPerSegment_;
}

Vector3 Spline::GetLocationAtDistance(float distance) const {
	return Evaluate(GetKeyAtDistance(distance));
}

Vector3 Spline::GetDirectionAtDistance(float distance) const {
	const Vector3 derivative = EvaluateDerivative(GetKeyAtDistance(distance));
	const float length = derivative.Length();
	return (length > 1e-6f) ? derivative * (1.0f / length) : Vector3{ 0.0f,0.0f,1.0f };
}

float Spline::GetDistanceAtKey(float key) const {
	if (lengths_.size() < 2) {
		return 0.0f;
	}
	const float scaled = std::clamp(key, 0.0f, static_cast<float>(GetSegmentCount())) * kStepsPerSegment_;
	const size_t i0 = (std::min)(static_cast<size_t>(scaled), lengths_.size() - 2);
	const float f = scaled - static_cast<float>(i0);
	return lengths_[i0] + (lengths_[i0 + 1] - lengths_[i0]) * f;
}

float Spline::FindClosestKey(const Vector3& position) const {
	const int segments = GetSegmentCount();
	if (segments == 0) {
		return 0.0f;
	}
	// 表の刻みでいちばん近い所を探し、前後の刻みの間を3分割で詰める
	const int steps = segments * kStepsPerSegment_;
	int best = 0;
	float bestDistance = 1e30f;
	for (int i = 0; i <= steps; ++i) {
		const Vector3 d = Evaluate(static_cast<float>(i) / kStepsPerSegment_) - position;
		const float distance = Vector3::Dot(d, d);
		if (distance < bestDistance) {
			bestDistance = distance;
			best = i;
		}
	}
	float low = static_cast<float>((std::max)(best - 1, 0)) / kStepsPerSegment_;
	float high = static_cast<float>((std::min)(best + 1, steps)) / kStepsPerSegment_;
	for (int iteration = 0; iteration < 20; ++iteration) {
		const float a = low + (high - low) / 3.0f;
		const float b = high - (high - low) / 3.0f;
		const Vector3 da = Evaluate(a) - position;
		const Vector3 db = Evaluate(b) - position;
		if (Vector3::Dot(da, da) < Vector3::Dot(db, db)) {
			high = b;
		} else {
			low = a;
		}
	}
	return (low + high) * 0.5f;
}
