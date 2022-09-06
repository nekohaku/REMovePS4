# REMove Communication Protocol

The protocol is dumb simple, but there are still some key things it assumes about the client and network:

- All packets that are sent arrive in order, from first sent to last sent.
- The connection may be interrupted at any time, there is no special packet for that, it is to be handled by the socket implementation.
- All fields and data is in Little Endian, as the PS4 is an AMD64 x86_64 machine.
- All network structs are defined as packed, so there's no alignment.
- `char` is a single byte type, and strings are in null terminated UTF-8 without BOM, as that is the string type for PS4.
- `float` is a 32bit single precision IEEE 754 floating point value.
- Other types are suffixed with C's `_t` and should be cross-platform.
- Gyroscope and Accelerometer data is sent in PS Move's coordinate system and units.

The server implementation is written in C, and can be found in the `remove/` folder.

The Android client implementation is written in Java, and can be found in the `REMoveAndroidApp/` folder.

All files that do not begin with `REMove` are just Android activities and the code will function without them
if copied to a different project.

There can be many client implementations but the Java implementation is the default one,
it is provided only as a reference implementation and is not to be taken seriously (because Java...)

Also, this README assumes Protocol `Version 2`, as `Version 1` is not supported and you should upgrade.

All `Version 2` changes are marked with `// -- V2 context extension` in definitions.

# Definitions

```c
/* 4 controllers... */
#define REMOVECTX_MAX_CLIENTS 4
```

```c
/* network packet struct attributes, packed is needed */
#define REMOVECTX_PKT_ATTR /* implementation defined, whatever disables alignment for types */
```

```c
/* our current protocol version */
#define REMOVECTX_SERVER_VERSION 2
```

```c
/* 'RM' in little endian ASCII */
#define REMOVECTX_PORT 19794
```

```c
typedef struct REMOVECTX_PKT_ATTR REMoveNetPacketHelloV2 {
    int32_t sizeThis; // size of the whole packet, including this field.
    // must be set to `REMOVECTX_SERVER_VERSION`, client must abort the socket if it doesn't know our protocol
    int32_t serverVersion;
    // current logged on users:
    int32_t user1;
    int32_t user2;
    int32_t user3;
    int32_t user4;
    char username1[16 + 1];
    char username2[16 + 1];
    char username3[16 + 1];
    char username4[16 + 1];
} REMoveNetPacketHelloV2;
```

```c
// treat as little endian bitflags
typedef enum REMoveNetUpdateFlagsV2 {
    // use to initialize a value
    UPDATE_FLAG_SET_NONE = 0,
    // `red`, `green`, `blue` are to be read and applied
    UPDATE_FLAG_SET_VIBRATION = (1 << 0),
    // `motor` is to be read and applied
    UPDATE_FLAG_SET_LIGHTSPHERE_COLOR = (1 << 1),
    // -- V2 context extension {
    // `extData` is to be read and applied
    UPDATE_FLAG_SET_EXTENSION_DATA_V2 = (1 << 2)
    // -- V2 context extension }
} REMoveNetUpdateFlagsV2;
```

```c
// this is the second and onward packet that is sent from the server to the client
typedef struct REMOVECTX_PKT_ATTR REMoveNetPacketToClientV2 {
    int32_t sizeThis; // size of the whole packet, including this field.
    // which information to update?
    int32_t updateFlagsV2; // cast to enum REMoveNetUpdateFlagsV2
    // if flag `UPDATE_FLAG_SET_LIGHTSPHERE_COLOR` is set: ignore otherwise
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    // if flag `UPDATE_FLAG_SET_VIBRATION` is set: ignore otherwise
    uint8_t motor;
    // -- V2 context extension {
    // if flag `UPDATE_FLAG_SET_EXTENSION_DATA` is set: ignore otherwise
    uint8_t extData[40];
    // -- V2 context extension }
} REMoveNetPacketToClientV2;
```

