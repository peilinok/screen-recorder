const { task, option, logger, argv, series, condition } = require('just-task');
const build = require('./scripts/build');
const { getArgvFromNpmEnv, getArgvFromPkgJson } = require('./scripts/npm_argv');

option('electron_version', {default: '4.1.3'});
option('runtime', {default: 'electron', choices: ['electron', 'node']});
option('platform', {default: process.platform, choices: ['darwin', 'win32']});
option('debug', {default: false, boolean: true});
option('silent', {default: false, boolean: true});
option('msvs_version', {default: '2015'});

const packageVersion = require('./package.json').version;

task('install',()=>{
  const config = Object.assign({}, getArgvFromNpmEnv(), getArgvFromPkgJson())
  build(Object.assign({}, config, {
    packageVersion: packageVersion
  }))
})
