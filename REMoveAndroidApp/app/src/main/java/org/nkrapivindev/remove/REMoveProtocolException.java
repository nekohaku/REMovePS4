package org.nkrapivindev.remove;

public class REMoveProtocolException extends REMoveException {
    public final int expectedProtocol, gotProtocol;

    public REMoveProtocolException(int expected, int got) {
        super(
                "Invalid protocol version, expected " + expected + ", got " + got
        );

        expectedProtocol = expected;
        gotProtocol = got;
    }
}
