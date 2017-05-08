DesktopDanmaku
==================

Intro
------------------
DesktopDanmaku is a program that allows you to display danmaku on top of other windows.
The program has two parts - frontend and backend. The frontend is implemented in C++ and using only Windows API 
so there is no extra dependency. The backend is implemented in Node.js and can fetch messages 
from WeChat.

Usage
------------------
This program is a terribly implemented one - so you have to modify the code on your own 
to satisfy your own needs.

The protocol between Frontend and Backend is super simple:
```
[length][control char][text]
```
e.g.
```
11 Lorem ipsum
5*dolor
```
The length of the text is represented in raw text(ASCII) and the control char can be <Space>(normal danmaku) 
or *(Red top danmaku).