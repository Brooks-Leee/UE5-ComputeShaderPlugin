

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

	// ˽�й��캯����������������ֱֹ�Ӵ��������ٶ���
	FBoxBlurCSManager() = default;
	~FBoxBlurCSManager() = default;

	// ���ø��ƹ��캯���͸�ֵ������
	FBoxBlurCSManager(const FBoxBlurCSManager&) = delete;
	FBoxBlurCSManager& operator=(const FBoxBlurCSManager&) = delete;

private:
	TRefCountPtr<IPooledRenderTarget> InputRenderTarget = nullptr;
	TRefCountPtr<IPooledRenderTarget> UAVRenderTarget = nullptr;

};
