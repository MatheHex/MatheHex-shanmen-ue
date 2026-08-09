[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet(
        'BeginPrompt',
        'MarkExecuted',
        'MarkReportWritten',
        'MarkAttachmentVisible',
        'MarkReportSent',
        'MarkNextPrompt',
        'ResumeBlocked',
        'MarkBlocked')]
    [string]$Action,
    [Parameter(Mandatory)][string]$TaskId,
    [string]$PromptPath,
    [string]$ReportPath,
    [string]$AttachmentName,
    [string]$NextPromptPath,
    [string]$Diagnostic,
    [string]$LedgerPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$project = Resolve-ShanmenProject -AllowMissingEngine

if ($TaskId -notmatch '^[A-Za-z0-9._-]+$') {
    throw "Invalid TaskId: $TaskId"
}
if ([string]::IsNullOrWhiteSpace($LedgerPath)) {
    $LedgerPath = Join-Path $project.ProjectRoot 'Saved\Handoff\handoff-ledger.json'
}
$LedgerPath = [IO.Path]::GetFullPath($LedgerPath)
if (-not (Test-ShanmenPathWithin -Candidate $LedgerPath -Parent (Join-Path $project.ProjectRoot 'Saved\Handoff'))) {
    throw 'Handoff ledger must stay under Saved\Handoff.'
}

function Get-FileIdentity {
    param([Parameter(Mandatory)][string]$Path)
    $full = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        throw "Required handoff file missing: $full"
    }
    $item = Get-Item -LiteralPath $full
    [ordered]@{
        path = $full
        file = $item.Name
        bytes = $item.Length
        sha256 = (Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash.ToUpperInvariant()
    }
}

if (Test-Path -LiteralPath $LedgerPath -PathType Leaf) {
    $ledger = Get-Content -LiteralPath $LedgerPath -Raw | ConvertFrom-Json
}
else {
    $ledger = [pscustomobject]@{
        schema_version = 2
        current_task_id = $null
        state = 'EMPTY'
        prompt = $null
        report = $null
        attachment = $null
        marker = $null
        next_prompt = $null
        blocked_diagnostic = $null
        blocked_from_state = $null
        history = @()
    }
}

if (-not $ledger.PSObject.Properties['blocked_from_state']) {
    $ledger | Add-Member -NotePropertyName blocked_from_state -NotePropertyValue $null
}
$ledger.schema_version = 2

function Require-State {
    param([string[]]$Allowed)
    if ($Allowed -notcontains [string]$ledger.state) {
        throw "Invalid handoff transition: action=$Action state=$($ledger.state) allowed=$($Allowed -join ',')."
    }
    if ($Action -ne 'BeginPrompt' -and $ledger.current_task_id -ne $TaskId) {
        throw "Task identity mismatch: ledger=$($ledger.current_task_id) requested=$TaskId."
    }
}

function Add-History {
    param([string]$NewState, [string]$Note)
    $event = [pscustomobject]@{
        utc = (Get-Date).ToUniversalTime().ToString('o')
        task_id = $TaskId
        action = $Action
        state = $NewState
        note = $Note
    }
    $ledger.history = @($ledger.history) + @($event)
    $ledger.state = $NewState
}

