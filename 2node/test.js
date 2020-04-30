const EasyRecorder = require('./index')


const recorder = new EasyRecorder();


const onPreviewYuv = function (size,width,height,type,data) {
  console.log('onPreviewYuv',size,width,height,type,Buffer.isBuffer(data),data.length);
  return
}

const speakers = recorder.GetSpeakers();
const mics = recorder.GetMics();

console.log(speakers);
console.log(mics);

let ret = recorder.Init(60,20,".\\save.mp4",speakers[0].name,speakers[0].id,mics[0].name,mics[0].id);
console.info('recorder init ret:',ret);


if(ret == 0){

  ret = recorder.SetPreviewYuvCallBack(onPreviewYuv);
  console.info('SetPreviewYuvCallBack',ret);

  ret = recorder.Start();
  console.info('start',ret);

  setTimeout(()=>{

    recorder.Stop();
    recorder.Release();
  },10000);
}