```c
// this is the only packet that is set from the client to the server (otherwise it aborts the socket)
typedef struct REMOVECTX_PKT_ATTR REMoveNetPacketFromClientV2 {
    int32_t sizeThis; // size of the whole packet, including this field.
    int32_t userHandle; // to which user this client belongs.
    /*
        In world coordinate system, in G's, where 1.0f means around 9.8 m/s^2
        with gravity force included.

        Coordinate system hint:

        Supposing the controller is standing vertically still,
        and you are looking right at the front of the controller where the Move button is.

        If standing perfectly still, each axis will be set to around 1/3G,
        and XYZ combined will be around 1G.
        (so .xyz=1.0f/3.0f; and x+y+z == 1.0f;)

        X - points to the right (from the LED sphere to the right)
        Y - points at you (from the LED sphere to you)
        Z - points to the floor (from the LED sphere to the floor)

        The values are relative, not absolute, so you can cheat a little bit when transposing
        to a different coordinate system, Android example:
        (from SensorEvent values to PS Move)
        Gyroscope (only coordinate system):
        X =  v[0]
        Y =  v[2]
        Z = -v[1]
        Accelerometer (coordinate system and units):
        X =  v[0] / G
        Y =  v[2] / G
        Z = -v[1] / G
        where G is the standard android gravity constant.
        Huge thanks to Igor "Gigabaza" Zhilemann!! Ave Vulkan!!
    */
    float accelX;
    float accelY;
    float accelZ;
    /*
        In world coordinate system, in radians per second.
        See accel comment for coordinate system explanation, the unit here is Radins per Second, not G's.
        This should match Android's units except for the coordinate system.
    */
    float gyroX;
    float gyroY;
    float gyroZ;
    // -- these fields are yet to be determined {
    float sensorTemperature;
    float sphereRadius;
    float accelToSphereX;
    float accelToSphereY;
    float accelToSphereZ;
    // -- these fields are yet to be determined }
    uint16_t digitalButtonsBitmask; // see remove/scemove.h for flags
    uint16_t analogTButtonValue; // 0 - not pressed, 0xff - full press
    // -- extension data fields {
    uint16_t uStatus;
    uint16_t uDigital1;
    uint16_t uDigital2;
    uint16_t uAnalogRightX;
    uint16_t uAnalogRightY;
    uint16_t uAnalogLeftX;
    uint16_t uAnalogLeftY;
    uint8_t baCustom[5];
    // -- extension data fields }
    // -- V2 context extension {
    uint32_t uiExtensionPortId;
    uint8_t baExtensionPortDeviceInfo[38];
    // -- V2 context extension }
} REMoveNetPacketFromClientV2;
```

`sizeThis` field must be set to `sizeof()` of the structure, otherwise the connection is to be terminated.

It serves both as a version check and as a sanity check, since you can check the amount of bytes received against `sizeThis`.

See `remove/scemove.h` for button bitmask definitions and other data type stuff.

The `FromClient` packet mostly resembles the `sceMoveData` struct.

# The protocol

- A server starts listening for clients, for up to `REMOVECTX_MAX_CLIENTS` in backlog (if your system supports backlog).

- When a client is connected, if there's already `REMOVECTX_MAX_CLIENTS` clients, the connection is terminated, otherwise:

- Server sends a `REMoveNetPacketHello` packet, with `sizeThis` set to `sizeof()` of the packet and `serverVersion` set to `REMOVECTX_SERVER_VERSION`.

- The client must enumerate users from the packet and show a `User Picker`, `0` or `-1` are invalid user handles and are to be skipped.
  If a `User Picker` has been cancelled, dismissed, or otherwise closed, the connection must be terminated, no packets are sent, otherwise:

- The client stores the chosen user handle and starts listening for motion events, brings up sensors and stuff.

- A `REMoveNetPacketFromClient` packet is prepared, where `sizeThis` is set to `sizeof()` of the packet,
  and `userHandle` is set to the chosen user handle, it cannot be `0` or `-1`.

- Client polls the sensors, buttons, etc and sets the appropriate fields in `REMoveNetPacketFromClient`.

- If some fields are not used or supported then they are set to all bits zero.

- The packet is then sent to the server, if send failed the connection must be terminated.

- Then the client receives a `REMoveNetPacketToClient` packet from the server.

- If the size of the packet does not match what is expected, or there are other issues, connection must be terminated.

- Look at the `updateFlags` field and apply the data you need accordingly, e.g. vibration or LED color.
  (pseudocode: `if ((updateFlags & UPDATE_FLAG_SET_VIBRATION) != 0) applyMotor(pkt.motor);`)

- Jump to Step 6 until the app is terminated or the connection is closed.

- Shut down sensors, prepare for termination if needed.

# Got questions?

Discord - nik (octothorpe) 5351

(I am refusing to call that symbol a h-tag, since that is zoomer cringe)

VK (cringe) - (at) nkrapivindev

Twitter (mega cringe) - (at) nkrapivindev

Telegram (based) - (at) nikthecat

GitHub Issues - you're here!

