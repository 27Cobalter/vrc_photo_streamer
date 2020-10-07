# vrc_photo_streamer
## これはなに
- 画像をrtspで流してVRCでアルバムとして見るやつ

## 必要なライブラリ
- boost
- opencv4
- gstreamer
- gst-plugins-base
- gst-plugins-good
- gst-plugins-ugly
- gst-plugins-bad
- gst-rtsp-server

## ビルド&実行
```sh
$ mkdir build && cd $_
$ cmake .. -G ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=1
$ ninja
$ ./bin/vrc_photo_streamer
```
