/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.textclassifier;

import static com.google.common.truth.Truth.assertThat;

import android.app.UiAutomation;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.icu.util.ULocale;
import android.provider.DeviceConfig;
import android.view.textclassifier.ConversationAction;
import android.view.textclassifier.ConversationActions;
import android.view.textclassifier.TextClassification;
import android.view.textclassifier.TextClassificationManager;
import android.view.textclassifier.TextClassifier;
import android.view.textclassifier.TextLanguage;
import android.view.textclassifier.TextLinks;
import android.view.textclassifier.TextLinks.TextLink;
import android.view.textclassifier.TextSelection;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;
import androidx.test.platform.app.InstrumentationRegistry;
import com.google.common.collect.ImmutableList;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExternalResource;
import org.junit.runner.RunWith;

/**
 * End-to-end tests for the {@link TextClassifier} APIs. Unlike {@link TextClassifierImplTest}.
 *
 * <p>Unlike {@link TextClassifierImplTest}, we are trying to run the tests in a environment that is
 * closer to the production environment. For example, we are not injecting the model files.
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class TextClassifierApiTest {

  private TextClassifier textClassifier;

  @Rule
  public final ExtServicesTextClassifierRule extServicesTextClassifierRule =
      new ExtServicesTextClassifierRule();

  @Before
  public void setup() {
    textClassifier = extServicesTextClassifierRule.getTextClassifier();
  }

  @Test
  public void suggestSelection() {
    String text = "Visit http://www.android.com for more information";
    String selected = "http";
    String suggested = "http://www.android.com";
    int startIndex = text.indexOf(selected);
    int endIndex = startIndex + selected.length();
    int smartStartIndex = text.indexOf(suggested);
    int smartEndIndex = smartStartIndex + suggested.length();

    TextSelection.Request request =
        new TextSelection.Request.Builder(text, startIndex, endIndex).build();

    TextSelection selection = textClassifier.suggestSelection(request);
    assertThat(selection.getEntityCount()).isGreaterThan(0);
    assertThat(selection.getEntity(0)).isEqualTo(TextClassifier.TYPE_URL);
    assertThat(selection.getSelectionStartIndex()).isEqualTo(smartStartIndex);
    assertThat(selection.getSelectionEndIndex()).isEqualTo(smartEndIndex);
  }

  @Test
  public void classifyText() {
    String text = "Contact me at droid@android.com";
    String classifiedText = "droid@android.com";
    int startIndex = text.indexOf(classifiedText);
    int endIndex = startIndex + classifiedText.length();
    TextClassification.Request request =
        new TextClassification.Request.Builder(text, startIndex, endIndex).build();

    TextClassification classification = textClassifier.classifyText(request);
    assertThat(classification.getEntityCount()).isGreaterThan(0);
    assertThat(classification.getEntity(0)).isEqualTo(TextClassifier.TYPE_EMAIL);
    assertThat(classification.getText()).isEqualTo(classifiedText);
    assertThat(classification.getActions()).isNotEmpty();
  }

  @Test
  public void generateLinks() {
    String text = "Check this out, http://www.android.com";

    TextLinks.Request request = new TextLinks.Request.Builder(text).build();

    TextLinks textLinks = textClassifier.generateLinks(request);

    List<TextLink> links = new ArrayList<>(textLinks.getLinks());
    assertThat(textLinks.getText().toString()).isEqualTo(text);
    assertThat(links).hasSize(1);
    assertThat(links.get(0).getEntityCount()).isGreaterThan(0);
    assertThat(links.get(0).getEntity(0)).isEqualTo(TextClassifier.TYPE_URL);
    assertThat(links.get(0).getConfidenceScore(TextClassifier.TYPE_URL)).isGreaterThan(0f);
  }

  @Test
  public void detectedLanguage() {
    String text = "朝、ピカチュウ";
    TextLanguage.Request request = new TextLanguage.Request.Builder(text).build();

    TextLanguage textLanguage = textClassifier.detectLanguage(request);

    assertThat(textLanguage.getLocaleHypothesisCount()).isGreaterThan(0);
    assertThat(textLanguage.getLocale(0).getLanguage()).isEqualTo("ja");
    assertThat(textLanguage.getConfidenceScore(ULocale.JAPANESE)).isGreaterThan(0f);
  }

  @Test
  public void suggestConversationActions() {
    ConversationActions.Message message =
        new ConversationActions.Message.Builder(ConversationActions.Message.PERSON_USER_OTHERS)
            .setText("Check this out: https://www.android.com")
            .build();
    ConversationActions.Request request =
        new ConversationActions.Request.Builder(ImmutableList.of(message)).build();

    ConversationActions conversationActions = textClassifier.suggestConversationActions(request);

    assertThat(conversationActions.getConversationActions()).hasSize(1);
    ConversationAction conversationAction = conversationActions.getConversationActions().get(0);
    assertThat(conversationAction.getType()).isEqualTo(ConversationAction.TYPE_OPEN_URL);
    assertThat(conversationAction.getAction()).isNotNull();
  }

  /** A rule that manages a text classifier that is backed by the ExtServices. */
  private static class ExtServicesTextClassifierRule extends ExternalResource {
    private static final String CONFIG_TEXT_CLASSIFIER_SERVICE_PACKAGE_OVERRIDE =
        "textclassifier_service_package_override";
    private static final String PKG_NAME_GOOGLE_EXTSERVICES = "com.google.android.ext.services";
    private static final String PKG_NAME_AOSP_EXTSERVICES = "android.ext.services";

    private String textClassifierServiceOverrideFlagOldValue;

    @Override
    protected void before() {
      UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
      try {
        uiAutomation.adoptShellPermissionIdentity();
        textClassifierServiceOverrideFlagOldValue =
            DeviceConfig.getString(
                DeviceConfig.NAMESPACE_TEXTCLASSIFIER,
                CONFIG_TEXT_CLASSIFIER_SERVICE_PACKAGE_OVERRIDE,
                null);
        DeviceConfig.setProperty(
            DeviceConfig.NAMESPACE_TEXTCLASSIFIER,
            CONFIG_TEXT_CLASSIFIER_SERVICE_PACKAGE_OVERRIDE,
            getExtServicesPackageName(),
            /* makeDefault= */ false);
      } finally {
        uiAutomation.dropShellPermissionIdentity();
      }
    }

    @Override
    protected void after() {
      UiAutomation uiAutomation = InstrumentationRegistry.getInstrumentation().getUiAutomation();
      try {
        uiAutomation.adoptShellPermissionIdentity();
        DeviceConfig.setProperty(
            DeviceConfig.NAMESPACE_TEXTCLASSIFIER,
            CONFIG_TEXT_CLASSIFIER_SERVICE_PACKAGE_OVERRIDE,
            textClassifierServiceOverrideFlagOldValue,
            /* makeDefault= */ false);
      } finally {
        uiAutomation.dropShellPermissionIdentity();
      }
    }

    private static String getExtServicesPackageName() {
      PackageManager packageManager =
          ApplicationProvider.getApplicationContext().getPackageManager();
      try {
        packageManager.getApplicationInfo(PKG_NAME_GOOGLE_EXTSERVICES, /* flags= */ 0);
        return PKG_NAME_GOOGLE_EXTSERVICES;
      } catch (NameNotFoundException e) {
        return PKG_NAME_AOSP_EXTSERVICES;
      }
    }

    public TextClassifier getTextClassifier() {
      TextClassificationManager textClassificationManager =
          ApplicationProvider.getApplicationContext()
              .getSystemService(TextClassificationManager.class);
      return textClassificationManager.getTextClassifier();
    }
  }
}
