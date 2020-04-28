const async = require('async')
const EasyRecorder = require('./index')

const recorder = new EasyRecorder();

console.log(recorder);

const speakers = recorder.GetSpeakers();
const mics = recorder.GetMics();
recorder.SetPreviewImageCallBack(()=>{
  console.log('on preview image');
})

console.log(speakers);
console.log(mics);

const ret = recorder.Init(60,20,".\\save.mp4",speakers[0].name,speakers[0].id,mics[0].name,mics[0].id);

console.info('recorder init ret:',ret);

if(ret == 0){
  recorder.Start();
  console.warn('22222')

  recorder.Wait(10000);

  recorder.Stop();
  recorder.Release();  
}

