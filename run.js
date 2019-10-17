const fs = require("fs")
const path = require("path")
const exec = require('child_process').exec
const config = require("./config.json")
const gmodServer = config.srcdsPath
const gmodServerLua = path.resolve(gmodServer, "garrysmod\\lua")

const copyFile = (from, to) => {
    fs.createReadStream(from).pipe(fs.createWriteStream(to))
}

const deleteFolderRecursive = function (path) {
    if (fs.existsSync(path)) {
        fs.readdirSync(path).forEach(function (file, index) {
            var curPath = path + "/" + file;
            if (fs.lstatSync(curPath).isDirectory()) { // recurse
                deleteFolderRecursive(curPath);
            } else { // delete file
                fs.unlinkSync(curPath);
            }
        });
        fs.rmdirSync(path);
    }
};


const pluginPath = path.resolve(gmodServerLua, "bin")
if (fs.existsSync(pluginPath) && fs.statSync(pluginPath).isDirectory()) {
    deleteFolderRecursive(pluginPath)
}
fs.mkdirSync(pluginPath)
copyFile("build/gmsv_subprocess_plugin_win32.dll", path.resolve(pluginPath, "gmsv_subprocess_plugin_win32.dll"))

exec(path.resolve(gmodServer, "srcds.exe -console -game garrysmod +map gm_construct +maxplayers 127 +gamemode sandbox -port 27026 -insecure"), {
    cwd: gmodServer,
})