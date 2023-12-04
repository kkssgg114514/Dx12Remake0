# Dx12Remake
v1.0.0.0  
1.0  
创建了一个基础Win32窗口  
里面什么都没有  

2.0  
初始化了DX12，做了十几件事  

开启D3D12调试层  
创建设备  
创建围栏，同步CPU和GPU  
获取描述符大小  
设置MSAA抗锯齿属性  
创建命令队列、命令列表、命令分配器  
创建交换链  
创建描述符堆  
创建描述符  
资源转换  
设置视口和裁剪矩形  
设置围栏刷新命令队列  
将命令从列表传至队列  
  
其中围栏比较难，现在的阻塞模型未来可能替换为帧资源模型  
MSAA抗锯齿中DX12的交换链flip不支持  
  
资源转换的理由不清晰  
  
3.0  
浅度重构了代码，完成了一个将RTV渲染成暗红色的工作，不过对于HWND这个对象的处理方式很不好，而且在创建DSV时候要提交DSV状态转换的命令，这时候关闭命令列表会报错，而不关闭命令列表直接提交会导致窗口无响应。  

以后会更加深度的重构代码  
  
3.1  
修改了主文件中退出消息循环的条件，解决了无响应问题，但是DSV创建那里依然有疑问  
  
4.0  
添加了一个计时器  
实现了如下功能  
重置程序时间，以重置执行的一刻为基准进行时间增加  
实现了每帧时间的计算  
实现了开始和停止功能  
实现了获取程序总时间的功能（不包括暂停时间）实际上减去了暂停停止的时间  
  
总结：程序计时器类似打点计时器，但是每台机器有可能有所不同，原理都是通过计算 振荡一次用的时间 * 振荡次数 算得时间  
下一个推送将会重构代码，不做版本的更新，仅算修改，重构后的代码更加贴近参考书  

4.1  
大幅度重构了代码，原有的文件基本完全删除  
新的结构更加有条理，同时消除了WHND对象的全局问题，深度缓冲区问题未知是否解决  
  
5.0  
完成了数据传输的准备工作  
创建了第一个示例，手动填写了需要传输的数据  
完成了顶点数据的传输  
几个工具类的构建，包括上传堆创建  
完成索引数据传输  
索引数据传输用到Blob指针，作为陌生点  
完成常量数据传输  
常量数据是一种需要经常修改的数据，因此直接分配上传堆作为写入的地方，拓展了上传堆工具类  
常量缓冲区描述符和商量缓冲区描述符堆也构建完成  
  
5.1  
添加了根签名表  
这是一种用来规范向着色器输入参数的类  
添加了PSO（流水线状态对象）  
完善了Init和Draw函数  
添加了Update函数，会每帧刷新矩阵  
但是最后并没有显示一个立方体，可能是部分数据没有传输到位  
  
6.0  
上个版本的最终版本保存在本地bug修复文件中  
中途未知的设备下线导致程序无法运行，回滚到最后一个能够运行但是不能正确构建的版本也会遇到设备下线的问题  
为了不妨碍继续学习，使用官方示例进行修改和填充，目前运行良好，除了有部分代码是不需要的  
未来还将结合另一篇专栏进行学习  
这个版本也添加了用鼠标控制立方体旋转和缩放功能  
同时修复了上一个（未能成功实现）的版本中，修改窗口大小就会导致画面变形的bug  
由于大量改错，目前已经彻底理解了每一个步骤的作用，未来某个版本将结合笔记和代码重复实现上个未能实现的版本 已实现  
  
7.0  
完成了常量缓冲区拆分的学习  
并成功学会拆分常量缓冲区  
重点和复杂点在于计算常量缓冲区视图的地址  
先取得常量缓冲区的基址，然后在这个基址上加上CBV堆中CBV元素的偏移地址，就是最终应该创建CBV的地方  
要修改的点比较多  
最基本的常量缓冲区  
要声明两个资源上传堆  
要创建两个CBV堆  
要创建两个CBV  
要分别将两种CB的数据复制到GPU  
构建根签名的时候也要创建两个根参数分别输入两个着色器寄存器  
设置根描述符表，用命令列表设置绑定命令，偏移索引要和CBV堆中的元素地址对应  
最后修改着色器程序  
分别输入b0,b1  
要用GPU运算两个未合并的矩阵  
  
8.0  
添加了程序几何体构造的类  
添加多个几何体构造的代码  
但是几何体颜色不对，未知错误  
  
9.0  
完成了渲染项的封装  
绘制了一个场景（和书上相同）
但是颜色还是不对，不过好像不是很重要，未来多数使用的还是贴图而不是顶点颜色  
主要的运算在物体数组下标和地址偏移的关系上，需要多加注意  
  
10.0  
封装了帧资源  
用来代替FlushCmdQueue  
帧资源里有：  
顶点结构体，常量缓冲结构体，帧资源类，里面有命令分配器，常量缓冲区指针，CPU的围栏值  
然后在构造函数里创建命令分配器和实例化两个常量缓冲区上传堆  
把之前受影响的部分都改成帧资源表达  
最后在Draw上传命令之前，要重置帧资源命令分配器，每个循环会进行帧资源切换，因此命令分配器也要每帧轮换  
运行效果没有变化，但是封装更完整了  
颜色问题已经解决，要注意DirectX使用的是小端序颜色BGRA，改成大端序即可（ARGB）  
  
11.0  
绘制了山峦  
只需构建几何体和构建渲染对象  
用根描述符替换根描述符表  
这样可以不用创建CBV堆和CBV