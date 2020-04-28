const path = require('path')

module.exports.getArgvFromNpmEnv = () => {
  return {
    electronVersion: process.env.npm_config_agora_electron_dependent,
    prebuilt: process.env.npm_config_agora_electron_prebuilt === 'false' ? false : true ,
    platform: process.env.npm_config_agora_electron_platform,
    msvsVersion: process.env.npm_config_agora_electron_msvs_version,
    debug: process.env.npm_config_agora_electron_debug === 'true',
    silent: process.env.npm_config_agora_electron_silent === 'true',
  }
}

module.exports.getArgvFromPkgJson = () => {
  const projectDir = path.join(process.env.INIT_CWD, 'package.json')
  const pkgMeta = require(projectDir);
  if (pkgMeta.agora_electron) {
    return {
      electronVersion: pkgMeta.agora_electron.electron_version,
      prebuilt: pkgMeta.agora_electron.prebuilt === false ? false : true,
      platform: pkgMeta.agora_electron.platform,
      msvsVersion: pkgMeta.agora_electron.msvs_version,
      debug: pkgMeta.agora_electron.debug === true,
      silent: pkgMeta.agora_electron.silent === true,
    }
  } else {
    return {
      prebuilt: true
    }
  }
}