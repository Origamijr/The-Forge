#include "../include/Application.h"

//--------------------------------------------------------------------------------------------
// RENDERING PIPELINE DATA
//--------------------------------------------------------------------------------------------
const uint32_t gImageCount = 3;
ProfileToken   gGpuProfileToken;

uint32_t       gFrameIndex = 0;
Renderer* pRenderer = NULL;

Queue* pGraphicsQueue = NULL;
CmdPool* pCmdPool = NULL;
Cmd** ppCmds = NULL;

SwapChain* pSwapChain = NULL;
RenderTarget* pDepthBuffer = NULL;
Fence* pRenderCompleteFences[gImageCount] = { NULL };
Semaphore* pImageAcquiredSemaphore = NULL;
Semaphore* pRenderCompleteSemaphores[gImageCount] = { NULL };

VirtualJoystickUI gVirtualJoystick;

Sampler* pDefaultSampler = NULL;

Shader* pSkyBoxDrawShader = NULL;
Buffer* pSkyBoxVertexBuffer = NULL;
Pipeline* pSkyBoxDrawPipeline = NULL;
Sampler* pSamplerSkyBox = NULL;
Texture* pSkyBoxTextures[6];
DescriptorSet* pDescriptorSetTexture = { NULL };
DescriptorSet* pDescriptorSetUniforms = { NULL };

Shader* pSkeletonShader = NULL;
Buffer* pJointVertexBuffer = NULL;
Buffer* pBoneVertexBuffer = NULL;
Pipeline* pSkeletonPipeline = NULL;
int       gNumberOfJointPoints;
int       gNumberOfBonePoints;

Shader* pPlaneDrawShader = NULL;
Buffer* pPlaneVertexBuffer = NULL;
Pipeline* pPlaneDrawPipeline = NULL;
RootSignature* pRootSignature = NULL;

Shader* pShaderSkinning = NULL;
RootSignature* pRootSignatureSkinning = NULL;
Pipeline* pPipelineSkinning = NULL;

DescriptorSet* pDescriptorSet = NULL;
DescriptorSet* pDescriptorSetSkinning[2] = { NULL };

struct Vertex
{
	float3 mPosition;
	float3 mNormal;
	float2 mUV;
	float4 mBoneWeights;
	uint4  mBoneIndices;
};

struct UniformDataBones
{
	mat4 mBoneMatrix[200];
};

VertexLayout     gVertexLayoutSkinned = {};
Geometry* pGeom = NULL;
Buffer* pUniformBufferBones[gImageCount] = { NULL };
UniformDataBones gUniformDataBones;
Texture* pTextureDiffuse = NULL;

struct UniformBlockPlane
{
	mat4 mProjectView;
	mat4 mToWorldMat;
};
UniformBlockPlane gUniformDataPlane;

Buffer* pPlaneUniformBuffer[gImageCount] = { NULL };

//--------------------------------------------------------------------------------------------
// CAMERA CONTROLLER & SYSTEMS (File/Log/UI)
//--------------------------------------------------------------------------------------------

ICameraController* pCameraController = NULL;
UIApp         gAppUI;
GuiComponent* pStandaloneControlsGUIWindow = NULL;

TextDrawDesc gFrameTimeDraw = TextDrawDesc(0, 0xff00ffff, 18);

//--------------------------------------------------------------------------------------------
// ANIMATION DATA
//--------------------------------------------------------------------------------------------

// AnimatedObjects
AnimatedObject gStickFigureAnimObject;

// Animations
Animation gAnimation;

// ClipControllers
ClipController gClipController;

// Clips
Clip gClip;

// Rigs
Rig gStickFigureRig;

// SkeletonBatcher
SkeletonBatcher gSkeletonBatcher;

// Filenames
const char* gStickFigureName = "stormtrooper/skeleton.ozz";
const char* gClipName = "stormtrooper/animations/dance.ozz";
const char* gDiffuseTexture = "Stormtrooper_D";

