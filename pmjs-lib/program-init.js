/**
 * @file        program-init.js
 *              Initialize the global context specifically for running JS programs
 */
globalThis.require = require;
globalThis.module = module;
globalThis.exports = exports;

const pmjsPaths = (python.getenv('PMJS_PATH') || '').split(':');
for (let path of pmjsPaths)
  require.path.unshift(path);

exports.setMainFilename = function setMainFilename(filename)
{
  require.cache[filename] = require.cache[module.filename];
  require.cache[filename].filename = __filename = filename;
  delete require.cache[module.filename]
}

