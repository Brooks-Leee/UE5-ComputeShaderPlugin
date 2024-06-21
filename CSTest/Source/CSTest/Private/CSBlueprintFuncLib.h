// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CSDeclaration.h"
#include "CSBlueprintFuncLib.generated.h"

/**
 * 
 */
UCLASS()
class UCSBlueprintFuncLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "CS FuncLib")
	static void ExecuteBoxBlurCS(const UTexture2D* InTex, UTextureRenderTarget2D* OutRT, int Step);

};
