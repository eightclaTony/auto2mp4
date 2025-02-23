
一个基于C++和FFmpeg开发的自动化视频转换工具，支持实时监控文件夹并将AVI/MKV文件转换为MP4格式，支持NVIDIA硬件加速。


**使用之前确保你已安装[ffmpeg](https://github.com/BtbN/FFmpeg-Builds/releases)**

**FFmpeg路径应加入系统PATH环境变量**

# 应用截图

![image](https://github.com/eightclaTony/auto2mp4/blob/master/image.png)


# 功能详解

-  实时监控指定文件夹（60秒轮询间隔）
-  NVIDIA NVENC硬件加速（支持RTX 20/30/40系列）
-  格式支持：AVI → MP4 / MKV → MP4
-  自动维护目录结构
-  详细日志记录（转换状态/耗时/时间戳）
-  断点续传功能（避免重复转换）

不同情况的处理策略：
| 文件类型         | 处理方式                     | 输出结果           |
|------------------|------------------------------|--------------------|
| 有视频+音频+字幕 | 全流转换                     | 完整MP4带字幕      |
| 有视频+音频      | 仅转换音视频                 | 标准MP4无字幕      |
| 有视频+字幕      | 转换视频+字幕（生成无声视频） | 带字幕的无声MP4    |
| 纯视频           | 转换视频流                   | 无声MP4            |

# 使用方法

1.下载适合你的release文件

2.在下载目录打开cmd或powershell

3.执行命令
如果您没有nvidia显卡,请使用2mp4.exe

如果您拥有nvidia显卡且已正确安装依赖,请使用n_2mp4.exe

有关nvidia显卡的依赖安装,请参阅[安装依赖](#使用nvidia硬件加速)

对于n_2mp4.exe
```
n_2mp4 .         监控当前目录
n_2mp4 folder    监控指定目录
```

对于2mp4.exe
```
2mp4 .         监控当前目录
2mp4 folde     监控指定目录
```

大功告成！

*tips:如果你想，你还可以将其设置为计划任务*

## 文件结构示例
```
源文件夹/
├── video1.avi
├── subdir/
│   └── video2.mkv
└── 2mp4 output/
    ├── conversion.log        # 转换日志
    ├── processed_files.txt   # 已处理文件记录
    ├── ffmpeg_log.txt        # 已处理文件记录
    ├── video1.mp4
    └── subdir/
        └── video2.mp4
```



# 使用nvidia硬件加速！

## 安装依赖
[在官网安装cuda toolkit](https://developer.nvidia.com/cuda-downloads)
