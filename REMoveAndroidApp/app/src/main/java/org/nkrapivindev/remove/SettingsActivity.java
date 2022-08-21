package org.nkrapivindev.remove;

import android.os.Bundle;
import android.text.InputType;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.EditTextPreference;
import androidx.preference.PreferenceFragmentCompat;

public class SettingsActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.settings_activity);
        if (savedInstanceState == null) {
            getSupportFragmentManager()
                    .beginTransaction()
                    .replace(R.id.settings, new SettingsFragment())
                    .commit();
        }
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
    }

    public static class SettingsFragment extends PreferenceFragmentCompat {
        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.root_preferences, rootKey);
            // *try* to fix the input type, don't care if we failed.
            try {
                EditTextPreference settings_port =
                        findPreference("settings_port");
                if (settings_port != null) {
                    settings_port.setOnBindEditTextListener(
                        (editText) -> editText.setInputType(
                            InputType.TYPE_CLASS_NUMBER |
                            InputType.TYPE_NUMBER_VARIATION_NORMAL
                        )
                    );
                }

                EditTextPreference settings_filter =
                        findPreference("settings_filter");
                if (settings_filter != null) {
                    settings_filter.setOnBindEditTextListener(
                        (editText)-> editText.setInputType(
                            InputType.TYPE_CLASS_NUMBER |
                            InputType.TYPE_NUMBER_FLAG_DECIMAL
                        )
                    );
                }
            }
            catch (Exception ignore) {
                // ...
            }
        }
    }
}