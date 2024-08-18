#pragma once
#include <fstream>
#include <cassert>
#include <xaudio2.h>
#include <wrl.h>


#pragma comment(lib,"xaudio2.lib")


struct ChunkHeader
{
	char id[4];
	int32_t size;
};


struct RiffHeader
{
	ChunkHeader chunk;
	char type[4];
};


struct FormatChunk
{
	ChunkHeader chunk;
	WAVEFORMATEX fmt;
};


struct SoundData
{
	WAVEFORMATEX wfex;
	// バッファの先頭アドレス
	BYTE* pBuffer;
	unsigned int bufferSize;
};


class Audio
{
public:
	Audio();
	~Audio();


	void Initialize();

	void Finalize();

	static Audio* GetInstance();

	SoundData SoundLoadWave(const char* filename);

	void SoundUnload(SoundData* soundData);

	void SoundPlayWave(const SoundData& soundData);

private:

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_;

};