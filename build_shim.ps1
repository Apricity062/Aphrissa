# build_shim.ps1 — 沙箱兼容的增量构建脚本
#
# 背景: 沙箱会挂起 ninja 实际执行时创建的子进程管道 (CreatePipe),
#       但 ninja -n -v (dry-run) 只打印待执行命令, 不受影响。
# 方案: 用 ninja -n -v 列出需要执行的命令, 用 cmd.exe 逐条执行
#       (每条前注入 cd /d build\Debug)。链接命令依赖 ninja 动态
#       生成的 .rsp 文件, 此处从 build.ninja 自行生成。
#
# 用法:  pwsh -File build_shim.ps1

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $root "build\Debug"
$log = Join-Path $env:TEMP "build_shim.log"
Write-Host "[shim] root=$root buildDir=$buildDir"

# 1) 收集待执行命令 (ninja dry-run 输出 [i/n] 前缀)
Write-Host "[shim] collecting commands..."
$raw = & ninja -C $buildDir -n -v Aphrissa.elf 2>&1
$cmds = @()
foreach ($line in $raw) {
    if ($line -match '^\[\d+/\d+\]\s+(.*)$') {
        $cmds += $Matches[1]
    }
}
if ($cmds.Count -eq 0) {
    Write-Host "[shim] nothing to build (already up to date)"
    exit 0
}
Write-Host "[shim] $($cmds.Count) command(s) to run"

# 2) 从 build.ninja 提取链接 rsp 内容 (若本次需要链接)
$ninjaFile = Join-Path $buildDir "build.ninja"
$rspPath = Join-Path $buildDir "CMakeFiles\Aphrissa.rsp"
$rspGenerated = $false
if (Test-Path $ninjaFile) {
    $content = Get-Content $ninjaFile -Raw
    # 提取 "build Aphrissa.elf: ... " 后的所有依赖 .obj
    if ($content -match '(?s)build Aphrissa\.elf:.*?\n') {
        $linkLine = $Matches[0]
        # 收集 .obj 路径
        $objs = [regex]::Matches($linkLine, '[\w\\/\.-]+\.obj') | ForEach-Object { $_.Value } | Select-Object -Unique
        # 收集 LINK_LIBRARIES
        $libs = ""
        if ($content -match '(?m)^  LINK_LIBRARIES = (.*)$') { $libs = $Matches[1] }
        $rsp = ($objs + $libs -split '\s+' | Where-Object { $_ -ne "" -and $_ -ne "-lm" }) -join "`n"
        if ($objs.Count -gt 0) {
            Set-Content -Path $rspPath -Value $rsp -Encoding ascii
            $rspGenerated = $true
            Write-Host "[shim] generated rsp with $($objs.Count) objects"
        }
    }
}

# 3) 逐条执行
$failed = 0
$idx = 0
foreach ($c in $cmds) {
    $idx++
    $short = if ($c.Length -gt 90) { $c.Substring(0, 90) + "..." } else { $c }
    Write-Host ("[{0}/{1}] {2}" -f $idx, $cmds.Count, $short)

    $wrapped = "cd /d `"$buildDir`" && $c"
    & cmd.exe /C $wrapped 2>&1 | Out-File -Append -Encoding utf8 $log
    $ec = $LASTEXITCODE
    if ($ec -ne 0) {
        Write-Host "[shim] FAILED (exit $ec)" -ForegroundColor Red
        $failed++
        if ($failed -ge 5) { Write-Host "[shim] too many failures, aborting"; break }
    }
}

# 4) 清理 rsp (ninja 行为: 用后即删)
if ($rspGenerated) { Remove-Item $rspPath -ErrorAction SilentlyContinue }

if ($failed -eq 0) {
    Write-Host "[shim] BUILD OK" -ForegroundColor Green
} else {
    Write-Host "[shim] BUILD FAILED: $failed error(s), see $log" -ForegroundColor Red
}
exit $failed
