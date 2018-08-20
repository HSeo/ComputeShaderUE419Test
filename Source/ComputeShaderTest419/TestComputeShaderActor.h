// Copyright (c) 2018 Hirofumi Seo, M.D. at SCIEMENT, Inc.
// http://www.sciement.com
// http://www.sciement.com/tech-blog/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicRHIResourceArray.h" // Core module
#include "RenderCommandFence.h" // RenderCore module
#include "TestComputeShader.h"
#include "TestComputeShaderActor.generated.h"

UCLASS()
class COMPUTESHADERTEST419_API ATestComputeShaderActor : public AActor {
  GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  ATestComputeShaderActor();

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;

  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
  // Called every frame
  virtual void Tick(float DeltaTime) override;

  UFUNCTION(BlueprintCallable, Category = "TestComputeShader")
    bool InitializeInputPositions(
      /*  input */const TArray<FVector>& input_positions);

  UFUNCTION(BlueprintCallable, Category = "TestComputeShader")
    bool InitializeInputScalars(
      /*  input */const TArray<float>& input_scalars);

  UFUNCTION(BlueprintCallable, Category = "TestComputeShader")
    void InitializeOffsetYZ(
      /*  input */const float y, const float z);

  UFUNCTION(BlueprintCallable, Category = "TestComputeShader")
    bool Calculate(
      /*  input */const float x,
      /* output */TArray<FVector>& output);

  UFUNCTION(BlueprintCallable, Category = "TestComputeShader")
    bool Calculate_YZ_updated(
      /*  input */const float x, const float y, const float z,
      /* output */TArray<FVector>& output);


private:
  int32 num_input_ = 0;
  FVector offset_;

  FRenderCommandFence render_command_fence_; // Necessary for waiting until a render command function finishes.
  const TShaderMap<FGlobalShaderType>* shader_map = GetGlobalShaderMap(GMaxRHIFeatureLevel);
  //Note:
  // GetWorld() function cannot be called from constructor, can be called after BeginPlay() instead.
  // So I used GMaxRHIFeatureLevel, instead of GetWorld()->Scene->GetFeatureLevel().
  //const TShaderMap<FTestComputeShader::ShaderMetaType>* shader_map = GetGlobalShaderMap(GetWorld()->Scene->GetFeatureLevel());

  //// Get the actual shader instance off the ShaderMap
  //TShaderMapRef<FTestComputeShader> test_compute_shader_{ shader_map }; // Note: test_compute_shader_(shader_map) causes error.

  TResourceArray<FVector> input_positions_RA_;
  FRHIResourceCreateInfo input_positions_resource_;
  FStructuredBufferRHIRef input_positions_buffer_;
  FShaderResourceViewRHIRef input_positions_SRV_;

  TResourceArray<float> input_scalars_RA_;
  FRHIResourceCreateInfo input_scalars_resource_;
  FStructuredBufferRHIRef input_scalars_buffer_;
  FShaderResourceViewRHIRef input_scalars_SRV_;

  TResourceArray<FVector> output_RA_; // Not necessary.
  FRHIResourceCreateInfo output_resource_;
  FStructuredBufferRHIRef output_buffer_;
  FUnorderedAccessViewRHIRef output_UAV_;

  void InitializeOffsetYZ_RenderThread(
    /*  input */const float y, const float z);

  void Calculate_RenderThread(
    /*  input */const FVector xyz, const bool yz_updated,
    /* output */TArray<FVector>* output);

  void PrintResult(const TArray<FVector>& output);
};
