package org.nkrapivindev.remove;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

public class REMoveClient implements Runnable, IREMoveUserPickerCallback {
    public static final int DEFAULT_PORT = 19794;

    private static final int AWAITING_PICKER = -123;

    private String targetHost;
    private int targetPort;
    private Thread targetThread;
    private final IREMoveApplication app;
    private final IREMoveLogger logger;
    private final AtomicBoolean running = new AtomicBoolean(false);
    private final AtomicInteger chosenUser = new AtomicInteger(AWAITING_PICKER);

    public REMoveClient(IREMoveApplication application, IREMoveLogger mylogger) {
        app = application;
        logger = mylogger;
    }

    private void log(String what) {
        if (logger != null) {
            logger.print(what);
        }
    }

    @Override
    public void run() {
        // the thread:
        // outside of the try clause since we need to be extra sure we
        // always close the socket on errors!
        Socket sck = null;
        try {
            // this stuff is GC'd and we don't really care when it's freed
            byte[] buff = new byte[128];
            REMoveStream buffstream = new REMoveStream(buff);
            REMoveUser user;
            ArrayList<REMoveUser> users;
            REMoveNetPacketToClientV1 toUs = new REMoveNetPacketToClientV1();
            REMoveNetPacketFromClientV1 send = new REMoveNetPacketFromClientV1();
            InputStream sckIn = null;
            OutputStream sckOut = null;
            REMoveNetPacketHelloV1 hello = new REMoveNetPacketHelloV1();
            int got;
            REMoveApplicationData appData;
            long startTime;


            while (running.get()) {
                // initialize the socket:
                if (sck == null) {
                    log("Creating a socket");
                    sck = new Socket(targetHost, targetPort);
                    // a very very tight timeout
                    sck.setSoTimeout(1000);
                    sck.setTcpNoDelay(false);
                    sckIn = sck.getInputStream();
                    sckOut = sck.getOutputStream();
                    // wait for the Hello packet
                    log("Waiting for data");
                    got = sckIn.read(buff);
                    if (got != REMoveNetPacketHelloV1.EXPECTED_SIZEOF) {
                        throw new REMoveSizeOfException(
                            REMoveNetPacketHelloV1.EXPECTED_SIZEOF,
                            got
                        );
                    }
                    buffstream.rewind();
                    hello.fromStream(buffstream);
                    // wait through the user picker...
                    users = hello.getUsers();
                    log("Got a user list");
                    if (app != null) {
                        int myuserind = AWAITING_PICKER;
                        chosenUser.set(myuserind);
                        app.onShowUserDialog(users, this);
                        log("Picker shown!");

                        // wait for picker on this thread.
                        while (running.get()) {
                            myuserind = chosenUser.get();
                            if (myuserind == AWAITING_PICKER) {
                                // need to wait till the picker is done...
                                continue;
                            }
                            break;
                        }

                        // still negative? cancelled, kill us
                        if (myuserind < 0) {
                            break;
                        }
                        else {
                            user = users.get(myuserind);
                            log("Chosen user is " + user.getUserName());
                        }
                    }
                    else {
                        // try the first user if we don't have an "App" implemented.
                        // this may crash if we have no users :(
                        user = users.get(0);
                    }
                    // socket is initialized and we know the user we're bound to
                    send.user = user;
                    log("Ready! 1");
                }

                // main loop...
                startTime = System.nanoTime();
                if (app != null) {
                    appData = app.onProvideData();
                    if (appData != null) {
                        send.gyroX = appData.gyroVec3[0];
                        send.gyroY = appData.gyroVec3[1];
                        send.gyroZ = appData.gyroVec3[2];
                        send.accelX = appData.accelVec3[0];
                        send.accelY = appData.accelVec3[1];
                        send.accelZ = appData.accelVec3[2];
                        send.digitalButtonsBitmask = appData.btnBitmask;
                        send.analogTButtonValue = appData.analogTValue;
                        send.sensorTemperature = appData.sensorTemperature;
                    }
                }
                // send it over:
                buffstream.rewind();
                send.toStream(buffstream);
                sckOut.write(
                    buff,
                    0,
                    REMoveNetPacketFromClientV1.EXPECTED_SIZEOF
                );

                // obtain any update frames from the server:
                got = sckIn.read(buff);
                if (got != REMoveNetPacketToClientV1.EXPECTED_SIZEOF) {
                    throw new REMoveSizeOfException(
                        REMoveNetPacketToClientV1.EXPECTED_SIZEOF,
                        got
                    );
                }
                buffstream.rewind();
                toUs.fromStream(buffstream);

                if (app != null) {
                    if ((
                            toUs.updateFlags
                            & REMoveNetPacketToClientV1.UPDATE_FLAG_SET_LIGHTSPHERE_COLOR) != 0) {
                        // we have color update data, send that
                        app.onColorUpdate(toUs.red, toUs.green, toUs.blue);
                    }

                    if ((
                            toUs.updateFlags
                            & REMoveNetPacketToClientV1.UPDATE_FLAG_SET_VIBRATION) != 0) {
                        // we have vibration data, send that
                        app.onMotorUpdate(toUs.motorValue);
                    }
                }

                long estimatedTime = System.nanoTime() - startTime;

                if ((System.currentTimeMillis() % 5000) < 10) {
                    log("Estimated time = " + (((double) estimatedTime) / 1000000) + "ms");
                }
            }

            log("The thread is requesting a close gracefully...");
        }
        catch (Exception threadException) {
            if (app != null) {
                app.onException(threadException);
            }
        }
        finally {
            try {
                // we may have thrown while running was true
                running.set(false);
                // this is kinda dangerous but who cares...
                targetThread = null;
                // close the socket
                if (sck != null) {
                    sck.close();
                    sck = null;
                }
            }
            catch (Exception ignore) {
                // ignore any exceptions here...
            }
        }
    }

    public void setClientInfo(String host, int port) {
        targetHost = host;
        targetPort = port;
    }

    public void stopServer() {
        try {
            if (targetThread == null) {
                log("Thread is already stopped");
                return;
            }

            log("Setting running flag to false");
            running.set(false);
            log("Joining thread");
            targetThread.join();
            targetThread = null;
            log("Stopped OK");
        }
        catch (Exception exc) {
            if (app != null) {
                log("an exception in stopServer " + exc);
                app.onException(exc);
            }
        }
    }

    public void startServer() {
        try {
            if (targetThread != null) {
                stopServer();
            }

            running.set(true);
            targetThread = new Thread(this);
            targetThread.start();
            log("Server thread started...");
        }
        catch (Exception exc) {
            if (app != null) {
                app.onException(exc);
            }
        }
    }

    public boolean isRunning() {
        try {
            // do we have a running server thread?
            return targetThread != null;
        }
        catch (Exception exc) {
            if (app != null) {
                app.onException(exc);
            }

            return false;
        }
    }

    @Override
    public void onPickerDone(int indexOrNegative) {
        try {
            if (indexOrNegative < 0) {
                chosenUser.set(-1);
            } else {
                chosenUser.set(indexOrNegative);
            }
        }
        catch (Exception exc) {
            if (app != null) {
                app.onException(exc);
            }
        }
    }
}
