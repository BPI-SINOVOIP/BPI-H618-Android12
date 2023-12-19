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

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.os.LocaleList;
import android.os.ParcelFileDescriptor;
import android.util.ArraySet;
import androidx.annotation.GuardedBy;
import androidx.collection.ArrayMap;
import com.android.textclassifier.common.ModelType.ModelTypeDef;
import com.android.textclassifier.common.base.TcLog;
import com.android.textclassifier.common.logging.ResultIdUtils.ModelInfo;
import com.android.textclassifier.utils.IndentingPrintWriter;
import com.google.android.textclassifier.ActionsSuggestionsModel;
import com.google.android.textclassifier.AnnotatorModel;
import com.google.android.textclassifier.LangIdModel;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Function;
import com.google.common.base.Optional;
import com.google.common.base.Preconditions;
import com.google.common.base.Supplier;
import com.google.common.collect.ImmutableList;
import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;
import javax.annotation.Nullable;

// TODO(licha): Consider making this a singleton class
// TODO(licha): Check whether this is thread-safe
/**
 * Manages all model files in storage. {@link TextClassifierImpl} depends on this class to get the
 * model files to load.
 */
public final class ModelFileManager {

  private static final String TAG = "ModelFileManager";

  private static final String DOWNLOAD_SUB_DIR_NAME = "textclassifier/downloads/models/";
  private static final File CONFIG_UPDATER_DIR = new File("/data/misc/textclassifier/");
  private static final String ASSETS_DIR = "textclassifier";

  private final List<ModelFileLister> modelFileListers;
  private final File modelDownloaderDir;

  public ModelFileManager(Context context, TextClassifierSettings settings) {
    Preconditions.checkNotNull(context);
    Preconditions.checkNotNull(settings);

    AssetManager assetManager = context.getAssets();
    this.modelDownloaderDir = new File(context.getFilesDir(), DOWNLOAD_SUB_DIR_NAME);
    modelFileListers =
        ImmutableList.of(
            // Annotator models.
            new RegularFilePatternMatchLister(
                ModelType.ANNOTATOR,
                this.modelDownloaderDir,
                "annotator\\.(.*)\\.model",
                settings::isModelDownloadManagerEnabled),
            new RegularFileFullMatchLister(
                ModelType.ANNOTATOR,
                new File(CONFIG_UPDATER_DIR, "textclassifier.model"),
                /* isEnabled= */ () -> true),
            new AssetFilePatternMatchLister(
                assetManager,
                ModelType.ANNOTATOR,
                ASSETS_DIR,
                "annotator\\.(.*)\\.model",
                /* isEnabled= */ () -> true),
            // Actions models.
            new RegularFilePatternMatchLister(
                ModelType.ACTIONS_SUGGESTIONS,
                this.modelDownloaderDir,
                "actions_suggestions\\.(.*)\\.model",
                settings::isModelDownloadManagerEnabled),
            new RegularFileFullMatchLister(
                ModelType.ACTIONS_SUGGESTIONS,
                new File(CONFIG_UPDATER_DIR, "actions_suggestions.model"),
                /* isEnabled= */ () -> true),
            new AssetFilePatternMatchLister(
                assetManager,
                ModelType.ACTIONS_SUGGESTIONS,
                ASSETS_DIR,
                "actions_suggestions\\.(.*)\\.model",
                /* isEnabled= */ () -> true),
            // LangID models.
            new RegularFilePatternMatchLister(
                ModelType.LANG_ID,
                this.modelDownloaderDir,
                "lang_id\\.(.*)\\.model",
                settings::isModelDownloadManagerEnabled),
            new RegularFileFullMatchLister(
                ModelType.LANG_ID,
                new File(CONFIG_UPDATER_DIR, "lang_id.model"),
                /* isEnabled= */ () -> true),
            new AssetFilePatternMatchLister(
                assetManager,
                ModelType.LANG_ID,
                ASSETS_DIR,
                "lang_id.model",
                /* isEnabled= */ () -> true));
  }

  @VisibleForTesting
  public ModelFileManager(Context context, List<ModelFileLister> modelFileListers) {
    this.modelDownloaderDir = new File(context.getFilesDir(), DOWNLOAD_SUB_DIR_NAME);
    this.modelFileListers = ImmutableList.copyOf(modelFileListers);
  }

