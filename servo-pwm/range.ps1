# ============================================================================
#  舵机行程测试（range.py 的包装，负责中文输出编码）
#
#  用法:
#     powershell -ExecutionPolicy Bypass -File range.ps1
#     powershell -ExecutionPolicy Bypass -File range.ps1 -Lo 800 -Hi 2200 -Step 50 -Hold 1.0
#
#  ⚠️ 跑之前先看 README 里「舵机行程测试」那节的安全说明。
# ============================================================================

param(
    [int]$Lo = 900,
    [int]$Hi = 2200,
    [int]$Step = 100,
    [double]$Hold = 1.2
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$env:PYTHONIOENCODING = 'utf-8'

$script = Join-Path $PSScriptRoot 'range.py'
if (-not (Test-Path $script)) { throw "找不到 $script" }

python $script --lo $Lo --hi $Hi --step $Step --hold $Hold
exit $LASTEXITCODE
