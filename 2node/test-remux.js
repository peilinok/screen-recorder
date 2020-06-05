"use strict";
const EasyRecorder = require('./lib/index')


const recorder = new EasyRecorder();

const onProgress = (src,progress,total)=>{
    console.info('on remux progress:',src,progress,total);
};

const onState = (src,state,error) =>{
    console.info('on remux state:',src,state,recorder.GetErrorStr(error));
};

let ret = recorder.RemuxFile(
    ".\\save.mkv",
    ".\\save.mp4",
    onProgress,
    onState
);

console.info('remux file ret:',ret,recorder.GetErrorStr(ret));
