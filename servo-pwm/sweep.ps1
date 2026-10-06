# ============================================================================
#  舵机 PWM 在线扫动测试（sweep.py 的包装，负责中文输出编码）
#
#  用法:
#     powershell -ExecutionPolicy Bypass -File sweep.ps1
#     powershell -ExecutionPolicy Bypass -File sweep.ps1 -From 1000 -To 1500 -Hold 2 -Cycles 5
#
#  跑起来之后盯着舵机看它动不动。
# ============================================================================

param(
    [int]$From = 1000,
    [int]$To = 2000,
    [double]$Hold = 1.5,
    [int]$Cycles = 10
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$env:PYTHONIOENCODING = 'utf-8'

$script = Join-Path $PSScriptRoot 'sweep.py'
if (-not (Test-Path $script)) { throw "找不到 $script" }

python $script --from $From --to $To --hold $Hold --cycles $Cycles
exit $LASTEXITCODE
