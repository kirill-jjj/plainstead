Add-Type @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public class W {
	[DllImport("user32.dll")] public static extern IntPtr FindWindow(string c, string t);
	[DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr p, EnumProc f, IntPtr l);
	public delegate bool EnumProc(IntPtr h, IntPtr l);
	[DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
	[DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
	[DllImport("user32.dll")] public static extern bool IsWindowEnabled(IntPtr h);
	[DllImport("user32.dll")] public static extern int GetWindowTextLength(IntPtr h);
	[DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
	[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
	public struct RECT { public int L, T, R, B; }
}
"@
$h = [W]::FindWindow($null, "games/cat")
if ($h -eq [IntPtr]::Zero) { $h = [W]::FindWindow($null, "PlainInstead") }
if ($h -eq [IntPtr]::Zero) {
	$p = Get-Process PlainInstead_debug -ErrorAction SilentlyContinue | Select-Object -First 1
	if ($p -and $p.MainWindowHandle -ne 0) { $h = $p.MainWindowHandle }
}
if ($h -eq [IntPtr]::Zero) { Write-Host "NO WINDOW"; exit 1 }
$cb = [W+EnumProc]{ param($ch, $l)
	$cn = New-Object System.Text.StringBuilder 256
	[void][W]::GetClassName($ch, $cn, 256)
	$cnt = [W]::SendMessage($ch, 0x018B, [IntPtr]::Zero, [IntPtr]::Zero) # LB_GETCOUNT
	$cnt2 = [W]::SendMessage($ch, 0x0118, [IntPtr]::Zero, [IntPtr]::Zero) # EM_GETLINECOUNT for edit
	$len = [W]::GetWindowTextLength($ch)
	$r = New-Object W+RECT
	[void][W]::GetWindowRect($ch, [ref]$r)
	$en = [W]::IsWindowEnabled($ch)
	$txt = ""
	if ($len -gt 0 -and $len -lt 200) { $sb = New-Object System.Text.StringBuilder ($len+1); [void][W]::GetWindowText($ch, $sb, $len+1); $txt = $sb.ToString().Substring(0, [Math]::Min(60, $len)) }
	Write-Host ("{0} count={1} lines={2} len={3} enabled={4} rect=({5},{6},{7},{8}) txt=[{9}]" -f $cn.ToString(), $cnt, $cnt2, $len, $en, $r.L, $r.T, $r.R, $r.B, $txt)
	return $true
}
[void][W]::EnumChildWindows($h, $cb, [IntPtr]::Zero)
