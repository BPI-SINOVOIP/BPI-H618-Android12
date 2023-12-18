/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : awmetadataretriever.h
 * Description : metadataretriever
 * History :
 *
 */

#ifndef AW_METADATA_RETRIEVER
#define AW_METADATA_RETRIEVER

#include <utils/String8.h>
#include <media/MediaMetadataRetrieverInterface.h>
#include <media/stagefright/MetaData.h>
#include <utils/KeyedVector.h>
#include <media/MediaPlayerInterface.h>

using namespace android;
#if (CONF_ANDROID_MAJOR_VER > 8)
struct AwMetadataRetriever : public MediaMetadataRetrieverBase
#else
struct AwMetadataRetriever : public MediaMetadataRetrieverInterface
#endif
{
    AwMetadataRetriever();
    virtual ~AwMetadataRetriever();

#if (CONF_ANDROID_MAJOR_VER >= 5)
    virtual status_t setDataSource(const sp<IMediaHTTPService> &httpService,
                const char* pUrl, const KeyedVector<String8, String8>* pHeaders);
#else
    virtual status_t setDataSource(const char* pUrl,
                const KeyedVector<String8, String8>* pHeaders);
#endif
    virtual status_t setDataSource(int fd, int64_t offset, int64_t length);

#if (((CONF_ANDROID_MAJOR_VER >= 6 && CONF_ANDROID_MAJOR_VER < 8)) || \
    ((CONF_ANDROID_MAJOR_VER == 8) && (CONF_ANDROID_SUB_VER == 0)))
    virtual status_t setDataSource(const sp<DataSource>& source);

#else
    virtual status_t setDataSource(const sp<DataSource>& source, const char *mime);
#endif

    /* option enumerate is defined in
     * 'android/framework/base/media/java/android/media/MediaMetadataRetriver.java'
     * option == 0: OPTION_PREVIOUS_SYNC
     * option == 1: OPTION_NEXT_SYNC
     * option == 2: OPTION_CLOSEST_SYNC
     * option == 3: OPTION_CLOSEST
     */
#if ((CONF_ANDROID_MAJOR_VER < 8) || \
        ((CONF_ANDROID_MAJOR_VER == 8) && (CONF_ANDROID_SUB_VER == 0)))
    virtual VideoFrame* getFrameAtTime(int64_t timeUs, int option);
#elif ((CONF_ANDROID_MAJOR_VER == 8) && (CONF_ANDROID_SUB_VER > 0))
    // android 8.1
    virtual VideoFrame* getFrameAtTime(int64_t timeUs, int option, int colorFormat, bool metaOnly);
#elif (CONF_ANDROID_MAJOR_VER == 9 || CONF_ANDROID_MAJOR_VER == 10)
    // android 9
    virtual sp<IMemory> getFrameAtTime(
            int64_t timeUs, int option, int colorFormat, bool metaOnly) override;
    virtual sp<IMemory> getImageAtIndex(
            int index, int colorFormat, bool metaOnly, bool thumbnail) override;
    virtual sp<IMemory> getImageRectAtIndex(
            int index, int colorFormat, int left, int top, int right, int bottom) override;
    virtual status_t getFrameAtIndex(
            std::vector<sp<IMemory> >* frames,
            int frameIndex, int numFrames, int colorFormat, bool metaOnly) override;
#else
    //android 11
    virtual sp<IMemory> getFrameAtTime(
            int64_t timeUs, int option, int colorFormat, bool metaOnly) override;
    virtual sp<IMemory> getImageAtIndex(
            int index, int colorFormat, bool metaOnly, bool thumbnail) override;
    virtual sp<IMemory> getImageRectAtIndex(
            int index, int colorFormat, int left, int top, int right, int bottom) override;
    virtual sp<IMemory> getFrameAtIndex(
            int frameIndex, int colorFormat, bool metaOnly) override;
#endif

    virtual MediaAlbumArt* extractAlbumArt();
    virtual const char* extractMetadata(int keyCode);
    virtual sp<IMemory> getStreamAtTime(int64_t timeUs);

private:
    struct MetadataPriData *mPriData;

private:
    void clear();
    void storeMetadata();
    AwMetadataRetriever(const AwMetadataRetriever &);
    AwMetadataRetriever &operator=(const AwMetadataRetriever &);
};

#endif
