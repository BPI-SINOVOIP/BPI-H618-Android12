/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.scopedstorage.cts;

import static android.scopedstorage.cts.lib.TestUtils.BYTES_DATA1;
import static android.scopedstorage.cts.lib.TestUtils.adoptShellPermissionIdentity;
import static android.scopedstorage.cts.lib.TestUtils.assertCanAccessPrivateAppAndroidDataDir;
import static android.scopedstorage.cts.lib.TestUtils.assertCanAccessPrivateAppAndroidObbDir;
import static android.scopedstorage.cts.lib.TestUtils.assertCanRenameFile;
import static android.scopedstorage.cts.lib.TestUtils.assertDirectoryContains;
import static android.scopedstorage.cts.lib.TestUtils.assertFileContent;
import static android.scopedstorage.cts.lib.TestUtils.assertMountMode;
import static android.scopedstorage.cts.lib.TestUtils.assertThrows;
import static android.scopedstorage.cts.lib.TestUtils.canOpen;
import static android.scopedstorage.cts.lib.TestUtils.canReadAndWriteAs;
import static android.scopedstorage.cts.lib.TestUtils.createFileAs;
import static android.scopedstorage.cts.lib.TestUtils.deleteFileAs;
import static android.scopedstorage.cts.lib.TestUtils.deleteFileAsNoThrow;
import static android.scopedstorage.cts.lib.TestUtils.dropShellPermissionIdentity;
import static android.scopedstorage.cts.lib.TestUtils.executeShellCommand;
import static android.scopedstorage.cts.lib.TestUtils.getAndroidDir;
import static android.scopedstorage.cts.lib.TestUtils.getAndroidMediaDir;
import static android.scopedstorage.cts.lib.TestUtils.getContentResolver;
import static android.scopedstorage.cts.lib.TestUtils.getDcimDir;
import static android.scopedstorage.cts.lib.TestUtils.getDefaultTopLevelDirs;
import static android.scopedstorage.cts.lib.TestUtils.getDownloadDir;
import static android.scopedstorage.cts.lib.TestUtils.getExternalFilesDir;
import static android.scopedstorage.cts.lib.TestUtils.getExternalMediaDir;
import static android.scopedstorage.cts.lib.TestUtils.getExternalStorageDir;
import static android.scopedstorage.cts.lib.TestUtils.getFileOwnerPackageFromDatabase;
import static android.scopedstorage.cts.lib.TestUtils.getFileRowIdFromDatabase;
import static android.scopedstorage.cts.lib.TestUtils.getFileUri;
import static android.scopedstorage.cts.lib.TestUtils.getMoviesDir;
import static android.scopedstorage.cts.lib.TestUtils.getMusicDir;
import static android.scopedstorage.cts.lib.TestUtils.getPicturesDir;
import static android.scopedstorage.cts.lib.TestUtils.openWithMediaProvider;
import static android.scopedstorage.cts.lib.TestUtils.pollForExternalStorageState;
import static android.scopedstorage.cts.lib.TestUtils.pollForManageExternalStorageAllowed;
import static android.scopedstorage.cts.lib.TestUtils.pollForPermission;
import static android.scopedstorage.cts.lib.TestUtils.setupDefaultDirectories;
import static android.scopedstorage.cts.lib.TestUtils.verifyInsertFromExternalMediaDirViaData_allowed;
import static android.scopedstorage.cts.lib.TestUtils.verifyInsertFromExternalMediaDirViaRelativePath_allowed;
import static android.scopedstorage.cts.lib.TestUtils.verifyInsertFromExternalPrivateDirViaData_denied;
import static android.scopedstorage.cts.lib.TestUtils.verifyInsertFromExternalPrivateDirViaRelativePath_denied;
import static android.scopedstorage.cts.lib.TestUtils.verifyUpdateToExternalDirsViaData_denied;
import static android.scopedstorage.cts.lib.TestUtils.verifyUpdateToExternalMediaDirViaRelativePath_allowed;
import static android.scopedstorage.cts.lib.TestUtils.verifyUpdateToExternalPrivateDirsViaRelativePath_denied;
import static android.system.OsConstants.F_OK;
import static android.system.OsConstants.R_OK;
import static android.system.OsConstants.W_OK;

import static androidx.test.InstrumentationRegistry.getContext;

import static com.google.common.truth.Truth.assertThat;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assume.assumeFalse;
import static org.junit.Assume.assumeTrue;

import android.Manifest;
import android.app.WallpaperManager;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.os.storage.StorageManager;
import android.platform.test.annotations.AppModeInstant;
import android.provider.MediaStore;
import android.system.ErrnoException;
import android.system.Os;
import android.util.Log;

import androidx.test.runner.AndroidJUnit4;

import com.android.cts.install.lib.TestApp;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Runs the scoped storage tests on primary external storage.
 *
 * <p>These tests are also run on a public volume by {@link PublicVolumeTest}.
 */
@RunWith(AndroidJUnit4.class)
public class ScopedStorageTest {
    static final String TAG = "ScopedStorageTest";
    static final String THIS_PACKAGE_NAME = getContext().getPackageName();
    static final int USER_SYSTEM = 0;

    /**
     * To help avoid flaky tests, give ourselves a unique nonce to be used for
     * all filesystem paths, so that we don't risk conflicting with previous
     * test runs.
     */
    static final String NONCE = String.valueOf(System.nanoTime());

    static final String TEST_DIRECTORY_NAME = "ScopedStorageTestDirectory" + NONCE;

    static final String AUDIO_FILE_NAME = "ScopedStorageTest_file_" + NONCE + ".mp3";
    static final String IMAGE_FILE_NAME = "ScopedStorageTest_file_" + NONCE + ".jpg";
    static final String NONMEDIA_FILE_NAME = "ScopedStorageTest_file_" + NONCE + ".pdf";

    // The following apps are installed before the tests are run via a target_preparer.
    // See test config for details.
    // An app with READ_EXTERNAL_STORAGE permission
    private static final TestApp APP_A_HAS_RES = new TestApp("TestAppA",
            "android.scopedstorage.cts.testapp.A.withres", 1, false,
            "CtsScopedStorageTestAppA.apk");
    // An app with no permissions
    private static final TestApp APP_B_NO_PERMS = new TestApp("TestAppB",
            "android.scopedstorage.cts.testapp.B.noperms", 1, false,
            "CtsScopedStorageTestAppB.apk");
    // A legacy targeting app with RES and WES permissions
    private static final TestApp APP_D_LEGACY_HAS_RW = new TestApp("TestAppDLegacy",
            "android.scopedstorage.cts.testapp.D", 1, false, "CtsScopedStorageTestAppCLegacy.apk");

