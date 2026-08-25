param(
    [switch]$IncludeLinuxE2E,
    [switch]$Reconfigure,
    [int]$Jobs = 3
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$testsRoot = Join-Path $repo 'tests'
$matrixRoot = Join-Path $repo 'build/test-matrix'

if ($Jobs -lt 1) {
    throw 'Jobs must be at least 1.'
}

New-Item -ItemType Directory -Force $matrixRoot | Out-Null
$results = [System.Collections.Generic.List[object]]::new()
$projects = Get-ChildItem -LiteralPath $testsRoot -Filter '*.pro' -File |
    Where-Object { $IncludeLinuxE2E -or $_.Name -ne 'transfer-service-linux-e2e-test.pro' } |
    Sort-Object Name

foreach ($project in $projects) {
    $targetLine = Select-String -LiteralPath $project.FullName -Pattern '^\s*TARGET\s*=' |
        Select-Object -First 1
    $target = if ($targetLine) {
        ($targetLine.Line -replace '^\s*TARGET\s*=\s*', '').Trim()
    } else {
        [IO.Path]::GetFileNameWithoutExtension($project.Name)
    }

    $buildName = [IO.Path]::GetFileNameWithoutExtension($project.Name)
    $buildDir = Join-Path $matrixRoot $buildName
    $reportPath = Join-Path $buildDir 'test-report.xml'
    New-Item -ItemType Directory -Force $buildDir | Out-Null

    Push-Location $buildDir
    try {
        $qmakePath = Join-Path '..\..\..\tests' $project.Name
        if ($Reconfigure -or -not (Test-Path 'Makefile')) {
            & qmake $qmakePath -o Makefile
            if ($LASTEXITCODE -ne 0) {
                $results.Add([pscustomobject]@{ Project = $project.Name; Target = $target; Status = 'qmake-failed' })
                continue
            }
        }

        & mingw32-make "-j$Jobs"
        if ($LASTEXITCODE -ne 0) {
            $results.Add([pscustomobject]@{ Project = $project.Name; Target = $target; Status = 'build-failed' })
            continue
        }

        $executable = Join-Path 'release' ($target + '.exe')
        if (-not (Test-Path $executable)) {
            $candidate = Get-ChildItem -LiteralPath 'release' -Filter '*.exe' -File | Select-Object -First 1
            if ($candidate) {
                $executable = $candidate.FullName
            } else {
                $results.Add([pscustomobject]@{ Project = $project.Name; Target = $target; Status = 'executable-missing' })
                continue
            }
        }

        & $executable -xml -o $reportPath
        $status = if ($LASTEXITCODE -eq 0) { 'passed' } else { 'test-failed' }
        $results.Add([pscustomobject]@{ Project = $project.Name; Target = $target; Status = $status })
    } finally {
        Pop-Location
    }
}

$results | Format-Table -AutoSize
$failed = @($results | Where-Object { $_.Status -ne 'passed' })
Write-Output ("QtTest matrix: {0} projects, {1} passed, {2} failed. Linux E2E included: {3}." -f
    $results.Count, ($results.Count - $failed.Count), $failed.Count, $IncludeLinuxE2E.IsPresent)

if ($failed.Count -gt 0) {
    exit 1
}
