[English](why-linux-still-works.md) · **中文**

关于「Wayland 下不能移动」这句话，到底是什么意思
==============================================

[English](why-linux-still-works.md)

上游作者在 Digital Clock 5 停止维护时，给出的放弃 Linux 的唯一技术理由是
（5.2.2 发布说明）：

> "Linux is not supported anymore (even technically it is possible to build),
> as clock is totally useless in Wayland environments just because it simply
> can't be moved"
>
> （不再支持 Linux——虽然技术上仍然编译得出来——因为在 Wayland 环境下这个时钟
> 完全没用，它根本没法被移动。）

这句话很容易被读成「这程序在 Linux 上不能用」。它并不是这个意思。

先看这个限制到底是什么
----------------------

时钟是一个无边框顶层窗口（`Qt::FramelessWindowHint`，见
`digital_clock/gui/clock_window.cpp`）。拖动是它自己实现的：鼠标事件里算出新
坐标然后调 `move()`；启动时也是用同样的方式把上次保存的位置恢复回去。

而 Wayland 在协议设计上就**不允许客户端知道或指定自己窗口在屏幕上的位置**——
协议里根本没有全局坐标这个概念。所以在 Wayland 下，顶层窗口的 `move()` 是空
操作：时钟出现在合成器决定的位置，你拖不动它，它也记不住你上次放在哪。作者这
部分说的是对的。

但这个限制不适用于 X11
----------------------

**在 X11 下，上面这些统统不成立。** `move()` 正常工作，拖动正常，位置也能在
下次启动时恢复。X11 至今仍是不少桌面的默认会话，GNOME 和 KDE 也都还保留 X11
登录选项。查自己在用哪个：

```sh
echo $XDG_SESSION_TYPE     # 输出 x11 或 wayland
```

如果输出是 `x11`，这个时钟的行为和 Windows 上完全一致，包括拖动摆放。本仓库
的构建实测过：窗口能用鼠标拖动，重启后位置保持不变。

就算在 Wayland 下，那也叫「退化」，不叫「没用」
--------------------------------------------

即使限制确实生效，「不能移动」和「完全没用」也不是一回事。一个固定在某处的
桌面时钟仍然是时钟——它照样显示时间、照样能换皮肤、闹钟／倒计时／日程这些插件
照样能用。这笔交易值不值，是关于「时钟是拿来干什么的」的判断，这个判断权在你。
所以本 fork 保留 Linux 支持，只是把限制在这里讲清楚。

如果你在 Wayland 下又希望位置能固定，现实的办法有两个：换成 X11 会话登录；或者
让这个时钟跑在 XWayland 上（多数 Wayland 合成器仍然支持运行 X11 程序，走
XWayland 时上面那套 X11 定位逻辑是有效的）。
