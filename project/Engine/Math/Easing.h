#pragma once
#include "Vector2.h"
#include "Vector3.h"

struct Easing {
	float time;
	float maxTime;
	float incrementTime_;
	float amplitude;
	float period;
};

//float Lerp(const float& start, const float& end, float t);
//
//Vector3 Lerp(const Vector3& start, const Vector3& end, float t);
//
//Vector2 Lerp(const Vector2& start, const Vector2& end, float t);
//
//Vector3 SLerp(const Vector3& start, const Vector3& end, float t);

float EaseInElasticAmplitude(float t, const float& totaltime, const float& amplitude, const float& period);

float EaseOutElasticAmplitude(float t, float totaltime, float amplitude, float period);

float EaseInOutElasticAmplitude(float t, float totaltime, float amplitude, float period);

template<typename T> T EaseAmplitudeScale(const T& initScale, const float& easeT, const float& totalTime, const float& amplitude, const float& period);

//*******************************************************************************************************************************************************************
// Sine**************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************

/// <summary>
/// EaseInSine
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInSine(const T& start, const T& end, float x, float totalX);

/// <summary>
/// EaseOutSine
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseOutSine(const T& start, const T& end, float x, float totalX);

/// <summary>
/// EaseInOutSine
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInOutSine(const T& start, const T& end, float x, float totalX);

//*******************************************************************************************************************************************************************
// Quint**************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************

/// <summary>
/// EaseInQuint
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInQuint(const T& start, const T& end, float x, float totalX);

/// <summary>
/// EaseOutQint
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseOutQuint(const T& start, const T& end, float x, float totalX);

/// <summary>
/// EaseInOutQint
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInOutQuint(const T& start, const T& end, float x, float totalX);

//*******************************************************************************************************************************************************************
// Circ**************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
/// <summary>
/// EaseInCirc
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInCirc(const T& start, const T& end, float x, float totalX);
/// <summary>
/// EaseOutCirc
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseOutCirc(const T& start, const T& end, float x, float totalX);
/// <summary>
/// EaseInOutCirc
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInOutCirc(const T& start, const T& end, float x, float totalX);

//*******************************************************************************************************************************************************************
// Expo**************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
/// <summary>
/// EaseInExpo
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInExpo(const T& start, const T& end, float x, float totalX);

/// <summary>
/// EaseOutExpo
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseOutExpo(const T& start, const T& end, float x, float totalX);

/// <summary>
/// EaseInOutExpo
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInOutExpo(const T& start, const T& end, float x, float totalX);

//*******************************************************************************************************************************************************************
// Cubic**************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
/// <summary>
/// EaseOutCubic
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseOutCubic(const T& start, const T& end, float x, float totalx);

/// <summary>
/// EaseInCubic
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInCubic(const T& start, const T& end, float x, float totalx);

/// <summary>
/// EaseInOutCubic
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInOutCubic(const T& start, const T& end, float x, float totalx);

//*******************************************************************************************************************************************************************
// Quad**************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
/// <summary>
/// EaseInQuad
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInQuad(const T& start, const T& end, float x, float totalx);

/// <summary>
/// EaseOutQuad
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseOutQuad(const T& start, const T& end, float x, float totalx);

/// <summary>
/// EaseInOutQuad
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInOutQuad(const T& start, const T& end, float x, float totalx);

//*******************************************************************************************************************************************************************
// Quart**************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************
/// <summary>
/// EaseInQuart
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInQuart(const T& start, const T& end, float x, float totalx);

/// <summary>
/// EaseOutQuart
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseOutQuart(const T& start, const T& end, float x, float totalx);

//*******************************************************************************************************************************************************************
// Bounce**************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************

// バウンス補助関数
float BounceEaseOut(float x);
/// <summary>
/// EaseInBounce
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInBounce(const T& start, const T& end, float x, float totalX);
/// <summary>
/// EaseOutBounce
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseOutBounce(const T& start, const T& end, float x, float totalX);

/// <summary>
/// EaseInOutBounce
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="start"></param>
/// <param name="end"></param>
/// <param name="x"></param>
/// <param name="totalx"></param>
/// <returns></returns>
template<typename T> T EaseInOutBounce(const T& start, const T& end, float x, float totalX);

///// <summary>
///// EaseTimeControl
///// </summary>
///// <typeparam name="T"></typeparam>
///// <param name="t"></param>
///// <param name="totalTime"></param>
///// <param name="start"></param>
///// <param name="end"></param>
// template<typename T> T EaseTimeControl(float& t, const float& totalTime, const T& start, const T& end);
