#pragma once
#include <fstream>
#include <xaudio2.h>
#include <vector>
#include <unordered_map>
#include <wrl.h>

#pragma comment(lib,"xaudio2.lib")


struct ChunkHeader {
	char id[4];
	int32_t size;
};

struct RiffHeader {
	ChunkHeader chunk;
	char type[4];
};

struct FormatChunk {
	ChunkHeader chunk;
	WAVEFORMATEX fmt;
};

struct SoundData {
	WAVEFORMATEX wfex;
	// バッファの先頭アドレス
	BYTE* pBuffer;
	unsigned int bufferSize;
	std::vector<IXAudio2SourceVoice*> pSourceVoices;
};


class AudioPlayer {
public:
	AudioPlayer();
	~AudioPlayer();

public:

	/// <summary>
	/// インスタンス取得
	/// </summary>
	/// <returns>AudioPlayer*</returns>
	static AudioPlayer* GetInstance();

	void Initialize();
	void Finalize();

	/// <summary>
	/// Waveファイルを読み込みコンテナに入れておく
	/// </summary>
	/// <param name="filename">ファイル名</param>
	void LoadWave(const char* filename);

	/// <summary>
	/// Waveファイルの読み込みとSoundDataの生成
	/// </summary>
	/// <param name="filename">ファイル名</param>
	/// <returns>SoundData&</returns>
	SoundData& SoundLoadWave(const char* filename);

	/// <summary>
	/// SoundDataのアンロード
	/// </summary>
	/// <param name="soundData">アンロードするSoundData</param>
	void SoundUnload(SoundData* soundData);

	/// <summary>
	/// Waveファイルの再生
	/// </summary>
	/// <param name="soundData">再生するSoundData</param>
	/// <param name="volume">音量(デフォルトは0.2)</param>
	void SoundPlayWave(SoundData& soundData, float volume = 0.2f);

	/// <summary>
	/// Waveファイルのループ再生
	/// </summary>
	/// <param name="soundData">ループ再生するSoundData</param>
	/// <param name="volume">音量(デフォルトは0.2)</param>
	void SoundLoop(SoundData& soundData, float volume = 0.2f);

	/// <summary>
	/// Waveファイルの停止
	/// </summary>
	/// <param name="soundData">停止するSoundData</param>
	void SoundStopWave(SoundData& soundData);

private:

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_;
	std::unordered_map<std::string, SoundData> container_;
	const std::string kDirectoryPath_ = "resource/audio/";

};