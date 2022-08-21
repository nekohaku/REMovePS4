package org.nkrapivindev.remove;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.PreferenceManager;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Bundle;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;

public class MainActivity extends AppCompatActivity implements IREMoveApplication, IREMoveLogger, SensorEventListener {

    private final REMoveClient moveClient = new REMoveClient(this, this);

    private Button btnCross, btnSquare, btnCircle, btnTriangle, btnSelect, btnStart, btnMove, btnT;

    private final AtomicInteger btnMask = new AtomicInteger(0);

    private final REMoveApplicationData appData = new REMoveApplicationData();

    private TextView statusTextView;

    private Vibrator vib;

    private SensorManager sensorManager;
    private Sensor gyroSensor;
    private Sensor accelSensor;

    private float[] lastGyroValues = null;
    private float[] prevAccelValues = null;
    private float[] lastAccelValues = null;
    private final float[] moveGyroValues = new float[]{0, 0, 0};
    private final float[] moveAccelValues = new float[]{
            1.0f/3.0f,
            1.0f/3.0f,
            1.0f/3.0f
    };
    private final Object dataLock = new Object();

    private int lastVibValue = -1;
    private boolean weArePaused = false;
    private SharedPreferences sharedPreferences;

    private final String NOBODY = "(nobody)";
    private String strConnectedTo = NOBODY;
    private String strUsername = NOBODY;

    // > 0 means do filter.
    private float filterFactor = 0.0f;

    private void updateText() {
        statusTextView.setText(
            getString(
                R.string.status_fmt,
                strConnectedTo,
                strUsername
            )
        );
    }

    private void startSensorListening() {
        // start listening for sensor events:
        sensorManager.registerListener(
                this,
                gyroSensor,
                SensorManager.SENSOR_DELAY_FASTEST
        );

        sensorManager.registerListener(
                this,
                accelSensor,
                SensorManager.SENSOR_DELAY_FASTEST
        );
    }

    private void stopSensorListening() {
        sensorManager.unregisterListener(this);

        // invalidate all sensor data we might have:
        // this will send neutral 0,0,0 data to the game
        // if called while the server is running
        lastGyroValues = null;
        lastAccelValues = null;
        prevAccelValues = null;
    }

    private void fullStop() {
        applyLedToUi(0, 0, 0);
        moveClient.stopServer();
        stopSensorListening();
        stopMotor();
        strConnectedTo = NOBODY;
        strUsername = NOBODY;
        updateText();
        applyLedToUi(0, 0, 0);
    }

