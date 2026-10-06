r"""
舵机 PWM 在线扫动测试
===================================================================
用来区分两类完全不同的故障：

  A. 芯片/固件问题      -> 这个脚本本来就不该出问题（寄存器已验证过）
  B. 接线/供电/共地问题  -> 脚本改了脉宽，舵机还是纹丝不动

原理
-------------------------------------------------------------------
通过调试器直接改 STM32 里的 TIM2->CCR1 寄存器，让 PA0 输出的脉宽
在两个值之间跳。固件的主循环跑完启动流程后就不再动 CCR1 了，
所以这里写入的值会被一直保持。

判读
-------------------------------------------------------------------
  舵机跟着来回摆        -> PWM 通路、共地、供电 全都正常。
                           之前"没转"是因为它本来就在 0 度附近。
  舵机完全纹丝不动      -> 信号没到舵机。按顺序查：
                            1. 独立电源的 GND 和板子 GND 有没有连起来（最常见）
                            2. 信号线是不是真的插在 PA0 上
                            3. 独立电源到底有没有输出 5V
                            4. 舵机插头有没有插反（棕=GND 红=VCC 橙=信号）

用法
-------------------------------------------------------------------
    powershell -ExecutionPolicy Bypass -File sweep.ps1
    powershell -ExecutionPolicy Bypass -File sweep.ps1 -From 1000 -To 1500 -Hold 2 -Cycles 5
"""

import argparse
import sys
import time

from pyocd.core.helpers import ConnectHelper

TARGET = 'stm32f103c8'
FREQ = 1_000_000
TIM2_CCR1 = 0x40000034

# 调试器读回来的日志别来捣乱
import logging
logging.getLogger('pyocd').setLevel(logging.ERROR)


def angle_of(us):
    """把 1000~2000us 换算成 0~180 度，方便肉眼对上"""
    return (us - 1000) * 180.0 / 1000.0


def main():
    ap = argparse.ArgumentParser(description='舵机 PWM 在线扫动测试')
    ap.add_argument('-f', '--from', dest='a', type=int, default=1000,
                    help='第一个脉宽 us（默认 1000 = 0 度）')
    ap.add_argument('-t', '--to', dest='b', type=int, default=2000,
                    help='第二个脉宽 us（默认 2000 = 180 度）')
    ap.add_argument('-H', '--hold', type=float, default=1.5,
                    help='每边保持秒数（默认 1.5）')
    ap.add_argument('-c', '--cycles', type=int, default=10,
                    help='来回次数（默认 10）')
    args = ap.parse_args()

    total = args.hold * 2 * args.cycles
    print('=' * 62)
    print(' 舵机在线扫动测试')
    print(' 脉宽在 %d us (%.0f 度) 和 %d us (%.0f 度) 之间来回切换'
          % (args.a, angle_of(args.a), args.b, angle_of(args.b)))
    print(' 每边保持 %.1f 秒，共 %d 个来回，总计约 %.0f 秒'
          % (args.hold, args.cycles, total))
    print('-' * 62)
    print(' 现在请盯着舵机，看它是不是跟着来回摆。')
    print('=' * 62)
    print()

    session = ConnectHelper.session_with_chosen_probe(
        target_override=TARGET,
        options={
            'frequency': FREQ,
            'connect_mode': 'attach',      # 不改动芯片状态
            'resume_on_disconnect': True,
        },
    )
    if session is None:
        print('错误: 没有找到 CMSIS-DAP 调试器')
        return 2

    moved = []
    try:
        with session:
            for i in range(args.cycles):
                for us in (args.a, args.b):
                    session.target.write32(TIM2_CCR1, us)
                    stamp = time.strftime('%H:%M:%S')
                    print('  [%s]  第 %2d/%d 次  ->  %4d us   (%.0f 度)'
                          % (stamp, i + 1, args.cycles, us, angle_of(us)))
                    time.sleep(args.hold)
            session.target.write32(TIM2_CCR1, args.a)
            print()
            print('  结束，脉宽停在 %d us' % args.a)
    except KeyboardInterrupt:
        print('\n  被中断，脉宽停在当前值')

    print()
    print('=' * 62)
    print(' 请回答：舵机在这段时间里动了吗？')
    print('   动了     -> 接线没问题，之前的"没转"是因为它本来就在 0 度')
    print('   没动     -> 信号没到舵机，先查共地，再查信号线是不是接在 PA0')
    print('=' * 62)
    return 0


if __name__ == '__main__':
    sys.exit(main())
