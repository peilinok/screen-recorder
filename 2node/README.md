# ffmpeg-recorder

This is a node-js addon by screen-recorder which is written by c++ based on ffmpeg.

Only support windows(at least windows7 sp1) for now.


## Features

- Save screen、speaker、miccrophone to a signle file as mp4


## Usage


### 1.Install DirectShow device(Optional After 1.1.1)

https://github.com/rdp/screen-capture-recorder-to-video-windows-free

### 2.Add ffmpeg-recorder addon to your package.json

```sh
$ npm install ffmpeg-recorder

# or

$ yarn add ffmpeg-recorder
```


## Test code

``` sh

const recorder = require('ffmpeg-recorder')

console.log(recorder);

const speakers = recorder.GetSpeakers();
const mics = recorder.GetMics();

console.log(speakers);
console.log(mics);

const ret = recorder.Init(60,20,"d:\\save.mp4",speakers[0].name,speakers[0].id,mics[0].name,mics[0].id);

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



```