const int   gSphereResolution = 30;                   // Increase for higher resolution joint spheres
const float gBoneWidthRatio = 0.2f;                   // Determines how far along the bone to put the max width [0,1]
const float gJointRadius = gBoneWidthRatio * 0.5f;    // set to replicate Ozz skeleton

// Timer to get animationsystem update time
static HiresTimer gAnimationUpdateTimer;

//--------------------------------------------------------------------------------------------
// UI DATA
//--------------------------------------------------------------------------------------------
struct UIData
{
	struct ClipData
	{
		bool* mPlay;
		bool* mLoop;
		float  mAnimationTime;    // will get set by clip controller
		float* mPlaybackSpeed;
	};
	ClipData mClip;

	struct GeneralSettingsData
	{
		bool mShowBindPose = false;
		bool mDrawBones = false;
		bool mDrawPlane = true;
	};
	GeneralSettingsData mGeneralSettings;
};
UIData gUIData;

// Hard set the controller's time ratio via callback when it is set in the UI
void ClipTimeChangeCallback() { gClipController.SetTimeRatioHard(gUIData.mClip.mAnimationTime); }
eastl::vector<mat4> inverseBindMatrices;

const char* pSkyBoxImageFileNames[] = { "skybox/right",  "skybox/left",  "skybox/top",
										"skybox/bottom", "skybox/front", "skybox/back" };

bool Application::Init() {
	// FILE PATHS
	PathHandle programDirectory = fsCopyProgramDirectoryPath();
	if (!fsPlatformUsesBundledResources()) {
		PathHandle resourceDirRoot = fsAppendPathComponent(programDirectory, ".");
		fsSetResourceDirectoryRootPath(resourceDirRoot);

		fsSetRelativePathForResourceDirectory(RD_TEXTURES, "Assets/Textures");
		fsSetRelativePathForResourceDirectory(RD_MESHES, "Assets/Meshes");
		fsSetRelativePathForResourceDirectory(RD_BUILTIN_FONTS, "Assets/Fonts");
		fsSetRelativePathForResourceDirectory(RD_ANIMATIONS, "Assets/Animation");

		fsSetRelativePathForResourceDirectory(RD_MIDDLEWARE_TEXT, "The-Forge/Middleware_3/Text");
		fsSetRelativePathForResourceDirectory(RD_MIDDLEWARE_UI, "The-Forge/Middleware_3/UI");
	}


	// window and renderer setup
	if (!InitRenderer()) return false;

	// Initialize shaders
	if (!InitProgram()) return false;

	// Initialize Objects
	if (!InitObjects()) return false;

	// Initialize Input

	if (!InitUI()) return false;

	return true;
}

bool Application::InitRenderer() {
	RendererDesc settings = { 0 };
	initRenderer(GetName(), &settings, &pRenderer);
	//check for init success
	if (!pRenderer)
		return false;

	QueueDesc queueDesc = {};
	queueDesc.mType = QUEUE_TYPE_GRAPHICS;
	queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
	addQueue(pRenderer, &queueDesc, &pGraphicsQueue);
	CmdPoolDesc cmdPoolDesc = {};

	cmdPoolDesc.pQueue = pGraphicsQueue;
	addCmdPool(pRenderer, &cmdPoolDesc, &pCmdPool);
	CmdDesc cmdDesc = {};
	cmdDesc.pPool = pCmdPool;
	addCmd_n(pRenderer, &cmdDesc, gImageCount, &ppCmds);

	for (uint32_t i = 0; i < gImageCount; ++i) {
		addFence(pRenderer, &pRenderCompleteFences[i]);
		addSemaphore(pRenderer, &pRenderCompleteSemaphores[i]);
	}
	addSemaphore(pRenderer, &pImageAcquiredSemaphore);

	return true;
}

