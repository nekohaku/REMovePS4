package org.nkrapivindev.remove;

public class REMoveNetPacketFromClientV2 implements IREMoveNetPacket {
    public static final int EXPECTED_SIZEOF = 117;

    // to which user we belong?
    public REMoveUser user;
    // main data:
    public float
        accelX, accelY, accelZ,
        gyroX, gyroY, gyroZ;
    // button data:
    public int
        digitalButtonsBitmask,
        analogTButtonValue;
    // extension port data:
    public int
        uStatus,
        uDigital1, uDigital2,
        uAnalogRightX, uAnalogRightY,
        uAnalogLeftX, uAnalogLeftY;
    public byte[] baCustom = new byte[5];
    // device info data:
    public float
        sensorTemperature,
        sphereRadius,
        accelToSphereX, accelToSphereY, accelToSphereZ;
    // v2 extensions:
    public int uiExtensionPortId;
    public byte[] baExtensionPortDeviceInfo = new byte[38];

    @Override
    public void fromStream(REMoveStream s) throws REMoveException {
        // because we have a `user` field, but we cannot obtain the username
        // from this packet alone. as such this operation is NOT supported.
        throw new REMoveNotSupportedException("fromStream");
    }

    @Override
    public void toStream(REMoveStream s) throws REMoveException {
        if (user == null) {
            throw new REMoveUserException();
        }

        s.writeInt32(EXPECTED_SIZEOF);
        s.writeInt32(user.getUserHandle());
        s.writeFloat(accelX);
        s.writeFloat(accelY);
        s.writeFloat(accelZ);
        s.writeFloat(gyroX);
        s.writeFloat(gyroY);
        s.writeFloat(gyroZ);
        s.writeFloat(sensorTemperature);
        s.writeFloat(sphereRadius);
        s.writeFloat(accelToSphereX);
        s.writeFloat(accelToSphereY);
        s.writeFloat(accelToSphereZ);
        s.writeInt16(digitalButtonsBitmask);
        s.writeInt16(analogTButtonValue);
        s.writeInt16(uStatus);
        s.writeInt16(uDigital1);
        s.writeInt16(uDigital2);
        s.writeInt16(uAnalogRightX);
        s.writeInt16(uAnalogRightY);
        s.writeInt16(uAnalogLeftX);
        s.writeInt16(uAnalogLeftY);
        s.writeBytes(baCustom, 0, baCustom.length);
        // v2 extensions:
        s.writeInt32(uiExtensionPortId);
        s.writeBytes(baExtensionPortDeviceInfo, 0, baExtensionPortDeviceInfo.length);
    }
}
