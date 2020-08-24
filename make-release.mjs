import fs from "fs";

import {rollup} from "rollup";
import commonjs from "rollup-plugin-commonjs";
import resolve from "rollup-plugin-node-resolve";

(async function main() {

  let pkg = JSON.parse(fs.readFileSync("./package.json", "utf-8"));

  let srcPath = `index.js`;
  let dstPath = `dist`;

  console.log(`Reserving directories..`);
  // reserve directories
  if (!fs.existsSync("dist")) fs.mkdirSync("dist");
  if (!fs.existsSync(dstPath)) fs.mkdirSync(dstPath);

  // create bundle.js
  let bundleName = pkg.name;
  console.log(`Creating bundled distribution file..`);
  let bundle = await rollup({
    input: srcPath,
    format: "cjs",
    external: [
      "fs",
      "path",
      "crypto"
    ],
    plugins: [
      resolve({
        extensions: [".js"],
        preferBuiltins: true
      }),
      commonjs({ }),
    ],
    output: [
      {
				file: pkg.main,
				format: "cjs",
				exports: "named"
			},
			{
				file: pkg.module,
				format: "es"
			}
    ]
  });
  await bundle.write({
    file: `${dstPath}/bundle.js`,
    format: "cjs"
  });

})();
