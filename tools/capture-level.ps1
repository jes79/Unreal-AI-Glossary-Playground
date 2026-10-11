# 예제 레벨을 -game 창 모드로 띄워 몇 초 뒤 게임 창을 캡처하고 종료한다 (README 스크린샷·검증용).
#   powershell -ExecutionPolicy Bypass -File tools/capture-level.ps1 -Level /Game/Levels/L_0000_Hub -Out shots/0000_Hub.png [-Wait 60]
# -Keys "1,2" 처럼 주면 캡처 직전에 그 키들을 차례로 눌러 본다(SendKeys 표기, 토글 데모 확인용).
param(
    [Parameter(Mandatory = $true)][string]$Level,
    [Parameter(Mandatory = $true)][string]$Out,
    [int]$Wait = 60,
    [int]$Width = 1600,
    [int]$Height = 900,
    [string]$Keys = ""
)
Add-Type @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class GlossaryWin {
    public delegate bool EnumProc(IntPtr h, IntPtr p);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    public static IntPtr FindLargestWindow(uint pid) {
        IntPtr best = IntPtr.Zero; long bestArea = 0;
        EnumWindows((h, p) => {
            uint wp; GetWindowThreadProcessId(h, out wp);
            if (wp == pid && IsWindowVisible(h)) {
                RECT r; GetWindowRect(h, out r);
                long a = (long)(r.Right - r.Left) * (r.Bottom - r.Top);
                if (a > bestArea) { bestArea = a; best = h; }
            }
            return true;
        }, IntPtr.Zero);
        return best;
    }
}
"@
[GlossaryWin]::SetProcessDPIAware() | Out-Null
Add-Type -AssemblyName System.Drawing, System.Windows.Forms

$root = Split-Path $PSScriptRoot -Parent
$ue = if ($env:UE_ROOT) { Join-Path $env:UE_ROOT "Engine\Binaries\Win64\UnrealEditor.exe" } else { "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" }
$proj = Join-Path $root "GlossaryPlayground\GlossaryPlayground.uproject"
$name = [IO.Path]::GetFileNameWithoutExtension($Out)
$log = Join-Path $root "GlossaryPlayground\Saved\Logs\Capture_$name.log"
$p = Start-Process -FilePath $ue -ArgumentList "`"$proj`" $Level -game -windowed -ResX=$Width -ResY=$Height -WinX=40 -WinY=40 -abslog=`"$log`" -nosplash" -PassThru
Start-Sleep -Seconds $Wait
$h = [GlossaryWin]::FindLargestWindow([uint32]$p.Id)
if ($h -eq [IntPtr]::Zero) { "no window (crashed?) log=$log"; Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue; exit 1 }
[GlossaryWin]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 300
foreach ($k in ($Keys -split ',' | Where-Object { $_ })) {
    [System.Windows.Forms.SendKeys]::SendWait($k)
    Start-Sleep -Milliseconds 1500
}
Start-Sleep -Milliseconds 500
$r = New-Object GlossaryWin+RECT
[GlossaryWin]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $hh = $r.Bottom - $r.Top
$bmp = New-Object System.Drawing.Bitmap $w, $hh
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
New-Item -ItemType Directory -Force (Split-Path $Out) | Out-Null
$bmp.Save($Out)
Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
"captured=$Out size=${w}x${hh} log=$log"
