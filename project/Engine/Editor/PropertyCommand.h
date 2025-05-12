#pragma once
#include "Engine/Editor/ICommand.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Math/Vector/Vector3.h"

template<typename T>
class PropertyCommand : public ICommand {
public:
	using MemberPtr = T Trans::*;

	PropertyCommand(Trans& target, MemberPtr member, const T& oldValue, const T& newValue)
		: target_(target), member_(member), oldValue_(oldValue), newValue_(newValue) {
	}

	void Do() override {
		target_.*member_ = newValue_;
	}

	void UnDo() override {
		target_.*member_ = oldValue_;
	}

	void ReDo() override {
		Do();
	}

private:
	Trans& target_;
	MemberPtr member_;
	T oldValue_;
	T newValue_;
};