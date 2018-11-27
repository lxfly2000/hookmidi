# HookMidi
本程序可以拦截应用程序的 midiOutOpen 函数调用，使你能够在利用了MIDI功能的程序或游戏自主选择MIDI输出设备。

## 系统要求
Windows XP SP3 及以后的操作系统，32位或64位均可。需要安装 [Visual C++ 2017 运行库](https://www.visualstudio.com/zh-hans/downloads)。

## 使用方法
选择Hook文件，点击启动；  
或使用命令行：`hookmidi <DLL路径> <Hook回调函数名> <钩子类型>`

### 注意
* 不要强制关闭程序，否则系统会因钩子未释放而引发错误，此时你需要重启系统使功能恢复正常。
* 如果提示无法设置Hook请尝试重新启动程序。

## 使用的第三方库
* [MinHook](https://github.com/TsudaKageyu/minhook)