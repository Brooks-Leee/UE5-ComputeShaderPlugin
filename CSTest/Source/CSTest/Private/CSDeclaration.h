

#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"


class FBoxBlurCSManager
{
public:


	static FBoxBlurCSManager& Get() {
		static FBoxBlurCSManager instance;
		return instance;
	}

	void ExecuteBoxBlurCSShader(UTextureRenderTarget2D* OutRenderTarget, uint32 Step, const UTexture2D* InTexture);

	void ExecuteBoxBlurCSShader_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRenderTargetResource* OutRenderTarget, const FTexture* InTexture, uint32 Step);
	
	
private:

	// 私有构造函数和析构函数，防止直接创建和销毁对象
	FBoxBlurCSManager() = default;
	~FBoxBlurCSManager() = default;

	// 禁用复制构造函数和赋值操作符
	FBoxBlurCSManager(const FBoxBlurCSManager&) = delete;
	FBoxBlurCSManager& operator=(const FBoxBlurCSManager&) = delete;

private:
	TRefCountPtr<IPooledRenderTarget> InputRenderTarget = nullptr;
	TRefCountPtr<IPooledRenderTarget> UAVRenderTarget = nullptr;

};
