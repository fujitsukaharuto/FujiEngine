#include "AudioPlayer.h"
#include <cassert>

using namespace Audio;


AudioPlayer::AudioPlayer() {
}

AudioPlayer::~AudioPlayer() {
}

AudioPlayer* AudioPlayer::GetInstance() {
	static AudioPlayer instance;
	return &instance;
}

void AudioPlayer::Initialize() {
	XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	xAudio2_->CreateMasteringVoice(&masterVoice_);
}

void AudioPlayer::Finalize() {
	if (masterVoice_) {
		masterVoice_->DestroyVoice();
		masterVoice_ = nullptr;
	}

	if (container_.size() > 0) {
		container_.clear();
	}

	if (xAudio2_) {
		xAudio2_.Reset();
		xAudio2_ = nullptr;
	}
}

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

	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	FormatChunk format{};
	ChunkHeader chunk{};

	bool formatFound = false;
	bool dataFound = false;

	SoundData soundData{};

	while (file.read(reinterpret_cast<char*>(&chunk), sizeof(chunk))) {

		if (strncmp(chunk.id, "fmt ", 4) == 0) {
			assert(chunk.size >= 16);
			file.read(reinterpret_cast<char*>(&format.fmt), 16);


			if (chunk.size > 16) {
				file.seekg(chunk.size - 16, std::ios_base::cur);
			}
			formatFound = true;

		} else if (strncmp(chunk.id, "data", 4) == 0) {

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

void AudioPlayer::SoundUnload(SoundData* soundData) {
	if (!soundData) return;

	soundData->buffer.clear();
	soundData->pSourceVoices.clear();
	soundData->wfex = {};
}

void AudioPlayer::SoundPlayWave(SoundData& soundData, float volume) {
	HRESULT result;
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

void AudioPlayer::SoundLoop(SoundData& soundData, float volume) {
	HRESULT result;
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
