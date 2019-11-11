const recorder = require('../platform/win32/recorder.node')

console.log(recorder);

const speakers = recorder.GetSpeakers();
const mics = recorder.GetMics();

console.log(speakers);
console.log(mics);

const ret = recorder.Init(60,20,"d:\\save.mp4",speakers[0].name,mics[0].name);

let running = false;

if(ret == 0){
  running = true;
  recorder.Start();

  setTimeout(()=>{
    running = false;

    recorder.Stop();
    recorder.Release();
  },10000);
}
