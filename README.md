# STM32 学习仓库

跟随江协科技 STM32 入门教程的练习工程记录（STM32F103C8T6 + 标准外设库，Keil MDK + ARMCC V5）。

## 工程列表

| 文件夹 | 内容 |
|---|---|
| 2-1 STM32工程模板 | 标准库工程模板（Start / Library / User / System / Hardware） |
| 3-1 LED闪烁 | GPIO 输出：点亮 LED |
| 3-2 LED流水灯 | GPIO 输出：LED 流水灯 |
| 3-3 蜂鸣器 | GPIO 输出：有源蜂鸣器 |
| 3-4 按键控制LED | GPIO 输入：按键控制 LED（按键消抖、LED/按键模块封装） |
| 3-5 光敏传感器控制蜂鸣器 | GPIO 输入：光敏传感器 + 蜂鸣器 |
| 4-1 OLED显示屏 | 软件 I2C 驱动 OLED 屏 |
| 5-1 对射式红外传感器计次 | EXTI 外部中断计数 |
| 5-2 旋转编码器计次 | 外部中断 + 编码器计数 |
| 6-1 定时器定时中断 | TIM 定时中断 |
| 6-2 定时器外部时钟 | TIM 外部时钟计数 |
| 6-3 PWM驱动LED呼吸灯 | TIM 输出比较 PWM |
| 6-4 PWM驱动舵机 | TIM PWM 驱动舵机 |
| 6-5 PWM驱动直流电机 | TIM PWM 驱动直流电机 |
| 6-6 输入捕获模式测频率 | TIM 输入捕获测频率 |
| 6-7 PWMI模式测频率占空比 | TIM PWMI 模式测频率与占空比 |
| 6-8 编码器接口测速 | TIM 编码器接口测速 |
| servo-pwm | GCC + Makefile 构建的舵机 PWM 实验（非 Keil 工程） |

## 说明

- Keil 工程直接双击各文件夹内的 `project.uvprojx` 打开
- 编译产物（Objects / Listings 等）已通过 `.gitignore` 过滤
- 所有 Keil 工程均已开启 **C99 Mode**（原因见下方踩坑记录）

## 踩坑记录

### C99 Mode：变量声明不能写在可执行语句后面（2026-10-25）

**现象**：在函数中途声明局部变量（例如把 `GPIO_InitTypeDef GPIO_InitStructure;` 写在"开启时钟"语句之后），Keil 报错：

```
error: #268: declaration may not appear after executable statement in block
```

**原因**：Keil 的 ARM 编译器（ARMCC 5）**默认按 C90 标准编译**。C90 规定一个代码块内的所有变量声明必须集中在块的开头，不能和可执行语句混排。江协教程的代码能把声明写在使用处附近，是因为其工程勾选了 **C99 Mode**（C99 允许任意位置声明变量）。

**解决**：

1. 检查/开启每个工程的 C99：魔术棒（Options for Target）→ **C/C++** 选项卡 → 勾选 **C99 Mode** → OK
2. 新建工程最省事的做法：**从 2-1 工程模板复制**（模板已开启 C99，复制后自动继承全部设置）
3. C90 风格（声明集中在函数开头）在 C99 下依然合法，老工程不需要改代码

**教训**：新建工程第一件事先确认 C99 Mode 是否勾选；否则 C90 模式下必须把所有变量声明集中在函数开头，工作量明显加大，而且和教程代码的书写位置不一致，对照学习时容易乱。

### 函数声明的括号里要写 void（2026-10-25）

**现象**：开启 C99 后编译出现多条警告：

```
warning: #1295-D: Deprecated declaration LED_Init - give arg types
```

**原因**：头文件里写的是 `void LED_Init();`（空括号）。在 C90 中空括号表示"参数未知"（旧的 K&R 风格声明），而在 C99 中空括号表示"无参数"——这种旧式声明在 C99 下被标记为**弃用（deprecated）**，因此编译器给出警告。

**解决**：函数声明显式写 `void`：

```c
void LED_Init(void);    // 推荐写法
void LED_Init();        // C99 下会产生 #1295-D 警告
```

**说明**：

- `#1295-D` 只是警告（warning），不影响编译产物和运行——只要 `0 Error(s)` 就能正常下载运行
- 另一条常见警告 `#1-D: last line of file ends without a newline` 表示文件末尾缺少换行符，同样无害，在文件最后补一个空行即可
- C99 向后兼容 C90：声明集中在开头的旧代码在 C99 下不会报错，只有空括号函数声明这类旧式写法会多出警告

### 新建工程检查清单

1. 从 2-1 模板复制，或手动建工程后确认 **C99 Mode 已勾选**（Options → C/C++）
2. 头文件里的函数声明统一写 `void xxx(void);`
3. 结构体/变量声明可以像教程一样写在第一次使用的位置附近
4. 编译日志检查：`0 Error(s)` 即可；warning 尽量清零
