let Module = {};
let WasmModule = null;
let SpirvBuffer = null;

function initModule() {
  WasmModule = require("./build/src/bnsh_cli/CLI.js");
  Module = {};
  WasmModule(Module);
  SpirvBuffer = Module._malloc(0x4000 * Uint32Array.BYTES_PER_ELEMENT);
};
// create module on creation
initModule();

function mallocBuffer(buffer) {
  let pointer = Module._malloc(buffer.byteLength);
  Module.HEAPU8.set(buffer, pointer);
  return {
    pointer,
    length: buffer.length
  };
};

module.exports.destroy = function() {
  Module = null;
  WasmModule = null;
  SpirvBuffer = null;
};

module.exports.decode = function(data, baseBindingIndex = 0, inputVaryings = []) {
  // module got destroyed, recreate it
  if (!WasmModule) initModule();

  let byteCodeData = null;
  // if input is a bnsh file, try to extract bytecode section
  let dataU32 = new Uint32Array(data.buffer);
  if (dataU32[0] === 0x48534E42) {
    let byteCodeOffset = 0x0;
    for (let ii = 0; ii < dataU32.length; ++ii) {
      // bytecode section starts with 0x12345678
      if (dataU32[ii] == 0x12345678) {
        if (byteCodeOffset != 0x0) {
          throw new Error(`Unimplemented: Multiple BNSH bytecode sections aren't supported`);
        }
        byteCodeOffset = ii * Uint32Array.BYTES_PER_ELEMENT + 0x30;
      }
    }
    if (byteCodeOffset == 0x0) {
      throw new Error(`Missing BNSH bytecode section`);
    }
    byteCodeData = data.subarray(
      byteCodeOffset,
      byteCodeOffset + data.byteLength - byteCodeOffset
    );
  } else {
    byteCodeData = data;
  }

  let dataBuffer = mallocBuffer(byteCodeData);
  let inputVaryingsBuffer = mallocBuffer(inputVaryings);

  let args = [
    // data
    dataBuffer.length, dataBuffer.pointer,
    // base_binding_index
    baseBindingIndex,
    // input_varyings
    inputVaryingsBuffer.length, inputVaryingsBuffer.pointer,
    // spirv destination pointer
    SpirvBuffer
  ];
  let args_signature = [
    "number", "number",
    "number",
    "number", "number",
    "number"
  ];

  let result = Module.ccall("Decode",	"number", args_signature, args);
  // decode json string
  let json = null;
  {
    let stringBuffer = Module.HEAPU8.subarray(result);
    let stringLength = 0x1000;
    for (let ii = 0; ii < 0x1000; ++ii) {
      let cc = stringBuffer[ii];
      if (cc === 0) { stringLength = ii; break; }
    };
    let jsonString = new TextDecoder("utf-8").decode(stringBuffer.subarray(0, stringLength));
    json = JSON.parse(jsonString);
  }

  // copy spirv buffer out of the heap back into js land
  let spirvLength = Module.HEAPU32[(SpirvBuffer + 0x0) >> 2];
  let spirvData = new Uint32Array(spirvLength);
  spirvData.set(
    Module.HEAPU32.subarray((SpirvBuffer + 0x8) >> 2, (SpirvBuffer + 0x8 + spirvLength) >> 2),
    0x0
  );

  Module._free(dataBuffer.pointer);
  Module._free(inputVaryingsBuffer.pointer);

  json.spirv = spirvData;
  return json;
};
/*
let baseBindingIndex = 0;
let inputVaryings = new Uint8Array([1, 2, 3, 4]);
let result = module.exports.decode(data, baseBindingIndex, inputVaryings);
*/