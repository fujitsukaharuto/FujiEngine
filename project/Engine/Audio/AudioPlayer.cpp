#include "AudioPlayer.h"
#include <cassert>

using namespace Audio;

namespace WaveConst {
	constexpr uint32_t kChunkIdSize = 4;
	constexpr uint32_t kRiffFmtSizePCM = 16;
}

/// <summary>
/// シングルトンのため外部からは生成できない
/// </summary>
AudioPlayer::AudioPlayer() {
}

/// <summary>
/// 解放はFinalizeで行うためここでは何もしない
/// </summary>
AudioPlayer::~AudioPlayer() {
}

/// <summary>
/// インスタンスの取得
/// </summary>
/// <returns>AudioPlayer*</returns>
AudioPlayer* AudioPlayer::GetInstance() {
	static AudioPlayer instance;
	return &instance;
}

/// <summary>
/// XAudio2エンジンとマスターボイスを作成する
/// </summary>
void AudioPlayer::Initialize() {
	XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	xAudio2_->CreateMasteringVoice(&masterVoice_);
}

/// <summary>
/// 終了処理、再生中のボイスと読み込み済みの音声データをすべて破棄する
/// </summary>
void AudioPlayer::Finalize() {
	for (auto& [name, sound] : container_) {
		for (auto* voice : sound.pSourceVoices) {
			if (voice) {
				voice->Stop();
				voice->FlushSourceBuffers();
				voice->DestroyVoice();
			}
		}
		sound.pSourceVoices.clear();
		sound.buffer.clear();
	}

	container_.clear();

	// MasterVoice
	if (masterVoice_) {
		masterVoice_->DestroyVoice();
		masterVoice_ = nullptr;
	}

	// XAudio2
	if (xAudio2_) {
		xAudio2_.Reset();
		xAudio2_ = nullptr;
	}
}

/// <summary>
/// Waveファイルを読み込んでコンテナへ登録する
/// </summary>
/// <param name="filename">ファイル名</param>
/// <remarks>読み込み済みなら何もしない、fmtとdataのチャンクが揃っていない場合はassertで止まる</remarks>
void AudioPlayer::LoadWave(const char* filename) {
	auto it = container_.find(filename);
	if (it != container_.end()) {
		return;
	}

	std::ifstream file;
	file.open((kDirectoryPath_ + filename), std::ios_base::binary);
	assert(file.is_open());

	RiffHeader riff;
	file.read(reinterpret_cast<char*>(&riff), sizeof(riff));

	// RIFFかチェック
	if (strncmp(riff.chunk.id, "RIFF", WaveConst::kChunkIdSize) != 0) {
		assert(0);
	}
	// WAVEかチェック
	if (strncmp(riff.type, "WAVE", WaveConst::kChunkIdSize) != 0) {
		assert(0);
	}

	FormatChunk format{};
	ChunkHeader chunk{};

	bool formatFound = false;
	bool dataFound = false;

	SoundData soundData{};

	// 各チャンクを順番に読み取る
	while (file.read(reinterpret_cast<char*>(&chunk), sizeof(chunk))) {

		if (strncmp(chunk.id, "fmt ", WaveConst::kChunkIdSize) == 0) {
			assert(chunk.size >= WaveConst::kRiffFmtSizePCM);
			file.read(reinterpret_cast<char*>(&format.fmt), WaveConst::kRiffFmtSizePCM);


			if (chunk.size > WaveConst::kRiffFmtSizePCM) {
				file.seekg(chunk.size - WaveConst::kRiffFmtSizePCM, std::ios_base::cur);
			}
			formatFound = true;

		} else if (strncmp(chunk.id, "data", WaveConst::kChunkIdSize) == 0) {

			soundData.buffer.resize(chunk.size);
			file.read(reinterpret_cast<char*>(soundData.buffer.data()), chunk.size);
			dataFound = true;

		} else {
			file.seekg(chunk.size, std::ios_base::cur);
		}

		if (formatFound && dataFound) {
			break;
		}
	}

	file.close();
	assert(formatFound && dataFound);

	soundData.wfex = format.fmt;
	container_.insert(std::make_pair(filename, std::move(soundData)));
}

