#include "CSDeclaration.h"

#include "GlobalShader.h"

#include "Rendering/Texture2DResource.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "RHIResources.h"
#include "RenderTargetPool.h"
#include "RenderGraphUtils.h"

#define THREADGROUP_SIZEX 8
#define THREADGROUP_SIZEY 8


class FBlurCS : public FGlobalShader
{
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	FBlurCS() = default;
	FBlurCS(const CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{}
};


class FBoxBlurCS : public FBlurCS
{
public:
	DECLARE_GLOBAL_SHADER(FBoxBlurCS);
	SHADER_USE_PARAMETER_STRUCT(FBoxBlurCS, FBlurCS);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, Step)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<float4>, InputTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), THREADGROUP_SIZEX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), THREADGROUP_SIZEY);
	}

};  

IMPLEMENT_GLOBAL_SHADER(FBoxBlurCS, "/Project/Plugins/CSShaders/CSBoxBlur.usf", "BoxBlurCS", SF_Compute);





void FBoxBlurCSManager::ExecuteBoxBlurCSShader(UTextureRenderTarget2D* OutRenderTarget, uint32 Step, const UTexture2D* InTexture)
{
	if (!OutRenderTarget)
	{
		return;
	}

	OutRenderTarget->InitAutoFormat(InTexture->GetResource()->GetSizeX(), InTexture->GetResource()->GetSizeY());
	OutRenderTarget->AddToRoot();
	OutRenderTarget->UpdateResourceImmediate(true);

	// get Render Resource
	FTextureRenderTargetResource* OutRenderTargetResource = OutRenderTarget->GameThread_GetRenderTargetResource();
	const FTexture* InTextureRenderThread = InTexture->GetResource();


	ENQUEUE_RENDER_COMMAND(ExecuteBoxBlurCSShaderCmd)(
		[OutRenderTargetResource, InTextureRenderThread, Step, this](FRHICommandListImmediate& RHICmdList)
		{
			ExecuteBoxBlurCSShader_RenderThread(RHICmdList, OutRenderTargetResource, InTextureRenderThread, Step);
		}
	);

}

void FBoxBlurCSManager::ExecuteBoxBlurCSShader_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRenderTargetResource* OutRenderTargetResource, const FTexture* InTexture, uint32 Step)
{

	check(IsInRenderingThread());


	// create a RDG builder
	FRDGBuilder	GraphBuilder(RHICmdList);

	SCOPED_DRAW_EVENT(GraphBuilder.RHICmdList, BoxBlurCSPass);

	FTextureRHIRef InputTextureRHI = InTexture->TextureRHI;
	FTextureRHIRef OutputTextureRHI = OutRenderTargetResource->GetTextureRHI();

	// create RDG Texture Resource
	FRDGTexture* InputRDGTexture;
	FRDGTexture* UAVRDGTexture;
	FRDGTexture* OutputTexture;

	FIntPoint Size = OutRenderTargetResource->GetSizeXY();

	{
		// only create texture at the first time
		// create and destroy resource is pretty costly
		if (!InputRenderTarget)
		{
			InputRenderTarget = CreateRenderTarget(InputTextureRHI, TEXT("InputTextureAsPooledRenderTarget"));
			// register external Texture to RDG
			//InputRDGTexture = GraphBuilder.RegisterExternalTexture(InputRenderTarget, TEXT("InputTexture"));
		}

		// the RDG reference of the input render target needs to be registered in the RDG builder in every execution
		InputRDGTexture = GraphBuilder.RegisterExternalTexture(InputRenderTarget, TEXT("InputTexture"));

		if (!UAVRenderTarget)
		{

			// create Descriptor
			FPooledRenderTargetDesc OutRTDesc(FPooledRenderTargetDesc::Create2DDesc(
				Size,
				InputTextureRHI->GetFormat(),
				FClearValueBinding::None,
				TexCreate_None,
				TexCreate_ShaderResource | TexCreate_UAV,
				false));

			// check if there is Element in pool, if not Create a new one
			GRenderTargetPool.FindFreeElement(RHICmdList, OutRTDesc, UAVRenderTarget, TEXT("OutputRenderTarget"));
		}

		UAVRDGTexture = GraphBuilder.RegisterExternalTexture(UAVRenderTarget, TEXT("OutPutRenderTarget"));

		OutputTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(OutputTextureRHI, TEXT("OutputTextureAsPooledRenderTarget")), TEXT("OutputTexture"));

	}

	// use RDG AllocParameters() set Parameters
	FBoxBlurCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FBoxBlurCS::FParameters>();
	
	FRDGTextureSRV* InputTextureSRV = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(InputRDGTexture));
	FRDGTextureUAV* OutputTextureUAV = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(UAVRDGTexture));

	PassParameters->Step = Step;
	PassParameters->InputTexture = InputTextureSRV;
	PassParameters->OutputTexture = OutputTextureUAV;

	// get shader ref
	TShaderMapRef<FBoxBlurCS> BoxBlurCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FIntVector DispathSize(
		FMath::DivideAndRoundUp(Size.X, THREADGROUP_SIZEX),
		FMath::DivideAndRoundUp(Size.Y, THREADGROUP_SIZEY),
		1
	);

	// add shader to render graph 
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("ExecuteBoxBlurCS"),
		PassParameters,
		ERDGPassFlags::Compute,
		[PassParameters, BoxBlurCS, DispathSize](FRHICommandList& RHICmdList)
		{
			FComputeShaderUtils::Dispatch(RHICmdList, BoxBlurCS, *PassParameters, DispathSize);
		}
	);

	//RHICmdList.CopyTexture()

	AddCopyTexturePass(GraphBuilder, UAVRDGTexture, OutputTexture, FRHICopyTextureInfo());

	GraphBuilder.Execute();
}
