 js-uv
 
 libuv 的 JS 包装库，尝试基于 V8 进行 API 暴露。
 
 如果可行的话，初步预想将JS内运行缓慢的内置函数通过 libuv 包装为异步模式。


 双击vcbuild.bat生成工程

 todo 写一个脚本 自动将最新版本的v8和uv源码下载到deps目录

 2013-03-22
 包出了一个uv_run方法
 测试用例在test/uv_run.js 在编译后的Debug目录运行 js2uv.exe ..\test\uv_run.js (也可直接运行js2uv.exe后敲命令行)