/// <summary>
/// Waveファイルを読み込んでSoundDataを取得する
/// </summary>
/// <param name="filename">ファイル名</param>
/// <returns>SoundData&</returns>
/// <remarks>読み込み済みならコンテナ内のものをそのまま返す</remarks>
SoundData& AudioPlayer::SoundLoadWave(const char* filename) {
	auto it = container_.find(filename);
	if (it != container_.end()) {
		return it->second;
	}

	LoadWave(filename);

	it = container_.find(filename);
	assert(it != container_.end()); // データが必ず存在するか
	return it->second;
}

/// <summary>
/// SoundDataの中身を空にする
/// </summary>
/// <param name="soundData">音声データ</param>
void AudioPlayer::SoundUnload(SoundData* soundData) {
	if (!soundData) return;

	soundData->buffer.clear();
	soundData->pSourceVoices.clear();
	soundData->wfex = {};
}

/// <summary>
/// 音を1回だけ再生する
/// </summary>
/// <param name="soundData">音声データ</param>
/// <param name="volume">音量</param>
/// <remarks>再生ごとにソースボイスを作るので、多重再生できる</remarks>
void AudioPlayer::SoundPlayWave(SoundData& soundData, float volume) {
	HRESULT result;
	// 冒頭などで掃除を行う
	SoundResourceCleaning(soundData);

	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	result = pSourceVoice->SetVolume(volume);
	assert(SUCCEEDED(result));

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.buffer.data();
	buf.AudioBytes = buf.AudioBytes = static_cast<UINT32>(soundData.buffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;

	pSourceVoice->SubmitSourceBuffer(&buf);
	pSourceVoice->Start();
	soundData.pSourceVoices.push_back(pSourceVoice);
}

/// <summary>
/// 音をループ再生する
/// </summary>
/// <param name="soundData">音声データ</param>
/// <param name="volume">音量</param>
/// <remarks>止めるまで鳴り続けるのでSoundStopWaveで停止する</remarks>
void AudioPlayer::SoundLoop(SoundData& soundData, float volume) {
	HRESULT result;
	SoundResourceCleaning(soundData);

	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	result = pSourceVoice->SetVolume(volume);
	assert(SUCCEEDED(result));

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.buffer.data();
	buf.AudioBytes = static_cast<UINT32>(soundData.buffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;
	buf.LoopCount = XAUDIO2_LOOP_INFINITE; // ループする

	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();
	soundData.pSourceVoices.push_back(pSourceVoice);
}

/// <summary>
/// 再生中のソースボイスをすべて止めて破棄する
/// </summary>
/// <param name="soundData">音声データ</param>
void AudioPlayer::SoundStopWave(SoundData& soundData) {
	for (auto& voice : soundData.pSourceVoices) {
		if (voice) {
			voice->Stop();
			voice->FlushSourceBuffers();
			voice->DestroyVoice();
		}
	}
	soundData.pSourceVoices.clear();
}

/// <summary>
/// 再生の終わったソースボイスを破棄してリストから取り除く
/// </summary>
/// <param name="soundData">音声データ</param>
/// <remarks>再生のたびに呼ぶことでボイスが溜まり続けるのを防いでいる</remarks>
void Audio::AudioPlayer::SoundResourceCleaning(SoundData& soundData) {
	for (auto it = soundData.pSourceVoices.begin(); it != soundData.pSourceVoices.end(); ) {
		XAUDIO2_VOICE_STATE state;
		(*it)->GetState(&state);
		// バッファキューが空 = 再生終わってる
		if (state.BuffersQueued == 0) {
			(*it)->DestroyVoice();
			it = soundData.pSourceVoices.erase(it);
		} else {
			++it;
		}
	}
}
