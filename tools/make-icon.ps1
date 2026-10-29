# Gera src/app.ico sem dependências externas (System.Drawing, Windows PowerShell).
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$output = Join-Path $root 'src\app.ico'
$sizes = @(16, 24, 32, 48, 64, 128, 256)
$pngs = New-Object System.Collections.ArrayList

function New-IconPng([int]$size) {
    $bmp = [System.Drawing.Bitmap]::new($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)

    $s = $size / 256.0
    $rect = [System.Drawing.RectangleF]::new(4 * $s, 4 * $s, 248 * $s, 248 * $s)
    $radius = 54 * $s
    $path = [System.Drawing.Drawing2D.GraphicsPath]::new()
    $d = $radius * 2
    $path.AddArc($rect.X, $rect.Y, $d, $d, 180, 90)
    $path.AddArc($rect.Right - $d, $rect.Y, $d, $d, 270, 90)
    $path.AddArc($rect.Right - $d, $rect.Bottom - $d, $d, $d, 0, 90)
    $path.AddArc($rect.X, $rect.Bottom - $d, $d, $d, 90, 90)
    $path.CloseFigure()

    $gradient = [System.Drawing.Drawing2D.LinearGradientBrush]::new($rect,
        [System.Drawing.Color]::FromArgb(255, 18, 108, 201),
        [System.Drawing.Color]::FromArgb(255, 11, 47, 87),
        45.0)
    $g.FillPath($gradient, $path)

    $borderPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(90, 255, 255, 255), [Math]::Max(1.0, 2.0 * $s))
    $g.DrawPath($borderPen, $path)

    $white = [System.Drawing.Color]::FromArgb(245, 255, 255, 255)
    $penWidth = [Math]::Max(1.2, 17.0 * $s)
    $pen = [System.Drawing.Pen]::new($white, $penWidth)
    $pen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $pen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round

    # Arco de restauração.
    $arcRect = [System.Drawing.RectangleF]::new(63 * $s, 61 * $s, 130 * $s, 130 * $s)
    $g.DrawArc($pen, $arcRect, 145, 285)

    # Ponta da seta.
    $cx = 128 * $s
    $cy = 126 * $s
    $arrow = @(
        ([System.Drawing.PointF]::new((168 * $s), (66 * $s))),
        ([System.Drawing.PointF]::new((190 * $s), (103 * $s))),
        ([System.Drawing.PointF]::new((145 * $s), (104 * $s)))
    )
    $arrowBrush = [System.Drawing.SolidBrush]::new($white)
    $g.FillPolygon($arrowBrush, [System.Drawing.PointF[]]$arrow)

    # Marca de verificação central.
    if ($size -ge 24) {
        $checkPen = [System.Drawing.Pen]::new($white, [Math]::Max(1.3, 15.0 * $s))
        $checkPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
        $checkPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
        $checkPen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
        $g.DrawLines($checkPen, [System.Drawing.PointF[]]@(
            ([System.Drawing.PointF]::new((96 * $s), (133 * $s))),
            ([System.Drawing.PointF]::new((118 * $s), (155 * $s))),
            ([System.Drawing.PointF]::new((163 * $s), (108 * $s)))
        ))
        $checkPen.Dispose()
    } elseif ($size -ge 16) {
        $g.FillEllipse(([System.Drawing.SolidBrush]::new($white)), 116 * $s, 121 * $s, 24 * $s, 24 * $s)
    }

    $stream = [System.IO.MemoryStream]::new()
    $bmp.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
    $bytes = $stream.ToArray()
    $stream.Dispose()
    $g.Dispose()
    $bmp.Dispose()
    $gradient.Dispose()
    $borderPen.Dispose()
    $pen.Dispose()
    $arrowBrush.Dispose()
    $path.Dispose()
    Write-Output -NoEnumerate $bytes
}

foreach ($size in $sizes) {
    [void]$pngs.Add((New-IconPng $size))
}

$fs = [System.IO.File]::Create($output)
$bw = [System.IO.BinaryWriter]::new($fs)
$bw.Write([UInt16]0)
$bw.Write([UInt16]1)
$bw.Write([UInt16]$sizes.Count)
$offset = 6 + (16 * $sizes.Count)
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $size = $sizes[$i]
    $data = $pngs[$i]
    $bw.Write([Byte]($(if ($size -eq 256) { 0 } else { $size })))
    $bw.Write([Byte]($(if ($size -eq 256) { 0 } else { $size })))
    $bw.Write([Byte]0)
    $bw.Write([Byte]0)
    $bw.Write([UInt16]1)
    $bw.Write([UInt16]32)
    $bw.Write([UInt32]$data.Length)
    $bw.Write([UInt32]$offset)
    $offset += $data.Length
}
foreach ($data in $pngs) {
    $bw.Write($data)
}
$bw.Flush()
$bw.Close()
$fs.Close()
Write-Host "Ícone gerado: $output ($($offset) bytes)"

