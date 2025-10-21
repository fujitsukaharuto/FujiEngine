#pragma once



/// <summary>
/// ステイトの基底クラス
/// </summary>
class OriginState {
public:
	OriginState() = default;
	virtual ~OriginState() = default;

	virtual void Initialize() = 0;
	virtual void Update() = 0;

private:

};
