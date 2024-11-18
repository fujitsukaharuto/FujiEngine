//#include "My.h"
//#include "MyWindow.h"
//#include "DirectXCom.h"
//
//#include <cassert>
//#include <string>
//#include <format>
//
//namespace {
//
//	void Log(const std::string& message)
//	{
//		OutputDebugStringA(message.c_str());
//	}
//
//	std::wstring ConvertString(const std::string& str) {
//		if (str.empty()) {
//			return std::wstring();
//		}
//
//		auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
//		if (sizeNeeded == 0) {
//			return std::wstring();
//		}
//		std::wstring result(sizeNeeded, 0);
//		MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
//		return result;
//	}
//
//	std::string ConvertString(const std::wstring& str) {
//		if (str.empty()) {
//			return std::string();
//		}
//
//		auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
//		if (sizeNeeded == 0) {
//			return std::string();
//		}
//		std::string result(sizeNeeded, 0);
//		WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
//		return result;
//	}
//
//	class MySystem
//	{
//		friend class My;
//
//	public:
//		MySystem() = default;
//		~MySystem() = default;
//
//		void Init();
//		void Reset();
//
//	private:
//		MySystem(const MySystem&) = delete;
//		MySystem& operator=(const MySystem&) = delete;
//
//		int ProcessMessage();
//		HWND GetWindowHandle();
//
//		MyWin* myWin_ = nullptr;
//		DirectXCom* dXCom_ = nullptr;
//
//	};
//
//	void MySystem::Init()
//	{
//		dXCom_ = DirectXCom::GetInstance();
//		myWin_ = myWin_ = MyWin::GetInstance();
//	}
//
//	void MySystem::Reset()
//	{
//	}
//
//	int MySystem::ProcessMessage(){ return myWin_->ProcessMessage() ? 1 : 0; }
//
//	HWND MySystem::GetWindowHandle(){ return myWin_->GetHwnd(); }
//
//	MyWin* sMyWin = nullptr;
//	DirectXCom* sDXCom = nullptr;
//	std::unique_ptr<MySystem> sMySystem = nullptr;
//
//}//namespace
//
//
//void My::Init(const char* title, int width, int height) {
//	assert(!sMyWin);
//	assert(!sDXCom);
//	assert(!sMySystem);
//
//
//	std::string titleLetter = std::string(title);
//	auto&& titleStr = ConvertString(titleLetter);
//	sMyWin = MyWin::GetInstance();
//	sMyWin->CreatGWindow(titleStr.c_str(), width, height);
//
//	sDXCom = DirectXCom::GetInstance();
//	sDXCom->Init(sMyWin, width, height);
//
//	sMySystem = std::make_unique<MySystem>();
//	sMySystem->Init();
//
//	sDXCom->PreDraw();
//	sDXCom->PostDraw();
//}
//
//void My::Fin()
//{
//	sMySystem.reset();
//
//	sMyWin->ThrowAwayWindow();
//}
//
//int My::ProcessMessage() { return sMySystem->ProcessMessage(); }