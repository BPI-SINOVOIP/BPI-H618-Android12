package com.softwinner.TvdFileManager;

import java.io.File;

interface IEventHandlerCallbacks {

    void playThumbVideo(String path);

    void showVideoMassage(String path);

    void releaseMediaPlayerAsync();

    void releaseMediaPlayerSync();

    void stopPlaying();

    boolean hasReleaseMedia();

    boolean returnFile(File file);

    boolean returnContentUri(String filePath);
}
