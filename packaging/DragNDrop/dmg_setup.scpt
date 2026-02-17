-- The variable 'dmg_name' is passed in by CPack
on run {dmg_name}
    tell application "Finder"
        tell disk dmg_name
            open

            -- Set the view mode to icon view without all the decorations
            set the current view of container window to icon view
            set the toolbar visible of container window to false
            set the statusbar visible of container window to false
            set sidebar width of container window to 0

            -- Set window geometry to match background image (Top, Left, Bottom, Right)
            set the bounds of container window to {400, 100, 1000, 522}

            -- Set the .app and Application icon sizes and positions
            set theViewOptions to the icon view options of container window
            set icon size of theViewOptions to 128
            set arrangement of theViewOptions to not arranged
            set position of item "OpenSim Creator.app" of container window to {140, 168}
            set position of item "Applications" of container window to {460, 168}

            set the background picture of icon view options of container window to file ".background:background.png"

            -- Open and close a finder window to encourage MacOS to write a .DS_Store
            close
            open
            update without registering applications
            delay 3 -- Give Finder a moment to write the .DS_Store
            close
            delay 2 -- Give it another chance to write it
            open
            delay 1
        end tell
    end tell
end run