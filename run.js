const fs = require("fs")
const path = require("path")
const exec = require('child_process').exec

const gmodServer = "C:\\my_garrysmod_server"
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

const luaFiles = fs.readdirSync("./lua/")
const luaNfgsPath = path.resolve(gmodServerLua, "autorun")
if (fs.existsSync(luaNfgsPath) && fs.statSync(luaNfgsPath).isDirectory()) {
    deleteFolderRecursive(luaNfgsPath)
}
fs.mkdirSync(luaNfgsPath)
luaFiles.forEach(x => {
    copyFile(path.resolve("lua", x), path.resolve(luaNfgsPath, x))
})


const pluginPath = path.resolve(gmodServerLua, "bin")
if (fs.existsSync(pluginPath) && fs.statSync(pluginPath).isDirectory()) {
    deleteFolderRecursive(pluginPath)
}
fs.mkdirSync(pluginPath)
copyFile("build/gmsv_example_win32.dll", path.resolve(pluginPath, "gmsv_nfgs_plugin_win32.dll"))

exec(path.resolve(gmodServer, "bat_sandbox.bat"), { cwd: gmodServer })