  /**
   * Returns an immutable list of model files listed by the given model files supplier.
   *
   * @param modelType which type of model files to look for
   */
  public ImmutableList<ModelFile> listModelFiles(@ModelTypeDef String modelType) {
    Preconditions.checkNotNull(modelType);

    ImmutableList.Builder<ModelFile> modelFiles = new ImmutableList.Builder<>();
    for (ModelFileLister modelFileLister : modelFileListers) {
      modelFiles.addAll(modelFileLister.list(modelType));
    }
    return modelFiles.build();
  }

  /** Lists model files. */
  public interface ModelFileLister {
    List<ModelFile> list(@ModelTypeDef String modelType);
  }

  /** Lists model files by performing full match on file path. */
  public static class RegularFileFullMatchLister implements ModelFileLister {
    private final String modelType;
    private final File targetFile;
    private final Supplier<Boolean> isEnabled;

    /**
     * @param modelType the type of the model
     * @param targetFile the expected model file
     * @param isEnabled whether this lister is enabled
     */
    public RegularFileFullMatchLister(
        @ModelTypeDef String modelType, File targetFile, Supplier<Boolean> isEnabled) {
      this.modelType = Preconditions.checkNotNull(modelType);
      this.targetFile = Preconditions.checkNotNull(targetFile);
      this.isEnabled = Preconditions.checkNotNull(isEnabled);
    }

    @Override
    public ImmutableList<ModelFile> list(@ModelTypeDef String modelType) {
      if (!this.modelType.equals(modelType)) {
        return ImmutableList.of();
      }
      if (!isEnabled.get()) {
        return ImmutableList.of();
      }
      if (!targetFile.exists()) {
        return ImmutableList.of();
      }
      try {
        return ImmutableList.of(ModelFile.createFromRegularFile(targetFile, modelType));
      } catch (IOException e) {
        TcLog.e(
            TAG, "Failed to call createFromRegularFile with: " + targetFile.getAbsolutePath(), e);
      }
      return ImmutableList.of();
    }
  }

  /** Lists model file in a specified folder by doing pattern matching on file names. */
  public static class RegularFilePatternMatchLister implements ModelFileLister {
    private final String modelType;
    private final File folder;
    private final Pattern fileNamePattern;
    private final Supplier<Boolean> isEnabled;

    /**
     * @param modelType the type of the model
     * @param folder the folder to list files
     * @param fileNameRegex the regex to match the file name in the specified folder
     * @param isEnabled whether the lister is enabled
     */
    public RegularFilePatternMatchLister(
        @ModelTypeDef String modelType,
        File folder,
        String fileNameRegex,
        Supplier<Boolean> isEnabled) {
      this.modelType = Preconditions.checkNotNull(modelType);
      this.folder = Preconditions.checkNotNull(folder);
      this.fileNamePattern = Pattern.compile(Preconditions.checkNotNull(fileNameRegex));
      this.isEnabled = Preconditions.checkNotNull(isEnabled);
    }

    @Override
    public ImmutableList<ModelFile> list(@ModelTypeDef String modelType) {
      if (!this.modelType.equals(modelType)) {
        return ImmutableList.of();
      }
      if (!isEnabled.get()) {
        return ImmutableList.of();
      }
      if (!folder.isDirectory()) {
        return ImmutableList.of();
      }
      File[] files = folder.listFiles();
      if (files == null) {
        return ImmutableList.of();
      }
      ImmutableList.Builder<ModelFile> modelFilesBuilder = ImmutableList.builder();
      for (File file : files) {
        final Matcher matcher = fileNamePattern.matcher(file.getName());
        if (!matcher.matches() || !file.isFile()) {
          continue;
        }
        try {
          modelFilesBuilder.add(ModelFile.createFromRegularFile(file, modelType));
        } catch (IOException e) {
          TcLog.w(TAG, "Failed to call createFromRegularFile with: " + file.getAbsolutePath());
        }
      }
      return modelFilesBuilder.build();
    }
  }

