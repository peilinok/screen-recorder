"use strict";
const EasyRecorder = require('./lib/index')


const recorder = new EasyRecorder();

const speakers = recorder.GetSpeakers();
const mics = recorder.GetMics();
const vencoders = recorder.GetVideoEncoders();

console.log(speakers);
console.log(mics);
console.log(vencoders);

let ret = recorder.Init(
  60,
  20,
  ".\\save.mkv",
  //".\\save.mp4",
  speakers[0].name,
  speakers[0].id,
  mics[0].name,
  mics[0].id,
  vencoders[0].id
  );


console.info('recorder init ret:',ret,recorder.GetErrorStr(ret));


if(ret == 0){

  ret = recorder.Start();

  console.info('start',ret);

  setTimeout(()=>{

    recorder.Stop();
    recorder.Release();
  },10000);
}
