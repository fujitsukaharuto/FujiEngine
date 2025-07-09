#include "AudioPlayer.h"

#include <cassert>


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
	file.read((char*)&riff, sizeof(riff));

	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	FormatChunk format = {};
	ChunkHeader data;

	bool formatFound = false;
	bool dataFound = false;
	char* pBuffer = nullptr;

	while (file.read((char*)&data, sizeof(data))) {

		if (strncmp(data.id, "fmt ", 4) == 0) {
			assert(data.size >= 16);
			file.read((char*)&format.fmt, 16);


			if (data.size > 16) {
				BYTE* extraBytes = new BYTE[data.size - 16];
				file.read((char*)extraBytes, data.size - 16);
				delete[] extraBytes;
			}
			formatFound = true;

		} else if (strncmp(data.id, "data", 4) == 0) {

			pBuffer = new char[data.size];
			file.read(pBuffer, data.size);
			dataFound = true;

		} else {
			file.seekg(data.size, std::ios_base::cur);
		}

		if (formatFound && dataFound) {
			break;
		}
	}

	file.close();
	assert(formatFound && dataFound);

	SoundData soundData = {};
	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	container_.insert(std::make_pair(filename, soundData));
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
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
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
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();
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
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
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
