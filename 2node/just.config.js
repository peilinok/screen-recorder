const { task, option, logger, argv, series, condition } = require('just-task');
const build = require('./scripts/build');
const { getArgvFromPkgJson } = require('./npm_argv');

option('electron_version', {default: '7.1.4'});
option('runtime', {default: 'node', choices: ['electron', 'node']});
option('platform', {default: process.platform, choices: ['darwin', 'win32']});
option('debug', {default: false, boolean: true});
option('silent', {default: false, boolean: true});
option('msvs_version', {default: '2015'});

const packageVersion = require('./package.json').version;

// npm run build:electron -- 
task('build:electron', () => {
  build({
    electronVersion: argv().electron_version, 
    runtime: 'electron', 
    platform: argv().platform, 
    packageVersion, 
    debug: argv().debug, 
    silent: argv().silent,
    msvsVersion: argv().msvs_version
  })
})
// npm run build:node --
task('build:node', () => {
  build({
    electronVersion: argv().electron_version, 
    runtime: 'node',
    packageVersion,
    platform: argv().platform,
    debug: argv().debug, 
    silent: argv().silent,
    msvsVersion: argv().msvs_version
  })
})
// trigger when run npm install
task('install',()=>{
  logger.info(getArgvFromPkgJson())
  const config = Object.assign({}, getArgvFromPkgJson())
  build(Object.assign({}, config, {
    packageVersion: packageVersion
  }))
})
