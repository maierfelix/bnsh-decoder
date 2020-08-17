let Module = {};
let WasmModule = null;
let JsonStringBuffer = null;

function initModule() {
  WasmModule = require("./build/src/bnsh_cli/CLI.js");
  Module = {};
  WasmModule(Module);
  SpirvBuffer = mallocBuffer(new Uint8Array(0x8000 * Uint32Array.BYTES_PER_ELEMENT));
  JsonStringBuffer = mallocBuffer(new Uint8Array(0x1000 * Uint8Array.BYTES_PER_ELEMENT));
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
  JsonStringBuffer = null;
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
    SpirvBuffer.pointer,
    // json string destination pointer
    JsonStringBuffer.pointer
  ];
  let args_signature = [
    "number", "number",
    "number",
    "number", "number",
    "number"
  ];

  Module.ccall("Decode",	"number", args_signature, args);
  // decode json string
  let json = null;
  {
    let stringLength = 0x1000;
    let jsonStringMemoryChunk = Module.HEAPU8.subarray(
      JsonStringBuffer.pointer,
      JsonStringBuffer.pointer + stringLength
    );
    for (let ii = 0; ii < stringLength; ++ii) {
      let cc = jsonStringMemoryChunk[ii];
      if (cc === 0) { stringLength = ii; break; }
    };
    let jsonString = new TextDecoder("utf-8").decode(jsonStringMemoryChunk.subarray(0, stringLength));
    jsonStringMemoryChunk.fill(0);
    json = JSON.parse(jsonString);
  }

  // copy spirv buffer out of the heap back into js land
  let spirvLength = json.spirvLength;
  let spirvMemoryChunk = Module.HEAPU32.subarray(
    SpirvBuffer.pointer >> 2,
    (SpirvBuffer.pointer >> 2) + spirvLength
  );

  // make heap copy of spirv
  let spirv = new Uint32Array(spirvLength);
  spirv.set(spirvMemoryChunk, 0x0);

  Module._free(dataBuffer.pointer);
  Module._free(inputVaryingsBuffer.pointer);

  // save spirv copy into json
  json.spirv = spirv;

  return json;
};
