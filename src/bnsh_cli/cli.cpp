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
  std::map<u32, ConstBuffer> cbufs;
  std::list<Sampler> samplers;
  std::map<GlobalMemoryBase, GlobalMemoryUsage> global;
  std::set<Attribute::Index> inputAttrs;
  std::set<Attribute::Index> outputAttrs;
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

ProgramCode LoadFileProgramCode(std::string& fileName) {
  std::ifstream file(fileName, std::ios::ate | std::ios::binary);
  if (!file.is_open()) throw std::runtime_error("Failed to open file!");

  size_t fileSize = (size_t)file.tellg();

  size_t bufferSize = (fileSize + sizeof(u64) - 1) / sizeof(u64);
  ProgramCode buffer(bufferSize);

  file.seekg(0);
  file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
  file.close();

  u32 magic = reinterpret_cast<u32*>(buffer.data())[0];

  ProgramCode out = buffer;

  // bnsh file, find bytecode section
  if (magic == 0x48534E42) {
    printf("Detected BNSH file\n");
    u32* dataU32 = reinterpret_cast<u32*>(buffer.data());
    u32 dataU32Len = (fileSize + sizeof(u32) - 1) / sizeof(u32);
    u32 byteCodeOffset = 0x0;
    for (int ii = 0; ii < dataU32Len; ++ii) {
      if (dataU32[ii] == 0x12345678) {
        // found bytecode section
        if (byteCodeOffset != 0x0)
          throw std::runtime_error(
              "Unimplemented: Multiple BNSH bytecode sections aren't "
              "unsupported");
        byteCodeOffset = ii * sizeof(u32) + 0x30;
      }
    }
    if (byteCodeOffset == 0x0)
      throw std::runtime_error("Missing BNSH bytecode section");
    u32 programCodeOffset = byteCodeOffset / sizeof(u64);
    printf("Found BNSH bytecode at 0x%X\n", byteCodeOffset);
    out = ProgramCode(
        &buffer[programCodeOffset],
        &buffer[programCodeOffset + bufferSize - programCodeOffset]);
  }

  return out;
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
  DeviceSettings deviceSettings{};
  deviceSettings.IsFloat16Supported = false;
  deviceSettings.IsWarpSizePotentiallyBiggerThanGuest = true;
  deviceSettings.IsFormatlessImageLoadSupported = true;
  deviceSettings.IsNvViewportSwizzleSupported = false;
  deviceSettings.IsKhrUniformBufferStandardLayoutSupported = false;
  deviceSettings.IsExtIndexTypeUint8Supported = false;
  deviceSettings.IsExtDepthRangeUnrestrictedSupported = true;
  deviceSettings.IsExtShaderViewportIndexLayerSupported = true;
  deviceSettings.IsExtTransformFeedbackSupported = false;
  deviceSettings.IsExtCustomBorderColorSupported = false;
  deviceSettings.IsExtExtendedDynamicStateSupported = false;
  return deviceSettings;
}

SPIRVData DecodeShader(std::string fileName,
                       std::vector<u8> customInputVaryings,
                       uint32_t baseBindingIndex) {
  ProgramCode code = LoadFileProgramCode(fileName);

  // extract shader stage
  CommonWord0 commonWord0 = reinterpret_cast<CommonWord0*>(code.data())[0];
  ShaderType stage = ConvertSPHStageToYuzuStage(commonWord0.Stage);

  struct SerializedRegistryInfo registryInfo;
  Registry registry(stage, registryInfo);

  CompilerSettings settings{CompileDepth::FullDecompile};

  ShaderIR shaderIR(code, 10, settings, registry);

  Specialization specialization =
      GetSpecialization(baseBindingIndex, customInputVaryings);

  DeviceSettings deviceSettings = GetDeviceSettings();

  std::vector<u32> spirv = VideoCommon::Shader::Decompile(
      deviceSettings, shaderIR, stage, registry, specialization);

  // IR data
  std::map<u32, ConstBuffer> cbufs = shaderIR.GetConstantBuffers();
  std::list<Sampler> samplers = shaderIR.GetSamplers();
  std::map<GlobalMemoryBase, GlobalMemoryUsage> global =
      shaderIR.GetGlobalMemory();
  std::set<Attribute::Index> inputAttrs = shaderIR.GetInputAttributes();
  std::set<Attribute::Index> outputAttrs = shaderIR.GetOutputAttributes();

  SPIRVData out{};
  out.spirv = spirv;
  out.cbufs = cbufs;
  out.global = global;
  out.samplers = samplers;
  out.inputAttrs = inputAttrs;
  out.outputAttrs = outputAttrs;

  return out;
}

void WriteFileSPIRV(std::string& file_name, std::vector<u32>& data) {
  FILE* pFile;
  pFile = fopen(file_name.c_str(), "w+b");
  fwrite(data.data(), data.size(), sizeof(u32), pFile);
  fclose(pFile);
}

