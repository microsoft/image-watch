<#
.SYNOPSIS
Updates an XML file in place and removes XML nodes.

.DESCRIPTION
Updates an XML file in place and removes XML nodes.
Will not change the file if the nodes aren't found.

.PARAMETER File
Full file path.

.PARAMETER Nodes
XML Node names to remove.

.PARAMETER DestinationPath
Place the new file in different destination directory.

.PARAMETER NumPathComponents
When DestinationPath is provided, use this many path components to construct
a new path.
Example: Providing '2' for "C:\Directory\File.xml" will keep "Directory\File.txt".
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string] $Files,

    [string[]] $Nodes,

    [string] $DestinationPath,

    [int] $NumPathComponents = 1
)

$ErrorActionPreference = "Stop"

function GetDestinationFile([string] $file)
{
    if ($DestinationPath -and $file)
    {
        $fileComponents = ""
        for ($iteration = 1; $iteration -le $NumPathComponents; $iteration++)
        {
            $pathComponent = Split-Path -Path $file -Leaf
            $file = Split-Path -Path $file

            if (!$file)
            {
                # 'pathComponent' is only the drive letter.
                break
            }

            if ($fileComponents)
            {
                $fileComponents = Join-Path $pathComponent $fileComponents
            }
            else
            {
                $fileComponents = $pathComponent
            }

        }

        Join-Path $DestinationPath $fileComponents
    }
    else
    {
        Resolve-Path $file
    }
}

function ProcessFile([string] $file)
{
    $file = Get-Item -Path $file
    $xml = New-Object Xml.XmlDocument
    $xml.Load($file)

    $elementsRemoved = $false

    foreach ($nodeName in $Nodes)
    {
        $xmlNodes = $xml.SelectNodes("//*[local-name()='$nodeName']")
        foreach ($xmlNode in $xmlNodes)
        {
            $xmlNode.ParentNode.RemoveChild($xmlNode) | Out-Null
            $elementsRemoved = $true
        }
    }

    if ($elementsRemoved)
    {
        $destinationFile = GetDestinationFile $file

        $directory = Split-Path -Path $destinationFile
        if (-not (Test-Path $directory))
        {
            mkdir "$directory" | Out-Null
        }

        $xml.Save($destinationFile)
    }
}

function Main()
{
    if ($Nodes.Count -eq 0)
    {
        return
    }

    if ($DestinationPath -and $NumPathComponents -le 0)
    {
        Write-Error "NumPathComponents must be greater than 0."
        exit 1
    }
    elseif ($DestinationPath)
    {
        $DestinationPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($DestinationPath)
    }

    $Files = $Files -split ";"

    foreach ($file in $Files)
    {
        ProcessFile $file
    }
}

Main