bool Application::InitProgram() {
	// Initialize shaders
	ShaderLoadDesc skyShader = {};
	skyShader.mStages[0] = { "skybox.vert", NULL, 0, RD_SHADER_SOURCES };
	skyShader.mStages[1] = { "skybox.frag", NULL, 0, RD_SHADER_SOURCES };
	addShader(pRenderer, &skyShader, &pSkyBoxDrawShader);

	// Initialize samplers
	SamplerDesc defaultSamplerDesc = {
			FILTER_LINEAR, FILTER_LINEAR, MIPMAP_MODE_LINEAR,
			ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT, ADDRESS_MODE_REPEAT
	};
	addSampler(pRenderer, &defaultSamplerDesc, &pDefaultSampler);

	// Create root signatures
	const char* pStaticSamplerNames[] = { "defaultSampler" };
	Sampler* pStaticSamplers[] = { pDefaultSampler };
	Shader* shaders[] = { pSkyBoxDrawShader };
	RootSignatureDesc rootDesc = {};
	rootDesc.mStaticSamplerCount = 1;
	rootDesc.ppStaticSamplerNames = pStaticSamplerNames;
	rootDesc.ppStaticSamplers = pStaticSamplers;
	rootDesc.mShaderCount = 1;
	rootDesc.ppShaders = shaders;
	addRootSignature(pRenderer, &rootDesc, &pRootSignature);

	// Add descriptor sets
	DescriptorSetDesc desc = {};
	desc.pRootSignature = pRootSignature;
	desc.mUpdateFrequency = DESCRIPTOR_UPDATE_FREQ_NONE;
	desc.mMaxSets = 1;
	addDescriptorSet(pRenderer, &desc, &pDescriptorSetTexture);

	desc.mUpdateFrequency = DESCRIPTOR_UPDATE_FREQ_PER_FRAME;
	desc.mMaxSets = gImageCount * 2;
	addDescriptorSet(pRenderer, &desc, &pDescriptorSetUniforms);

	return true;
}

bool Application::InitObjects() {
	// Loads Skybox Textures
	for (int i = 0; i < 6; ++i) {
		PathHandle textureFilePath = fsCopyPathInResourceDirectory(RD_TEXTURES, pSkyBoxImageFileNames[i]);
		TextureLoadDesc textureDesc = {};
		textureDesc.pFilePath = textureFilePath;
		textureDesc.ppTexture = &pSkyBoxTextures[i];
		addResource(&textureDesc, NULL, LOAD_PRIORITY_NORMAL);
	}

	//Generate sky box vertex buffer
	float skyBoxPoints[] = {
		10.0f,  -10.0f, -10.0f, 6.0f,    // -z
		-10.0f, -10.0f, -10.0f, 6.0f,   -10.0f, 10.0f,  -10.0f, 6.0f,   -10.0f, 10.0f,
		-10.0f, 6.0f,   10.0f,  10.0f,  -10.0f, 6.0f,   10.0f,  -10.0f, -10.0f, 6.0f,

		-10.0f, -10.0f, 10.0f,  2.0f,    //-x
		-10.0f, -10.0f, -10.0f, 2.0f,   -10.0f, 10.0f,  -10.0f, 2.0f,   -10.0f, 10.0f,
		-10.0f, 2.0f,   -10.0f, 10.0f,  10.0f,  2.0f,   -10.0f, -10.0f, 10.0f,  2.0f,

		10.0f,  -10.0f, -10.0f, 1.0f,    //+x
		10.0f,  -10.0f, 10.0f,  1.0f,   10.0f,  10.0f,  10.0f,  1.0f,   10.0f,  10.0f,
		10.0f,  1.0f,   10.0f,  10.0f,  -10.0f, 1.0f,   10.0f,  -10.0f, -10.0f, 1.0f,

		-10.0f, -10.0f, 10.0f,  5.0f,    // +z
		-10.0f, 10.0f,  10.0f,  5.0f,   10.0f,  10.0f,  10.0f,  5.0f,   10.0f,  10.0f,
		10.0f,  5.0f,   10.0f,  -10.0f, 10.0f,  5.0f,   -10.0f, -10.0f, 10.0f,  5.0f,

		-10.0f, 10.0f,  -10.0f, 3.0f,    //+y
		10.0f,  10.0f,  -10.0f, 3.0f,   10.0f,  10.0f,  10.0f,  3.0f,   10.0f,  10.0f,
		10.0f,  3.0f,   -10.0f, 10.0f,  10.0f,  3.0f,   -10.0f, 10.0f,  -10.0f, 3.0f,

		10.0f,  -10.0f, 10.0f,  4.0f,    //-y
		10.0f,  -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f,
		-10.0f, 4.0f,   -10.0f, -10.0f, 10.0f,  4.0f,   10.0f,  -10.0f, 10.0f,  4.0f,
	};

	uint64_t       skyBoxDataSize = 4 * 6 * 6 * sizeof(float);
	BufferLoadDesc skyboxVbDesc = {};
	skyboxVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
	skyboxVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
	skyboxVbDesc.mDesc.mSize = skyBoxDataSize;
	skyboxVbDesc.pData = skyBoxPoints;
	skyboxVbDesc.ppBuffer = &pSkyBoxVertexBuffer;
	addResource(&skyboxVbDesc, NULL, LOAD_PRIORITY_NORMAL);

	BufferLoadDesc ubDesc = {};
	ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
	ubDesc.mDesc.mSize = sizeof(UniformBlock);
	ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
	ubDesc.pData = NULL;
	for (uint32_t i = 0; i < gImageCount; ++i)
	{
		ubDesc.ppBuffer = &pProjViewUniformBuffer[i];
		addResource(&ubDesc, NULL, LOAD_PRIORITY_NORMAL);
		ubDesc.ppBuffer = &pSkyboxUniformBuffer[i];
		addResource(&ubDesc, NULL, LOAD_PRIORITY_NORMAL);
	}

	return true;
}

