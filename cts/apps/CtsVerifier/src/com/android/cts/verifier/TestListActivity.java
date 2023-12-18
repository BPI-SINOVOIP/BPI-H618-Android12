/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.cts.verifier;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.PermissionInfo;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.Toast;

/** Top-level {@link ListActivity} for launching tests and managing results. */
public class TestListActivity extends AbstractTestListActivity implements View.OnClickListener {
    private static final int CTS_VERIFIER_PERMISSION_REQUEST = 1;

    private static final String TAG = TestListActivity.class.getSimpleName();
    // Records the current display mode.
    // Default is unfolded mode, and it will be changed when clicking the switch button.
    public static String sCurrentDisplayMode = DisplayMode.UNFOLDED.toString();
    // Flag of launch app to fetch the unfolded/folded tests in main view from AndroidManifest.xml.
    protected static boolean sInitialLaunch;

    // Enumerates the display modes, including unfolded and folded.
    protected enum DisplayMode {
        UNFOLDED, FOLDED;

        @Override
        public String toString() {
            return name().toLowerCase();
        }

        /**
         * Coverts the mode as suffix with brackets for test name.
         *
         * @return A string containing mode with brackets for folded mode;
         *         empty string for unfolded mode.
         */
        public String asSuffix() {
            if (name().equals(FOLDED.name())) {
                return String.format("[%s]", toString());
            }
            return "";
        }
    }

    @Override
    public void onClick (View v) {
        handleMenuItemSelected(v.getId());
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        try {
            PackageManager pm = getPackageManager();
            PackageInfo packageInfo = pm.getPackageInfo(
                    getApplicationInfo().packageName, PackageManager.GET_PERMISSIONS);

            if (packageInfo.requestedPermissions != null) {
                for (String permission : packageInfo.requestedPermissions) {
                    Log.v(TAG, "Checking permissions for: " + permission);
                    try {
                        PermissionInfo info = pm.getPermissionInfo(permission, 0);
                        if ((info.protectionLevel & PermissionInfo.PROTECTION_DANGEROUS) == 0) {
                            continue;
                        }
                    } catch (NameNotFoundException e) {
                        Log.v(TAG, "Checking permissions for: " + permission + "not found");
                        continue;
                    }
                    if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                        requestPermissions(packageInfo.requestedPermissions,
                                CTS_VERIFIER_PERMISSION_REQUEST);
                        /* don't return here. Some tests (i.e. USB Restrict Access test)
                         * which need to run even if permissions are incomplete.
                         */
                    }
                }
            }
            createContinue();
        } catch (NameNotFoundException e) {
            Log.e(TAG, "Unable to load package's permissions", e);
            Toast.makeText(this, R.string.runtime_permissions_error, Toast.LENGTH_SHORT).show();
        }
    }

    private void createContinue() {
        if (!isTaskRoot()) {
            finish();
        }
        sInitialLaunch = true;

        setTitle(getString(R.string.title_version, Version.getVersionName(this)));

        if (!getWindow().hasFeature(Window.FEATURE_ACTION_BAR)) {
            View footer = getLayoutInflater().inflate(R.layout.test_list_footer, null);

            footer.findViewById(R.id.clear).setOnClickListener(this);
            footer.findViewById(R.id.export).setOnClickListener(this);

            getListView().addFooterView(footer);
        }

        setTestListAdapter(new ManifestTestListAdapter(this, null));
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, String permissions[], int[] grantResults) {
        if (requestCode == CTS_VERIFIER_PERMISSION_REQUEST) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                createContinue();
                return;
            }
            Log.v(TAG, "Permission not granted.");
            Toast.makeText(this, R.string.runtime_permissions_error, Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.test_list_menu, menu);

        // Switch button for unfolded and folded tests.
        MenuItem item = (MenuItem) menu.findItem(R.id.switch_item);
        item.setActionView(R.layout.display_mode_switch);
        Switch displayModeSwitch = item.getActionView().findViewById(R.id.switch_button);

        // Restores the original display mode when launching the app after killing the process.
        // Otherwise, gets the current display mode to show switch status.
        boolean isFoldedMode;
        if (sInitialLaunch) {
            isFoldedMode = getCurrentDisplayMode().equals(DisplayMode.FOLDED.toString());
        } else {
            isFoldedMode = sCurrentDisplayMode.equals(DisplayMode.FOLDED.toString());
        }
        displayModeSwitch.setChecked(isFoldedMode);

        displayModeSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView,
                boolean isChecked) {
                if (isChecked) {
                    sCurrentDisplayMode = DisplayMode.FOLDED.toString();
                } else {
                    sCurrentDisplayMode = DisplayMode.UNFOLDED.toString();
                }
                handleSwitchItemSelected();
            }
        });
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        return handleMenuItemSelected(item.getItemId()) ? true : super.onOptionsItemSelected(item);
    }

    private void handleClearItemSelected() {
        new AlertDialog.Builder(this)
            .setMessage(R.string.test_results_clear_title)
            .setPositiveButton(R.string.test_results_clear_yes,
                    new DialogInterface.OnClickListener() {
                       public void onClick(DialogInterface dialog, int id) {
                            mAdapter.clearTestResults();
                            Toast.makeText(
                                TestListActivity.this,
                                R.string.test_results_cleared,
                                Toast.LENGTH_SHORT)
                                    .show();
                       }
                   })
            .setNegativeButton(R.string.test_results_clear_cancel, null)
            .show();
    }

    private void handleExportItemSelected() {
        new ReportExporter(this, mAdapter).execute();
    }

    // Sets up the flags after switching display mode and reloads tests on UI.
    private void handleSwitchItemSelected() {
        setCurrentDisplayMode(sCurrentDisplayMode);
        mAdapter.loadTestResults();
    }

    private boolean handleMenuItemSelected(int id) {
        if (id == R.id.clear) {
            handleClearItemSelected();
        } else if (id == R.id.export) {
            handleExportItemSelected();
        } else {
            return false;
        }

        return true;
    }

    /**
     * Sets current display mode to sharedpreferences.
     *
     * @param mode A string of current display mode.
     */
    private void setCurrentDisplayMode(String mode) {
        SharedPreferences pref = getSharedPreferences(DisplayMode.class.getName(), MODE_PRIVATE);
        pref.edit().putString(DisplayMode.class.getName(), mode).commit();
    }

    /**
     * Gets current display mode from sharedpreferences.
     *
     * @return A string of current display mode.
     */
    private String getCurrentDisplayMode() {
        String mode = getSharedPreferences(DisplayMode.class.getName(), MODE_PRIVATE)
            .getString(DisplayMode.class.getName(), "");
        return mode;
    }
}