  /** Lists the model files preloaded in the APK file. */
  public static class AssetFilePatternMatchLister implements ModelFileLister {
    private final AssetManager assetManager;
    private final String modelType;
    private final String pathToList;
    private final Pattern fileNamePattern;
    private final Supplier<Boolean> isEnabled;
    private final Object lock = new Object();
    // Assets won't change without updating the app, so cache the result for performance reason.
    @GuardedBy("lock")
    private final Map<String, ImmutableList<ModelFile>> resultCache;

    /**
     * @param modelType the type of the model.
     * @param pathToList the folder to list files
     * @param fileNameRegex the regex to match the file name in the specified folder
     * @param isEnabled whether this lister is enabled
     */
    public AssetFilePatternMatchLister(
        AssetManager assetManager,
        @ModelTypeDef String modelType,
        String pathToList,
        String fileNameRegex,
        Supplier<Boolean> isEnabled) {
      this.assetManager = Preconditions.checkNotNull(assetManager);
      this.modelType = Preconditions.checkNotNull(modelType);
      this.pathToList = Preconditions.checkNotNull(pathToList);
      this.fileNamePattern = Pattern.compile(Preconditions.checkNotNull(fileNameRegex));
      this.isEnabled = Preconditions.checkNotNull(isEnabled);
      resultCache = new ArrayMap<>();
    }

    @Override
    public ImmutableList<ModelFile> list(@ModelTypeDef String modelType) {
      if (!this.modelType.equals(modelType)) {
        return ImmutableList.of();
      }
      if (!isEnabled.get()) {
        return ImmutableList.of();
      }
      synchronized (lock) {
        if (resultCache.get(modelType) != null) {
          return resultCache.get(modelType);
        }
        String[] fileNames = null;
        try {
          fileNames = assetManager.list(pathToList);
        } catch (IOException e) {
          TcLog.e(TAG, "Failed to list assets", e);
        }
        if (fileNames == null) {
          return ImmutableList.of();
        }
        ImmutableList.Builder<ModelFile> modelFilesBuilder = ImmutableList.builder();
        for (String fileName : fileNames) {
          final Matcher matcher = fileNamePattern.matcher(fileName);
          if (!matcher.matches()) {
            continue;
          }
          String absolutePath =
              new StringBuilder(pathToList).append('/').append(fileName).toString();
          try {
            modelFilesBuilder.add(ModelFile.createFromAsset(assetManager, absolutePath, modelType));
          } catch (IOException e) {
            TcLog.w(TAG, "Failed to call createFromAsset with: " + absolutePath);
          }
        }
        ImmutableList<ModelFile> result = modelFilesBuilder.build();
        resultCache.put(modelType, result);
        return result;
      }
    }
  }

  /**
   * Returns the best model file for the given localelist, {@code null} if nothing is found.
   *
   * @param modelType the type of model to look up (e.g. annotator, lang_id, etc.)
   * @param localePreferences an ordered list of user preferences for locales, use {@code null} if
   *     there is no preference.
   */
  @Nullable
  public ModelFile findBestModelFile(
      @ModelTypeDef String modelType, @Nullable LocaleList localePreferences) {
    final String languages =
        localePreferences == null || localePreferences.isEmpty()
            ? LocaleList.getDefault().toLanguageTags()
            : localePreferences.toLanguageTags();
    final List<Locale.LanguageRange> languageRangeList = Locale.LanguageRange.parse(languages);

    ModelFile bestModel = null;
    for (ModelFile model : listModelFiles(modelType)) {
      // TODO(licha): update this when we want to support multiple languages
      if (model.isAnyLanguageSupported(languageRangeList)) {
        if (model.isPreferredTo(bestModel)) {
          bestModel = model;
        }
      }
    }
    return bestModel;
  }

