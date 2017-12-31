# HookMidi
本程序可以拦截应用程序的 midiOutOpen 函数调用，使你能够在利用了MIDI功能的程序或游戏自主选择MIDI输出设备。

## 系统要求
Windows Vista 及以后的操作系统，32位或64位均可。需要安装 [Visual C++ 2017 运行库](https://www.visualstudio.com/zh-hans/downloads)。

## 使用方法
`hookmidi <程序路径> <参数>`
如果没有指定路径，程序将会弹出一个选择程序文件的对话框。

### 注意
* 部分较早版本RPGMaker制作的游戏程序实际路径在start.exe所在目录下的一个子目录中（比如game或data中），对于这种程序不应该使用start.exe，而应该使用game\RPG_RT.exe、data\RPG_RT.exe等类似的路径并附带上程序启动的参数，否则hookmidi会拦截不到实际运行的程序。
* 如果提示无法设置Hook请尝试重新启动。

## 使用的第三方库
* [MinHook](https://github.com/TsudaKageyu/minhook)