    @Override
    public boolean onCreateOptionsMenu(@NonNull Menu menu) {
        getMenuInflater()
                .inflate(R.menu.main_app_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int iid = item.getItemId();

        if (iid == R.id.menu_item_open_settings) {
            if (!moveClient.isRunning()) {
                startActivity(new Intent(this, SettingsActivity.class));
            }
            else {
                (new AlertDialog.Builder(this))
                        .setTitle(R.string.err_title)
                        .setMessage(R.string.ui_toast_stop_server)
                        .create()
                        .show();
            }
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @SuppressLint("ClickableViewAccessibility")
    /* I'm not sure blind people can use even the original PS Move :( that's sad though. */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        statusTextView = findViewById(R.id.status_text_view);
        sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);

        findViewById(R.id.button_server_start).setOnClickListener(
            view -> {
                String myHost = sharedPreferences.getString(
                    "settings_hostname",
                    "<hostname not set>"
                );

                int myPort = Integer.parseInt(
                    sharedPreferences.getString(
                        "settings_port",
                        Integer.toString(REMoveClient.DEFAULT_PORT)
                    )
                );

                float myFactor = filterFactor;
                try {
                    myFactor = Float.parseFloat(
                        sharedPreferences.getString(
                            "settings_filter",
                            Float.toString(filterFactor)
                        )
                    );
                }
                catch (Exception ignore) {
                    // just use the previous factor if it fails...
                }

                filterFactor = myFactor;

                String ctxVersion = "ctxV" + REMoveClient.CONTEXT_VERSION;

                // refresh value from prefs.
                Toast.makeText(
                    this,
                    getString(
                        R.string.ui_toast_starting,
                        myHost + ":" + myPort + "," + myFactor + "," + ctxVersion
                    ),
                    Toast.LENGTH_SHORT
                ).show();
                startSensorListening();
                moveClient.setClientInfo(myHost, myPort);

                moveClient.startServer();

                strConnectedTo = myHost + ":" + myPort;
                updateText();
            }
        );

        findViewById(R.id.button_server_stop).setOnClickListener(
            view -> {
                Toast.makeText(
                    this,
                    R.string.ui_toast_stopping,
                    Toast.LENGTH_SHORT
                ).show();
                fullStop();
            }
        );

        View.OnTouchListener btnHandler = (view, event) -> {
            view.performClick();

            int act = event.getAction();
            int mask;

            if (view == btnCross) {
                mask = (1 << 6);
            }
            else if (view == btnCircle) {
                mask = (1 << 5);
            }
            else if (view == btnSquare) {
                mask = (1 << 7);
            }
            else if (view == btnTriangle) {
                mask = (1 << 4);
            }
            else if (view == btnStart) {
                mask = (1 << 3);
            }
            else if (view == btnSelect) {
                mask = (1);
            }
            else if (view == btnT) {
                mask = (1 << 1);
            }
            else if (view == btnMove) {
                mask = (1 << 2);
            }
            else {
                // unknown view?
                return false;
            }

            if (act == MotionEvent.ACTION_DOWN) {
                btnMask.set(btnMask.get() | mask);
            }
            else if (act == MotionEvent.ACTION_UP || act == MotionEvent.ACTION_CANCEL) {
                btnMask.set(btnMask.get() & (~mask));
            }

            return false;
        };

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            VibratorManager vibman = (VibratorManager)
                getSystemService(Context.VIBRATOR_MANAGER_SERVICE);
            if (vibman != null) {
                vib = vibman.getDefaultVibrator();
            }
        }
        else {
            vib = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        }

        btnCross = findViewById(R.id.button_cross);
        btnCircle = findViewById(R.id.button_circle);
        btnSquare = findViewById(R.id.button_square);
        btnTriangle = findViewById(R.id.button_triangle);
        btnStart = findViewById(R.id.button_start);
        btnSelect = findViewById(R.id.button_select);
        btnMove = findViewById(R.id.button_move);
        btnT = findViewById(R.id.button_t);

        btnCross.setOnTouchListener(btnHandler);
        btnCircle.setOnTouchListener(btnHandler);
        btnSquare.setOnTouchListener(btnHandler);
        btnTriangle.setOnTouchListener(btnHandler);
        btnStart.setOnTouchListener(btnHandler);
        btnSelect.setOnTouchListener(btnHandler);
        btnMove.setOnTouchListener(btnHandler);
        btnT.setOnTouchListener(btnHandler);

        sensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        gyroSensor = sensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        accelSensor = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);

        appData.sensorTemperature = 22.2f; // komu not pohui stav'te laik)))00
        appData.gyroVec3 = moveGyroValues;
        appData.accelVec3 = moveAccelValues;

        updateText();

