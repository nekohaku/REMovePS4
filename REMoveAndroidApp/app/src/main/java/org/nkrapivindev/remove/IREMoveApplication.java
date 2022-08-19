package org.nkrapivindev.remove;

import java.util.ArrayList;

public interface IREMoveApplication {
    // The client thread wants the app to report an error,
    // usually this means that the server had been killed as well.
    // (non-blocking)
    void onException(Exception exc);

    // The client thread wants the app to show the user picker
    // (non-blocking)
    void onShowUserDialog(ArrayList<REMoveUser> userArrayList, IREMoveUserPickerCallback c);

    // The client thread wants the app to update the LED color
    // (non-blocking)
    void onColorUpdate(int red, int green, int blue);

    // The client thread wants the app to update the motor
    // (non-blocking)
    void onMotorUpdate(int motorValue);

    // The client thread polls app's data
    REMoveApplicationData onProvideData();
}
