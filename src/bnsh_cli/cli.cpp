#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "common/common_types.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/shader/shader_ir.h"
#include "video_core/shader/spirv_decompiler.h"

#include <emscripten.h>

namespace {

using Tegra::Engines::ShaderType;
using Tegra::Shader::Attribute;

using VideoCommon::Shader::CompileDepth;
using VideoCommon::Shader::CompilerSettings;
using VideoCommon::Shader::ConstBuffer;
using VideoCommon::Shader::DeviceSettings;
using VideoCommon::Shader::GlobalMemoryBase;
using VideoCommon::Shader::GlobalMemoryUsage;
using VideoCommon::Shader::ProgramCode;
using VideoCommon::Shader::Registry;
using VideoCommon::Shader::Sampler;
using VideoCommon::Shader::SerializedRegistryInfo;
using VideoCommon::Shader::ShaderIR;
using VideoCommon::Shader::Specialization;

using Maxwell = Tegra::Engines::Maxwell3D::Regs;

typedef enum : u32 {
  Compute,
  Vertex,
  TesselationControl,
  TesselationEvaluation,
  Geometry,
  Fragment
} ShaderStage;

typedef struct CommonWord0 {
  uint32_t SphType : 5;
  uint32_t Version : 5;
  ShaderStage Stage : 4;
  bool MrtEnable : 1;
  bool KillsPixels : 1;
  bool DoesGlobalStore : 1;
  uint32_t SassVersion : 4;
  uint32_t Reserved : 5;
  bool DoesLoadOrStore : 1;
  bool DoesFp64 : 1;
  uint32_t StreamOutMask : 4;
} CommonWord0;

typedef struct SPIRVData {
  std::vector<u32> spirv;
  std::list<Sampler> samplers;
  std::map<u32, ConstBuffer> constant_buffers;
  std::set<Attribute::Index> input_attributes;
  std::set<Attribute::Index> output_attributes;
} SPIRVData;

ShaderType ConvertSPHStageToYuzuStage(ShaderStage stage) {
  switch (stage) {
    case ShaderStage::Compute:
      return ShaderType::Compute;
    case ShaderStage::Vertex:
      return ShaderType::Vertex;
    case ShaderStage::TesselationControl:
      return ShaderType::TesselationControl;
    case ShaderStage::TesselationEvaluation:
      return ShaderType::TesselationEval;
    case ShaderStage::Geometry:
      return ShaderType::Geometry;
    case ShaderStage::Fragment:
      return ShaderType::Fragment;
  };
  return ShaderType::Compute;
}

Specialization GetSpecialization(uint32_t baseBindingIndex,
                                 std::vector<u8> customInputVaryings) {
  Specialization specialization{};
  specialization.base_binding = baseBindingIndex;
  specialization.custom_input_varyings = customInputVaryings;
  specialization.ndc_minus_one_to_one = true;
  specialization.point_size = 1.0f;
  specialization.shared_memory_size = 0;
  for (std::size_t i = 0; i < Maxwell::NumVertexAttributes; ++i) {
    specialization.enabled_attributes[i] = true;
    specialization.attribute_types[i] = Maxwell::VertexAttribute::Type::Float;
  }
  return specialization;
}

DeviceSettings GetDeviceSettings() {
  DeviceSettings device_settings{};
  device_settings.IsFloat16Supported = false;
  device_settings.IsWarpSizePotentiallyBiggerThanGuest = true;
  device_settings.IsFormatlessImageLoadSupported = true;
  device_settings.IsNvViewportSwizzleSupported = false;
  device_settings.IsKhrUniformBufferStandardLayoutSupported = false;
  device_settings.IsExtIndexTypeUint8Supported = false;
  device_settings.IsExtDepthRangeUnrestrictedSupported = true;
  device_settings.IsExtShaderViewportIndexLayerSupported = true;
  device_settings.IsExtTransformFeedbackSupported = false;
  device_settings.IsExtCustomBorderColorSupported = false;
  device_settings.IsExtExtendedDynamicStateSupported = false;
  return device_settings;
}

std::string GenerateJSON(SPIRVData& spirv_data) {
  std::string json = "";
  json += "{";
  // write constantBuffers
  {
    json += "\"constantBuffers\":[";
    uint32_t counter = 0;
    for (const auto& [index, size] : spirv_data.constant_buffers) {
      json += "{";
      json += "\"index\":";
      json += std::to_string(index);
      json += ",";
      json += "\"maxOffset\":";
      json += std::to_string(size.GetMaxOffset());
      json += ",";
      json += "\"size\":";
      json += std::to_string(size.GetSize());
      json += "}";
      if (counter++ < spirv_data.constant_buffers.size() - 1) json += ",";
    }
    json += "]";
  }
  json += ",";
  // write samplers
  {
    json += "\"samplers\":[";
    uint32_t counter = 0;
    for (const auto& sampler : spirv_data.samplers) {
      json += "{";
      json += "\"index\":";
      json += std::to_string(sampler.index);
      json += ",";
      json += "\"offset\":";
      json += std::to_string(sampler.offset);
      json += ",";
      json += "\"isShadow\":";
      json += std::to_string(sampler.is_shadow);
      json += "}";
      if (counter++ < spirv_data.samplers.size() - 1) json += ",";
    }
    json += "]";
  }
  json += ",";
  // input attributes
  {
    json += "\"inputAttributes\":[";
    uint32_t counter = 0;
    for (const auto& attr : spirv_data.input_attributes) {
      json += std::to_string(static_cast<u64>(attr));
      if (counter++ < spirv_data.input_attributes.size() - 1) json += ",";
    }
    json += "]";
  }
  json += ",";
  // output attributes
  {
    json += "\"outputAttributes\":[";
    uint32_t counter = 0;
    for (const auto& attr : spirv_data.output_attributes) {
      json += std::to_string(static_cast<u64>(attr));
      if (counter++ < spirv_data.output_attributes.size() - 1) json += ",";
    }
    json += "]";
  }
  json += "}";
  json += "\0";
  return json;
}

}  // namespace