switch ($Action) {
    'BeginPrompt' {
        Require-State -Allowed @('EMPTY', 'NEXT_PROMPT_HASHED')
        if ([string]::IsNullOrWhiteSpace($PromptPath)) { throw 'BeginPrompt requires -PromptPath.' }
        $prompt = Get-FileIdentity -Path $PromptPath
        $promptRoot = Join-Path $project.ProjectRoot 'Docs\Prompt'
        if (-not (Test-ShanmenPathWithin -Candidate $prompt.path -Parent $promptRoot)) {
            throw 'Prompt must be archived under Docs\Prompt before execution begins.'
        }
        $ledger.current_task_id = $TaskId
        $ledger.prompt = $prompt
        $ledger.report = $null
        $ledger.attachment = $null
        $ledger.marker = $null
        $ledger.next_prompt = $null
        $ledger.blocked_diagnostic = $null
        $ledger.blocked_from_state = $null
        Add-History -NewState 'DOWNLOADED_HASHED' -Note $prompt.sha256
    }

    'MarkExecuted' {
        Require-State -Allowed @('DOWNLOADED_HASHED')
        Add-History -NewState 'EXECUTED' -Note 'Prompt implementation completed; Report not yet handed off.'
    }

    'MarkReportWritten' {
        Require-State -Allowed @('EXECUTED')
        if ([string]::IsNullOrWhiteSpace($ReportPath)) { throw 'MarkReportWritten requires -ReportPath.' }
        $report = Get-FileIdentity -Path $ReportPath
        $reportRoot = Join-Path $project.ProjectRoot 'Docs\Report'
        if (-not (Test-ShanmenPathWithin -Candidate $report.path -Parent $reportRoot)) {
            throw 'Report must be stored under Docs\Report.'
        }
        $ledger.report = $report
        Add-History -NewState 'REPORT_WRITTEN_HASHED' -Note $report.sha256
    }

    'MarkAttachmentVisible' {
        Require-State -Allowed @('REPORT_WRITTEN_HASHED')
        $expected = [string]$ledger.report.file
        if ([string]::IsNullOrWhiteSpace($AttachmentName) -or $AttachmentName -ne $expected) {
            throw "Attachment confirmation must exactly match the Report file: $expected"
        }
        $ledger.attachment = [pscustomobject]@{
            file = $AttachmentName
            confirmed_visible_utc = (Get-Date).ToUniversalTime().ToString('o')
        }
        Add-History -NewState 'ATTACHMENT_VISIBLE' -Note $AttachmentName
    }

    'MarkReportSent' {
        Require-State -Allowed @('ATTACHMENT_VISIBLE')
        $marker = '[CSEMI:REPORT_SENT] {"task_id":"' + $TaskId + '","file":"' + $ledger.report.file + '"}'
        $ledger.marker = [pscustomobject]@{
            text = $marker
            sent_utc = (Get-Date).ToUniversalTime().ToString('o')
        }
        Add-History -NewState 'MARKER_SENT' -Note $marker
    }

    'MarkNextPrompt' {
        Require-State -Allowed @('MARKER_SENT')
        if ([string]::IsNullOrWhiteSpace($NextPromptPath)) { throw 'MarkNextPrompt requires -NextPromptPath.' }
        $nextPrompt = Get-FileIdentity -Path $NextPromptPath
        $promptRoot = Join-Path $project.ProjectRoot 'Docs\Prompt'
        if (-not (Test-ShanmenPathWithin -Candidate $nextPrompt.path -Parent $promptRoot)) {
            throw 'The next Prompt must be archived under Docs\Prompt.'
        }
        $ledger.next_prompt = $nextPrompt
        Add-History -NewState 'NEXT_PROMPT_HASHED' -Note $nextPrompt.sha256
    }

    'ResumeBlocked' {
        Require-State -Allowed @('BLOCKED')
        $resumeState = [string]$ledger.blocked_from_state
        $resumableStates = @(
            'DOWNLOADED_HASHED',
            'EXECUTED',
            'REPORT_WRITTEN_HASHED',
            'ATTACHMENT_VISIBLE',
            'MARKER_SENT')
        if ($resumableStates -notcontains $resumeState) {
            throw "Blocked ledger has no valid resume state: $resumeState"
        }
        $resumeNote = "Resumed from BLOCKED to $resumeState. Previous diagnostic: $($ledger.blocked_diagnostic)"
        $ledger.blocked_diagnostic = $null
        $ledger.blocked_from_state = $null
        Add-History -NewState $resumeState -Note $resumeNote
    }

    'MarkBlocked' {
        Require-State -Allowed @(
            'DOWNLOADED_HASHED',
            'EXECUTED',
            'REPORT_WRITTEN_HASHED',
            'ATTACHMENT_VISIBLE',
            'MARKER_SENT')
        if ([string]::IsNullOrWhiteSpace($Diagnostic)) { throw 'MarkBlocked requires -Diagnostic.' }
        $ledger.blocked_from_state = [string]$ledger.state
        $ledger.blocked_diagnostic = $Diagnostic
        Add-History -NewState 'BLOCKED' -Note $Diagnostic
    }
}

Write-ShanmenJsonAtomic -Path $LedgerPath -Value $ledger
Write-Host "Handoff state: $($ledger.state)"
Write-Host "Ledger: $LedgerPath"
if ($ledger.marker) { Write-Host "Marker: $($ledger.marker.text)" }