    @Before
    public void setup() throws Exception {
        if (!getContext().getPackageManager().isInstantApp()) {
            pollForExternalStorageState();
            getExternalFilesDir().mkdirs();
        }
    }

    /**
     * This method needs to be called once before running the whole test.
     */
    @Test
    public void setupExternalStorage() {
        setupDefaultDirectories();
    }

    /**
     * Test that Installer packages can access app's private directories in Android/obb
     */
    @Test
    public void testCheckInstallerAppAccessToObbDirs() throws Exception {
        assertCanAccessPrivateAppAndroidObbDir(true /*canAccess*/, APP_B_NO_PERMS,
                THIS_PACKAGE_NAME, NONMEDIA_FILE_NAME);
        final int uid = getContext().getPackageManager().getPackageUid(THIS_PACKAGE_NAME, 0);
        if (isAtLeastS()) {
            assertMountMode(THIS_PACKAGE_NAME, uid, StorageManager.MOUNT_MODE_EXTERNAL_INSTALLER);
        }
    }

    /**
     * Test that Installer packages cannot access app's private directories in Android/data
     */
    @Test
    public void testCheckInstallerAppCannotAccessDataDirs() throws Exception {
        assertCanAccessPrivateAppAndroidDataDir(false /*canAccess*/, APP_B_NO_PERMS,
                THIS_PACKAGE_NAME, NONMEDIA_FILE_NAME);
        final int uid = getContext().getPackageManager().getPackageUid(THIS_PACKAGE_NAME, 0);
        if (isAtLeastS()) {
            assertMountMode(THIS_PACKAGE_NAME, uid, StorageManager.MOUNT_MODE_EXTERNAL_INSTALLER);
        }
    }

    @Test
    public void testManageExternalStorageCanCreateFilesAnywhere() throws Exception {
        pollForManageExternalStorageAllowed();

        final File topLevelPdf = new File(getExternalStorageDir(), NONMEDIA_FILE_NAME);
        final File musicFileInMovies = new File(getMoviesDir(), AUDIO_FILE_NAME);
        final File imageFileInDcim = new File(getDcimDir(), IMAGE_FILE_NAME);

        // Nothing special about this, anyone can create an image file in DCIM
        assertCanCreateFile(imageFileInDcim);
        // This is where we see the special powers of MANAGE_EXTERNAL_STORAGE, because it can
        // create a top level file
        assertCanCreateFile(topLevelPdf);
        // It can even create a music file in Pictures
        assertCanCreateFile(musicFileInMovies);
    }

    @Test
    public void testManageExternalStorageCantReadWriteOtherAppExternalDir() throws Exception {
        pollForManageExternalStorageAllowed();

        // Let app A create a file in its data dir
        final File otherAppExternalDataDir = new File(getExternalFilesDir().getPath().replace(
                THIS_PACKAGE_NAME, APP_A_HAS_RES.getPackageName()));
        final File otherAppExternalDataFile = new File(otherAppExternalDataDir,
                NONMEDIA_FILE_NAME);
        assertCreateFilesAs(APP_A_HAS_RES, otherAppExternalDataFile);

        // File Manager app gets global access with MANAGE_EXTERNAL_STORAGE permission, however,
        // file manager app doesn't have access to other app's external files directory
        assertThat(canOpen(otherAppExternalDataFile, /* forWrite */ false)).isFalse();
        assertThat(canOpen(otherAppExternalDataFile, /* forWrite */ true)).isFalse();
        assertThat(otherAppExternalDataFile.delete()).isFalse();

        assertThat(deleteFileAs(APP_A_HAS_RES, otherAppExternalDataFile.getPath())).isTrue();

        assertThrows(IOException.class,
                () -> {
                    otherAppExternalDataFile.createNewFile();
                });
    }

    @Test
    public void testManageExternalStorageCanDeleteOtherAppsContents() throws Exception {
        pollForManageExternalStorageAllowed();

        final File otherAppPdf = new File(getDownloadDir(), "other" + NONMEDIA_FILE_NAME);
        final File otherAppImage = new File(getDcimDir(), "other" + IMAGE_FILE_NAME);
        final File otherAppMusic = new File(getMusicDir(), "other" + AUDIO_FILE_NAME);
        try {
            // Create all of the files as another app
            assertThat(createFileAs(APP_B_NO_PERMS, otherAppPdf.getPath())).isTrue();
            assertThat(createFileAs(APP_B_NO_PERMS, otherAppImage.getPath())).isTrue();
            assertThat(createFileAs(APP_B_NO_PERMS, otherAppMusic.getPath())).isTrue();

            assertThat(otherAppPdf.delete()).isTrue();
            assertThat(otherAppPdf.exists()).isFalse();

            assertThat(otherAppImage.delete()).isTrue();
            assertThat(otherAppImage.exists()).isFalse();

            assertThat(otherAppMusic.delete()).isTrue();
            assertThat(otherAppMusic.exists()).isFalse();
        } finally {
            deleteFileAsNoThrow(APP_B_NO_PERMS, otherAppPdf.getAbsolutePath());
            deleteFileAsNoThrow(APP_B_NO_PERMS, otherAppImage.getAbsolutePath());
            deleteFileAsNoThrow(APP_B_NO_PERMS, otherAppMusic.getAbsolutePath());
        }
    }

