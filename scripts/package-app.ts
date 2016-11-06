import * as fs from "fs";
import * as path from "path";

export function packageApp(callback:() => void) {
  let files = {};
  for(let file of fs.readdirSync("examples/")) {
    if(path.extname(file) === ".eve") {
      files[file] = fs.readFileSync(path.join("examples", file)).toString();
    }
  }

  const file = fs.readFileSync("edit/app.eve").toString();

  fs.writeFileSync("build/app.js", `var app = ${JSON.stringify(file)}`)

  callback();
}

if(require.main === module) {
  console.log("Packaging...")
  packageApp(() => console.log("done."));
}
