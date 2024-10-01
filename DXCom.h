#pragma once
#include <Windows.h>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <dxgidebug.h>
#include <vector>
#include <cassert>
#include <chrono>
#include <random>
#include <wrl.h>

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"


#include "MatrixCalculation.h"
#include "MyWindow.h"
#include "DXCommand.h"


const int particleIndex = 400;
const UINT instanceCount_ = 10;

struct D3DResourceLeakChecker
{
	~D3DResourceLeakChecker()
	{
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};



enum BlendMode
{
	// ブレンド無し
	kBlendModeNone,
	// 通常αブレンド
	kBlendModeNormal,
	// 加算
	kBlendModeAdd,
	// 現在
	kBlendModeSubtract,
	// 乗算
	kBlendModeMultiply,
	// スクリーン
	kBlendModeScreen,
};


enum LightMode
{
	kLightNone,
	kLightHalfLambert,
	kLightLambert,
};


class DebugCamera;//今だけ、後から消す


class DXCom
{
public:

	static DXCom* GetInstance();

	void Initialize(MyWin* myWin);

	void SettingTexture();


	void PreDraw();
	void Command();
	void PostEffect();
	void PostDraw();

	void PreModelDraw();

	void CommandExecution();
	void IncreaseDescriptorIndex();
	uint32_t GetDescriptorIndex() const;

	void SetRenderTargets();
	void ClearRenderTarget();
	void ClearDepthBuffer();


	ID3D12Device* GetDevice() const { return device_.Get(); }

	size_t GetBackBufferCount() const { return swapChainDesc_.BufferCount; }

	ID3D12GraphicsCommandList* GetCommandList() const { return command_->GetList(); }

	void UpDate();

	void ReleaseData();

	Matrix4x4 GetView();

	float GetAspect();

	IDxcBlob* CompileShader(const std::wstring& filePath,
		const wchar_t* profile,
		IDxcUtils* dxcUtils,
		IDxcCompiler3* dxcCompiler,
		IDxcIncludeHandler* includeHandler);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible
	);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateOffscreenTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height, D3D12_CLEAR_VALUE color);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);


	/*void Tick();*/

	void SetIsDebugCamera(const bool cameraMode) { isDebugCamera_ = cameraMode; }

	void SetDebugCamera(DebugCamera* instanse);//今だけ、後から消す

private:

	void CreateDevice();

	void CreateCommand();

	void CreateSwapChain();

	void CreateRenderTargets();

	void CreateDepthBuffer();

	void SettingDxcUtil();
	void SettingIncludeHandle();
	void SettingRootSignature();
	void SettingGraphicPipeline();


private:

	DXCom() = default;
	~DXCom() = default;

	D3DResourceLeakChecker leakCheck_;

	MyWin* myWin_;


