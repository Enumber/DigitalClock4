#ifndef FORK_VERSION
#define FORK_VERSION

// 这个 fork 自己的版本号——跟上面 main.cpp 里 QApplication::setApplicationVersion
// 设的上游版本（4.7.9，只在 fork 时同步过一次、以后不会跟着上游改）是两回事，
// 不要混用。之前这个 fork 一直没有自己的版本号，ENum Setup 也就没法判断"已装的
// 是不是最新"；从这里开始起 1.0，以后发新版改这一行 + 打同名 tag 就行，
// 跟 BeeBEEP 那份 Version.h 里 BEEBEEP_FORK_VERSION 的做法一样。
const char DIGITALCLOCK4_FORK_VERSION[] = "1.0";
const char DIGITALCLOCK4_FORK_API_LATEST[] = "https://api.github.com/repos/Enumber/DigitalClock4/releases/latest";

#endif // FORK_VERSION
