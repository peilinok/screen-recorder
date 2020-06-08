# screen-recorder
This is a screen recorder by ffmpeg that include desktop、speaker、mircphone

Here is Les's blog:

https://blog.csdn.net/leixiaohua1020/article/details/18893769#comments

## Features

- Record desktop 、microphone、speaker at the same time

- Mux to mp4 for all players, video and audio is absolute sync

- Nodejs addon project

- Support Duplication and gdi desktop grabber in windows

- Use wasapi to record microphone & speaker in windows

- YUV data preview callback

- Remux file from one format to another

- NVIDIA encode

- Auto select Duplication API or GDI to get desktop pictures on windows

## ToDo

- Review all codes,seperate encoder 、filter、muxer

- Add more devices enumer for different platforms

- Add hardware use(amf,qsv,libavi)

- Add cross-platform support(for linux and mac)

- Add more filters for video and audio


## How to use?

use visual studio

just compile it with visual studio(at least vs2015)

use nodejs 

```sh

$ npm install ffmpeg-recorder

# or

$ yarn add ffmpeg-recorder --save

```
