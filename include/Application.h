#pragma once

// Interfaces
#include "../The-Forge/Common_3/OS/Interfaces/ICameraController.h"
#include "../The-Forge/Common_3/OS/Interfaces/IApp.h"
#include "../The-Forge/Common_3/OS/Interfaces/ILog.h"
#include "../The-Forge/Common_3/OS/Interfaces/IFileSystem.h"
#include "../The-Forge/Common_3/OS/Interfaces/ITime.h"
#include "../The-Forge/Common_3/OS/Interfaces/IProfiler.h"
#include "../The-Forge/Common_3/OS/Interfaces/IInput.h"

// Rendering
#include "../The-Forge/Common_3/Renderer/IRenderer.h"
#include "../The-Forge/Common_3/Renderer/IResourceLoader.h"

// Middleware packages
#include "../The-Forge/Middleware_3/Animation/SkeletonBatcher.h"
#include "../The-Forge/Middleware_3/Animation/AnimatedObject.h"
#include "../The-Forge/Middleware_3/Animation/Animation.h"
#include "../The-Forge/Middleware_3/Animation/Clip.h"
#include "../The-Forge/Middleware_3/Animation/ClipController.h"
#include "../The-Forge/Middleware_3/Animation/Rig.h"

#include "../The-Forge/Middleware_3/UI/AppUI.h"

// tiny stl
#include "../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/vector.h"
#include "../The-Forge/Common_3/ThirdParty/OpenSource/EASTL/string.h"

#include "../The-Forge/Common_3/ThirdParty/OpenSource/cgltf/GLTFLoader.h"

// Math
#include "../The-Forge/Common_3/OS/Math/MathTypes.h"

// Memory
#include "../The-Forge/Common_3/OS/Interfaces/IMemory.h"


class Application : public IApp
{
public:
	bool Init() override;
	bool InitRenderer();
	bool InitProgram();
	bool InitObjects();
	bool InitUI();
	bool InitInput();
	void Exit() override;
	bool Load() override;
	void Unload() override;
	void Update(float deltaTime) override;
	void Draw() override;
	const char* GetName() override { return "Application"; }

	bool addSwapChain();
	bool addDepthBuffer();
};

DEFINE_APPLICATION_MAIN(Application)