void printUsage() {
  fprintf(stdout,
          "Options:\n"
          "  -h, --help            Show this dialog.\n"
          "  -i, --input           Input file.\n"
          "  -o, --output          Output file.\n"
          "Additional Options:\n"
          "  --json                Output file in JSON format.\n"
          "  --base-binding-index  Base binding index.\n"
          "  --input-varyings      Specify custom input varyings.\n");
}

extern "C" {

  const char* EMSCRIPTEN_KEEPALIVE Decode(
    uint32_t len_raw_data, u64* raw_data,
    uint8_t base_binding_index,
    uint32_t len_raw_input_varyings, uint8_t* raw_input_varyings,
    u64* spirv_out
  ) {
    ProgramCode code(raw_data, raw_data + len_raw_data / sizeof(u64));
    std::vector<uint8_t> input_varyings(raw_input_varyings, raw_input_varyings + len_raw_input_varyings);

    // extract shader stage
    CommonWord0 common_word_0 = reinterpret_cast<CommonWord0*>(code.data())[0];
    ShaderType stage = ConvertSPHStageToYuzuStage(common_word_0.Stage);

    struct SerializedRegistryInfo registry_info;
    Registry registry(stage, registry_info);

    CompilerSettings settings{CompileDepth::FullDecompile};

    ShaderIR shader_ir(code, 10, settings, registry);

    Specialization specialization =
        GetSpecialization(base_binding_index, input_varyings);

    DeviceSettings device_settings = GetDeviceSettings();

    std::vector<u32> spirv = VideoCommon::Shader::Decompile(
        device_settings, shader_ir, stage, registry, specialization);

    SPIRVData out_data{};
    out_data.spirv = spirv;
    out_data.samplers = shader_ir.GetSamplers();
    out_data.constant_buffers = shader_ir.GetConstantBuffers();
    out_data.input_attributes = shader_ir.GetInputAttributes();
    out_data.output_attributes = shader_ir.GetOutputAttributes();

    printf("spirv[6]: %i\n", spirv[6]);
    printf("spirv[8]: %i\n", spirv[8]);
    printf("spirv[10]: %i\n", spirv[10]);
    // pos 0 is reserved to contain the spirv data length
    spirv_out[0] = spirv.size();
    // copy spirv data to outer world
    memcpy(&spirv_out[1], spirv.data(), spirv.size() * sizeof(spirv[0]));

    std::string json = GenerateJSON(out_data);
    return json.c_str();
  }

}
