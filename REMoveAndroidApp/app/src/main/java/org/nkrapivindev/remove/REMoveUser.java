package org.nkrapivindev.remove;

public class REMoveUser {
    private int userHandle;
    private String userName;

    public REMoveUser(int myUserHandle, String myUserName) {
        userHandle = myUserHandle;
        userName = myUserName;
    }

    public int getUserHandle() {
        return userHandle;
    }

    public String getUserName() {
        return userName;
    }
}
