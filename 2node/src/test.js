const recorder = require('../platform/win32/recorder.node')

console.log(recorder);

const speakers = recorder.GetSpeakers();
const mics = recorder.GetMics();

console.log(speakers);
console.log(mics);

const ret = recorder.Init(60,20,".\\save.mp4",speakers[0].name,speakers[0].id,mics[0].name,mics[0].id);

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
