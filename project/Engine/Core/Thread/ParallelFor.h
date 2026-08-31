#pragma once
#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>
#include <objbase.h>

namespace Core {

	/// <summary>0〜count-1 の処理をワーカースレッドで分担する</summary>
	/// <remarks>body は互いに独立していること。呼び出し元も1本ぶん働くので戻った時点で完了している</remarks>
	template <class Body>
	void ParallelFor(size_t count, Body body) {
		if (count == 0) {
			return;
		}

		unsigned int hardware = std::thread::hardware_concurrency();
		if (hardware == 0) {
			hardware = 1;
		}
		const size_t workers = (std::min)(count, static_cast<size_t>(hardware));

		std::atomic<size_t> next{ 0 };
		auto pump = [&]() {
			for (size_t i = next.fetch_add(1); i < count; i = next.fetch_add(1)) {
				body(i);
			}
		};

		std::vector<std::thread> threads;
		threads.reserve(workers - 1);
		for (size_t i = 0; i + 1 < workers; ++i) {
			threads.emplace_back([&pump]() {
				// WIC を使う body もそのまま渡せるように MTA で初期化する
				const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
				pump();
				if (SUCCEEDED(hr)) {
					CoUninitialize();
				}
			});
		}

		pump();

		for (std::thread& thread : threads) {
			thread.join();
		}
	}
}
