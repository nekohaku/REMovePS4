package org.nkrapivindev.remove;

import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;

public class REMoveStream {
    private final byte[] buf;
    private int pos;
    private final Charset charset;

    public REMoveStream(byte[] byteArray) {
        buf = byteArray;
        pos = 0;
        // all PS4 strings are in UTF-8 so we use that.
        charset = StandardCharsets.UTF_8;
    }

    public int readByte() {
        int v = buf[pos++] & 0xff;
        return v;
    }

    public int readInt16() {
        int v1 = readByte();
        int v2 = readByte();
        int v = v1 | (v2 << 8);
        return v;
    }

    public int readInt32() {
        int v1 = readInt16();
        int v2 = readInt16();
        int v = v1 | (v2 << 16);
        return v;
    }

    public float readFloat() {
        int v1 = readInt32();
        float v = Float.intBitsToFloat(v1);
        return v;
    }

    public String peekCString() {
        int end = pos;
        while (buf[end++] != 0);
        int length = (end - pos) - 1;
        String v = new String(buf, pos, length, charset);
        return v;
    }

    public String readFixedCString(int sizeofArray) {
        String s = peekCString();
        skip(sizeofArray);
        return s;
    }

    public void writeByte(int v) {
        buf[pos++] = (byte) v;
    }

    public void writeInt16(int v) {
        writeByte(v & 0xff);
        writeByte((v >>> 8) & 0xff);
    }

    public void writeInt32(int v) {
        writeInt16(v & 0xffff);
        writeInt16((v >>> 16) & 0xffff);
    }

    public void writeFloat(float v) {
        int raw = Float.floatToRawIntBits(v);
        writeInt32(raw);
    }

    public void pokeCString(String v) {
        byte[] bytes = v.getBytes(charset);
        int i;
        for (i = 0; i < bytes.length; ++i) {
            buf[pos + i] = bytes[i];
        }
        // null terminator
        buf[pos + i] = 0;
    }

    public void writeFixedCString(String v, int sizeofArray) {
        pokeCString(v);
        skip(sizeofArray);
    }

    public void rewind() {
        pos = 0;
    }

    public void skip(int byAmount) {
        pos += byAmount;
    }
}
