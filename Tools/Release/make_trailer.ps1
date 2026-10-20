# Phase 20: cuts the trailer from the frames the game wrote (-WEShots=P20T, see Source/.../Dev/WEDevTrailer.cpp):
# title card, the 9 scenes with captions, end card; music (Holizna "Make Money", CC0), rain and thunder (CC0).
#   powershell -File Tools\Release\make_trailer.ps1 [-FFmpeg <ffmpeg.exe>]
# Record the frames first (1920x1080, fixed 30 fps):
#   UnrealEditor.exe WorkshopEmpire.uproject /Game/WorkshopEmpire/Maps/L_Garage -game -windowed -ResX=1920 -ResY=1080
#       -nosteam -WEShots=P20T -UseFixedTimeStep -FPS=30
# Output: <workspace>\Builds\Trailer\WorkshopEmpire_Trailer.mp4 (not in the repository).
param([string]$FFmpeg = 'F:\Spiele\_tools\ffmpeg\ffmpeg-9.0.2-essentials_build\bin\ffmpeg.exe')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$frames = Join-Path $root 'Saved\Trailer'
$out = Join-Path (Split-Path $root -Parent) 'Builds\Trailer'
New-Item -ItemType Directory -Force $out | Out-Null
$music = Join-Path $root 'SourceArt\OpenGameArt\Funk\Funk ( Collection )\04 HoliznaCC0 - Make Money.mp3'
$rain = Join-Path $root 'SourceArt\OpenGameArt\Wav\SFX_RainLoop.wav'
$thunder = Join-Path $root 'SourceArt\OpenGameArt\Wav\SFX_Thunder.wav'

$intro = 3.0; $main = 45.0; $outro = 4.0; $total = $intro + $main + $outro
$font = "fontfile='C\:/Windows/Fonts/bahnschrift.ttf'"
# Scene captions (seconds in the scene part), matching the scene list in WEDevTrailer.cpp.
$captions = @(
    @(0.3, 5.3, 'Deine eigene Werkstatt'),
    @(5.8, 10.3, 'Radio an, Kaffee auf - die Werkstatt lebt'),
    @(10.8, 14.8, 'Schraube, die Werkstattkatze'),
    @(15.3, 19.3, 'Mitarbeiter in deiner Firmenfarbe'),
    @(19.8, 23.8, 'Dein Firmenlogo - gestaltet im Ferrit OS'),
    @(24.3, 28.3, 'auf Transporter, Overall, Tor und Schild'),
    @(28.8, 33.8, 'Regen'),
    @(34.3, 39.8, 'und Gewitter bei Nacht'),
    @(40.3, 44.8, 'Ankaufen, reparieren, verkaufen - allein oder im Koop')
)
$draw = @()
foreach ($c in $captions) {
    $a = $c[0]; $b = $c[1]
    $alpha = "if(lt(t,$a+0.4),(t-$a)/0.4,if(gt(t,$b-0.4),($b-t)/0.4,1))"
    $draw += "drawtext=$($font):text='$($c[2])':fontsize=54:fontcolor=white:borderw=3:bordercolor=black@0.6:x=(w-text_w)/2:y=h-150:enable='between(t,$a,$b)':alpha='$alpha'"
}
$title = "drawtext=$($font):text='WORKSHOP':fontsize=150:fontcolor=white:x=(w-text_w)/2:y=330,drawtext=$($font):text='EMPIRE':fontsize=150:fontcolor=0xF2A71B:x=(w-text_w)/2:y=480"
$graph = @"
color=c=0x0B0D10:s=1920x1080:r=30:d=$intro,$title,drawtext=$($font):text='Werkstatt-Simulation für 1 bis 4 Spieler':fontsize=44:fontcolor=0xB8BEC6:x=(w-text_w)/2:y=680,fade=t=in:st=0:d=0.6,fade=t=out:st=$($intro - 0.5):d=0.5,format=yuv420p[intro];
[0:v]fps=30,trim=duration=$main,setpts=PTS-STARTPTS,$($draw -join ','),fade=t=in:st=0:d=0.5,fade=t=out:st=$($main - 0.6):d=0.6,format=yuv420p[main];
color=c=0x0B0D10:s=1920x1080:r=30:d=$outro,$title,drawtext=$($font):text='Bald auf Steam':fontsize=56:fontcolor=white:x=(w-text_w)/2:y=700,fade=t=in:st=0:d=0.6,format=yuv420p[outro];
[intro][main][outro]concat=n=3:v=1:a=0[v];
[1:a]atrim=0:$total,asetpts=PTS-STARTPTS,volume=0.8,afade=t=in:st=0:d=0.8,afade=t=out:st=$($total - 3):d=3[music];
[2:a]atrim=0:11.5,asetpts=PTS-STARTPTS,afade=t=in:st=0:d=1,afade=t=out:st=10:d=1.5,volume=0.55,adelay=$(($intro + 28.5) * 1000)|$(($intro + 28.5) * 1000)[rain];
[3:a]asplit=2[t1][t2];
[t1]volume=0.9,adelay=$(($intro + 35.8) * 1000)|$(($intro + 35.8) * 1000)[th1];
[t2]atrim=0:8,volume=0.8,adelay=$(($intro + 38.9) * 1000)|$(($intro + 38.9) * 1000)[th2];
[music][rain][th1][th2]amix=inputs=4:normalize=0:duration=first[a]
"@
$script = Join-Path $out 'trailer_filter.txt'
[IO.File]::WriteAllText($script, $graph)
& $FFmpeg -y -loglevel error -framerate 30 -i "$frames\f_%05d.png" -i $music -stream_loop -1 -i $rain -i $thunder `
    -/filter_complex $script -map '[v]' -map '[a]' -c:v libx264 -preset slow -crf 18 -pix_fmt yuv420p -c:a aac -b:a 192k `
    -movflags +faststart -t $total (Join-Path $out 'WorkshopEmpire_Trailer.mp4')
if ($LASTEXITCODE -ne 0) { throw "ffmpeg failed ($LASTEXITCODE)" }
Write-Output "Trailer: $(Join-Path $out 'WorkshopEmpire_Trailer.mp4')"
