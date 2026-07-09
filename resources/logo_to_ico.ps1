# Convertit resources/logo.png en resources/app.ico multi-resolutions.
# Usage : deposez votre logo (carre, fond transparent de preference) dans
#         resources/logo.png puis lancez, depuis le dossier resources :
#             powershell.exe -ExecutionPolicy Bypass -File logo_to_ico.ps1
#         Recompilez ensuite l'application (cmake --build --preset default).
Add-Type -AssemblyName System.Drawing

$here   = Split-Path -Parent $MyInvocation.MyCommand.Path
$srcPng = Join-Path $here "logo.png"
$outIco = Join-Path $here "app.ico"

if (-not (Test-Path $srcPng)) {
    Write-Error "Fichier introuvable : $srcPng (deposez-y votre logo en PNG)."
    exit 1
}

$src   = [System.Drawing.Image]::FromFile($srcPng)
$sizes = 16,24,32,48,64,128,256
$pngs  = @()
foreach ($s in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap($s, $s)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.DrawImage($src, 0, 0, $s, $s)
    $g.Dispose()
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $pngs += ,($ms.ToArray())
    $bmp.Dispose(); $ms.Dispose()
}
$src.Dispose()

$fs = New-Object System.IO.FileStream($outIco, [System.IO.FileMode]::Create)
$bw = New-Object System.IO.BinaryWriter($fs)
$bw.Write([UInt16]0); $bw.Write([UInt16]1); $bw.Write([UInt16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $s = $sizes[$i]; $data = $pngs[$i]
    $b = $(if ($s -ge 256) { 0 } else { $s })
    $bw.Write([Byte]$b); $bw.Write([Byte]$b)
    $bw.Write([Byte]0); $bw.Write([Byte]0)
    $bw.Write([UInt16]1); $bw.Write([UInt16]32)
    $bw.Write([UInt32]$data.Length); $bw.Write([UInt32]$offset)
    $offset += $data.Length
}
foreach ($data in $pngs) { $bw.Write($data) }
$bw.Flush(); $bw.Close(); $fs.Close()
"app.ico regenere depuis logo.png ($((Get-Item $outIco).Length) octets)."
