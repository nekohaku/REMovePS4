package org.nkrapivindev.remove;

public class REMoveNetPacketToClientV2 implements IREMoveNetPacket {
    public static final int EXPECTED_SIZEOF = 52;
    public static final int UPDATE_FLAG_SET_NONE = 0;
    public static final int UPDATE_FLAG_SET_VIBRATION = 1;
    public static final int UPDATE_FLAG_SET_LIGHTSPHERE_COLOR = 1 << 1;
    public static final int UPDATE_FLAG_SET_EXTENSION_DATA_V2 = 1 << 2;

    public int updateFlags;
    public int red, green, blue, motorValue;
    public byte[] extData = new byte[40];

    @Override
    public void fromStream(REMoveStream s) throws REMoveException {
        int sizeThis = s.readInt32();
        if (sizeThis != EXPECTED_SIZEOF) {
            throw new REMoveSizeOfException(EXPECTED_SIZEOF, sizeThis);
        }

        updateFlags = s.readInt32();
        red = s.readByte();
        green = s.readByte();
        blue = s.readByte();
        motorValue = s.readByte();
        s.readBytesInto(extData, 0, extData.length);
    }

    @Override
    public void toStream(REMoveStream s) throws REMoveException {
        s.writeInt32(EXPECTED_SIZEOF);
        s.writeInt32(updateFlags);
        s.writeByte(red);
        s.writeByte(green);
        s.writeByte(blue);
        s.writeByte(motorValue);
        s.writeBytes(extData, 0, extData.length);
    }
}
