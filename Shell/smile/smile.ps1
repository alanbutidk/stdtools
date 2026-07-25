[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$Host.UI.RawUI.CursorSize = 0 # Hide cursor

# Define face components
$faceOpen = @(
    "       ———             ———",
    "        (•)           (•)",
    "               ||",
    "          |           |",
    "           \         /",
    "            —————————",
    "          	    Hi!"
)

$faceClosed = @(
    "       ———             ———",
    "        (-)           (-)",
    "               ||",
    "          |           |",
    "           \         /",
    "            —————————",
    "               Hi!"
)

$colors = @('Cyan', 'Green', 'Yellow', 'Magenta', 'Red', 'White')
$colorIndex = 0
$frameCount = 0

# Position and Velocity (Speed directions)
$currentX = 0
$currentY = 0
$dx = 1  # 1 = right, -1 = left
$dy = 1  # 1 = down, -1 = up

Clear-Host

try {
    while ($true) {
        # Dynamically sample window bounds
        $windowWidth = $Host.UI.RawUI.WindowSize.Width
        $windowHeight = $Host.UI.RawUI.WindowSize.Height
        
        $maxRight = $windowWidth - 40 # 40 accounts for artwork width
        $maxBottom = $windowHeight - 9 # 9 accounts for artwork height and shell padding
        
        # Guard against small window sizes
        if ($maxRight -lt 1) { $maxRight = 1 }
        if ($maxBottom -lt 1) { $maxBottom = 1 }

        # Determine eye state
        if ($frameCount -eq 12) {
            $activeFace = $faceClosed
            $frameCount = 0
            $isBlinking = $true
        } else {
            $activeFace = $faceOpen
            $frameCount++
            $isBlinking = $false
        }

        # Select color loop
        $currentColor = $colors[$colorIndex]
        $colorIndex = ($colorIndex + 1) % $colors.Count

        # Instead of SetCursorPosition(0,0), clear the specific box of the old position
        # This completely stops vertical line flickering across the screen
        [Console]::SetCursorPosition($currentX, $currentY)
        $paddingSpace = " " * 40
        for ($i = 0; $i -lt 8; $i++) {
            if (($currentY + $i) -lt $windowHeight) {
                [Console]::SetCursorPosition($currentX, $currentY + $i)
                Write-Host $paddingSpace -NoNewline
            }
        }

        # Calculate next position physics
        if (-not $isBlinking) {
            $currentX += $dx
            $currentY += $dy
            $hitWall = $false

            # Horizontal Bounce Physics
            if ($currentX -ge $maxRight) {
                $currentX = $maxRight
                $dx = -1
                $hitWall = $true
            } elseif ($currentX -le 0) {
                $currentX = 0
                $dx = 1
                $hitWall = $true
            }

            # Vertical Bounce Physics
            if ($currentY -ge $maxBottom) {
                $currentY = $maxBottom
                $dy = -1
                $hitWall = $true
            } elseif ($currentY -le 0) {
                $currentY = 0
                $dy = 1
                $hitWall = $true
            }

            # Inject RANDOM variance into velocities upon hitting walls
            if ($hitWall) {
                # Randomly alter velocities to break standard predictable loops
                if ((Get-Random -Minimum 0 -Maximum 2) -eq 1) { $dx = -$dx }
                if ((Get-Random -Minimum 0 -Maximum 2) -eq 1) { $dy = -$dy }
            }
        }

        # Draw the face at its new target coordinates
        for ($i = 0; $i -lt $activeFace.Count; $i++) {
            $targetY = $currentY + $i
            if ($targetY -lt $windowHeight) {
                [Console]::SetCursorPosition($currentX, $targetY)
                Write-Host $activeFace[$i] -ForegroundColor $currentColor -NoNewline
            }
        }

        # Render timing cadence
        if ($isBlinking) {
            Start-Sleep -Milliseconds 180
        } else {
            Start-Sleep -Milliseconds 50  # Lower milliseconds = faster movement
        }
    }
}
finally {
    # Restore cursor on exit
    $Host.UI.RawUI.CursorSize = 25
    Clear-Host
    Write-Host "Goodbye!" -ForegroundColor Green
}
