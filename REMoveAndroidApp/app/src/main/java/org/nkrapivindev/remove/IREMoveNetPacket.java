package org.nkrapivindev.remove;

public interface IREMoveNetPacket {
    void fromStream(REMoveStream s) throws REMoveException;

    void toStream(REMoveStream s) throws REMoveException;
}
