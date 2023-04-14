# NeSTiNg-GCLadaptive

这个项目主要是在NeSTiNg的基础上增加GCL自适应功能，目前代码还在开发过程中

## 功能描述

1. gatecontroller 里面实现了basetime
使用方式
```
<schedule cycleTime="400us" baseTime="5us"> 
```
## 可能存在的bug
1. gatecontroller 里面实现basetime的方式是在初始化的时候做了basetime的偏移，后面每次更新GCL的时候，basetime就不起作用了。
目前basetime只是简单的做了从0到新时间的偏移（omnet里面仿真时间是从0开始的），没有做成真实的basetime形式（真实basetime是从1970.1.1开始或者从上电时间开始），不过问题应该不大