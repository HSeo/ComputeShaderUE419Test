// Copyright (c) 2018 Hirofumi Seo, M.D. at SCIEMENT, Inc.
// http://www.sciement.com
// http://www.sciement.com/tech-blog/

#include "TestComputeShaderActor.h"

// Sets default values
ATestComputeShaderActor::ATestComputeShaderActor() {
  // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
  PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATestComputeShaderActor::BeginPlay() {
  // Has to be initialized at BeginPlay(), instead of the class's constructor.
  ERHIFeatureLevel::Type shader_feature_level_test = GetWorld()->Scene->GetFeatureLevel();
  UE_LOG(LogTemp, Warning, TEXT("Shader Feature Level: %d"), shader_feature_level_test);
  UE_LOG(LogTemp, Warning, TEXT("Max Shader Feature Level: %d"), GMaxRHIFeatureLevel);

  Super::BeginPlay();
}

void ATestComputeShaderActor::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  // I'm not sure where is the appropriate place to call the following SafeRelease methods.
  // Destructor? EndPlay? BeginDestroy??
  input_positions_buffer_.SafeRelease();
  input_positions_SRV_.SafeRelease();

  input_scalars_buffer_.SafeRelease();
  input_scalars_SRV_.SafeRelease();

  output_buffer_.SafeRelease();
  output_UAV_.SafeRelease();

  Super::EndPlay(EndPlayReason);
}

// Called every frame
void ATestComputeShaderActor::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
}

bool ATestComputeShaderActor::InitializeInputPositions(
  /*  input */const TArray<FVector>& input_positions) {
  if (input_positions.Num() == 0) {
    UE_LOG(LogTemp, Warning, TEXT("Error: input_positions is empty at ATestComputeShaderActor::InitializeInputPosition."));
    return false;
  }
  num_input_ = input_positions.Num();

  // We need to copy TArray to TResourceArray to set RHICreateStructuredBuffer.
  input_positions_RA_.SetNum(num_input_);
  FMemory::Memcpy(input_positions_RA_.GetData(), input_positions.GetData(), sizeof(FVector) * num_input_);

  input_positions_resource_.ResourceArray = &input_positions_RA_;
  // Note: In D3D11StructuredBuffer.cpp, ResourceArray->Discard() function is called, but not discarded??
  input_positions_buffer_ = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector) * num_input_, BUF_ShaderResource, input_positions_resource_);
  input_positions_SRV_ = RHICreateShaderResourceView(input_positions_buffer_);
  return true;
}

bool ATestComputeShaderActor::InitializeInputScalars(
  /*  input */const TArray<float>& input_scalars) {
  if (input_scalars.Num() == 0) {
    UE_LOG(LogTemp, Warning, TEXT("Error: input_scalars is empty at ATestComputeShaderActor::InitializeInputScalar."));
    return false;
  }
  if (input_scalars.Num() != num_input_) {
    UE_LOG(LogTemp, Warning, TEXT("Error: input_scalars and input_positions do not have the same elements at ATestComputeShaderActor::InitializeInputScalar."));
    return false;
  }

  input_scalars_RA_.SetNum(num_input_);
  FMemory::Memcpy(input_scalars_RA_.GetData(), input_scalars.GetData(), sizeof(float) * num_input_);

  input_scalars_resource_.ResourceArray = &input_scalars_RA_;
  input_scalars_buffer_ = RHICreateStructuredBuffer(sizeof(float), sizeof(float) * num_input_, BUF_ShaderResource, input_scalars_resource_);
  input_scalars_SRV_ = RHICreateShaderResourceView(input_scalars_buffer_);
  return true;
}

void ATestComputeShaderActor::InitializeOffsetYZ(
  /*  input */const float y, const float z) {
  offset_.Y = y;
  offset_.Z = z;

  ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(InitializeOffsetYZCommand,
    ATestComputeShaderActor*, compute_shader_actor, this,
    const float, y, y, // The same parameter name seems to have no problem?
    const float, z, z, {
      compute_shader_actor->InitializeOffsetYZ_RenderThread(y, z);
    });
  render_command_fence_.BeginFence();
  render_command_fence_.Wait();
}

void ATestComputeShaderActor::InitializeOffsetYZ_RenderThread(
  /*  input */const float y, const float z) {
  check(IsInRenderingThread());

  // Get global RHI command list
  FRHICommandListImmediate& rhi_command_list = GRHICommandList.GetImmediateCommandList();

  // Get the actual shader instance off the ShaderMap
  TShaderMapRef<FTestComputeShader> test_compute_shader_(shader_map);

  rhi_command_list.SetComputeShader(test_compute_shader_->GetComputeShader());
  test_compute_shader_->SetOffsetYZ(rhi_command_list, y, z);
}