  /**
   * Deletes model files that are not preferred for any locales in user's preference.
   *
   * <p>This method will be invoked as a clean-up after we download a new model successfully. Race
   * conditions are hard to avoid because we do not hold locks for files. But it should rarely cause
   * any issues since it's safe to delete a model file in use (b/c we mmap it to memory).
   */
  public void deleteUnusedModelFiles() {
    TcLog.d(TAG, "Start to delete unused model files.");
    LocaleList localeList = LocaleList.getDefault();
    for (@ModelTypeDef String modelType : ModelType.values()) {
      ArraySet<ModelFile> allModelFiles = new ArraySet<>(listModelFiles(modelType));
      for (int i = 0; i < localeList.size(); i++) {
        // If a model file is preferred for any local in locale list, then keep it
        ModelFile bestModel = findBestModelFile(modelType, new LocaleList(localeList.get(i)));
        allModelFiles.remove(bestModel);
      }
      for (ModelFile modelFile : allModelFiles) {
        if (modelFile.canWrite()) {
          TcLog.d(TAG, "Deleting model: " + modelFile);
          if (!modelFile.delete()) {
            TcLog.w(TAG, "Failed to delete model: " + modelFile);
          }
        }
      }
    }
  }

  /** Returns the directory containing models downloaded by the downloader. */
  public File getModelDownloaderDir() {
    return modelDownloaderDir;
  }

  /**
   * Dumps the internal state for debugging.
   *
   * @param printWriter writer to write dumped states
   */
  public void dump(IndentingPrintWriter printWriter) {
    printWriter.println("ModelFileManager:");
    printWriter.increaseIndent();
    for (@ModelTypeDef String modelType : ModelType.values()) {
      printWriter.println(modelType + " model file(s):");
      printWriter.increaseIndent();
      for (ModelFile modelFile : listModelFiles(modelType)) {
        printWriter.println(modelFile.toString());
      }
      printWriter.decreaseIndent();
    }
    printWriter.decreaseIndent();
  }

  /** Fetch metadata of a model file. */
  private static class ModelInfoFetcher {
    private final Function<AssetFileDescriptor, Integer> versionFetcher;
    private final Function<AssetFileDescriptor, String> supportedLocalesFetcher;

    private ModelInfoFetcher(
        Function<AssetFileDescriptor, Integer> versionFetcher,
        Function<AssetFileDescriptor, String> supportedLocalesFetcher) {
      this.versionFetcher = versionFetcher;
      this.supportedLocalesFetcher = supportedLocalesFetcher;
    }

    int getVersion(AssetFileDescriptor assetFileDescriptor) {
      return versionFetcher.apply(assetFileDescriptor);
    }

    String getSupportedLocales(AssetFileDescriptor assetFileDescriptor) {
      return supportedLocalesFetcher.apply(assetFileDescriptor);
    }

    static ModelInfoFetcher create(@ModelTypeDef String modelType) {
      switch (modelType) {
        case ModelType.ANNOTATOR:
          return new ModelInfoFetcher(AnnotatorModel::getVersion, AnnotatorModel::getLocales);
        case ModelType.ACTIONS_SUGGESTIONS:
          return new ModelInfoFetcher(
              ActionsSuggestionsModel::getVersion, ActionsSuggestionsModel::getLocales);
        case ModelType.LANG_ID:
          return new ModelInfoFetcher(
              LangIdModel::getVersion, afd -> ModelFile.LANGUAGE_INDEPENDENT);
        default: // fall out
      }
      throw new IllegalStateException("Unsupported model types");
    }
  }

  /** Describes TextClassifier model files on disk. */
  public static class ModelFile {
    @VisibleForTesting static final String LANGUAGE_INDEPENDENT = "*";

    @ModelTypeDef public final String modelType;
    public final String absolutePath;
    public final int version;
    public final LocaleList supportedLocales;
    public final boolean languageIndependent;
    public final boolean isAsset;

    public static ModelFile createFromRegularFile(File file, @ModelTypeDef String modelType)
        throws IOException {
      ParcelFileDescriptor pfd =
          ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY);
      try (AssetFileDescriptor afd = new AssetFileDescriptor(pfd, 0, file.length())) {
        return createFromAssetFileDescriptor(
            file.getAbsolutePath(), modelType, afd, /* isAsset= */ false);
      }
    }

    public static ModelFile createFromAsset(
        AssetManager assetManager, String absolutePath, @ModelTypeDef String modelType)
        throws IOException {
      try (AssetFileDescriptor assetFileDescriptor = assetManager.openFd(absolutePath)) {
        return createFromAssetFileDescriptor(
            absolutePath, modelType, assetFileDescriptor, /* isAsset= */ true);
      }
    }

