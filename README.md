# screen-recorder
this is a screen recorder by ffmpeg that include desktop、speaker、mircphone

首先感谢雷神的无私奉献，也是出于对其致敬，完全公开此录屏代码并持续更新.

https://blog.csdn.net/leixiaohua1020/article/details/18893769#comments

版本功能:

1.录制桌面图像、系统声音以及指定麦克风声音

2.最长53分钟450MB，且声音画面完全同步支持各中播放器

Features:

1.重构整体组织，抽取mp4 muxer,对各个模块进行解耦

2.增加设备识别与枚举

3.增加硬件使用

4.增加更多数据捕获方式DX、DSHOW、WSAPI等

5.跨平台适配，增加对MAC、LINUX系统的支持

6.增加更多filter对视频、音频进行预处理

