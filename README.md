# bnsh-decoder

A decoder for Nintendo Switch BNSH shaders, based on [Yuzu's](https://github.com/yuzu-emu/yuzu) SPIR-V assembler.
The decoder emits SPIR-V compatible with [WebGPU](https://gpuweb.github.io/gpuweb/), Vulkan and D3D12 (through [spirv-cross](https://github.com/KhronosGroup/SPIRV-Cross)).

In order to extract information such as driver jump tables for bindless textures, reflection data or resource layouts from a BNSH, you can use [this](https://github.com/maierfelix/bnsh-decoder/blob/master/BNSH.ksy) Kaitai format specification.

Also note that at the moment for each texture slot, an individual sampler slot is associated since WebGPU doesn't support combined image samplers.

## Usage

Decoding a BNSH shader results in 2 files:
 - A .json file containing additional information about the resource layout of the shader
 - A .spv file which contains the actual SPIR-V shader code
 
In order to decode a BNSH shader, run for example:
````
bnsh-decoder --input shader.bnsh_fsh --output-json shader.json --output-spirv shader.spv
````

In order to convert the resulting SPIR-V into GLSL, you can use the spirv-cross tool that is part of the binary, for example:
````
spirv-cross shader.spv --output shader.glsl
````

## Run BNSH shaders on desktop:

Games like LGPE use bindless textures in mostly every shader.

In order to run these shaders on desktop hardware, you have to resolve the driver jump table which associates the material textures with their relative textures by their sampler names:
````js
function getConstantBufferBindingIndices(slt) {
  let size = slt.length;
  let out = new Uint8Array(size);
  for (let ii = 0; ii < size; ++ii) out[slt[ii]] = ii;
  return out;
};

let strIndexBuffer = new Uint8Array(2 ** 5);
function getSamplerBindingIndices(str, slt, smp) {
  let size = str.length;
  let out = new Uint8Array(size);
  for (let ii = 0; ii < size; ++ii) strIndexBuffer[slt[ii]] = str[ii];
  for (let ii = 0; ii < size; ++ii) out[ii] = strIndexBuffer[smp[ii]];
  return out;
};
````

## Further notes:

The header of the binary shader section is described [here](http://download.nvidia.com/open-gpu-doc/Shader-Program-Header/1/Shader-Program-Header.html)
