# REMove

This is basically PS Move for the poor.

# How to install?

- For Mira CFW: Copy `libREMove.sprx` to `/data/mira/substitute/CUSAXXXXX/`, where `CUSAXXXXX` is the game you wish to patch.
- For GoldHEN (TODO!!!! NOT FINISHED!!!!): Rename `libREMove.sprx` to `fps.prx` and copy to `/data/GoldHEN/`.
- Install the APK on your device.
- Open the app, go to Settings and set your Console IP, usually you do not need to touch the port.
- Run the game you wanted to patch, press "CONNECT" in the app, select your user, enjoy!
- If the Android app goes into background the connection will stop and you will have to reconnect, this is because of Android rules.

# Limitations

- Requires a decent modern Android phone with a **good** accelerometer, gyroscope, and preferrably a magnetometer to improve readings.
- This simulates the PS Move only in the game process, the System UI (also keyboards and PSN dialogs :p) know nothing about our funny simulation.
- While you can log in as multiple users, the users need to be signed in to the game, as I cannot sign in for you.
  Hint: "Switch User" in the PS4 Quick Menu.
- You may encounter sudden movement frame drops, that is sadly normal, I physically cannot poll the device as fast as the Move is.
- Games using the "MoveTracker" library (the ones that track the Move through the Camera) will not work due to physical shape differences.

# iOS?

If you have the money to buy an Apple phone, you have the money to buy four PS Moves and a PS Camera, GO AWAY FROM HERE!

# Windows Phone? Sailfish? UBPorts?

Maybe, the actual TCP protocol is dumb simple, but again, feel free to PR.

UPD: Now explained in `PROTOCOL.md`!

# Credits

- zhilemann, BlackDoomer, `[?] aka Der Leberkässemmel Fan` - answering my stupid questions about physics.
- sleirsgoevy - answering my stupid questions about PS4.
- Sony - being a pain in the ass, as usual. (we `<3` sony)
