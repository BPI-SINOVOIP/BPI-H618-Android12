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

package com.android.textclassifier.common;

import static com.android.textclassifier.common.ModelFileManager.ModelFile.LANGUAGE_INDEPENDENT;
import static com.google.common.truth.Truth.assertThat;

import android.os.LocaleList;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;
import com.android.textclassifier.TestDataUtils;
import com.android.textclassifier.common.ModelFileManager.ModelFile;
import com.android.textclassifier.common.ModelFileManager.RegularFileFullMatchLister;
import com.android.textclassifier.common.ModelFileManager.RegularFilePatternMatchLister;
import com.android.textclassifier.common.ModelType.ModelTypeDef;
import com.android.textclassifier.common.logging.ResultIdUtils.ModelInfo;
import com.android.textclassifier.testing.SetDefaultLocalesRule;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;
import com.google.common.io.Files;
import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.Locale;
import java.util.stream.Collectors;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@SmallTest
@RunWith(AndroidJUnit4.class)
public final class ModelFileManagerTest {
  private static final Locale DEFAULT_LOCALE = Locale.forLanguageTag("en-US");

  @ModelTypeDef private static final String MODEL_TYPE = ModelType.ANNOTATOR;

  @Mock private TextClassifierSettings.IDeviceConfig mockDeviceConfig;

  @Rule public final SetDefaultLocalesRule setDefaultLocalesRule = new SetDefaultLocalesRule();

  private File rootTestDir;
  private ModelFileManager modelFileManager;

