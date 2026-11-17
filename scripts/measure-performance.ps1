[CmdletBinding()]
param(
    [string]$Executable,
    [ValidateRange(1, 9)][int]$Runs = 3,
    [ValidateRange(1, 10000)][int]$Instances = 1000,
    [ValidateSet('Release', 'Unknown')][string]$Configuration = 'Unknown',
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $Executable) {
    foreach ($relative in @('build\windows\bin\Release\EndlessRunner.exe', 'bin\Release\EndlessRunner.exe')) {
        $candidate = Join-Path $repoRoot $relative
        if (Test-Path -LiteralPath $candidate) { $Executable = $candidate; $Configuration = 'Release'; break }
    }
}
if (-not $Executable -or -not (Test-Path -LiteralPath $Executable)) {
    throw 'Build Release first, or supply -Executable with the full path to the game.'
}
$Executable = (Resolve-Path -LiteralPath $Executable).Path
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $repoRoot ('artifacts\performance-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
}
if (Test-Path -LiteralPath $OutputDirectory) {
    throw 'Choose a new output directory so an earlier measurement is not overwritten.'
}
$null = New-Item -ItemType Directory -Path $OutputDirectory
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$culture = [Globalization.CultureInfo]::InvariantCulture
$samples = @()
$firstLog = @()

for ($run = 1; $run -le $Runs; $run++) {
    $stdout = Join-Path $OutputDirectory "run-$run.stdout.txt"
    $stderr = Join-Path $OutputDirectory "run-$run.stderr.txt"
    Write-Host "Measuring instancing ON/OFF, run $run of $Runs..."
    $process = Start-Process -FilePath $Executable -ArgumentList @('--benchmark', '--instances', $Instances.ToString($culture)) -WorkingDirectory $repoRoot -Wait -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    if ($process.ExitCode -ne 0) { throw "Benchmark failed ($($process.ExitCode)); see $stdout and $stderr" }
    $lines = @(Get-Content -LiteralPath $stdout)
    if ($run -eq 1) { $firstLog = $lines }
    $csv = @($lines | Where-Object { $_ -match '^mode,pool,visible,obstacle_draws,width,height,frames,frame_ms,fps$|^(ON|OFF),' })
    if ($csv.Count -ne 3) { throw "Expected one CSV header and two benchmark rows in $stdout" }
    $rows = @($csv | ConvertFrom-Csv)
    if ($rows[0].mode -ne 'ON' -or $rows[1].mode -ne 'OFF') { throw 'Unexpected benchmark mode ordering.' }
    foreach ($field in @('pool', 'visible', 'width', 'height', 'frames')) {
        if ($rows[0].$field -ne $rows[1].$field) { throw "ON/OFF used different values for $field" }
    }
    foreach ($row in $rows) {
        if ([int]$row.pool -ne $Instances -or [int]$row.visible -ne $Instances) {
            throw 'The benchmark did not submit every requested stress instance. Rebuild the latest integrated version.'
        }
        $ms = [double]::Parse($row.frame_ms, $culture)
        $fps = [double]::Parse($row.fps, $culture)
        if ($ms -le 0 -or $fps -le 0 -or [double]::IsInfinity($ms) -or [double]::IsNaN($ms) -or [double]::IsInfinity($fps) -or [double]::IsNaN($fps)) {
            throw 'The benchmark returned an invalid timing.'
        }
        $samples += [pscustomobject][ordered]@{
            run = $run; mode = $row.mode; pool = [int]$row.pool;
            visible = [int]$row.visible; obstacle_draws = [int]$row.obstacle_draws;
            width = [int]$row.width; height = [int]$row.height; frames = [int]$row.frames;
            frame_ms = $ms; fps = $fps
        }
    }
}

