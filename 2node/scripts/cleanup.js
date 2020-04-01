const rimraf = require('rimraf');
const { logger } = require('just-task');

module.exports = dir => {
  return new Promise((resolve, reject) => {
    rimraf(dir, err => {
      if (err) {
        logger.error(err);
        reject(err);
      }

      resolve();
    });
  });
};