  @Before
  public void setup() {
    MockitoAnnotations.initMocks(this);

    rootTestDir =
        new File(ApplicationProvider.getApplicationContext().getCacheDir(), "rootTestDir");
    rootTestDir.mkdirs();
    modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            new TextClassifierSettings(mockDeviceConfig));
  }

  @After
  public void removeTestDir() {
    recursiveDelete(rootTestDir);
  }

  @Test
  public void annotatorModelPreloaded() {
    verifyModelPreloadedAsAsset(ModelType.ANNOTATOR, "textclassifier/annotator.universal.model");
  }

  @Test
  public void actionsModelPreloaded() {
    verifyModelPreloadedAsAsset(
        ModelType.ACTIONS_SUGGESTIONS, "textclassifier/actions_suggestions.universal.model");
  }

  @Test
  public void langIdModelPreloaded() {
    verifyModelPreloadedAsAsset(ModelType.LANG_ID, "textclassifier/lang_id.model");
  }

  private void verifyModelPreloadedAsAsset(
      @ModelTypeDef String modelType, String expectedModelPath) {
    List<ModelFileManager.ModelFile> modelFiles = modelFileManager.listModelFiles(modelType);
    List<ModelFile> assetFiles =
        modelFiles.stream().filter(modelFile -> modelFile.isAsset).collect(Collectors.toList());

    assertThat(assetFiles).hasSize(1);
    assertThat(assetFiles.get(0).absolutePath).isEqualTo(expectedModelPath);
  }

  @Test
  public void findBestModel_versionCode() {
    ModelFileManager.ModelFile olderModelFile =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            new File(rootTestDir, "a").getAbsolutePath(),
            /* version= */ 1,
            LANGUAGE_INDEPENDENT,
            /* isAsset= */ false);
    ModelFileManager.ModelFile newerModelFile =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            new File(rootTestDir, "b").getAbsolutePath(),
            /* version= */ 2,
            LANGUAGE_INDEPENDENT,
            /* isAsset= */ false);
    ModelFileManager modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            ImmutableList.of(modelType -> ImmutableList.of(olderModelFile, newerModelFile)));

    ModelFile bestModelFile = modelFileManager.findBestModelFile(MODEL_TYPE, null);
    assertThat(bestModelFile).isEqualTo(newerModelFile);
  }

  @Test
  public void findBestModel_languageDependentModelIsPreferred() {
    ModelFileManager.ModelFile languageIndependentModelFile =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            new File(rootTestDir, "a").getAbsolutePath(),
            /* version= */ 1,
            LANGUAGE_INDEPENDENT,
            /* isAsset= */ false);
    ModelFileManager.ModelFile languageDependentModelFile =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            new File(rootTestDir, "b").getAbsolutePath(),
            /* version= */ 2,
            DEFAULT_LOCALE.toLanguageTag(),
            /* isAsset= */ false);
    ModelFileManager modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            ImmutableList.of(
                modelType ->
                    ImmutableList.of(languageIndependentModelFile, languageDependentModelFile)));

    ModelFile bestModelFile =
        modelFileManager.findBestModelFile(MODEL_TYPE, new LocaleList(DEFAULT_LOCALE));
    assertThat(bestModelFile).isEqualTo(languageDependentModelFile);
  }

  @Test
  public void findBestModel_noMatchedLanguageModel() {
    ModelFileManager.ModelFile languageIndependentModelFile =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            new File(rootTestDir, "a").getAbsolutePath(),
            /* version= */ 1,
            LANGUAGE_INDEPENDENT,
            /* isAsset= */ false);
    ModelFileManager.ModelFile languageDependentModelFile =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            new File(rootTestDir, "b").getAbsolutePath(),
            /* version= */ 2,
            DEFAULT_LOCALE.toLanguageTag(),
            /* isAsset= */ false);
    ModelFileManager modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            ImmutableList.of(
                modelType ->
                    ImmutableList.of(languageIndependentModelFile, languageDependentModelFile)));

    ModelFileManager.ModelFile bestModelFile =
        modelFileManager.findBestModelFile(MODEL_TYPE, LocaleList.forLanguageTags("zh-hk"));
    assertThat(bestModelFile).isEqualTo(languageIndependentModelFile);
  }

  @Test
  public void findBestModel_languageIsMoreImportantThanVersion() {
    ModelFileManager.ModelFile matchButOlderModel =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            new File(rootTestDir, "a").getAbsolutePath(),
            /* version= */ 1,
            "fr",
            /* isAsset= */ false);
    ModelFileManager.ModelFile mismatchButNewerModel =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            new File(rootTestDir, "b").getAbsolutePath(),
            /* version= */ 1,
            "ja",
            /* isAsset= */ false);
    ModelFileManager modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            ImmutableList.of(
                modelType -> ImmutableList.of(matchButOlderModel, mismatchButNewerModel)));

    ModelFileManager.ModelFile bestModelFile =
        modelFileManager.findBestModelFile(MODEL_TYPE, LocaleList.forLanguageTags("fr"));
    assertThat(bestModelFile).isEqualTo(matchButOlderModel);
  }

  @Test
  public void findBestModel_preferMatchedLocaleModel() {
    ModelFileManager.ModelFile matchLocaleModel =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            new File(rootTestDir, "a").getAbsolutePath(),
            /* version= */ 1,
            "ja",
            /* isAsset= */ false);
    ModelFileManager.ModelFile languageIndependentModel =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            new File(rootTestDir, "b").getAbsolutePath(),
            /* version= */ 1,
            LANGUAGE_INDEPENDENT,
            /* isAsset= */ false);
    ModelFileManager modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            ImmutableList.of(
                modelType -> ImmutableList.of(matchLocaleModel, languageIndependentModel)));

    ModelFileManager.ModelFile bestModelFile =
        modelFileManager.findBestModelFile(MODEL_TYPE, LocaleList.forLanguageTags("ja"));

    assertThat(bestModelFile).isEqualTo(matchLocaleModel);
  }

  @Test
  public void deleteUnusedModelFiles_olderModelDeleted() throws Exception {
    File model1 = new File(rootTestDir, "model1.fb");
    model1.createNewFile();
    File model2 = new File(rootTestDir, "model2.fb");
    model2.createNewFile();
    ModelFileManager.ModelFile modelFile1 =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, model1.getAbsolutePath(), /* version= */ 1, "ja", /* isAsset= */ false);
    ModelFileManager.ModelFile modelFile2 =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, model2.getAbsolutePath(), /* version= */ 2, "ja", /* isAsset= */ false);
    setDefaultLocalesRule.set(new LocaleList(Locale.forLanguageTag("ja")));
    ModelFileManager modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            ImmutableList.of(modelType -> ImmutableList.of(modelFile1, modelFile2)));

    modelFileManager.deleteUnusedModelFiles();

    assertThat(model1.exists()).isFalse();
    assertThat(model2.exists()).isTrue();
  }

  @Test
  public void deleteUnusedModelFiles_languageIndependentOlderModelDeleted() throws Exception {
    File model1 = new File(rootTestDir, "model1.fb");
    model1.createNewFile();
    File model2 = new File(rootTestDir, "model2.fb");
    model2.createNewFile();
    ModelFileManager.ModelFile modelFile1 =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            model1.getAbsolutePath(),
            /* version= */ 1,
            LANGUAGE_INDEPENDENT,
            /* isAsset= */ false);
    ModelFileManager.ModelFile modelFile2 =
        new ModelFileManager.ModelFile(
            MODEL_TYPE,
            model2.getAbsolutePath(),
            /* version= */ 2,
            LANGUAGE_INDEPENDENT,
            /* isAsset= */ false);
    setDefaultLocalesRule.set(new LocaleList(Locale.forLanguageTag("ja")));
    ModelFileManager modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            ImmutableList.of(modelType -> ImmutableList.of(modelFile1, modelFile2)));

    modelFileManager.deleteUnusedModelFiles();

    assertThat(model1.exists()).isFalse();
    assertThat(model2.exists()).isTrue();
  }

  @Test
  public void deleteUnusedModelFiles_modelOnlySupportingLocalesNotInListDeleted() throws Exception {
    File model1 = new File(rootTestDir, "model1.fb");
    model1.createNewFile();
    File model2 = new File(rootTestDir, "model2.fb");
    model2.createNewFile();
    ModelFileManager.ModelFile modelFile1 =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, model1.getAbsolutePath(), /* version= */ 1, "ja", /* isAsset= */ false);
    ModelFileManager.ModelFile modelFile2 =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, model2.getAbsolutePath(), /* version= */ 1, "en", /* isAsset= */ false);
    setDefaultLocalesRule.set(new LocaleList(Locale.forLanguageTag("ja")));
    ModelFileManager modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            ImmutableList.of(modelType -> ImmutableList.of(modelFile1, modelFile2)));

    modelFileManager.deleteUnusedModelFiles();

    assertThat(model1.exists()).isTrue();
    assertThat(model2.exists()).isFalse();
  }

  @Test
  public void deleteUnusedModelFiles_multiLocalesInLocaleList() throws Exception {
    File model1 = new File(rootTestDir, "model1.fb");
    model1.createNewFile();
    File model2 = new File(rootTestDir, "model2.fb");
    model2.createNewFile();
    ModelFileManager.ModelFile modelFile1 =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, model1.getAbsolutePath(), /* version= */ 1, "ja", /* isAsset= */ false);
    ModelFileManager.ModelFile modelFile2 =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, model2.getAbsolutePath(), /* version= */ 2, "en", /* isAsset= */ false);
    setDefaultLocalesRule.set(
        new LocaleList(Locale.forLanguageTag("ja"), Locale.forLanguageTag("en")));
    ModelFileManager modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            ImmutableList.of(modelType -> ImmutableList.of(modelFile1, modelFile2)));
    modelFileManager.deleteUnusedModelFiles();

    assertThat(model1.exists()).isTrue();
    assertThat(model2.exists()).isTrue();
  }

  @Test
  public void deleteUnusedModelFiles_readOnlyModelsUntouched() throws Exception {
    File readOnlyDir = new File(rootTestDir, "read_only/");
    readOnlyDir.mkdirs();
    File model1 = new File(readOnlyDir, "model1.fb");
    model1.createNewFile();
    readOnlyDir.setWritable(false);
    ModelFileManager.ModelFile modelFile =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, model1.getAbsolutePath(), /* version= */ 1, "ja", /* isAsset= */ false);
    ModelFileManager modelFileManager =
        new ModelFileManager(
            ApplicationProvider.getApplicationContext(),
            ImmutableList.of(modelType -> ImmutableList.of(modelFile)));
    setDefaultLocalesRule.set(new LocaleList(Locale.forLanguageTag("ja")));

    modelFileManager.deleteUnusedModelFiles();

    assertThat(model1.exists()).isTrue();
  }

  @Test
  public void modelFileEquals() {
    ModelFileManager.ModelFile modelA =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, "/path/a", /* version= */ 1, "ja", /* isAsset= */ false);

    ModelFileManager.ModelFile modelB =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, "/path/a", /* version= */ 1, "ja", /* isAsset= */ false);

    assertThat(modelA).isEqualTo(modelB);
  }

  @Test
  public void modelFile_different() {
    ModelFileManager.ModelFile modelA =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, "/path/a", /* version= */ 1, "ja", /* isAsset= */ false);
    ModelFileManager.ModelFile modelB =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, "/path/b", /* version= */ 1, "ja", /* isAsset= */ false);

    assertThat(modelA).isNotEqualTo(modelB);
  }

  @Test
  public void modelFile_isPreferredTo_languageDependentIsBetter() {
    ModelFileManager.ModelFile modelA =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, "/path/a", /* version= */ 1, "ja", /* isAsset= */ false);

    ModelFileManager.ModelFile modelB =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, "/path/b", /* version= */ 2, LANGUAGE_INDEPENDENT, /* isAsset= */ false);

    assertThat(modelA.isPreferredTo(modelB)).isTrue();
  }

  @Test
  public void modelFile_isPreferredTo_version() {
    ModelFileManager.ModelFile modelA =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, "/path/a", /* version= */ 2, "ja", /* isAsset= */ false);

    ModelFileManager.ModelFile modelB =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, "/path/b", /* version= */ 1, "ja", /* isAsset= */ false);

    assertThat(modelA.isPreferredTo(modelB)).isTrue();
  }

  @Test
  public void modelFile_toModelInfo() {
    ModelFileManager.ModelFile modelFile =
        new ModelFileManager.ModelFile(
            MODEL_TYPE, "/path/a", /* version= */ 2, "ja", /* isAsset= */ false);

    ModelInfo modelInfo = modelFile.toModelInfo();

    assertThat(modelInfo.toModelName()).isEqualTo("ja_v2");
  }

  @Test
  public void modelFile_toModelInfos() {
    ModelFile englishModelFile =
        new ModelFile(MODEL_TYPE, "/path/a", /* version= */ 1, "en", /* isAsset= */ false);
    ModelFile japaneseModelFile =
        new ModelFile(MODEL_TYPE, "/path/a", /* version= */ 2, "ja", /* isAsset= */ false);

    ImmutableList<Optional<ModelInfo>> modelInfos =
        ModelFileManager.ModelFile.toModelInfos(
            Optional.of(englishModelFile), Optional.of(japaneseModelFile));

    assertThat(
            modelInfos.stream()
                .map(modelFile -> modelFile.transform(ModelInfo::toModelName).or(""))
                .collect(Collectors.toList()))
        .containsExactly("en_v1", "ja_v2")
        .inOrder();
  }

  @Test
  public void regularFileFullMatchLister() throws IOException {
    File modelFile = new File(rootTestDir, "test.model");
    Files.copy(TestDataUtils.getTestAnnotatorModelFile(), modelFile);
    File wrongFile = new File(rootTestDir, "wrong.model");
    Files.copy(TestDataUtils.getTestAnnotatorModelFile(), wrongFile);

    RegularFileFullMatchLister regularFileFullMatchLister =
        new RegularFileFullMatchLister(MODEL_TYPE, modelFile, () -> true);
    ImmutableList<ModelFile> listedModels = regularFileFullMatchLister.list(MODEL_TYPE);

    assertThat(listedModels).hasSize(1);
    assertThat(listedModels.get(0).absolutePath).isEqualTo(modelFile.getAbsolutePath());
    assertThat(listedModels.get(0).isAsset).isFalse();
  }

  @Test
  public void regularFilePatternMatchLister() throws IOException {
    File modelFile1 = new File(rootTestDir, "annotator.en.model");
    Files.copy(TestDataUtils.getTestAnnotatorModelFile(), modelFile1);
    File modelFile2 = new File(rootTestDir, "annotator.fr.model");
    Files.copy(TestDataUtils.getTestAnnotatorModelFile(), modelFile2);
    File mismatchedModelFile = new File(rootTestDir, "actions.en.model");
    Files.copy(TestDataUtils.getTestAnnotatorModelFile(), mismatchedModelFile);

    RegularFilePatternMatchLister regularFilePatternMatchLister =
        new RegularFilePatternMatchLister(
            MODEL_TYPE, rootTestDir, "annotator\\.(.*)\\.model", () -> true);
    ImmutableList<ModelFile> listedModels = regularFilePatternMatchLister.list(MODEL_TYPE);

    assertThat(listedModels).hasSize(2);
    assertThat(listedModels.get(0).isAsset).isFalse();
    assertThat(listedModels.get(1).isAsset).isFalse();
    assertThat(ImmutableList.of(listedModels.get(0).absolutePath, listedModels.get(1).absolutePath))
        .containsExactly(modelFile1.getAbsolutePath(), modelFile2.getAbsolutePath());
  }

  @Test
  public void regularFilePatternMatchLister_disabled() throws IOException {
    File modelFile1 = new File(rootTestDir, "annotator.en.model");
    Files.copy(TestDataUtils.getTestAnnotatorModelFile(), modelFile1);

    RegularFilePatternMatchLister regularFilePatternMatchLister =
        new RegularFilePatternMatchLister(
            MODEL_TYPE, rootTestDir, "annotator\\.(.*)\\.model", () -> false);
    ImmutableList<ModelFile> listedModels = regularFilePatternMatchLister.list(MODEL_TYPE);

    assertThat(listedModels).isEmpty();
  }

  private static void recursiveDelete(File f) {
    if (f.isDirectory()) {
      for (File innerFile : f.listFiles()) {
        recursiveDelete(innerFile);
      }
    }
    f.delete();
  }
}