    private static ModelFile createFromAssetFileDescriptor(
        String absolutePath,
        @ModelTypeDef String modelType,
        AssetFileDescriptor assetFileDescriptor,
        boolean isAsset) {
      ModelInfoFetcher modelInfoFetcher = ModelInfoFetcher.create(modelType);
      return new ModelFile(
          modelType,
          absolutePath,
          modelInfoFetcher.getVersion(assetFileDescriptor),
          modelInfoFetcher.getSupportedLocales(assetFileDescriptor),
          isAsset);
    }

    @VisibleForTesting
    ModelFile(
        @ModelTypeDef String modelType,
        String absolutePath,
        int version,
        String supportedLocaleTags,
        boolean isAsset) {
      this.modelType = modelType;
      this.absolutePath = absolutePath;
      this.version = version;
      this.languageIndependent = LANGUAGE_INDEPENDENT.equals(supportedLocaleTags);
      this.supportedLocales =
          languageIndependent
              ? LocaleList.getEmptyLocaleList()
              : LocaleList.forLanguageTags(supportedLocaleTags);
      this.isAsset = isAsset;
    }

    /** Returns if this model file is preferred to the given one. */
    public boolean isPreferredTo(@Nullable ModelFile model) {
      // A model is preferred to no model.
      if (model == null) {
        return true;
      }

      // A language-specific model is preferred to a language independent
      // model.
      if (!languageIndependent && model.languageIndependent) {
        return true;
      }
      if (languageIndependent && !model.languageIndependent) {
        return false;
      }

      // A higher-version model is preferred.
      if (version > model.version) {
        return true;
      }
      return false;
    }

    /** Returns whether the language supports any language in the given ranges. */
    public boolean isAnyLanguageSupported(List<Locale.LanguageRange> languageRanges) {
      Preconditions.checkNotNull(languageRanges);
      if (languageIndependent) {
        return true;
      }
      List<String> supportedLocaleTags =
          Arrays.asList(supportedLocales.toLanguageTags().split(","));
      return Locale.lookupTag(languageRanges, supportedLocaleTags) != null;
    }

    public AssetFileDescriptor open(AssetManager assetManager) throws IOException {
      if (isAsset) {
        return assetManager.openFd(absolutePath);
      }
      File file = new File(absolutePath);
      ParcelFileDescriptor parcelFileDescriptor =
          ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY);
      return new AssetFileDescriptor(parcelFileDescriptor, 0, file.length());
    }

    public boolean canWrite() {
      if (isAsset) {
        return false;
      }
      return new File(absolutePath).canWrite();
    }

    public boolean delete() {
      if (isAsset) {
        throw new IllegalStateException("asset is read-only, deleting it is not allowed.");
      }
      return new File(absolutePath).delete();
    }

    @Override
    public boolean equals(Object o) {
      if (this == o) {
        return true;
      }
      if (!(o instanceof ModelFile)) {
        return false;
      }
      ModelFile modelFile = (ModelFile) o;
      return version == modelFile.version
          && languageIndependent == modelFile.languageIndependent
          && isAsset == modelFile.isAsset
          && Objects.equals(modelType, modelFile.modelType)
          && Objects.equals(absolutePath, modelFile.absolutePath)
          && Objects.equals(supportedLocales, modelFile.supportedLocales);
    }

    @Override
    public int hashCode() {
      return Objects.hash(
          modelType, absolutePath, version, supportedLocales, languageIndependent, isAsset);
    }

    public ModelInfo toModelInfo() {
      return new ModelInfo(version, supportedLocales.toLanguageTags());
    }

    @Override
    public String toString() {
      return String.format(
          Locale.US,
          "ModelFile { type=%s path=%s version=%d locales=%s isAsset=%b}",
          modelType,
          absolutePath,
          version,
          languageIndependent ? LANGUAGE_INDEPENDENT : supportedLocales.toLanguageTags(),
          isAsset);
    }

    public static ImmutableList<Optional<ModelInfo>> toModelInfos(
        Optional<ModelFileManager.ModelFile>... modelFiles) {
      return Arrays.stream(modelFiles)
          .map(modelFile -> modelFile.transform(ModelFileManager.ModelFile::toModelInfo))
          .collect(Collectors.collectingAndThen(Collectors.toList(), ImmutableList::copyOf));
    }
  }
}
