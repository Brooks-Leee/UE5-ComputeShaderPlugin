// Fill out your copyright notice in the Description page of Project Settings.


#include "CSBlueprintFuncLib.h"

void UCSBlueprintFuncLib::ExecuteBoxBlurCS(const UTexture2D* InTex, UTextureRenderTarget2D* OutRT, int Step)
{
	

	FBoxBlurCSManager::Get().ExecuteBoxBlurCSShader(OutRT, Step, InTex);
}