void WriteFileJSON(std::string& file_name, SPIRVData& spirvData) {
  std::string json = "";
  json += "{";
  /*for (const auto& cbuf : spirvData.cbufs) {
    dataCbufs.push_back(cbuf.first);
  }*/
  // write spirv data
  {
    json += "\"spirv\": [";
    for (uint32_t ii = 0; ii < spirvData.spirv.size(); ++ii) {
      json += std::to_string(spirvData.spirv.at(ii));
      if (ii < spirvData.spirv.size() - 1) json += ",";
    }
    json += "]";
  }
  json += ",";
  // write cbufs
  {
    json += "\"cbufs\": [";
    uint32_t counter = 0;
    for (const auto& [index, size] : spirvData.cbufs) {
      json += "{ ";
      json += "\"index\": ";
      json += std::to_string(index);
      json += ", ";
      json += "\"maxOffset\": ";
      json += std::to_string(size.GetMaxOffset());
      json += ", ";
      json += "\"size\": ";
      json += std::to_string(size.GetSize());
      json += " }";
      if (counter++ < spirvData.cbufs.size() - 1) json += ", ";
    }
    json += "]";
  }
  json += ",";
  // write samplers
  {
    json += "\"samplers\": [";
    uint32_t counter = 0;
    for (const auto& sampler : spirvData.samplers) {
      json += "{ ";
      json += "\"index\": ";
      json += std::to_string(sampler.index);
      json += ", ";
      json += "\"offset\": ";
      json += std::to_string(sampler.offset);
      json += ", ";
      json += "\"isShadow\": ";
      json += std::to_string(sampler.is_shadow);
      json += " }";
      if (counter++ < spirvData.samplers.size() - 1) json += ", ";
    }
    json += "]";
  }
  json += ",";
  // write global
  {
    json += "\"global\": [";
    uint32_t counter = 0;
    for (const auto& [base, usage] : spirvData.global) {
      json += "{ ";
      json += "\"index\": ";
      json += std::to_string(base.cbuf_index);
      json += ", ";
      json += "\"offset\": ";
      json += std::to_string(base.cbuf_offset);
      json += " }";
      if (counter++ < spirvData.global.size() - 1) json += ", ";
    }
    json += "]";
  }
  json += ",";
  // input attributes
  {
    json += "\"inputAttributes\": [";
    uint32_t counter = 0;
    for (const auto& attr : spirvData.inputAttrs) {
      json += std::to_string(static_cast<u64>(attr));
      if (counter++ < spirvData.inputAttrs.size() - 1) json += ", ";
    }
    json += "]";
  }
  json += ",";
  // output attributes
  {
    json += "\"outputAttributes\": [";
    uint32_t counter = 0;
    for (const auto& attr : spirvData.outputAttrs) {
      json += std::to_string(static_cast<u64>(attr));
      if (counter++ < spirvData.outputAttrs.size() - 1) json += ", ";
    }
    json += "]";
  }
  json += "}";
  // save json file
  FILE* pFile;
  pFile = fopen(file_name.c_str(), "w+b");
  fwrite(json.c_str(), json.size(), sizeof(char), pFile);
  fclose(pFile);
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
  int EMSCRIPTEN_KEEPALIVE Decode(char* cmd) {
    int argc = 0;
    char *argv[512];
    char *p2 = strtok(cmd, " ");
    while (p2 && argc < 512-1) {
      argv[argc++] = p2;
      p2 = strtok(0, " ");
    }
    argv[argc] = 0;

    std::vector<std::string> args(argv + 1, argv + argc);

    std::string inputName;
    std::string outputName;
    uint32_t baseBindingIndex = 0;
    std::vector<u8> customInputVaryings{};

    bool outputJSON = true;
    for (auto arg = args.begin(); arg != args.end(); ++arg) {
      if (*arg == "-h" || *arg == "--help") {
        printUsage();
        return EXIT_SUCCESS;
      } else if (*arg == "-i" || *arg == "--input") {
        inputName = *(arg + 1);
      } else if (*arg == "-o" || *arg == "--output") {
        outputName = *(arg + 1);
      } else if (*arg == "--json") {
        outputJSON = true;
      } else if (*arg == "--base-binding-index") {
        baseBindingIndex = std::stoi(*(arg + 1), nullptr, 0);
      } else if (*arg == "--input-varyings") {
        std::string arr = (*(arg + 1));
        if (arr[0] != '[' || arr[arr.size() - 1] != ']') {
          printf("Input varyings parse error\n");
          return EXIT_FAILURE;
        }
        // extract varying locations
        char num_buf[8] = {};
        u8 buf_index = 0;
        for (u8 ii = 1; ii < arr.size(); ++ii) {
          if (arr[ii] == ',' || arr[ii] == ']') {
            customInputVaryings.push_back(std::stoi(num_buf, nullptr, 0));
            // reset number buffer
            memset(&num_buf, 0, sizeof(num_buf));
            buf_index = 0;
          } else {
            num_buf[buf_index++] = arr[ii];
          }
        }
      }
    }

    if (!inputName.size() && !outputName.size()) {
      printUsage();
      return EXIT_FAILURE;
    }

    if (inputName.size()) {
      if (outputJSON) {
        SPIRVData spirvData =
            DecodeShader(inputName, customInputVaryings, baseBindingIndex);
        //printf("Successfully decoded\n");
        WriteFileJSON(outputName, spirvData);
      } else if (outputName.size()) {
        SPIRVData spirvData =
            DecodeShader(inputName, customInputVaryings, baseBindingIndex);
        //printf("Successfully decoded\n");
        WriteFileSPIRV(outputName, spirvData.spirv);
      }
    }

    return EXIT_SUCCESS;
  }

}
