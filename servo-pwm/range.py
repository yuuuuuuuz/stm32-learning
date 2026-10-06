r"""
舵机行程测试 —— 看它到底能转多少度
=====================================================================
标准做法：从中间位置（1500us）往两头慢慢外扩，看它在哪个脉宽上
「不再跟着动」或者「开始嗡嗡响」—— 那个位置就是它的机械限位。

因为普通 PWM 舵机没有角度回读，芯片不知道舵机转到哪了，
所以这个测试必须由人盯着舵机来判断，脚本负责把脉宽一步一步喂出去。

⚠️ 安全说明
    舵机顶到机械限位叫「堵转」：电流最大、发热、长时间会崩齿。
    本脚本默认范围 900~2200us，每步只停 1.2 秒，走到头立刻返回，
    不会长时间堵转，正常舵机不会坏。
    如果听到明显异响、卡死声或闻到焦味 —— 直接拔电源。

判读
    你只要记住一个值：舵机**最后一次正常跟着动**的那个脉宽。
    换个说法：脚本喊到某个数之后它就不动了 / 开始嗡嗡响，
    那它的极限就在这个数附近。

用法
    powershell -ExecutionPolicy Bypass -File range.ps1
    powershell -ExecutionPolicy Bypass -File range.ps1 -Lo 800 -Hi 2200 -Step 50 -Hold 1.0
"""

import argparse
import logging
import sys
import time

from pyocd.core.helpers import ConnectHelper

TARGET = 'stm32f103c8'
FREQ = 1_000_000
TIM2_CCR1 = 0x40000034

logging.getLogger('pyocd').setLevel(logging.ERROR)


def nominal_deg(us):
    """按 1000us=0度 / 2000us=180度 的标称关系换算，仅作参考"""
    return (us - 1000) * 180.0 / 1000.0


def main():
    ap = argparse.ArgumentParser(description='舵机行程测试')
    ap.add_argument('--lo', type=int, default=900, help='往小探到多少 us（默认 900）')
    ap.add_argument('--hi', type=int, default=2200, help='往大探到多少 us（默认 2200）')
    ap.add_argument('--step', type=int, default=100, help='步长 us（默认 100）')
    ap.add_argument('--hold', type=float, default=1.2, help='每步停留秒数（默认 1.2）')
    args = ap.parse_args()

    print('=' * 66)
    print(' 舵机行程测试')
    print(' 范围 %d ~ %d us，步长 %d us，每步停 %.1f 秒' % (args.lo, args.hi, args.step, args.hold))
    print('-' * 66)
    print(' 请盯着舵机。记住它「最后一次还能正常跟着动」的那个数值，')
    print(' 之后它不动了或者开始嗡嗡响，那里就是机械限位。')
    print('=' * 66)
    print()

    session = ConnectHelper.session_with_chosen_probe(
        target_override=TARGET,
        options={'frequency': FREQ, 'connect_mode': 'attach',
                 'resume_on_disconnect': True},
    )
    if session is None:
        print('错误: 没有找到 CMSIS-DAP 调试器')
        return 2

    def move(us, hold):
        session.target.write32(TIM2_CCR1, us)
        print('      %4d us    标称 %+6.1f 度' % (us, nominal_deg(us)))
        time.sleep(hold)

    try:
        with session:
            print('[0] 回到中位 1500us')
            move(1500, 1.2)
            print()

            print('[1] 往小探（脉宽变小 -> 标称角度往负方向走）')
            us = 1500 - args.step
            while us >= args.lo:
                move(us, args.hold)
                us -= args.step
            print('      --- 已到达探测下限 %d us，停 0.8 秒后返回 ---' % args.lo)
            time.sleep(0.8)
            print()

            print('[2] 回中位')
            move(1500, 1.0)
            print()

            print('[3] 往大探（脉宽变大 -> 标称角度往 180 度以上走）')
            us = 1500 + args.step
            while us <= args.hi:
                move(us, args.hold)
                us += args.step
            print('      --- 已到达探测上限 %d us，停 0.8 秒后返回 ---' % args.hi)
            time.sleep(0.8)
            print()

            print('[4] 收尾：回到固件定义的位置 1000us (0 度)')
            session.target.write32(TIM2_CCR1, 1000)
            print('      %4d us    标称 %+6.1f 度' % (1000, nominal_deg(1000)))

    except KeyboardInterrupt:
        session.target.write32(TIM2_CCR1, 1000)
        print('\n  被 Ctrl+C 中断，已复位到 1000us (0 度)')

    print()
    print('=' * 66)
    print(' 请回答两个问题：')
    print('   1. 往小探的时候，从哪个数开始它就不动了 / 开始嗡嗡响？')
    print('   2. 往大探的时候，从哪个数开始它就不动了 / 开始嗡嗡响？')
    print(' 如果两头全程都跟着动，说明它的行程还没探到头，可以再加大范围。')
    print('=' * 66)
    return 0


if __name__ == '__main__':
    sys.exit(main())