$samples | Export-Csv -LiteralPath (Join-Path $OutputDirectory 'samples.csv') -NoTypeInformation -Encoding UTF8
$summary = @()
foreach ($mode in @('ON', 'OFF')) {
    $group = @($samples | Where-Object { $_.mode -eq $mode })
    foreach ($field in @('pool', 'visible', 'obstacle_draws', 'width', 'height', 'frames')) {
        if (@($group | ForEach-Object { $_.$field } | Select-Object -Unique).Count -ne 1) {
            throw "Scene changed between repeats ($field); keep raw samples and repeat with a fixed scene."
        }
    }
    $times = @($group.frame_ms | Sort-Object)
    $middle = [int][Math]::Floor($times.Count / 2)
    $median = if ($times.Count % 2) { $times[$middle] } else { ($times[$middle - 1] + $times[$middle]) / 2 }
    $summary += [pscustomobject][ordered]@{
        mode = $mode; pool = $group[0].pool; visible = $group[0].visible;
        obstacle_draws = $group[0].obstacle_draws; width = $group[0].width;
        height = $group[0].height; runs = $Runs; frames_per_run = $group[0].frames;
        median_frame_ms = [Math]::Round($median, 6);
        fps_from_median_frame_time = [Math]::Round(1000.0 / $median, 3);
        minimum_frame_ms = $times[0]; maximum_frame_ms = $times[-1]
    }
}
$summary | Export-Csv -LiteralPath (Join-Path $OutputDirectory 'summary.csv') -NoTypeInformation -Encoding UTF8

$openGl = [string]($firstLog | Where-Object { $_ -like 'OpenGL:*' } | Select-Object -First 1)
$renderer = [string]($firstLog | Where-Object { $_ -like 'Renderer:*' } | Select-Object -First 1)
$drivers = @()
$os = [Environment]::OSVersion.VersionString
if ($env:OS -eq 'Windows_NT') {
    try { $drivers = @(Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion) } catch { }
    try { $os = (Get-CimInstance Win32_OperatingSystem).Caption } catch { }
}
$commit = 'unknown'
$dirty = $null
if (Get-Command git -ErrorAction SilentlyContinue) {
    try {
        $gitRoot = & git -C $repoRoot rev-parse --show-toplevel 2>$null
        if ($LASTEXITCODE -eq 0 -and [IO.Path]::GetFullPath([string]$gitRoot) -eq [IO.Path]::GetFullPath($repoRoot)) {
            $commit = [string](& git -C $repoRoot rev-parse HEAD)
            & git -C $repoRoot diff --quiet HEAD -- src shaders CMakeLists.txt cmake third_party
            $dirty = $LASTEXITCODE -ne 0
        }
    } catch { $commit = 'unknown'; $dirty = $null }
}
$metadata = [ordered]@{
    captured_utc = [DateTime]::UtcNow.ToString('o'); operating_system = $os;
    opengl = $openGl; active_renderer = $renderer; video_drivers = $drivers;
    configuration = $Configuration; source_commit = $commit; tracked_source_modified = $dirty;
    executable_sha256 = (Get-FileHash -LiteralPath $Executable -Algorithm SHA256).Hash;
    runs = $Runs; requested_instances = $Instances;
    method = 'Fixed stress scene, every requested instance submitted, 1280x720, 20 warmup frames and 120 timed frames per mode; glFinish after each rendered frame. CPU wall time, not a GPU timestamp query. Update, instance upload, HUD and buffer swap are excluded.';
    limitation = 'Submitting an instance does not guarantee that all of its pixels are visible: instances can overlap or be occluded. Check normal gameplay FPS separately on the target lab PC.'
}
$metadata | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'machine-and-method.json') -Encoding UTF8
$report = @(
    '# Rendering performance', '', $openGl, $renderer, '',
    '| Mode | Visible | Draw calls | Median frame ms | FPS from median time |',
    '|---|---:|---:|---:|---:|'
)
foreach ($row in $summary) {
    $report += '| {0} | {1} | {2} | {3} | {4} |' -f $row.mode, $row.visible, $row.obstacle_draws, $row.median_frame_ms.ToString('F6', $culture), $row.fps_from_median_frame_time.ToString('F3', $culture)
}
$report += @('', $metadata.method, '', $metadata.limitation, '', 'Raw repetitions: samples.csv. Environment and executable hash: machine-and-method.json.')
$report | Set-Content -LiteralPath (Join-Path $OutputDirectory 'report-table.md') -Encoding UTF8
$summary | Format-Table -AutoSize
Write-Host "Results saved to: $OutputDirectory"