    @Test
    public void testAccess_file() throws Exception {
        pollForPermission(Manifest.permission.READ_EXTERNAL_STORAGE, /*granted*/ true);

        final File downloadDir = getDownloadDir();
        final File otherAppPdf = new File(downloadDir, "other-" + NONMEDIA_FILE_NAME);
        final File shellPdfAtRoot = new File(getExternalStorageDir(),
                "shell-" + NONMEDIA_FILE_NAME);
        final File otherAppImage = new File(getDcimDir(), "other-" + IMAGE_FILE_NAME);
        final File myAppPdf = new File(downloadDir, "my-" + NONMEDIA_FILE_NAME);
        final File doesntExistPdf = new File(downloadDir, "nada-" + NONMEDIA_FILE_NAME);

        try {
            assertThat(createFileAs(APP_B_NO_PERMS, otherAppPdf.getPath())).isTrue();
            assertThat(createFileAs(APP_B_NO_PERMS, otherAppImage.getPath())).isTrue();

            // We can read our image and pdf files.
            assertThat(myAppPdf.createNewFile()).isTrue();
            assertFileAccess_readWrite(myAppPdf);

            // We can read the other app's image file because we hold R_E_S, but we can
            // check only exists for the pdf files.
            assertFileAccess_readOnly(otherAppImage);
            assertFileAccess_existsOnly(otherAppPdf);
            assertAccess(doesntExistPdf, false, false, false);

            // We can check only exists for another app's files on root.
            createFileAsLegacyApp(shellPdfAtRoot);
            MediaStore.scanFile(getContentResolver(), shellPdfAtRoot);
            assertFileAccess_existsOnly(shellPdfAtRoot);
        } finally {
            deleteFileAsNoThrow(APP_B_NO_PERMS, otherAppPdf.getAbsolutePath());
            deleteFileAsNoThrow(APP_B_NO_PERMS, otherAppImage.getAbsolutePath());
            deleteAsLegacyApp(shellPdfAtRoot);
            MediaStore.scanFile(getContentResolver(), shellPdfAtRoot);
            myAppPdf.delete();
        }
    }

