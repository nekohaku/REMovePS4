package org.nkrapivindev.remove;

public interface IREMoveUserPickerCallback {
    // Call this when you want to submit the user picker result
    // Must be -1 if no user was chosen (will shut down the server)
    // Or >=0 which is the user index in the users List that was given to you.
    void onPickerDone(int indexOrNegative);
}

