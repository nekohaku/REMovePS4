package org.nkrapivindev.remove;

public class REMoveApplicationData {
    // Digital button bitmask, see PS Move headers
    public int btnBitmask;

    // Analog value of the "(T)" button, 0 - released, 255 - full pressed
    public int analogTValue;

    // Gyroscope vec3 in PS Move format
    public float[] gyroVec3;

    // Accelerometer vec3 in PS Move format
    public float[] accelVec3;

    // You do not have to set this. In PS Move format.
    public float sensorTemperature;
}
