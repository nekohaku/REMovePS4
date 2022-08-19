package org.nkrapivindev.remove;

public class REMoveNotSupportedException extends REMoveException {
    public final String whatNotSupported;

    public REMoveNotSupportedException(String what) {
        super(what + " is not supported");
        whatNotSupported = what;
    }
}
