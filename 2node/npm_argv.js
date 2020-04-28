const path = require('path')

module.exports.getArgvFromPkgJson = () => {
  const projectDir = path.join(process.env.INIT_CWD, 'package.json')
  const pkgMeta = require(projectDir);
  if (pkgMeta.ffmpeg_recorder) {
    return {
      electronVersion: pkgMeta.ffmpeg_recorder.electron_version,
      platform: pkgMeta.ffmpeg_recorder.platform,
      msvsVersion: pkgMeta.ffmpeg_recorder.msvs_version,
      debug: pkgMeta.ffmpeg_recorder.debug === true,
      silent: pkgMeta.ffmpeg_recorder.silent === true,
    }
  } else {
    return {
      prebuilt: true
    }
  }
}