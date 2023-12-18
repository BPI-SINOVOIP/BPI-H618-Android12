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

/** Automotive UI Resource Config Class */
public class AutoConfigResource {
    // Type of UI Resource - Resource ID, Text, Description
    private String resourceType;

    // Value for the UI Resource - id, text value or description for the resource
    private String resourceValue;

    // Application Package for the UI Resource if the type is Resource ID,
    private String resourcePackage;

    // Constructors
    public AutoConfigResource(String resourceType, String resourceValue) {
        this.resourceType = resourceType;
        this.resourceValue = resourceValue;
    }

    public AutoConfigResource(String resourceType, String resourceValue, String resourcePackage) {
        this.resourceType = resourceType;
        this.resourceValue = resourceValue;
        this.resourcePackage = resourcePackage;
    }

    /** Get Resource Type ( RESOURCE_ID, TEXT, DESCRIPTION ) */
    public String getResourceType() {
        return this.resourceType;
    }

    /** Get Resource Value ( resource id, text value, description ) */
    public String getResourceValue() {
        return this.resourceValue;
    }

    /** Get Resource Package */
    public String getResourcePackage() {
        return this.resourcePackage;
    }

    /** Set Resource Type ( RESOURCE_ID, TEXT, DESCRIPTION ) */
    public void setResourceType(String resourceType) {
        this.resourceType = resourceType;
    }

    /** Set Resource Value ( resource id, text value, description ) */
    public void setResourceValue(String resourceValue) {
        this.resourceValue = resourceValue;
    }

    /** Set Resource Package */
    public void setResourcePackage(String resourcePackage) {
        this.resourcePackage = resourcePackage;
    }
}
