package org.nkrapivindev.remove;

public class REMoveSizeOfException extends REMoveException {
    public final int expectedSize, gotSize;

    public REMoveSizeOfException(int expected, int got) {
        super(
                "Invalid packet size, expected " + expected + ", got " + got
        );

        expectedSize = expected;
        gotSize = got;
    }
}
