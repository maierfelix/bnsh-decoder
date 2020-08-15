// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include <cstring>
#include <optional>
#include "common/assert.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/shader_type.h"
#include "video_core/gpu.h"
//#include "video_core/rasterizer_interface.h"

namespace Tegra::Engines {

//using VideoCore::QueryType;

/// First register id that is actually a Macro call.
constexpr u32 MacroRegistersStart = 0xE00;

Maxwell3D::Maxwell3D(Core::System& system, VideoCore::RasterizerInterface& rasterizer,
                     MemoryManager& memory_manager)
    : system{system}, rasterizer{rasterizer}, memory_manager{memory_manager}, upload_state{memory_manager, regs.upload} {
    dirty.flags.flip();
}

} // namespace Tegra::Engines
