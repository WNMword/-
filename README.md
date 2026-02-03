# Snake Game (C++)

一个可以在电脑上直接运行的命令行贪吃蛇小游戏，支持 Windows / macOS / Linux。

## 编译

使用 CMake：

```bash
cmake -S . -B build
cmake --build build
```

## 运行

```bash
./build/snake
```

## 操作说明

- W/A/S/D：控制方向
- Q：退出游戏
- 游戏结束后按 K 退出结果页面

## 玩法

吃到 `*` 会变长并加分，撞墙或撞到自己则游戏结束，并显示获得的豆豆数和存活时间（秒）。
