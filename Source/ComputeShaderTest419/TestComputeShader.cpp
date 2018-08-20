// Copyright (c) 2018 Hirofumi Seo, M.D. at SCIEMENT, Inc.
// http://www.sciement.com
// http://www.sciement.com/tech-blog/

#include "TestComputeShader.h"
#include "ShaderParameterUtils.h" // Necessary for SetShaderValue, SetUniformBufferParameter.
//#include "RHIStaticStates.h"

// xxx of TEXT("xxx") has to be the same variable name of the corresponding global shader.
IMPLEMENT_UNIFORM_BUFFER_STRUCT(OffsetYZ, TEXT("offset_yz"))

FTestComputeShader::FTestComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) {
  offset_x_.Bind(Initializer.ParameterMap, TEXT("test_offset_x"), SPF_Mandatory);
  input_position_.Bind(Initializer.ParameterMap, TEXT("test_input_position"), SPF_Mandatory);
  input_scalar_.Bind(Initializer.ParameterMap, TEXT("test_input_scalar"), SPF_Mandatory);
  output_.Bind(Initializer.ParameterMap, TEXT("test_output"), SPF_Mandatory);
}

bool FTestComputeShader::Serialize(FArchive& Ar) {
  bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
  Ar << offset_x_ << input_position_ << input_scalar_ << output_;
  return bShaderHasOutdatedParameters;
}

void FTestComputeShader::SetOffsetX(FRHICommandList& rhi_command_list, const float offset_x) {
  SetShaderValue(rhi_command_list, GetComputeShader(), offset_x_, offset_x);
}

void FTestComputeShader::SetOffsetYZ(FRHICommandList& rhi_command_list, const float offset_y, const float offset_z) {
  OffsetYZ offset_yz;
  offset_yz.y = offset_y;
  offset_yz.z = offset_z;
  SetUniformBufferParameter(rhi_command_list, GetComputeShader(), GetUniformBufferParameter<OffsetYZ>(),
    TUniformBufferRef<OffsetYZ>:: CreateUniformBufferImmediate(offset_yz, UniformBuffer_MultiFrame));
  // NOTE: The last parameter: "EUniformBufferUsage Usage" has not been used in D3D11UniformBuffer.cpp.
  // NOTE: "r.UniformBufferPooling" is set to 1 (on) by default in ConsoleManager.cpp
  // UniformBuffer_SingleDraw: The uniform buffer is temporary, used for a single draw call then discarded
  // UniformBuffer_MultiFrame: The uniform buffer is used for multiple draw calls, possibly across multiple frames
}

void FTestComputeShader::SetInputPosition(FRHICommandList& rhi_command_list, FShaderResourceViewRHIRef input_position) {
  if (input_position_.IsBound()) {
    rhi_command_list.SetShaderResourceViewParameter(GetComputeShader(), input_position_.GetBaseIndex(), input_position);
  }
}

void FTestComputeShader::SetInputScalar(FRHICommandList& rhi_command_list, FShaderResourceViewRHIRef input_scalar) {
  if (input_scalar_.IsBound()) {
    rhi_command_list.SetShaderResourceViewParameter(GetComputeShader(), input_scalar_.GetBaseIndex(), input_scalar);
  }
}

void FTestComputeShader::SetOutput(FRHICommandList& rhi_command_list, FUnorderedAccessViewRHIParamRef output) {
  if (output_.IsBound()) {
    rhi_command_list.SetUAVParameter(GetComputeShader(), output_.GetBaseIndex(), output);
  }
}

// for StructuredBuffer.
void FTestComputeShader::ClearParameters(FRHICommandList& rhi_command_list) {
  if (input_position_.IsBound()) {
    rhi_command_list.SetShaderResourceViewParameter(GetComputeShader(), input_position_.GetBaseIndex(), FShaderResourceViewRHIParamRef());
  }
  if (input_scalar_.IsBound()) {
    rhi_command_list.SetShaderResourceViewParameter(GetComputeShader(), input_scalar_.GetBaseIndex(), FShaderResourceViewRHIParamRef());
  }
}

// for RWStructuredBuffer.
void FTestComputeShader::ClearOutput(FRHICommandList& rhi_command_list) {
  if (output_.IsBound()) {
    rhi_command_list.SetUAVParameter(GetComputeShader(), output_.GetBaseIndex(), FUnorderedAccessViewRHIParamRef());
  }
}

IMPLEMENT_SHADER_TYPE(, FTestComputeShader, TEXT("/Project/Private/compute_shader_test.usf"), TEXT("CalculateOutput"), SF_Compute);
