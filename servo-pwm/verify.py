r"""
从 STM32 芯片读回 Flash，与固件 .bin 逐字节比对，确认烧录真的成功了。

用法:
    python verify.py <固件.bin> [长度] [起始地址]

例:
    python verify.py build\servo-pwm.bin 1608 0x08000000

为什么不用 `pyocd commander -c "save ..."`:
    pyocd 0.45.1 的 commander `save` 命令对文件参数一律报
    "Error: invalid argument"，绝对路径和相对路径都一样，用不了。
    这里直接调 pyOCD 的 Python API，稳定可靠。
"""

import os
import sys

from pyocd.core.helpers import ConnectHelper

TARGET = 'stm32f103c8'
FREQ = 1_000_000


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2

    bin_path = sys.argv[1]
    if not os.path.isfile(bin_path):
        print('找不到文件: %s' % bin_path)
        return 2

    ref = open(bin_path, 'rb').read()
    if len(sys.argv) > 2:
        ref = ref[:int(sys.argv[2], 0)]
    addr = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0x08000000

    print('目标芯片 : %s' % TARGET)
    print('烧录地址 : 0x%08X' % addr)
    print('固件长度 : %d 字节' % len(ref))

    session = ConnectHelper.session_with_chosen_probe(
        target_override=TARGET,
        options={
            'frequency': FREQ,
            'connect_mode': 'attach',   # attach 不改动芯片状态
            'resume_on_disconnect': True,
        },
    )
    if session is None:
        print('错误: 没有找到 CMSIS-DAP 调试器')
        return 2

    with session:
        data = bytes(session.target.read_memory_block8(addr, len(ref)))

    diff = [i for i, (a, b) in enumerate(zip(ref, data)) if a != b]
    print('芯片读回 : %d 字节' % len(data))

    if not diff:
        print('校验通过: 芯片 Flash 与固件逐字节完全一致。')
        return 0

    print('校验失败: %d 个字节不一致' % len(diff))
    for i in diff[:16]:
        print('  0x%08X  固件=0x%02X  芯片=0x%02X' % (addr + i, ref[i], data[i]))
    return 1


if __name__ == '__main__':
    sys.exit(main())
