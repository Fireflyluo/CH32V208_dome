$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[Console]::InputEncoding = $utf8NoBom
[Console]::OutputEncoding = $utf8NoBom
$OutputEncoding = $utf8NoBom

try { chcp 65001 > $null } catch {}

if ($PSVersionTable.PSVersion.Major -ge 7) {
  $PSDefaultParameterValues['Out-File:Encoding'] = 'utf8NoBOM'
  $PSDefaultParameterValues['Set-Content:Encoding'] = 'utf8NoBOM'
  $PSDefaultParameterValues['Add-Content:Encoding'] = 'utf8NoBOM'
} else {
  $PSDefaultParameterValues['Out-File:Encoding'] = 'utf8'
  $PSDefaultParameterValues['Set-Content:Encoding'] = 'utf8'
  $PSDefaultParameterValues['Add-Content:Encoding'] = 'utf8'
}

