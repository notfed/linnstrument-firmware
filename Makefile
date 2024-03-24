default: build/linnstrument-firmware.ino
build/linnstrument-firmware.ino:
	arduino-cli compile --fqbn arduino:sam:arduino_due_x_dbg --build-path build linnstrument-firmware.ino
upload: build/linnstrument-firmware.ino
	(cd build && open -n 'LinnStrument Updater 2.3.3.app') || exit 'Oops: download "LinnStrument Updater 2.3.3.app" into build/'
upload-with-full-reflash:
	arduino-cli upload --fqbn arduino:sam:arduino_due_x_dbg --input-dir build -p /dev/cu.usbmodem101 
