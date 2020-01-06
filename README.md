# screen-recorder
This is a screen recorder by ffmpeg that include desktop、speaker、mircphone

Here is Les's blog:

https://blog.csdn.net/leixiaohua1020/article/details/18893769#comments

## Features

1.Record desktop 、microphone、speaker at the same time

2.Mux to mp4 for all players, video and audio is absolute sync

3.Nodejs addon project

## ToDo

1.Review all codes,seperate encoder 、filter、muxer

2.Add more devices enumer for different platforms

3.Add hardware use

4.Add more capture with DX、DSHOW、WSAPI

5.Add cross-platform support

6.Add more filters for video and audio


## How to use?

use visual studio

just compile it with visual studio(at least vs2015)

use nodejs 

```sh

$ npm install ffmpeg-recorder

# or

$ yarn add ffmpeg-recorder --save

```