#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_ = nullptr;
#endif // _DEBUG


	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;


	std::unique_ptr<DXCommand> command_ = nullptr;


	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_ = nullptr;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[3];

	Microsoft::WRL::ComPtr<ID3D12Resource> offscreenrt_ = nullptr;
	D3D12_RENDER_TARGET_VIEW_DESC offscreenrtvDesc_{};
	D3D12_CLEAR_VALUE clearColorValue{};
	D3D12_GPU_DESCRIPTOR_HANDLE offTextureHandle_;
	D3D12_CPU_DESCRIPTOR_HANDLE offTextureHandleCPU_;


	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2] = { nullptr };
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_ = nullptr;
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc_{};



	IDxcUtils* dxcUtils_ = nullptr;
	IDxcCompiler3* dxcCompiler_ = nullptr;
	IDxcIncludeHandler* includeHandler_ = nullptr;
	ID3DBlob* signatureBlob_ = nullptr;
	ID3DBlob* errorBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	
	IDxcBlob* vertexShaderBlob_ = nullptr;
	IDxcBlob* pixelShaderBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;


	ID3DBlob* signatureParticleBlob_ = nullptr;
	ID3DBlob* errorParticleBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureParticle_ = nullptr;
	IDxcBlob* vertexShaderParticleBlob_ = nullptr;
	IDxcBlob* pixelShaderParticleBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateParticle_ = nullptr;


	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = nullptr;
	TransformationMatrix* instancingData = nullptr;


	ID3DBlob* signatureGrayBlob_ = nullptr;
	ID3DBlob* errorGrayBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureGrayscale_ = nullptr;
	IDxcBlob* vertexShaderGrayBlob_ = nullptr;
	IDxcBlob* pixelShaderGrayBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateGary_ = nullptr;
	

	ID3DBlob* signatureMetaBlob_ = nullptr;
	ID3DBlob* errorMetaBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureMetaBall_ = nullptr;
	IDxcBlob* vertexShaderMetaBlob_ = nullptr;
	IDxcBlob* pixelShaderMetaBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateMeta_ = nullptr;


	ID3DBlob* signatureGaussBlob_ = nullptr;
	ID3DBlob* errorGaussBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureGauss_ = nullptr;
	IDxcBlob* vertexShaderGaussBlob_ = nullptr;
	IDxcBlob* pixelShaderGaussBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateGauss_ = nullptr;


	ID3DBlob* signatureNoneBlob_ = nullptr;
	ID3DBlob* errorNoneBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureNone_ = nullptr;
	IDxcBlob* vertexShaderNoneBlob_ = nullptr;
	IDxcBlob* pixelShaderNoneBlob_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateNone_ = nullptr;


	bool isGrayscale_ = true;
	bool isNonePost_ = true;
	bool isMetaBall_ = true;
	bool isGaussian_ = true;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexGrayResource_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexGrayBufferView_{};
	GrayscaleVertex* grayVertexDate_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexGrayResourece_ = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexGrayBufferView_{};
	uint32_t* indexGrayData_ = nullptr;


	uint32_t descriptorIndex_ = 1;


	////三角形１
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	//VertexDate* vertexDate_ = nullptr;

	//Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	//TransformationMatrix* wvpDate_ = nullptr;
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
	//Materials* materialDate_ = nullptr;
	//Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_ = nullptr;
	//DirectionalLight* directionalLightData_ = nullptr;
	//bool useMonsterBall = true;
	//bool isTriangleDraw_ = true;

	////三角形2
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource2_ = nullptr;
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView2_{};
	//VertexData* vertexDate2_ = nullptr;

	//Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource2_ = nullptr;
	//TransformationMatrix* wvpDate2_ = nullptr;
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResource2_ = nullptr;
	//Material* materialDate2_ = nullptr;
	//Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource2_ = nullptr;
	//DirectionalLight* directionalLightData2_ = nullptr;
	//bool useMonsterBall2 = true;

	////パーティクル用三角
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexParticleResource_[particleIndex]{};
	//D3D12_VERTEX_BUFFER_VIEW vertexParticleBufferView_[particleIndex]{};
	//VertexData* vertexParticleDate_[particleIndex]{};
	//Microsoft::WRL::ComPtr<ID3D12Resource> wvpParticleResource_[particleIndex] = { nullptr };
	//
	//TransformationMatrix* wvpParticleDate_[particleIndex] = { nullptr };
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialParticleResource_[particleIndex] = { nullptr };
	//Material* materialParticleDate_[particleIndex] = { nullptr };


	////modelData
	//ModelData modelData_;
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexModelResource_ = nullptr;
	//D3D12_VERTEX_BUFFER_VIEW vertexModelBufferView_{};
	//VertexData* vertexDataModel_ = nullptr;
	//Microsoft::WRL::ComPtr<ID3D12Resource> wvpResourceModel_ = nullptr;
	//TransformationMatrix* wvpDateModel_ = nullptr;
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceModel_ = nullptr;
	//Material* materialDateModel_ = nullptr;
	//Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResourceModel_ = nullptr;
	//DirectionalLight* directionalLightDataModel_ = nullptr;


	//FenceModelData
	/*ModelData fenceModelData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexFenceModelResource_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexFenceModelBufferView_{};
	VertexDate* vertexDataFenceModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResourceFenceModel_ = nullptr;
	TransformationMatrix* wvpDateFenceModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceFenceModel_ = nullptr;
	Materials* materialDateFenceModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResourceFenceModel_ = nullptr;
	DirectionalLight* directionalLightDataFenceModel_ = nullptr;*/


	//SuzanneModel
	/*ModelData suzanneModelData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexSuzanneModelResource_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexSuzanneModelBufferView_{};
	VertexDate* vertexDataSuzanneModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResourceSuzanneModel_ = nullptr;
	TransformationMatrix* wvpDateSuzanneModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSuzanneModel_ = nullptr;
	Materials* materialDateSuzanneModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResourceSuzanneModel_ = nullptr;
	DirectionalLight* directionalLightDataSuzanneModel_ = nullptr;*/


	//MMeshModel
	/*ModelData mMeshModelData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexMMeshModelResource_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexMMeshModelBufferView_{};
	VertexDate* vertexDataMMeshModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResourceMMeshModel_ = nullptr;
	TransformationMatrix* wvpDateMMeshModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceMMeshModel_ = nullptr;
	Materials* materialDateMMeshModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResourceMMeshModel_ = nullptr;
	DirectionalLight* directionalLightDataMMeshModel_ = nullptr;*/


	//PlaneModel
	/*ModelData planeModelData_;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexPlaneModelResource_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexPlaneModelBufferView_{};
	VertexDate* vertexDataPlaneModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResourcePlaneModel_ = nullptr;
	TransformationMatrix* wvpDatePlaneModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourcePlaneModel_ = nullptr;
	Materials* materialDatePlaneModel_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResourcePlaneModel_ = nullptr;
	DirectionalLight* directionalLightDataPlaneModel_ = nullptr;*/

	
	//Sprite 
	/*Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite_{};
	VertexDate* vertexDataSprite_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResoureceSprite = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	uint32_t* indexDataSprite = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatResourceSprite_ = nullptr;
	TransformationMatrix* transformationMatDataSprite_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite_ = nullptr;
	Materials* materialDateSprite_ = nullptr;*/

	//パーティクルflow
	/*Microsoft::WRL::ComPtr<ID3D12Resource> vertexFlowResource_[particleIndex]{};
	VertexDate* vertexFlowDate_[particleIndex]{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexFlowResource_[particleIndex]{};
	D3D12_INDEX_BUFFER_VIEW indexFlowBufferView_{};
	uint32_t* indexFlowData_[particleIndex]{};
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpFlowResource_[particleIndex] = { nullptr };
	TransformationMatrix* wvpFlowDate_[particleIndex] = { nullptr };
	D3D12_VERTEX_BUFFER_VIEW vertexFlowBufferView_;
	Microsoft::WRL::ComPtr<ID3D12Resource> materialFlowResource_;
	Materials* materialFlowDate_;

	std::vector<Particle> particles;
	ParticleDate* particleDate_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> particleDateResource_ = nullptr;
	float setradius_ = 6.0f;*/

	//bool isFluidMode_ = false;

	//Vector2 G_ = { 0.0f,9.8f };
	//int gravityChangeTime_ = 0;
	//float mpi_ = 3.14159274f;

	//float restDens = 300.0f;//静止状態の密度
	//float gasConst = 2000.0f;
	//float h_ = 16.0f;
	//float halfH_ = h_ * 0.5f;
	//float h2_ = h_ * h_;
	//float mass_ = 2.5f;//質量
	//float visc = 150.0f;//粘度
	//float poly6 = 4.0f / (mpi_ * powf(h_, 8));
	//float spikyGrad = -10.0f / (mpi_ * powf(h_, 5));
	//float viscLap = 40.0f / (mpi_ * powf(h_, 5));

	//float restDens2 = 300.0f;//静止状態の密度
	//float mass2_ = 2.0f;//質量
	//float visc2 = 150.0f;//粘度

	//float dt = 0.0007f;

	//int texture_;

	/*float eps = h_;
	float boundDamping = -0.3f;*/


	bool isPlaneAndSprite_ = true;
	bool isPlaneParticle_ = false;
	bool isSphere_ = false;
	bool isFenceModel_ = false;
	bool isSuzanneModel_ = false;
	bool isMMesh_ = false;


	bool isDebugCamera_ = false;


	DebugCamera* debugCamera_ = nullptr;//今だけ、後から消す

	/*Trans transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	Trans transform2{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };*/
	Trans cameraTrans{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} };
	/*Trans transSprite = {{1.0f,1.0f,1.0f,},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}};
	Trans uvTransSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f},
	};
	Trans transformModel{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	
	Trans transformFenceModel_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Trans transformSuzanneModel_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Trans transformMMeshModel_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Trans transformPlaneModel_{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };*/
	/*Grain particles_[instanceCount_];
	const float kDeltatime = 1.0f / 60.0f;

	Trans transformParticle[particleIndex]{};
	Vector3 particleVel[particleIndex]{};
	Vector3 particleRot[particleIndex]{};
	bool isParticleLive[particleIndex]{ false };
	Vector4 particleColor = { 0.0f,0.0f,1.0f,1.0f };
	float colorRandomAdd = 0;
	std::random_device repopSeed;*/

	/*DirectX::ScratchImage mipImages_;
	DirectX::ScratchImage mipImages2_;
	DirectX::ScratchImage fenceMipImages_;
	DirectX::ScratchImage suzanneMipImages_;
	DirectX::ScratchImage planeMipImages_;
	DirectX::ScratchImage mMeshMipImages_;

	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2_;
	Microsoft::WRL::ComPtr<ID3D12Resource> fenceTextureResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> suzanneTextureResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> planeTextureResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> mMeshTextureResource_;

	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource1;
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource2;
	Microsoft::WRL::ComPtr<ID3D12Resource> fenceIntermediateResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> suzanneIntermediateResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> planeIntermediateResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> mMeshIntermediateResource_;


	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU;

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2;
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2;

	D3D12_CPU_DESCRIPTOR_HANDLE fenceTextureSrvHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE fenceTextureSrvHandleGPU_;

	D3D12_CPU_DESCRIPTOR_HANDLE suzanneTextureSrvHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE suzanneTextureSrvHandleGPU_;

	D3D12_CPU_DESCRIPTOR_HANDLE mMeshTextureSrvHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE mMeshTextureSrvHandleGPU_;

	D3D12_CPU_DESCRIPTOR_HANDLE planeTextureSrvHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE planeTextureSrvHandleGPU_;*/

	//D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU_;
	//D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_;

	/*std::chrono::high_resolution_clock::time_point lastTime;
	float deltaTime;
	float elapsedTime;
	int frameCount;
	int frameRate;*/




};
