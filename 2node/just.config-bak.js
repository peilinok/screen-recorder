const { task, option, logger, argv, series, condition } = require('just-task');
const path = require('path');
const shell = require('shelljs');

const cleanup = require('./scripts/cleanup');

const { platform } = process;

const buildWin32 = () => {
  let command = ['node-gyp configure'];
  let copyCmd = '';

  command.push('-arch=ia32');
  copyCmd =
    'xcopy /r /y ".\\build\\Release\\recorder.node" ".\\platform\\win32\\"';

  const commandStr = command.join(' ');

  logger.info(`command str:${commandStr}`);

  shell.exec('node-gyp clean', { silent: true }, (code, stdout, stderror) => {
    if (code !== 0) {
      logger.error(stderror);
      process.exit(1);
    }

    shell.exec(commandStr, { silent: false }, (code, stdout, stderror) => {
      if (code !== 0) {
        logger.error(stderror);
        process.exit(1);
      }

      shell.exec(
        'node-gyp build',
        { silent: false },
        (code, stdout, stderror) => {
          logger.info(`run copy:${copyCmd}`);

          //copy node file to target platform
          shell.exec(copyCmd, { silent: false }, (code, stdout, stderror) => {
            logger.info('copy result:', code, stdout, stderror);

            if (code !== 0) {
              logger.error(stderror);
              process.exit(1);
            }

            logger.info('ffmpeg-recorder build finish');
            process.exit(0);
          }); //copy node file
        }
      ); //node-gyp build
    }); //node-gyp configure
  }); //node-gyp clean
};

const buildDarwin = () => {
  let command = ['node-gyp configure'];
  let copyCmd = '';

  // add more custom options
  command.push(`--target=4.1.3 --dist-url=https://atom.io/download/electron`);
  copyCmd =
    'cp ./build/Release/sunloginsdk.node ./platform/mac/sunloginsdk.node';

  const commandStr = command.join(' ');

  logger.info(`command str:${commandStr}`);

  shell.exec('node-gyp clean', { silent: true }, (code, stdout, stderror) => {
    if (code !== 0) {
      logger.error(stderror);
      process.exit(1);
    }

    cleanup(path.join(__dirname, './platform/mac/ffmpeg-recorder.framework')).then(() => {
      shell.exec(
        'unzip -o ./platform/mac/ffmpeg-recorder.framework.zip -d ./platform/mac/',
        { silent: false },
        (code, stdout, stderror) => {
          shell.exec(
            commandStr,
            { silent: false },
            (code, stdout, stderror) => {
              if (code !== 0) {
                logger.error(stderror);
                process.exit(1);
              }

              shell.exec(
                'node-gyp build',
                { silent: false },
                (code, stdout, stderror) => {
                  logger.info(`run copy:${copyCmd}`);

                  //copy node file to target platform
                  shell.exec(
                    copyCmd,
                    { silent: false },
                    (code, stdout, stderror) => {
                      logger.info('copy result:', code, stdout, stderror);

                      if (code !== 0) {
                        logger.error(stderror);
                        process.exit(1);
                      }

                      //if platform is darwin ,should add loader_path for node file
                      if (platform === 'darwin') {
                        shell.exec(
                          'install_name_tool -add_rpath "@loader_path/" ./platform/mac/sunloginsdk.node',
                          { silent: false },
                          (code, stdout, stderror) => {
                            if (code !== 0) {
                              logger.error(stderror);
                              process.exit(1);
                            }

                            logger.info('ffmpeg-recorder build finish');
                            process.exit(0);
                          }
                        ); //install_name_tool
                      } else {
                        logger.info('ffmpeg-recorder build finish');
                        process.exit(0);
                      }
                    }
                  ); //copy node file
                }
              ); //node-gyp build
            }
          ); //node-gyp configure
        }
      ); //unzip
    });
  });
};

// trigger when run npm install
task('install', () => {
  if (platform === 'win32') return buildWin32();
  //else if (platform === 'darwin') return buildDarwin();
  else {
    logger.error('unsupport platform:' + platform);
    return;
  }
});
