const {logger} = require('just-task');
const path = require('path');
const shell = require("shelljs");


module.exports = ({
  platform=process.platform,
  silent = false,
}) => {
  let commandStr = '';
  
  // check platform
  if (platform === 'win32') {
    commandStr = 'xcopy /r /y ".\\build\\Release\\recorder.node" ".\\platform\\win32\\"';
  }
  else if(platform === 'darwin') {
    commandStr = 'cp ./build/Release/recorder.node ./platform/mac/recorder.node';
  }

 
  /** start deploy */
  logger.info(commandStr, '\n');
  
  shell.exec(commandStr, {silent}, (code, stdout, stderr) => {
    // handle error
    if (code !== 0) {
      logger.error(stderr);
      process.exit(1)
    }

    // handle success
    logger.info('Deploy complete')
    process.exit(0)  
  })
}