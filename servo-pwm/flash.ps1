# ============================================================================
#  用 pyOCD 通过 CMSIS-DAP 调试器把固件烧进 STM32F103，并读回逐字节校验
#
#  用法:   powershell -ExecutionPolicy Bypass -File flash.ps1
#          （如果系统已允许运行脚本，直接 .\flash.ps1 就行）
#
#  依赖:   pyocd / python / arm-none-eabi-* 都在 PATH 中
#          调试器插好，且不要同时开着 Keil 的在线调试（会抢同一个调试器）
# ============================================================================

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$env:PYTHONIOENCODING = 'utf-8'

$root  = $PSScriptRoot
$build = Join-Path $root 'build'
$name  = 'servo-pwm'
$hex   = Join-Path $build "$name.hex"
$bin   = Join-Path $build "$name.bin"
$elf   = Join-Path $build "$name.elf"

$TARGET = 'stm32f103c8'
$FREQ   = '1m'

if (-not (Test-Path $hex)) { throw "找不到 $hex ，请先运行 build.ps1" }

Write-Host '=== 已连接的调试器 ===' -ForegroundColor Cyan
pyocd list

Write-Host ''
Write-Host "=== 烧录 $name.hex ===" -ForegroundColor Cyan
pyocd flash -t $TARGET -f $FREQ $hex
if ($LASTEXITCODE -ne 0) { throw '烧录失败' }

Write-Host ''
Write-Host '=== 读回 Flash 逐字节校验 ===' -ForegroundColor Cyan

# 用 ELF 算出固件实际占用字节数 = text + data
$sizeOut = & arm-none-eabi-size $elf
$nums = ($sizeOut | Select-Object -Last 1) -split '\s+' | Where-Object { $_ -match '^\d+$' }
$length = [int]$nums[0] + [int]$nums[1]

# 用 Python API 读回比对。
# 不用 `pyocd commander -c "save ..."`：pyocd 0.45.1 的 save 命令对文件参数
# 一律报 "Error: invalid argument"，绝对/相对路径都一样，是它的 bug。
python (Join-Path $root 'verify.py') $bin $length 0x08000000
if ($LASTEXITCODE -ne 0) { throw '校验失败：芯片里的内容和固件不一致' }

Write-Host ''
Write-Host '完成：板子已复位并在跑新固件。' -ForegroundColor Green
Write-Host '预期现象：舵机先转到中位停 1.5 秒 -> 再转到 0 度停住；PC13 每秒闪一下。' -ForegroundColor Green
Write-Host '如果舵机一直转不停，说明它是连续旋转型：立刻断电，把 main.c 里的' -ForegroundColor Yellow
Write-Host 'SERVO_IS_CONTINUOUS 改成 1，重新 build + flash。' -ForegroundColor Yellow
