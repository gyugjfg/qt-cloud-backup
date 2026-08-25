param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path $RepositoryRoot).Path

# Keep the dependency direction explicit even though qmake uses flat include paths.
$rules = @(
    @{ Name = 'core -> features'; Source = 'src/core'; Forbidden = 'src/features' },
    @{ Name = 'core -> ui'; Source = 'src/core'; Forbidden = 'src/ui' },
    @{ Name = 'features -> ui'; Source = 'src/features'; Forbidden = 'src/ui' }
)

$includePattern = '^\s*#\s*include\s*["<]([^">]+)[">]'
$violations = [System.Collections.Generic.List[string]]::new()

foreach ($rule in $rules) {
    $sourceRoot = Join-Path $root $rule.Source
    $forbiddenRoot = Join-Path $root $rule.Forbidden
    $forbiddenNames = @{}
    Get-ChildItem -LiteralPath $forbiddenRoot -Recurse -File -Include *.h, *.hpp |
        ForEach-Object { $forbiddenNames[$_.Name] = $true }

    Get-ChildItem -LiteralPath $sourceRoot -Recurse -File -Include *.h, *.hpp, *.cpp |
        ForEach-Object {
            $file = $_
            $lineNumber = 0
            foreach ($line in Get-Content -LiteralPath $file.FullName) {
                $lineNumber++
                $match = [regex]::Match($line, $includePattern)
                if (-not $match.Success) {
                    continue
                }

                $includeName = $match.Groups[1].Value
                $includeFileName = Split-Path -Leaf $includeName
                if ($forbiddenNames.ContainsKey($includeFileName)) {
                    $relativeFile = [IO.Path]::GetRelativePath($root, $file.FullName)
                    $violations.Add("$($rule.Name): $relativeFile`:$lineNumber includes $includeName")
                }
            }
        }
}

if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Output 'Layer dependency guard passed: core/features do not include higher UI layers.'