        // show a toast if the onCreate happened without issues
        Toast.makeText(this, R.string.app_ready, Toast.LENGTH_SHORT).show();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        fullStop();
        print("onDestroy is successful. Goodbye!");
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
    }

    private void applyMotorToUi(int newMotor) {
        lastVibValue = newMotor;
        if (vib != null && vib.hasVibrator() && !weArePaused) {
            if (lastVibValue > 63) {
                // the Move physically vibrates for 5 seconds after setting this.
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                    vib.vibrate(
                        VibrationEffect.createOneShot(
                            5 * 1000,
                            lastVibValue
                        )
                    );
                }
                else {
                    vib.vibrate(5 * 1000);
                }
            }
            else {
                vib.cancel();
            }
        }
    }

    private void stopMotor() {
        // don't check for pauses or the motor value, since we can be called in onPause
        if (vib != null && vib.hasVibrator()) {
            vib.cancel();
        }
    }

    private void applyLedToUi(int r, int g, int b) {
        if (!weArePaused) {
            // ???? this works??
            getWindow()
                .getDecorView()
                .setBackgroundColor(Color.rgb(r, g, b));
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        weArePaused = true;
        if (moveClient.isRunning()) {
            // will make the thread send neutral values instead of real ones.
            // (will also invalidate the motion stack)
            stopSensorListening();
            // cancel any current vibration:
            stopMotor();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        weArePaused = false;
        if (moveClient.isRunning()) {
            startSensorListening();
            // resume any vibration:
            applyMotorToUi(lastVibValue);
        }
    }

    @Override
    public void onException(Exception exc) {
        runOnUiThread(
            () -> {
                fullStop();
                (new AlertDialog.Builder(this))
                    .setTitle(R.string.err_title)
                    .setMessage(
                        getString(
                            R.string.err_message,
                            exc.getMessage()
                        )
                    )
                    .create()
                    .show();
            }
        );
    }

    @Override
    public void onShowUserDialog(ArrayList<REMoveUser> list, IREMoveUserPickerCallback c) {
        runOnUiThread(
            () -> {
                CharSequence[] items = new CharSequence[list.size()];
                for (int i = 0; i < items.length; ++i) {
                    REMoveUser it = list.get(i);
                    // Example: "1 - User1 (UID=0x100001)"
                    items[i] = (i + 1) + " - " + it.getUserName()
                            + " - "
                            + "(UID=0x" + Integer.toHexString(it.getUserHandle()) + ")";
                }

                (new AlertDialog.Builder(this))
                    .setTitle(R.string.user_picker_title)
                    .setItems(items,
                        (dialogInterface, i) -> {
                            if (i < 0) {
                                fullStop();
                                return;
                            }

                            strUsername = list.get(i).getUserName();
                            updateText();
                            c.onPickerDone(i);
                        }
                    )
                    .setNegativeButton(R.string.user_picker_nobody,
                        (dialog, i) -> fullStop()
                    )
                    .setOnCancelListener(
                        (dialog) -> fullStop()
                    )
                    .create()
                    .show();
            }
        );
    }

    @Override
    public void onColorUpdate(int red, int green, int blue) {
        // apply LED color:
        runOnUiThread(() -> applyLedToUi(red, green, blue));
    }

    @Override
    public void onMotorUpdate(int motor) {
        /*
            in PS Move format.
            if m>63 the motor will actually start physically spinning
            otherwise it'll stop.
         */

        runOnUiThread(() -> applyMotorToUi(lastVibValue));
    }

    @Override
    public REMoveApplicationData onProvideData() {
        appData.btnBitmask = btnMask.get();
        // if T is pressed digitally, press it analog...ally, awful but who cares?
        appData.analogTValue = ((appData.btnBitmask & (1 << 1)) != 0) ? 0xff : 0x00;

        /*
            PS Move Format:
            Imagine the controller is held vertically straight, with the buttons
            and the logo facing at you.
            X - points to the right
            Y - points at you
            Z - points at the floor
            ------------------------------------------------------------------------------
            Android Sensor Format:
            Imagine the phone is held vertically straight, the screen is pointing at you.
            X - points to the right
            Y - points at the ceiling, not at you like in PS Move
            Z - points at you, not at the floor like in PS Move (in Y the direction is reversed)
            ------------------------------------------------------------------------------
            not only that, PS Move expects:
            - accelerometer data in G's, while Android outputs data in m/s^2's
            - gyro data in rad/s (Android is the same)
            ------------------------------------------------------------------------------
            The REMove protocol expects all data in native PS Move format,
            to allow the client to implement the sensors in it's own way.
         */
        synchronized (dataLock) {
            if (lastGyroValues != null) {
                // Y<->Z + -Z, already in rad/s:
                moveGyroValues[0] =  lastGyroValues[0];
                moveGyroValues[1] =  lastGyroValues[2];
                moveGyroValues[2] = -lastGyroValues[1];
            }

            if (lastAccelValues != null) {
                if (prevAccelValues != null && filterFactor > 0.0f) {
                    // a very awful filter implementation:
                    float invFilterFactor = 1.0f - filterFactor;
                    // the factor must be in (0;1) range.
                    lastAccelValues[0] =
                        prevAccelValues[0] * invFilterFactor + lastAccelValues[0] * filterFactor;
                    lastAccelValues[1] =
                        prevAccelValues[1] * invFilterFactor + lastAccelValues[1] * filterFactor;
                    lastAccelValues[2] =
                        prevAccelValues[2] * invFilterFactor + lastAccelValues[2] * filterFactor;
                    //.
                }
                // Y<->Z + -Z + m/s^2 -> G's:
                moveAccelValues[0] =  lastAccelValues[0] / SensorManager.STANDARD_GRAVITY;
                moveAccelValues[1] =  lastAccelValues[2] / SensorManager.STANDARD_GRAVITY;
                moveAccelValues[2] = -lastAccelValues[1] / SensorManager.STANDARD_GRAVITY;
            }
        }

        return appData;
    }

    @Override
    public void print(String what) {
        Log.i("REMove", what);
    }

    @Override
    public void onSensorChanged(SensorEvent sensorEvent) {
        int st = sensorEvent.sensor.getType();
        if (st == Sensor.TYPE_GYROSCOPE) {
            synchronized (dataLock) {
                lastGyroValues = sensorEvent.values;
            }
        }
        else if (st == Sensor.TYPE_ACCELEROMETER) {
            synchronized (dataLock) {
                prevAccelValues = lastAccelValues;
                lastAccelValues = sensorEvent.values;
            }
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int i) {
        // doesn't matter, the accuracy is still present in SensorEvents...
        // (we still don't care)
    }
}