// According to some result, UniformBuffer does not seem to be kept saved even if UniformBuffer_MultiFrame flag is set...
bool ATestComputeShaderActor::Calculate(
  /*  input */const float x,
  /* output */TArray<FVector>& output) {

  if ((num_input_ == 0) || (input_positions_RA_.Num() != input_scalars_RA_.Num())) {
    UE_LOG(LogTemp, Warning, TEXT("Error: input_positions or input_scalars have not been set correctly at ATestComputeShaderActor::Calculate."));
    return false;
  }

  // In this sample code, output_buffer_ has not input values, so what we need here is just the pointer to output_resource_.
  //output_RA_.SetNum(num_input_);
  //output_resource_.ResourceArray = &output_RA_;
  output_buffer_ = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector) * num_input_, BUF_ShaderResource | BUF_UnorderedAccess, output_resource_);
  output_UAV_ = RHICreateUnorderedAccessView(output_buffer_, /* bool bUseUAVCounter */ false, /* bool bAppendBuffer */ false);

  output.SetNum(num_input_);
  offset_.X = x;

  ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(CalculateCommand,
    ATestComputeShaderActor*, compute_shader_actor, this,
    const FVector, offset, offset_,
    const bool, yz_updated, false,
    TArray<FVector>*, output, &output, {
      compute_shader_actor->Calculate_RenderThread(offset, yz_updated, output);
    });
  render_command_fence_.BeginFence();
  render_command_fence_.Wait(); // Waits for pending fence commands to retire.

  UE_LOG(LogTemp, Warning, TEXT("===== Calculate ====="));
  PrintResult(output);
  return true;
}

bool ATestComputeShaderActor::Calculate_YZ_updated(
  /*  input */const float x, const float y, const float z,
  /* output */TArray<FVector>& output) {

  if ((num_input_ == 0) || (input_positions_RA_.Num() != input_scalars_RA_.Num())) {
    UE_LOG(LogTemp, Warning, TEXT("Error: input_positions or input_scalars have not been set correctly at ATestComputeShaderActor::Calculate."));
    return false;
  }

  output_buffer_ = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector) * num_input_, BUF_ShaderResource | BUF_UnorderedAccess, output_resource_);
  output_UAV_ = RHICreateUnorderedAccessView(output_buffer_, /* bool bUseUAVCounter */ false, /* bool bAppendBuffer */ false);

  output.SetNum(num_input_);
  offset_.Set(x, y, z);

  ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(CalculateCommand,
    ATestComputeShaderActor*, compute_shader_actor, this,
    const FVector, offset, offset_,
    const bool, yz_updated, true,
    TArray<FVector>*, output, &output, {
      compute_shader_actor->Calculate_RenderThread(offset, yz_updated, output);
    });
  render_command_fence_.BeginFence();
  render_command_fence_.Wait();

  UE_LOG(LogTemp, Warning, TEXT("===== Calculate_YZ_Updated ====="));
  PrintResult(output);
  return true;
}

void ATestComputeShaderActor::Calculate_RenderThread(
  /*  input */const FVector xyz, const bool yz_updated,
  /* output */TArray<FVector>* output) {
  check(IsInRenderingThread());

  // Get global RHI command list
  FRHICommandListImmediate& rhi_command_list = GRHICommandList.GetImmediateCommandList();

  // Get the actual shader instance off the ShaderMap
  TShaderMapRef<FTestComputeShader> test_compute_shader_(shader_map);

  rhi_command_list.SetComputeShader(test_compute_shader_->GetComputeShader());
  test_compute_shader_->SetOffsetX(rhi_command_list, xyz.X);
  if (yz_updated) {
    test_compute_shader_->SetOffsetYZ(rhi_command_list, xyz.Y, xyz.Z);
  }
  test_compute_shader_->SetInputPosition(rhi_command_list, input_positions_SRV_);
  test_compute_shader_->SetInputScalar(rhi_command_list, input_scalars_SRV_);
  test_compute_shader_->SetOutput(rhi_command_list, output_UAV_);

  DispatchComputeShader(rhi_command_list, *test_compute_shader_, num_input_, 1, 1);

  test_compute_shader_->ClearOutput(rhi_command_list);
  test_compute_shader_->ClearParameters(rhi_command_list);

  const FVector* shader_data = (const FVector*)rhi_command_list.LockStructuredBuffer(output_buffer_, 0, sizeof(FVector) * num_input_, EResourceLockMode::RLM_ReadOnly);
  FMemory::Memcpy(output->GetData(), shader_data, sizeof(FVector) * num_input_);
  // If you would like to get the partial data, (*output)[index] = *(shader_data + index) is more efficient??

  //for (int32 index = 0; index < num_input_; ++index) {
  //  (*output)[index] = *(shader_data + index);
  //}

  rhi_command_list.UnlockStructuredBuffer(output_buffer_);
}

// TResourceArray's values are still alive...
void ATestComputeShaderActor::PrintResult(const TArray<FVector>& output) {
  for (int32 index = 0; index < num_input_; ++index) {
    UE_LOG(LogTemp, Warning, TEXT("(%f, %f, %f) * %f + (%f, %f, %f) = (%f, %f, %f)"),
      input_positions_RA_[index].X, input_positions_RA_[index].Y, input_positions_RA_[index].Z,
      input_scalars_RA_[index],
      offset_.X, offset_.Y, offset_.Z,
      output[index].X, output[index].Y, output[index].Z);
  }
}