    @Test
    public void testAccess_directory() throws Exception {
        pollForPermission(Manifest.permission.READ_EXTERNAL_STORAGE, /*granted*/ true);
        pollForPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE, /*granted*/ true);
        File topLevelDir = new File(getExternalStorageDir(), "Test");
        try {
            // Let app B create a file in its data dir
            final File otherAppExternalDataDir = new File(getExternalFilesDir().getPath().replace(
                    THIS_PACKAGE_NAME, APP_B_NO_PERMS.getPackageName()));
            final File otherAppExternalDataSubDir = new File(otherAppExternalDataDir, "subdir");
            final File otherAppExternalDataFile = new File(otherAppExternalDataSubDir, "abc.jpg");
            assertThat(createFileAs(APP_B_NO_PERMS, otherAppExternalDataFile.getAbsolutePath()))
                    .isTrue();

            // We cannot read or write the file, but app B can.
            assertThat(canReadAndWriteAs(APP_B_NO_PERMS,
                    otherAppExternalDataFile.getAbsolutePath())).isTrue();
            assertCannotReadOrWrite(otherAppExternalDataFile);

            // We cannot read or write the dir, but app B can.
            assertThat(canReadAndWriteAs(APP_B_NO_PERMS,
                    otherAppExternalDataDir.getAbsolutePath())).isTrue();
            assertCannotReadOrWrite(otherAppExternalDataDir);

            // We cannot read or write the sub dir, but app B can.
            assertThat(canReadAndWriteAs(APP_B_NO_PERMS,
                    otherAppExternalDataSubDir.getAbsolutePath())).isTrue();
            assertCannotReadOrWrite(otherAppExternalDataSubDir);

            // We can read and write our own app dir, but app B cannot.
            assertThat(canReadAndWriteAs(APP_B_NO_PERMS,
                    getExternalFilesDir().getAbsolutePath())).isFalse();
            assertCanAccessMyAppFile(getExternalFilesDir());

            assertDirectoryAccess(getDcimDir(), /* exists */ true, /* canWrite */ true);
            assertDirectoryAccess(getExternalStorageDir(),true, false);
            assertDirectoryAccess(new File(getExternalStorageDir(), "Android"), true, false);
            assertDirectoryAccess(new File(getExternalStorageDir(), "doesnt/exist"), false, false);

            createDirectoryAsLegacyApp(topLevelDir);
            assertDirectoryAccess(topLevelDir, true, false);

            // We can see "/storage/emulated" exists, but not read/write to it, since it's
            // outside the scope of external storage.
            assertAccess(new File("/storage/emulated"), true, false, false);

            // Verify we can enter "/storage/emulated/<userId>" and read
            int userId = getContext().getUserId();
            assertAccess(new File("/storage/emulated/" + userId), true, true, false);

            // Verify we can't get another userId
            int otherUserId = userId + 1;
            assertAccess(new File("/storage/emulated/" + otherUserId), false, false, false);

            // Or an obviously invalid userId (b/172629984)
            assertAccess(new File("/storage/emulated/100000000000"), false, false, false);
        } finally {
            deleteAsLegacyApp(topLevelDir);
        }
    }

    @Test
    public void testManageExternalStorageCanRenameOtherAppsContents() throws Exception {
        pollForManageExternalStorageAllowed();

        final File otherAppPdf = new File(getDownloadDir(), "other" + NONMEDIA_FILE_NAME);
        final File pdf = new File(getDownloadDir(), NONMEDIA_FILE_NAME);
        final File pdfInObviouslyWrongPlace = new File(getPicturesDir(), NONMEDIA_FILE_NAME);
        final File topLevelPdf = new File(getExternalStorageDir(), NONMEDIA_FILE_NAME);
        final File musicFile = new File(getMusicDir(), AUDIO_FILE_NAME);
        try {
            // Have another app create a PDF
            assertThat(createFileAs(APP_B_NO_PERMS, otherAppPdf.getPath())).isTrue();
            assertThat(otherAppPdf.exists()).isTrue();


            // Write some data to the file
            try (final FileOutputStream fos = new FileOutputStream(otherAppPdf)) {
                fos.write(BYTES_DATA1);
            }
            assertFileContent(otherAppPdf, BYTES_DATA1);

            // Assert we can rename the file and ensure the file has the same content
            assertCanRenameFile(otherAppPdf, pdf, /* checkDatabase */ false);
            assertFileContent(pdf, BYTES_DATA1);
            // We can even move it to the top level directory
            assertCanRenameFile(pdf, topLevelPdf, /* checkDatabase */ false);
            assertFileContent(topLevelPdf, BYTES_DATA1);
            // And even rename to a place where PDFs don't belong, because we're an omnipotent
            // external storage manager
            assertCanRenameFile(topLevelPdf, pdfInObviouslyWrongPlace, /* checkDatabase */ false);
            assertFileContent(pdfInObviouslyWrongPlace, BYTES_DATA1);

            // And we can even convert it into a music file, because why not?
            assertCanRenameFile(pdfInObviouslyWrongPlace, musicFile, /* checkDatabase */ false);
            assertFileContent(musicFile, BYTES_DATA1);
        } finally {
            pdf.delete();
            pdfInObviouslyWrongPlace.delete();
            topLevelPdf.delete();
            musicFile.delete();
            deleteFileAsNoThrow(APP_B_NO_PERMS, otherAppPdf.getAbsolutePath());
        }
    }

    @Test
    public void testManageExternalStorageCannotRenameAndroid() throws Exception {
        pollForManageExternalStorageAllowed();

        final File androidDir = getAndroidDir();
        final File fooDir = new File(getAndroidDir().getAbsolutePath() + "foo");
        assertThat(androidDir.renameTo(fooDir)).isFalse();
    }

    @Test
    public void testManageExternalStorageReaddir() throws Exception {
        pollForManageExternalStorageAllowed();

        final File otherAppPdf = new File(getDownloadDir(), "other" + NONMEDIA_FILE_NAME);
        final File otherAppImg = new File(getDcimDir(), "other" + IMAGE_FILE_NAME);
        final File otherAppMusic = new File(getMusicDir(), "other" + AUDIO_FILE_NAME);
        final File otherTopLevelFile = new File(getExternalStorageDir(),
                "other" + NONMEDIA_FILE_NAME);
        try {
            assertCreateFilesAs(APP_B_NO_PERMS, otherAppImg, otherAppMusic, otherAppPdf);
            createFileAsLegacyApp(otherTopLevelFile);
            MediaStore.scanFile(getContentResolver(), otherTopLevelFile);

            // We can list other apps' files
            assertDirectoryContains(otherAppPdf.getParentFile(), otherAppPdf);
            assertDirectoryContains(otherAppImg.getParentFile(), otherAppImg);
            assertDirectoryContains(otherAppMusic.getParentFile(), otherAppMusic);
            // We can list top level files
            assertDirectoryContains(getExternalStorageDir(), otherTopLevelFile);

            // We can also list all top level directories
            assertDirectoryContains(getExternalStorageDir(), getDefaultTopLevelDirs());
        } finally {
            deleteAsLegacyApp(otherTopLevelFile);
            MediaStore.scanFile(getContentResolver(), otherTopLevelFile);
            deleteFilesAs(APP_B_NO_PERMS, otherAppImg, otherAppMusic, otherAppPdf);
        }
    }

    @Test
    public void testManageExternalStorageQueryOtherAppsFile() throws Exception {
        pollForManageExternalStorageAllowed();

        final File otherAppPdf = new File(getDownloadDir(), "other" + NONMEDIA_FILE_NAME);
        final File otherAppImg = new File(getDcimDir(), "other" + IMAGE_FILE_NAME);
        final File otherAppMusic = new File(getMusicDir(), "other" + AUDIO_FILE_NAME);
        final File otherHiddenFile = new File(getPicturesDir(), ".otherHiddenFile.jpg");
        try {
            // Apps can't query other app's pending file, hence create file and publish it.
            assertCreatePublishedFilesAs(
                    APP_B_NO_PERMS, otherAppImg, otherAppMusic, otherAppPdf, otherHiddenFile);

            assertCanQueryAndOpenFile(otherAppPdf, "rw");
            assertCanQueryAndOpenFile(otherAppImg, "rw");
            assertCanQueryAndOpenFile(otherAppMusic, "rw");
            assertCanQueryAndOpenFile(otherHiddenFile, "rw");
        } finally {
            deleteFilesAs(APP_B_NO_PERMS, otherAppImg, otherAppMusic, otherAppPdf, otherHiddenFile);
        }
    }

    /*
     * b/174211425: Test that for apps bypassing database operations we mark the nomedia directory
     * as dirty for create/rename/delete.
     */
    @Test
    public void testManageExternalStorageDoesntSkipScanningDirtyNomediaDir() throws Exception {
        pollForManageExternalStorageAllowed();

        final File nomediaDir = new File(getDownloadDir(), TEST_DIRECTORY_NAME);
        final File nomediaFile = new File(nomediaDir, ".nomedia");
        final File mediaFile = new File(nomediaDir, IMAGE_FILE_NAME);
        final File renamedMediaFile = new File(nomediaDir, "Renamed_" + IMAGE_FILE_NAME);
        try {
            if (!nomediaDir.exists()) {
                assertTrue(nomediaDir.mkdirs());
            }
            assertThat(nomediaFile.createNewFile()).isTrue();
            MediaStore.scanFile(getContentResolver(), nomediaDir);

            assertThat(mediaFile.createNewFile()).isTrue();
            MediaStore.scanFile(getContentResolver(), nomediaDir);
            assertThat(getFileRowIdFromDatabase(mediaFile)).isNotEqualTo(-1);

            assertThat(mediaFile.renameTo(renamedMediaFile)).isTrue();
            MediaStore.scanFile(getContentResolver(), nomediaDir);
            assertThat(getFileRowIdFromDatabase(renamedMediaFile)).isNotEqualTo(-1);

            assertThat(renamedMediaFile.delete()).isTrue();
            MediaStore.scanFile(getContentResolver(), nomediaDir);
            assertThat(getFileRowIdFromDatabase(renamedMediaFile)).isEqualTo(-1);
        } finally {
            nomediaFile.delete();
            mediaFile.delete();
            renamedMediaFile.delete();
            nomediaDir.delete();
        }
    }

    @Test
    public void testScanDoesntSkipDirtySubtree() throws Exception {
        pollForManageExternalStorageAllowed();

        final File nomediaDir = new File(getDownloadDir(), TEST_DIRECTORY_NAME);
        final File topLevelNomediaFile = new File(nomediaDir, ".nomedia");
        final File nomediaSubDir = new File(nomediaDir, "child_" + TEST_DIRECTORY_NAME);
        final File nomediaFileInSubDir = new File(nomediaSubDir, ".nomedia");
        final File mediaFile1InSubDir = new File(nomediaSubDir, "1_" + IMAGE_FILE_NAME);
        final File mediaFile2InSubDir = new File(nomediaSubDir, "2_" + IMAGE_FILE_NAME);
        try {
            if (!nomediaDir.exists()) {
                assertTrue(nomediaDir.mkdirs());
            }
            if (!nomediaSubDir.exists()) {
                assertTrue(nomediaSubDir.mkdirs());
            }
            assertThat(topLevelNomediaFile.createNewFile()).isTrue();
            assertThat(nomediaFileInSubDir.createNewFile()).isTrue();
            MediaStore.scanFile(getContentResolver(), nomediaDir);

            // Verify creating a new file in subdirectory sets dirty state, and scanning the top
            // level nomedia directory will not skip scanning the subdirectory.
            assertCreateFileAndScanNomediaDirDoesntNoOp(mediaFile1InSubDir, nomediaDir);

            // Verify creating a new file in subdirectory sets dirty state, and scanning the
            // subdirectory will not no-op.
            assertCreateFileAndScanNomediaDirDoesntNoOp(mediaFile2InSubDir, nomediaSubDir);
        } finally {
            nomediaFileInSubDir.delete();
            mediaFile1InSubDir.delete();
            mediaFile2InSubDir.delete();
            topLevelNomediaFile.delete();
            nomediaSubDir.delete();
            nomediaDir.delete();
            // Scan the directory to remove stale db rows.
            MediaStore.scanFile(getContentResolver(), nomediaDir);
        }
    }

    @Test
    public void testAndroidMedia() throws Exception {
        // Check that the app does not have legacy external storage access
        if (isAtLeastS()) {
            assertThat(Environment.isExternalStorageLegacy()).isFalse();
        }

        pollForPermission(Manifest.permission.READ_EXTERNAL_STORAGE, /*granted*/ true);

        final File myMediaDir = getExternalMediaDir();
        final File otherAppMediaDir = new File(myMediaDir.getAbsolutePath()
                .replace(THIS_PACKAGE_NAME, APP_B_NO_PERMS.getPackageName()));

        // Verify that accessing other app's /sdcard/Android/media behaves exactly like DCIM for
        // image files and exactly like Downloads for documents.
        assertSharedStorageAccess(otherAppMediaDir, otherAppMediaDir, APP_B_NO_PERMS);
        assertSharedStorageAccess(getDcimDir(), getDownloadDir(), APP_B_NO_PERMS);
    }

    @Test
    public void testWallpaperApisNoPermission() throws Exception {
        WallpaperManager wallpaperManager = WallpaperManager.getInstance(getContext());
        assumeTrue("Test skipped as wallpaper is not supported.",
                wallpaperManager.isWallpaperSupported());
        assertThrows(SecurityException.class, () -> wallpaperManager.getFastDrawable());
        assertThrows(SecurityException.class, () -> wallpaperManager.peekFastDrawable());
        assertThrows(SecurityException.class,
                () -> wallpaperManager.getWallpaperFile(WallpaperManager.FLAG_SYSTEM));
    }

    @Test
    public void testWallpaperApisReadExternalStorage() throws Exception {
        pollForPermission(Manifest.permission.READ_EXTERNAL_STORAGE, /*granted*/ true);
        WallpaperManager wallpaperManager = WallpaperManager.getInstance(getContext());
        wallpaperManager.getFastDrawable();
        wallpaperManager.peekFastDrawable();
        wallpaperManager.getWallpaperFile(WallpaperManager.FLAG_SYSTEM);
    }

    @Test
    public void testWallpaperApisManageExternalStorageAppOp() throws Exception {
        pollForManageExternalStorageAllowed();

        WallpaperManager wallpaperManager = WallpaperManager.getInstance(getContext());
        wallpaperManager.getFastDrawable();
        wallpaperManager.peekFastDrawable();
        wallpaperManager.getWallpaperFile(WallpaperManager.FLAG_SYSTEM);
    }

    @Test
    public void testWallpaperApisManageExternalStoragePrivileged() throws Exception {
        adoptShellPermissionIdentity(Manifest.permission.MANAGE_EXTERNAL_STORAGE);
        try {
            WallpaperManager wallpaperManager = WallpaperManager.getInstance(getContext());
            wallpaperManager.getFastDrawable();
            wallpaperManager.peekFastDrawable();
            wallpaperManager.getWallpaperFile(WallpaperManager.FLAG_SYSTEM);
        } finally {
            dropShellPermissionIdentity();
        }
    }

    /**
     * Test that File Manager can't insert files from private directories.
     */
    @Test
    public void testInsertExternalFilesViaData() throws Exception {
        verifyInsertFromExternalMediaDirViaData_allowed();
        verifyInsertFromExternalPrivateDirViaData_denied();
    }

    /**
     * Test that File Manager can't update file path to private directories.
     */
    @Test
    public void testUpdateExternalFilesViaData() throws Exception {
        verifyUpdateToExternalDirsViaData_denied();
    }

    /**
     * Test that File Manager can't insert files from private directories.
     */
    @Test
    public void testInsertExternalFilesViaRelativePath() throws Exception {
        verifyInsertFromExternalMediaDirViaRelativePath_allowed();
        verifyInsertFromExternalPrivateDirViaRelativePath_denied();
    }

    /**
     * Test that File Manager can't update file path to private directories.
     */
    @Test
    public void testUpdateExternalFilesViaRelativePath() throws Exception {
        verifyUpdateToExternalMediaDirViaRelativePath_allowed();
        verifyUpdateToExternalPrivateDirsViaRelativePath_denied();
    }

    private void assertCreateFileAndScanNomediaDirDoesntNoOp(File newFile, File scanDir)
            throws Exception {
        assertThat(newFile.createNewFile()).isTrue();
        // File is not added to database yet, but the directory is marked as dirty so that next
        // scan doesn't no-op.
        assertThat(getFileRowIdFromDatabase(newFile)).isEqualTo(-1);

        MediaStore.scanFile(getContentResolver(), scanDir);
        assertThat(getFileRowIdFromDatabase(newFile)).isNotEqualTo(-1);
    }

    /**
     * Verifies that files created by {@code otherApp} in shared locations {@code imageDir}
     * and {@code documentDir} follow the scoped storage rules. Requires the running app to hold
     * {@code READ_EXTERNAL_STORAGE}.
     */
    private void assertSharedStorageAccess(File imageDir, File documentDir, TestApp otherApp)
            throws Exception {
        final File otherAppImage = new File(imageDir, "abc.jpg");
        final File otherAppBinary = new File(documentDir, "abc.bin");
        try {
            assertCreateFilesAs(otherApp, otherAppImage, otherAppBinary);

            // We can read the other app's image
            assertFileAccess_readOnly(otherAppImage);
            assertFileContent(otherAppImage, new String().getBytes());

            // .. but not the binary file
            assertFileAccess_existsOnly(otherAppBinary);
            assertThrows(FileNotFoundException.class, () -> {
                assertFileContent(otherAppBinary, new String().getBytes());
            });
        } finally {
            deleteFileAsNoThrow(otherApp, otherAppImage.getAbsolutePath());
            deleteFileAsNoThrow(otherApp, otherAppBinary.getAbsolutePath());
        }
    }


    @Test
    public void testOpenOtherPendingFilesFromFuse() throws Exception {
        pollForPermission(Manifest.permission.READ_EXTERNAL_STORAGE, /*granted*/ true);
        final File otherPendingFile = new File(getDcimDir(), IMAGE_FILE_NAME);
        try {
            assertCreateFilesAs(APP_B_NO_PERMS, otherPendingFile);

            // We can read other app's pending file from FUSE via filePath
            assertCanQueryAndOpenFile(otherPendingFile, "r");

            // We can also read other app's pending file via MediaStore API
            assertNotNull(openWithMediaProvider(otherPendingFile, "r"));
        } finally {
            deleteFileAsNoThrow(APP_B_NO_PERMS, otherPendingFile.getAbsolutePath());
        }
    }

    @Test
    public void testNoIsolatedStorageCanCreateFilesAnywhere() throws Exception {
        if (isAtLeastS()) {
            assertThat(Environment.isExternalStorageLegacy()).isTrue();
        }
        final File topLevelPdf = new File(getExternalStorageDir(), NONMEDIA_FILE_NAME);
        final File musicFileInMovies = new File(getMoviesDir(), AUDIO_FILE_NAME);
        final File imageFileInDcim = new File(getDcimDir(), IMAGE_FILE_NAME);
        // Nothing special about this, anyone can create an image file in DCIM
        assertCanCreateFile(imageFileInDcim);
        // This is where we see the special powers of MANAGE_EXTERNAL_STORAGE, because it can
        // create a top level file
        assertCanCreateFile(topLevelPdf);
        // It can even create a music file in Pictures
        assertCanCreateFile(musicFileInMovies);
    }

    @Test
    public void testNoIsolatedStorageCantReadWriteOtherAppExternalDir() throws Exception {
        if (isAtLeastS()) {
            assertThat(Environment.isExternalStorageLegacy()).isTrue();
        }
        // Let app A create a file in its data dir
        final File otherAppExternalDataDir = new File(getExternalFilesDir().getPath().replace(
                THIS_PACKAGE_NAME, APP_A_HAS_RES.getPackageName()));
        final File otherAppExternalDataFile = new File(otherAppExternalDataDir,
                NONMEDIA_FILE_NAME);
        assertCreateFilesAs(APP_A_HAS_RES, otherAppExternalDataFile);

        // File Manager app gets global access with MANAGE_EXTERNAL_STORAGE permission, however,
        // file manager app doesn't have access to other app's external files directory
        assertThat(canOpen(otherAppExternalDataFile, /* forWrite */ false)).isFalse();
        assertThat(canOpen(otherAppExternalDataFile, /* forWrite */ true)).isFalse();
        assertThat(otherAppExternalDataFile.delete()).isFalse();

        assertThat(deleteFileAs(APP_A_HAS_RES, otherAppExternalDataFile.getPath())).isTrue();

        assertThrows(IOException.class,
                () -> {
                    otherAppExternalDataFile.createNewFile();
                });
    }

    @Test
    public void testNoIsolatedStorageStorageReaddir() throws Exception {
        if (isAtLeastS()) {
            assertThat(Environment.isExternalStorageLegacy()).isTrue();
        }
        final File otherAppPdf = new File(getDownloadDir(), "other" + NONMEDIA_FILE_NAME);
        final File otherAppImg = new File(getDcimDir(), "other" + IMAGE_FILE_NAME);
        final File otherAppMusic = new File(getMusicDir(), "other" + AUDIO_FILE_NAME);
        final File otherTopLevelFile = new File(getExternalStorageDir(),
                "other" + NONMEDIA_FILE_NAME);
        try {
            assertCreateFilesAs(APP_B_NO_PERMS, otherAppImg, otherAppMusic, otherAppPdf);
            createFileAsLegacyApp(otherTopLevelFile);

            // We can list other apps' files
            assertDirectoryContains(otherAppPdf.getParentFile(), otherAppPdf);
            assertDirectoryContains(otherAppImg.getParentFile(), otherAppImg);
            assertDirectoryContains(otherAppMusic.getParentFile(), otherAppMusic);
            // We can list top level files
            assertDirectoryContains(getExternalStorageDir(), otherTopLevelFile);

            // We can also list all top level directories
            assertDirectoryContains(getExternalStorageDir(), getDefaultTopLevelDirs());
        } finally {
            deleteAsLegacyApp(otherTopLevelFile);
            deleteFilesAs(APP_B_NO_PERMS, otherAppImg, otherAppMusic, otherAppPdf);
        }
    }

    @Test
    public void testNoIsolatedStorageQueryOtherAppsFile() throws Exception {
        if (isAtLeastS()) {
            assertThat(Environment.isExternalStorageLegacy()).isTrue();
        }
        final File otherAppPdf = new File(getDownloadDir(), "other" + NONMEDIA_FILE_NAME);
        final File otherAppImg = new File(getDcimDir(), "other" + IMAGE_FILE_NAME);
        final File otherAppMusic = new File(getMusicDir(), "other" + AUDIO_FILE_NAME);
        final File otherHiddenFile = new File(getPicturesDir(), ".otherHiddenFile.jpg");
        try {
            // Apps can't query other app's pending file, hence create file and publish it.
            assertCreatePublishedFilesAs(
                    APP_B_NO_PERMS, otherAppImg, otherAppMusic, otherAppPdf, otherHiddenFile);

            assertCanQueryAndOpenFile(otherAppPdf, "rw");
            assertCanQueryAndOpenFile(otherAppImg, "rw");
            assertCanQueryAndOpenFile(otherAppMusic, "rw");
            assertCanQueryAndOpenFile(otherHiddenFile, "rw");
        } finally {
            deleteFilesAs(APP_B_NO_PERMS, otherAppImg, otherAppMusic, otherAppPdf, otherHiddenFile);
        }
    }

    @Test
    public void testRenameFromShell() throws Exception {
        // This test is for shell and shell always runs as USER_SYSTEM
        assumeTrue("Test is applicable only for System User.", getCurrentUser() == USER_SYSTEM);
        final File imageFile = new File(getPicturesDir(), IMAGE_FILE_NAME);
        final File dir = new File(getMoviesDir(), TEST_DIRECTORY_NAME);
        final File renamedDir = new File(getMusicDir(), TEST_DIRECTORY_NAME);
        final File renamedImageFile = new File(dir, IMAGE_FILE_NAME);
        final File imageFileInRenamedDir = new File(renamedDir, IMAGE_FILE_NAME);
        try {
            assertTrue(imageFile.createNewFile());
            assertThat(getFileRowIdFromDatabase(imageFile)).isNotEqualTo(-1);
            if (!dir.exists()) {
                assertThat(dir.mkdir()).isTrue();
            }

            final String renameFileCommand = String.format("mv %s %s",
                    imageFile.getAbsolutePath(), renamedImageFile.getAbsolutePath());
            executeShellCommand(renameFileCommand);
            assertFalse(imageFile.exists());
            assertThat(getFileRowIdFromDatabase(imageFile)).isEqualTo(-1);
            assertTrue(renamedImageFile.exists());
            assertThat(getFileRowIdFromDatabase(renamedImageFile)).isNotEqualTo(-1);

            final String renameDirectoryCommand = String.format("mv %s %s",
                    dir.getAbsolutePath(), renamedDir.getAbsolutePath());
            executeShellCommand(renameDirectoryCommand);
            assertFalse(dir.exists());
            assertFalse(renamedImageFile.exists());
            assertThat(getFileRowIdFromDatabase(renamedImageFile)).isEqualTo(-1);
            assertTrue(renamedDir.exists());
            assertTrue(imageFileInRenamedDir.exists());
            assertThat(getFileRowIdFromDatabase(imageFileInRenamedDir)).isNotEqualTo(-1);
        } finally {
            imageFile.delete();
            renamedImageFile.delete();
            imageFileInRenamedDir.delete();
            dir.delete();
            renamedDir.delete();
        }
    }

    @Test
    public void testClearPackageData() throws Exception {
        // Check that the app does not have legacy external storage access
        if (isAtLeastS()) {
            assertThat(Environment.isExternalStorageLegacy()).isFalse();
        }

        pollForPermission(Manifest.permission.READ_EXTERNAL_STORAGE, /*granted*/ true);

        File fileToRemain = new File(getPicturesDir(), IMAGE_FILE_NAME);
        String testAppPackageName = APP_B_NO_PERMS.getPackageName();
        File fileToBeDeleted =
                new File(
                        getAndroidMediaDir(),
                        String.format("%s/%s", testAppPackageName, IMAGE_FILE_NAME));
        File nestedFileToBeDeleted =
                new File(
                        getAndroidMediaDir(),
                        String.format("%s/nesteddir/%s", testAppPackageName, IMAGE_FILE_NAME));

        try {
            createAndCheckFileAsApp(APP_B_NO_PERMS, fileToRemain);
            createAndCheckFileAsApp(APP_B_NO_PERMS, fileToBeDeleted);
            createAndCheckFileAsApp(APP_B_NO_PERMS, nestedFileToBeDeleted);

            executeShellCommand("pm clear --user " + getCurrentUser() + " " + testAppPackageName);

            // Wait a max of 5 seconds for the cleaning after "pm clear" command to complete.
            int i = 0;
            while(i < 10 && getFileRowIdFromDatabase(fileToBeDeleted) != -1
                && getFileRowIdFromDatabase(nestedFileToBeDeleted) != -1) {
                Thread.sleep(500);
                i++;
            }

            assertThat(getFileOwnerPackageFromDatabase(fileToRemain)).isNull();
            assertThat(getFileRowIdFromDatabase(fileToRemain)).isNotEqualTo(-1);

            assertThat(getFileOwnerPackageFromDatabase(fileToBeDeleted)).isNull();
            assertThat(getFileRowIdFromDatabase(fileToBeDeleted)).isEqualTo(-1);

            assertThat(getFileOwnerPackageFromDatabase(nestedFileToBeDeleted)).isNull();
            assertThat(getFileRowIdFromDatabase(nestedFileToBeDeleted)).isEqualTo(-1);
        } finally {
            deleteFilesAs(APP_B_NO_PERMS, fileToRemain);
            deleteFilesAs(APP_B_NO_PERMS, fileToBeDeleted);
            deleteFilesAs(APP_B_NO_PERMS, nestedFileToBeDeleted);
        }
    }

    /**
     * Tests that an instant app can't access external storage.
     */
    @Test
    @AppModeInstant
    public void testInstantAppsCantAccessExternalStorage() throws Exception {
        assumeTrue("This test requires that the test runs as an Instant app",
                getContext().getPackageManager().isInstantApp());
        assertThat(getContext().getPackageManager().isInstantApp()).isTrue();

        // Check that the app does not have legacy external storage access
        assertThat(Environment.isExternalStorageLegacy()).isFalse();

        // Can't read ExternalStorageDir
        assertThat(getExternalStorageDir().list()).isNull();

        // Can't create a top-level direcotry
        final File topLevelDir = new File(getExternalStorageDir(), TEST_DIRECTORY_NAME);
        assertThat(topLevelDir.mkdir()).isFalse();

        // Can't create file under root dir
        final File newTxtFile = new File(getExternalStorageDir(), NONMEDIA_FILE_NAME);
        assertThrows(IOException.class,
                () -> {
                    newTxtFile.createNewFile();
                });

        // Can't create music file under /MUSIC
        final File newMusicFile = new File(getMusicDir(), AUDIO_FILE_NAME);
        assertThrows(IOException.class,
                () -> {
                    newMusicFile.createNewFile();
                });

        // getExternalFilesDir() is not null
        assertThat(getExternalFilesDir()).isNotNull();

        // Can't read/write app specific dir
        assertThat(getExternalFilesDir().list()).isNull();
        assertThat(getExternalFilesDir().exists()).isFalse();
    }

    private static boolean isAtLeastS() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.S;
    }

    private void createAndCheckFileAsApp(TestApp testApp, File newFile) throws Exception {
        assertThat(createFileAs(testApp, newFile.getPath())).isTrue();
        assertThat(getFileOwnerPackageFromDatabase(newFile))
            .isEqualTo(testApp.getPackageName());
        assertThat(getFileRowIdFromDatabase(newFile)).isNotEqualTo(-1);
    }

    private static void assertCreateFilesAs(TestApp testApp, File... files) throws Exception {
        for (File file : files) {
            assertFalse("File already exists: " + file, file.exists());
            assertTrue("Failed to create file " + file + " on behalf of "
                            + testApp.getPackageName(), createFileAs(testApp, file.getPath()));
        }
    }

    /**
     * Makes {@code testApp} create {@code files}. Publishes {@code files} by scanning the file.
     * Pending files from FUSE are not visible to other apps via MediaStore APIs. We have to publish
     * the file or make the file non-pending to make the file visible to other apps.
     * <p>
     * Note that this method can only be used for scannable files.
     */
    private static void assertCreatePublishedFilesAs(TestApp testApp, File... files)
            throws Exception {
        for (File file : files) {
            assertTrue("Failed to create published file " + file + " on behalf of "
                    + testApp.getPackageName(), createFileAs(testApp, file.getPath()));
            assertNotNull("Failed to scan " + file,
                    MediaStore.scanFile(getContentResolver(), file));
        }
    }

    private static void deleteFilesAs(TestApp testApp, File... files) throws Exception {
        for (File file : files) {
            deleteFileAs(testApp, file.getPath());
        }
    }

    /**
     * For possible values of {@code mode}, look at {@link android.content.ContentProvider#openFile}
     */
    private static void assertCanQueryAndOpenFile(File file, String mode) throws IOException {
        // This call performs the query
        final Uri fileUri = getFileUri(file);
        // The query succeeds iff it didn't return null
        assertThat(fileUri).isNotNull();
        // Now we assert that we can open the file through ContentResolver
        try (final ParcelFileDescriptor pfd =
                        getContentResolver().openFileDescriptor(fileUri, mode)) {
            assertThat(pfd).isNotNull();
        }
    }

    private static void assertCanCreateFile(File file) throws IOException {
        // If the file somehow managed to survive a previous run, then the test app was uninstalled
        // and MediaProvider will remove our its ownership of the file, so it's not guaranteed that
        // we can create nor delete it.
        if (!file.exists()) {
            assertThat(file.createNewFile()).isTrue();
            assertThat(file.delete()).isTrue();
        } else {
            Log.w(TAG,
                    "Couldn't assertCanCreateFile(" + file + ") because file existed prior to "
                            + "running the test!");
        }
    }

    private static void assertFileAccess_existsOnly(File file) throws Exception {
        assertThat(file.isFile()).isTrue();
        assertAccess(file, true, false, false);
    }

    private static void assertFileAccess_readOnly(File file) throws Exception {
        assertThat(file.isFile()).isTrue();
        assertAccess(file, true, true, false);
    }

    private static void assertFileAccess_readWrite(File file) throws Exception {
        assertThat(file.isFile()).isTrue();
        assertAccess(file, true, true, true);
    }

    private static void assertDirectoryAccess(File dir, boolean exists, boolean canWrite)
            throws Exception {
        // This util does not handle app data directories.
        assumeFalse(dir.getAbsolutePath().startsWith(getAndroidDir().getAbsolutePath())
                && !dir.equals(getAndroidDir()));
        assertThat(dir.isDirectory()).isEqualTo(exists);
        // For non-app data directories, exists => canRead().
        assertAccess(dir, exists, exists, exists && canWrite);
    }

    private static void assertAccess(File file, boolean exists, boolean canRead, boolean canWrite)
            throws Exception {
        assertAccess(file, exists, canRead, canWrite, true /* checkExists */);
    }

    private static void assertCannotReadOrWrite(File file)
            throws Exception {
        // App data directories have different 'x' bits on upgrading vs new devices. Let's not
        // check 'exists', by passing checkExists=false. But assert this app cannot read or write
        // the other app's file.
        assertAccess(file, false /* value is moot */, false /* canRead */,
                false /* canWrite */, false /* checkExists */);
    }

    private static void assertCanAccessMyAppFile(File file)
            throws Exception {
        assertAccess(file, true, true /* canRead */,
                true /*canWrite */, true /* checkExists */);
    }

    private static void assertAccess(File file, boolean exists, boolean canRead, boolean canWrite,
            boolean checkExists) throws Exception {
        if (checkExists) {
            assertThat(file.exists()).isEqualTo(exists);
        }
        assertThat(file.canRead()).isEqualTo(canRead);
        assertThat(file.canWrite()).isEqualTo(canWrite);
        if (file.isDirectory()) {
            if (checkExists) {
                assertThat(file.canExecute()).isEqualTo(exists);
            }
        } else {
            assertThat(file.canExecute()).isFalse(); // Filesytem is mounted with MS_NOEXEC
        }

        // Test some combinations of mask.
        assertAccess(file, R_OK, canRead);
        assertAccess(file, W_OK, canWrite);
        assertAccess(file, R_OK | W_OK, canRead && canWrite);
        assertAccess(file, W_OK | F_OK, canWrite);

        if (checkExists) {
            assertAccess(file, F_OK, exists);
        }
    }

    private static void assertAccess(File file, int mask, boolean expected) throws Exception {
        if (expected) {
            assertThat(Os.access(file.getAbsolutePath(), mask)).isTrue();
        } else {
            assertThrows(ErrnoException.class, () -> { Os.access(file.getAbsolutePath(), mask); });
        }
    }

    /**
     * Creates a file at any location on storage (except external app data directory).
     * The owner of the file is not the caller app.
     */
    private void createFileAsLegacyApp(File file) throws Exception {
        // Use a legacy app to create this file, since it could be outside shared storage.
        Log.d(TAG, "Creating file " + file);
        assertThat(createFileAs(APP_D_LEGACY_HAS_RW, file.getAbsolutePath())).isTrue();
    }

    /**
     * Creates a file at any location on storage (except external app data directory).
     * The owner of the file is not the caller app.
     */
    private void createDirectoryAsLegacyApp(File file) throws Exception {
        // Use a legacy app to create this file, since it could be outside shared storage.
        Log.d(TAG, "Creating directory " + file);
        // Create a tmp file in the target directory, this would also create the required
        // directory, then delete the tmp file. It would leave only new directory.
        assertThat(createFileAs(APP_D_LEGACY_HAS_RW, file.getAbsolutePath() + "/tmp.txt")).isTrue();
        assertThat(deleteFileAs(APP_D_LEGACY_HAS_RW, file.getAbsolutePath() + "/tmp.txt")).isTrue();
    }

    /**
     * Deletes a file at any location on storage (except external app data directory).
     */
    private void deleteAsLegacyApp(File file) throws Exception {
        // Use a legacy app to delete this file, since it could be outside shared storage.
        Log.d(TAG, "Deleting file " + file);
        deleteFileAs(APP_D_LEGACY_HAS_RW, file.getAbsolutePath());
    }

    private int getCurrentUser() throws Exception {
        String userId = executeShellCommand("am get-current-user");
        return Integer.parseInt(userId.trim());
    }
}