bool Application::InitUI() {
	initResourceLoaderInterface(pRenderer);

	if (!gAppUI.Init(pRenderer))
		return false;

	gAppUI.LoadFont("TitilliumText/TitilliumText-Bold.otf", RD_BUILTIN_FONTS);

	initProfiler();

	gGpuProfileToken = addGpuProfiler(pRenderer, pGraphicsQueue, "GpuProfiler");
}

bool Application::InitInput() {
	if (!gVirtualJoystick.Init(pRenderer, "circlepad", RD_TEXTURES)) {
		LOGF(LogLevel::eERROR, "Could not initialize Virtual Joystick.");
		return false;
	}
}

void Application::Exit()
{
}

bool Application::Load()
{
	return false;
}

void Application::Unload()
{
}

void Application::Update(float deltaTime)
{
}

void Application::Draw()
{
}

bool Application::addSwapChain() {
	SwapChainDesc swapChainDesc = {};
	swapChainDesc.mWindowHandle = pWindow->handle;
	swapChainDesc.mPresentQueueCount = 1;
	swapChainDesc.ppPresentQueues = &pGraphicsQueue;
	swapChainDesc.mWidth = mSettings.mWidth;
	swapChainDesc.mHeight = mSettings.mHeight;
	swapChainDesc.mImageCount = gImageCount;
	swapChainDesc.mColorFormat = getRecommendedSwapchainFormat(true);
	swapChainDesc.mEnableVsync = false;
	::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

	return pSwapChain != NULL;
}

bool Application::addDepthBuffer() {
	// Add depth buffer
	RenderTargetDesc depthRT = {};
	depthRT.mArraySize = 1;
	depthRT.mClearValue.depth = 0.0f;
	depthRT.mClearValue.stencil = 0;
	depthRT.mDepth = 1;
	depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
	depthRT.mHeight = mSettings.mHeight;
	depthRT.mSampleCount = SAMPLE_COUNT_1;
	depthRT.mSampleQuality = 0;
	depthRT.mWidth = mSettings.mWidth;
	depthRT.mFlags = TEXTURE_CREATION_FLAG_ON_TILE;
	addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);

	return pDepthBuffer != NULL;
}
