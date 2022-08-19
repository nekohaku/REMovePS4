package org.nkrapivindev.remove;


import java.util.ArrayList;


public class REMoveNetPacketHelloV1 implements IREMoveNetPacket {
    public static final int EXPECTED_SIZEOF = 92;
    public static final int EXPECTED_SIZEOF_USER_STRING = 17;
    public static final int EXPECTED_PROTOCOL = 1;

    private ArrayList<REMoveUser> users;

    @Override
    public void fromStream(REMoveStream s) throws REMoveException { ;
        int sizeThis = s.readInt32();
        if (sizeThis != EXPECTED_SIZEOF) {
            throw new REMoveSizeOfException(EXPECTED_SIZEOF, sizeThis);
        }

        int serverVersion = s.readInt32();
        if (serverVersion != EXPECTED_PROTOCOL) {
            throw new REMoveProtocolException(EXPECTED_PROTOCOL, serverVersion);
        }

        // ;-; I'm sorry...
        int user1 = s.readInt32();
        int user2 = s.readInt32();
        int user3 = s.readInt32();
        int user4 = s.readInt32();
        String username1 = s.readFixedCString(EXPECTED_SIZEOF_USER_STRING);
        String username2 = s.readFixedCString(EXPECTED_SIZEOF_USER_STRING);
        String username3 = s.readFixedCString(EXPECTED_SIZEOF_USER_STRING);
        String username4 = s.readFixedCString(EXPECTED_SIZEOF_USER_STRING);

        users = new ArrayList<REMoveUser>();

        if (user1 != -1 && user1 != 0) {
            users.add(new REMoveUser(user1, username1));
        }

        if (user2 != -1 && user2 != 0) {
            users.add(new REMoveUser(user2, username2));
        }

        if (user3 != -1 && user3 != 0) {
            users.add(new REMoveUser(user3, username3));
        }

        if (user4 != -1 && user4 != 0) {
            users.add(new REMoveUser(user4, username4));
        }
    }

    @Override
    public void toStream(REMoveStream s) throws REMoveException {
        throw new REMoveNotSupportedException("toStream");
    }

    public ArrayList<REMoveUser> getUsers() {
        return users;
    }
}
