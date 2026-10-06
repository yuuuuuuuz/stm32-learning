# ============================================================================
#  STM32F103 舵机 PWM 工程 —— GCC 构建脚本
#
#  用法:   pwsh -File build.ps1
#  依赖:   arm-none-eabi-gcc 已在 PATH 中
#          (本机: C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin)
#
#  产物:   build\servo-pwm.elf / .hex / .bin / .map
# ============================================================================

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$env:PYTHONIOENCODING = 'utf-8'

$root  = $PSScriptRoot
$build = Join-Path $root 'build'
$name  = 'servo-pwm'

New-Item -ItemType Directory -Force -Path $build | Out-Null

$cpuFlags = @('-mcpu=cortex-m3', '-mthumb')
$defs     = @('-DUSE_STDPERIPH_DRIVER', '-DSTM32F10X_MD')
$incs     = @(
    "-I$(Join-Path $root 'start')",
    "-I$(Join-Path $root 'Library')",
    "-I$(Join-Path $root 'User')"
)
$cflags = @('-O2', '-g3', '-std=gnu11', '-Wall',
            '-fdata-sections', '-ffunction-sections') + $cpuFlags + $defs + $incs

$sources = @(
    (Join-Path $root 'startup_stm32f10x_md_gcc.s'),
    (Join-Path $root 'start\system_stm32f10x.c'),
    (Join-Path $root 'Library\stm32f10x_rcc.c'),
    (Join-Path $root 'Library\stm32f10x_gpio.c'),
    (Join-Path $root 'Library\stm32f10x_tim.c'),
    (Join-Path $root 'Library\misc.c'),
    (Join-Path $root 'User\main.c')
)

Write-Host '=== 编译 ===' -ForegroundColor Cyan
$objs = @()
foreach ($src in $sources) {
    if (-not (Test-Path $src)) { throw "源文件不存在: $src" }
    $obj = Join-Path $build ((Split-Path $src -Leaf) -replace '\.(c|s)$', '.o')
    Write-Host ("  CC   " + (Split-Path $src -Leaf))
    & arm-none-eabi-gcc @cflags -c $src -o $obj
    if ($LASTEXITCODE -ne 0) { throw "编译失败: $src" }
    $objs += $obj
}

Write-Host '=== 链接 ===' -ForegroundColor Cyan
$elf = Join-Path $build "$name.elf"
$ldflags = @(
    '-T', (Join-Path $root 'STM32F103C8Tx_FLASH.ld'),
    '-Wl,--gc-sections',
    "-Wl,-Map=$build\$name.map",
    '--specs=nosys.specs',
    '-nostartfiles'
) + $cpuFlags

& arm-none-eabi-gcc @ldflags @objs -o $elf
if ($LASTEXITCODE -ne 0) { throw '链接失败' }

Write-Host '=== 生成 hex / bin ===' -ForegroundColor Cyan
& arm-none-eabi-objcopy -O ihex   $elf (Join-Path $build "$name.hex")
if ($LASTEXITCODE -ne 0) { throw 'objcopy (ihex) 失败' }
& arm-none-eabi-objcopy -O binary $elf (Join-Path $build "$name.bin")
if ($LASTEXITCODE -ne 0) { throw 'objcopy (binary) 失败' }

Write-Host ''
Write-Host '=== 体积 ===' -ForegroundColor Green
& arm-none-eabi-size $elf

Write-Host ''
Write-Host '=== 产物校验 ===' -ForegroundColor Green
Get-ChildItem $build -Filter "$name.*" |
    Where-Object { $_.Extension -in '.elf', '.hex', '.bin', '.map' } |
    Select-Object Name, Length, LastWriteTime | Format-Table -AutoSize

Write-Host '构建成功。烧录请运行:  pwsh -File flash.ps1' -ForegroundColor Green
