/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.platform.helpers;

import androidx.test.InstrumentationRegistry;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class AutoJsonUtility {
    private static final String LOG_TAG = AutoJsonUtility.class.getSimpleName();

    // Config File Path Param
    private static final String CONFIG_FILE_PATH_PARAM = "config-file-path";

    // Application Config
    public static final String APPLICATION_CONFIG = "APPLICATION_CONFIG";

    private Map<String, String> mApplicationConfigMap;

    // Config Utility Map
    private Map<String, IAutoConfigUtility> configUtilityMap;

    // Resources
    private static final String PATH_KEY = "PATH";
    private static final String OPTIONS_KEY = "OPTIONS";
    private static final String PATH_DELIMETER = ">";
    private static final String TYPE_KEY = "TYPE";
    private static final String VALUE_KEY = "VALUE";
    private static final String PACKAGE_KEY = "PACKAGE";

    private static AutoJsonUtility sAutoJsonUtilityInstance = null;

    private AutoJsonUtility() {
        initializeConfigMaps();
        buildConfigMaps();
    }

    /** Get Auto Json Utility Instance */
    public static AutoJsonUtility getInstance() {
        if (sAutoJsonUtilityInstance == null) {
            sAutoJsonUtilityInstance = new AutoJsonUtility();
        }
        return sAutoJsonUtilityInstance;
    }

    /** Get Path for given Setting */
    public String[] getSettingPath(String setting) {
        return configUtilityMap.get(AutoConfigConstants.SETTINGS).getPath(setting);
    }

    /** Get menu options in given setting */
    public String[] getSettingOptions(String setting) {
        return configUtilityMap.get(AutoConfigConstants.SETTINGS).getAvailableOptions(setting);
    }

    /** Get config value for given config */
    public String getApplicationConfig(String config) {
        return mApplicationConfigMap.get(config);
    }

    /** Get the UI resource configuration */
    public AutoConfigResource getResourceFromConfig(
            String application, String config, String resource) {
        if (configUtilityMap.get(application) == null) {
            // Unknown Application
            return null;
        }
        return configUtilityMap.get(application).getResourceConfiguration(config, resource);
    }

    /** Initialize Config Maps */
    private void initializeConfigMaps() {
        // Initialize Application Config Map
        mApplicationConfigMap = new HashMap<String, String>();

        // Initialize Config Utility Map
        configUtilityMap = new HashMap<String, IAutoConfigUtility>();

        // load config uitilities for supported applications
        loadConfigUtilitiesForSupportedApplications();
    }

    /** Load config uitilities for supported applications */
    private void loadConfigUtilitiesForSupportedApplications() {
        // Phone/Dialer
        configUtilityMap.put(AutoConfigConstants.PHONE, AutoDialConfigUtility.getInstance());

        // Settings
        configUtilityMap.put(AutoConfigConstants.SETTINGS, AutoSettingsConfigUtility.getInstance());

        // Notifications
        configUtilityMap.put(
                AutoConfigConstants.NOTIFICATIONS, AutoNotificationsConfigUtility.getInstance());

        // Media Center
        configUtilityMap.put(
                AutoConfigConstants.MEDIA_CENTER, AutoMediaCenterConfigUtility.getInstance());

        // App Grid
        configUtilityMap.put(AutoConfigConstants.APP_GRID, AutoAppGridConfigUtility.getInstance());

        // Home
        configUtilityMap.put(AutoConfigConstants.HOME, AutoHomeConfigUtility.getInstance());

        // Lock Screen
        configUtilityMap.put(
                AutoConfigConstants.LOCK_SCREEN, AutoLockScreenConfigUtility.getInstance());
    }

    /** Build Config Maps */
    private void buildConfigMaps() {
        // Build Config Maps with default values ( based on Reference Devices )
        buildDefaultConfigMaps();

        // Read Config File Path Param
        String configFilePath =
                InstrumentationRegistry.getArguments().getString(CONFIG_FILE_PATH_PARAM, null);

        // If Config File is provided update the config maps with data from the file
        if (configFilePath != null) {
            updateConfigMapsFromFile(configFilePath);
        }
    }

    /** Build Default Config Maps ( Based on Reference Device ) */
    private void buildDefaultConfigMaps() {
        for (String application : configUtilityMap.keySet()) {
            configUtilityMap.get(application).loadDefaultConfig(mApplicationConfigMap);
        }
    }

    /**
     * Reads and parses the Config JSON file and update the Config Maps Sample JSON {
     * APPLICATION_NAME_1: { APPLICATION_CONFIG: { APPLICATION_PACKAGE:
     * "android.application.package" }, APPLICATION_RESOURCES: { PATH: "APPLICATION_PATH [ e.g.
     * Settings > System ]", OPTIONS: [ "APPLICATION_SETTINGS_OPTION_1"
     * "APPLICATION_SETTINGS_OPTION_2" ], APPLICATION_RESOURCE_1: { TYPE: "RESOURCE_ID", VALUE:
     * "RESOURCE_ID_VALUE", PACKAGE: "RESOURCE_PACKAGE" }, APPLICATION_RESOURCE_2: { TYPE: "TEXT",
     * VALUE: "TEXT_VALUE" } } }, APPLICATION_NAME_2: { ... } }
     *
     * @param configFilePath Path for the config file
     */
    private void updateConfigMapsFromFile(String configFilePath) {
        // Read data from config file
        String configFileData = readConfigFile(configFilePath);
        if (configFileData == null || configFileData.isEmpty()) {
            throw new RuntimeException("Config file is empty.");
        }
        try {
            // Parse json data from config file
            // and update config maps from the parsed JSON config data
            JSONObject configFileDataJson = new JSONObject(configFileData);

            Iterator<String> applicationNames = configFileDataJson.keys();
            while (applicationNames.hasNext()) {
                String applicationName = applicationNames.next();
                if (!isApplicationSupported(applicationName)) {
                    throwUnknownObjectException(applicationName);
                }
                Object applicationConfig = configFileDataJson.get(applicationName);
                if (applicationConfig == null || !isJsonObject(applicationConfig)) {
                    throwJsonObjectException(applicationName);
                }
                parseJsonAndUpdateConfigMaps(applicationName, (JSONObject) applicationConfig);
            }
        } catch (JSONException e) {
            throw new RuntimeException(e);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    /** Parse JSON data and Update Config Maps with data from JSON */
    private void parseJsonAndUpdateConfigMaps(String appName, JSONObject appConfiguration)
            throws JSONException {
        Iterator<String> appConfigAndResourceKeys = appConfiguration.keys();
        while (appConfigAndResourceKeys.hasNext()) {
            String appConfigAndResourceKey = appConfigAndResourceKeys.next();
            Object appConfigAndResource = appConfiguration.get(appConfigAndResourceKey);
            if (appConfigAndResource == null || !isJsonObject(appConfigAndResource)) {
                throwJsonObjectException(appName, appConfigAndResourceKey);
            }
            if (APPLICATION_CONFIG.equals(appConfigAndResourceKey)) {
                parseJsonAndUpdateApplicationConfigMap(
                        appName, appConfigAndResourceKey, (JSONObject) appConfigAndResource);
                continue;
            }
            if (!isValidConfiguration(appName, appConfigAndResourceKey)) {
                throwUnknownObjectException(appName, appConfigAndResourceKey);
            }
            parseJsonAndUpdateApplicationResources(
                    appName, appConfigAndResourceKey, (JSONObject) appConfigAndResource);
        }
    }

    /** Parse JSON and Update Application Config Map with data from JSON */
    private void parseJsonAndUpdateApplicationConfigMap(
            String appName, String configName, JSONObject config) throws JSONException {
        Iterator<String> configKeys = config.keys();
        while (configKeys.hasNext()) {
            String configKey = configKeys.next();
            if (!isValidApplicationConfig(configKey)) {
                throwUnknownObjectException(appName, configName, configKey);
            }
            Object applicationConfig = config.get(configKey);
            if (applicationConfig == null || !isJsonString(applicationConfig)) {
                throwJsonStringException(appName, configName, configKey);
            }
            String appConfig = (String) applicationConfig;
            mApplicationConfigMap.put(configKey, appConfig);
        }
    }

    /** Parse JSON and Update Resources for given application with the data from JSON */
    private void parseJsonAndUpdateApplicationResources(
            String appName, String configName, JSONObject config) throws JSONException {
        Iterator<String> resourceNames = config.keys();
        while (resourceNames.hasNext()) {
            String resourceName = resourceNames.next();
            if (PATH_KEY.equals(resourceName)) {
                parseJsonAndUpdateConfigPath(appName, configName, config);
                continue;
            }
            if (OPTIONS_KEY.equals(resourceName)) {
                parseJsonAndUpdateConfigOptions(appName, configName, config);
                continue;
            }
            if (!isValidResourceConfig(appName, configName, resourceName)) {
                throwUnknownObjectException(appName, configName, resourceName);
            }
            Object applicationResource = config.get(resourceName);
            if (applicationResource == null || !isJsonObject(applicationResource)) {
                throwJsonObjectException(appName, configName, resourceName);
            }
            parseJsonAndUpdateResourcesInConfigMaps(
                    appName, configName, resourceName, (JSONObject) applicationResource);
        }
    }

    /** Validate if given resource exists in the configuration for the given application */
    private boolean isValidResourceConfig(String appName, String configName, String resourceName) {
        return configUtilityMap.get(appName).isValidResource(configName, resourceName);
    }

    /** Validate if given config exists for the application */
    private boolean isValidConfiguration(String appName, String configName) {
        return configUtilityMap.get(appName).isValidConfiguration(configName);
    }

    /** Parse Json and Update resources in Config Maps for given Application */
    private void parseJsonAndUpdateResourcesInConfigMaps(
            String appName, String configName, String resourceName, JSONObject config)
            throws JSONException {
        Object resourceType = config.get(TYPE_KEY);
        if (resourceType == null || !isJsonString(resourceType)) {
            throwJsonStringException(appName, configName, resourceName, TYPE_KEY);
        }
        Object resourceValue = config.get(VALUE_KEY);
        if (resourceValue == null || !isJsonString(resourceValue)) {
            throwJsonStringException(appName, configName, resourceName, VALUE_KEY);
        }
        configUtilityMap
                .get(appName)
                .getResourceConfiguration(configName, resourceName)
                .setResourceType((String) resourceType);
        configUtilityMap
                .get(appName)
                .getResourceConfiguration(configName, resourceName)
                .setResourceValue((String) resourceValue);
        if (AutoConfigConstants.RESOURCE_ID.equals((String) resourceType)) {
            if (!config.has(PACKAGE_KEY) || config.isNull(PACKAGE_KEY)) {
                throw new RuntimeException(
                        String.format(
                                "Config file is invalid. %s is required for "
                                        + "Resource %s for %s in %s application.",
                                PACKAGE_KEY, resourceName, configName, appName));
            }
            Object resourcePackage = config.get(PACKAGE_KEY);
            if (resourcePackage == null || !isJsonString(resourcePackage)) {
                throwJsonStringException(appName, configName, resourceName, PACKAGE_KEY);
            }
            configUtilityMap
                    .get(appName)
                    .getResourceConfiguration(configName, resourceName)
                    .setResourcePackage((String) resourcePackage);
        }
    }

    /** Parse JSON and update config options for the given application */
    private void parseJsonAndUpdateConfigOptions(
            String appName, String configName, JSONObject config) throws JSONException {
        if (!isValidOptionsConfig(appName, configName)) {
            throwUnknownObjectException(appName, configName, OPTIONS_KEY);
        }
        Object configOptions = config.get(OPTIONS_KEY);
        if (configOptions == null || !isJsonArray(configOptions)) {
            throwJsonArrayException(appName, configName, OPTIONS_KEY);
        }
        JSONArray options = (JSONArray) configOptions;
        String[] optionsArray = new String[options.length()];
        for (int i = 0; i < options.length(); i++) {
            Object option = options.get(i);
            if (option == null || !isJsonString(option)) {
                throwJsonStringException(appName, configName, OPTIONS_KEY, "index-" + i);
            }
            String configOption = (String) option;
            if (configOption == null || configOption.trim().isEmpty()) {
                throw new RuntimeException(
                        String.format(
                                "Config file is invalid. %s in %s for %s contains null or empty"
                                        + " string.",
                                OPTIONS_KEY, configName, appName));
            }
            optionsArray[i] = configOption.trim();
        }
        addOptionsToMap(appName, configName, optionsArray);
    }

    /** Add given options to application options map */
    private void addOptionsToMap(String appName, String configName, String[] options) {
        configUtilityMap.get(appName).addAvailableOptions(configName, options);
    }

    /** Validate if given config exist in application options map */
    private boolean isValidOptionsConfig(String appName, String configName) {
        return configUtilityMap.get(appName).isValidOption(configName);
    }

    /** Parse JSON and update path for config in the given application */
    private void parseJsonAndUpdateConfigPath(String appName, String configName, JSONObject config)
            throws JSONException {
        if (!isValidPathConfig(appName, configName)) {
            throwUnknownObjectException(appName, configName, PATH_KEY);
        }
        Object configPath = config.get(PATH_KEY);
        if (configPath == null || !isJsonString(configPath)) {
            throwJsonStringException(appName, configName, PATH_KEY);
        }
        String path = (String) configPath;
        String[] paths = path.split(PATH_DELIMETER);
        if (paths.length <= 1) {
            throw new RuntimeException(
                    String.format(
                            "Config file is invalid. %s in %s for %s should be a valid path.",
                            PATH_KEY, configName, appName));
        }
        String[] pathsArray = new String[paths.length - 1];
        for (int i = 1; i < paths.length; i++) {
            pathsArray[i - 1] = paths[i].trim();
        }
        addPathToMap(appName, configName, pathsArray);
    }

    /** Add the given path for Application Paths Map */
    private void addPathToMap(String appName, String configName, String[] paths) {
        configUtilityMap.get(appName).addPath(configName, paths);
    }

    /** Read data from config file */
    private String readConfigFile(String configFilePath) {
        String configFileData = null;
        try {
            File configFile = new File(configFilePath);
            if (configFile == null || !configFile.canRead()) {
                throw new IOException("Cannot read the Resource file - " + configFilePath);
            }
            FileReader configFileReader = new FileReader(configFile);
            BufferedReader configFileBufferedReader = new BufferedReader(configFileReader);
            StringBuilder stringBuilderObj = new StringBuilder();
            String configFileDataLine = configFileBufferedReader.readLine();
            while (configFileDataLine != null) {
                stringBuilderObj.append(configFileDataLine).append("\n");
                configFileDataLine = configFileBufferedReader.readLine();
            }
            configFileBufferedReader.close();
            configFileData = stringBuilderObj.toString();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
        return configFileData;
    }

    /** Validate if given config exist in application paths map */
    private boolean isValidPathConfig(String appName, String configName) {
        return configUtilityMap.get(appName).isValidPath(configName);
    }

    /** Validate for Supported Applications */
    private boolean isApplicationSupported(String applicationName) {
        if (configUtilityMap.get(applicationName) != null) {
            return true;
        }
        return false;
    }

    /** Validate given application config */
    private boolean isValidApplicationConfig(String appConfig) {
        return (mApplicationConfigMap.get(appConfig) != null);
    }

    /** Validate for Json String */
    private boolean isJsonString(Object object) {
        return !(object instanceof JSONObject || object instanceof JSONArray);
    }

    /** Validate for Json Object */
    private boolean isJsonObject(Object object) {
        return (object instanceof JSONObject);
    }

    /** Validate for Json Array */
    private boolean isJsonArray(Object object) {
        return (object instanceof JSONArray);
    }

    /** Throw Exception for Invalid Json Array */
    private void throwJsonArrayException(String... configs) {
        String message =
                String.format(
                        "Config file is invalid. Please validate %s config. It should be a JSON"
                                + " Array.",
                        Arrays.toString(configs));
        throw new RuntimeException(message);
    }

    /** Throw Exception for Invalid Json String Object */
    private void throwJsonStringException(String... configs) {
        String message =
                String.format(
                        "Config file is invalid. Please validate %s config. It should be a String.",
                        Arrays.toString(configs));
        throw new RuntimeException(message);
    }

    /** Throw Exception for Invalid Json Object */
    private void throwJsonObjectException(String... configs) {
        String message =
                String.format(
                        "Config file is invalid. Please validate %s config. It should be a JSON"
                                + " Object.",
                        Arrays.toString(configs));
        throw new RuntimeException(message);
    }

    /** Throw Exception for Unknown Object */
    private void throwUnknownObjectException(String... configs) {
        String message =
                String.format(
                        "Config file is invalid. Unknown config %s.", Arrays.toString(configs));
        throw new RuntimeException(message);
    }
}
