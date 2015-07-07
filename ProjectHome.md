**xlview** is a handy image viewer just can replace the default windows image viewer. You can double-click an image (jpg, png) in explorer to start xlview after installed.


## Features ##
  * Very small(the size of the exe is less than 700KB), and fast
  * Support **mouse gesture** (configurable)
    * if you don't know what is mouse gesture, see [here](http://en.wikipedia.org/wiki/Pointing_device_gesture)(from wikipedia)
  * Easy to use, easy to navigate between the images


## Download ##
  * [Download the latest version](http://code.google.com/p/xlview/downloads/detail?name=xlview-setup_r145.exe&can=2&q=) (installer)



## Screenshot ##
[![](http://xlview.googlecode.com/svn/trunk/screenshot/xlview-navigating-tn.jpg)](http://code.google.com/p/xlview/wiki/Screenshot)




## To compile (for developer) ##
  * VS2010 - because some C++0x features such as lambda and auto are used
  * libxl( http://code.google.com/p/libxl ) - the support library
  * TortoiseSVN - SubWCRev is used to generate the version info


---

Below is in Chinese
以下为中文

**xlview** 是一个很方便的，适合用来替换 windows 自带图片浏览器的软件，安装以后，在文件管理器中双击某个图片文件，就可以启动它了。


## 特点 ##
  * 非常小（exe 只有600多K），因为小，所以快
  * 支持鼠标手势 （可以配置）
    * 鼠标手势参见 [这里](http://zh.wikipedia.org/zh-cn/%E9%BC%A0%E6%A0%87%E6%89%8B%E5%8A%BF)
  * 天生就是为了纯看图方便而设计的，略熟悉了鼠标手势和快捷键以后，用起来可以做到行云流水，毫无滞碍


## 下载 ##
  * [下载最新的安装程式](http://code.google.com/p/xlview/downloads/detail?name=xlview-setup_r145.exe&can=2&q=)


## 屏幕截图 ##
[![](http://xlview.googlecode.com/svn/trunk/screenshot/xlview-navigating-tn.jpg)](http://code.google.com/p/xlview/wiki/Screenshot)


## 我要自己编译 ##
自己编译需要下面的东东：
  * VS2010 - 为了装B，我用了部分 C++0x 的功能，比如 lambda 等，所以只能在 VS2010 及其之后的版本上编译
  * libxl( http://code.google.com/p/libxl ) - 用以提供支撑的库
  * WTL ( http://wtl.sourceforge.net/ ) - libxl 的 UI 部分，尽管是流行的“无窗口”设计，但主窗口还是基于 WTL 完成的
  * TortoiseSVN - 使用了 SubWCRev.exe 来生成它的版本号