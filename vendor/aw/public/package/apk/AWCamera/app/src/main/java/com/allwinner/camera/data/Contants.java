package com.allwinner.camera.data;

import android.net.Uri;

import com.allwinner.camera.R;

public class Contants {
    public static final String PICTURE_SCALE_DEFAULT_VALUE = "4:3";
    public static final String VIDEO_SCALE_DEFAULT_VALUE = "480P (4:3)";
    public static final String ISBEAUTY = "isBeauty";
    public static final String STICKER = "sticker";
    public static final String KEY_SOUND_CAPTURE = "sound_capture";
    public static final String KEY_SAVE_POSITION = "save_position";
    public static final String KEY_TIME_WATERSIGN = "time_watersign";
    public static final String KEY_BACK_PICTURE_SCALE = "back_picture_scale";
    public static final String KEY_MODEL_WATERSIGN = "model_watersign";
    public static final String KEY_FRONT_PICTURE_SCALE = "front_picture_scale";
    public static final String MODEL_WATERSIGN_DEFAULT_VALUE = "off";
    public static final String STICKER_ROOT_FOLDER = "sticker";
    public static final String KEY_SMOOTH_PROGRESS = "smooth_progress";
    public static final String KEY_WHITE_PROGRESS = "white_progress";
    public static final String KEY_BIGEYE_PROGRESS = "bigeye_progress";
    public static final String KEY_FACELIFT_PROGRESS = "facelift_progress";
    public static final String KEY_HAS_SEEN_PERMISSIONS_DIALOGS ="has_seen_permissions_dialogs" ;
    public static final int FACE_DATA_WIGHT = 45;
    public static final int FACE_DATA_HEIGHT = 80;
    public static final int EYELEFTPOINT = 104 * 2;
    public static final int EYERIGHTPOINT = 105 * 2;
    public static final int FACELEFT1POINT = 8 * 2;
    public static final int FACELEFT2POINT = 11 * 2;
    public static final int FACELEFT3POINT = 15 * 2;
    public static final int FACERIGHT1POINT = 25 * 2;
    public static final int FACERIGHT2POINT = 22 * 2;
    public static final int FACERIGHT3POINT = 17 * 2;
    public static final int FACESTARTINDEX = 4 * 2;
    public static final int FACEENDINDEX = 28 * 2;


    public static final int AUTOMODEINDEX = 1;
    public static final int VIDEOMODEINDEX = 2;

    public static float MAXFORWARDDISTANCE = 2.7f;
    public static float MAXREVERSEDISTANCE = 0.4f;
    public static final int[] TIMEIMAGEARRAY = {R.mipmap.ic_timer_off_normal, R.mipmap.ic_timer_3s_normal, R.mipmap.ic_timer_10s_normal};
    public static final int[] TIMEARRAY={0,3,10};
    public static final float SMOOTH_START =0.99f;
    public static final float SMOOTH_END =0.333f;
    public static final float WHITE_START =0.1f;
    public static final float WHITE_END =0.9f;
    public static final float BIGEYE_START =0.4f;
    public static final float BIGEYE_END =0.5f;
    public static final float FACELIFT_START =0.08f;
    public static final float FACELIFT_END =0.15f;
    public static final long UNAVAILABLE = -1L;
    public static final long PREPARING = -2L;
    public static final long UNKNOWN_SIZE = -3L;
    public static final long ACCESS_FAILURE = -4L;
    public static final long LOW_STORAGE_THRESHOLD_BYTES = 500000000;
    public enum ModeType {
        SquareMode(0,R.string.squareMode),
        ProfessionMode(11,R.string.professionMode),
        AutoMode(1, R.string.autoMode),
        VideMode(2,R.string.videoMode),
        PanoMode(41, R.string.panoMode);
        private int mId;
        private int mModeName;

        ModeType(int id , int name){
            mId = id;
            mModeName = name;
        }

        public int getId(){
            return mId;
        }

        public int getName(){
            return mModeName;
        }

        public static ModeType convertModeType(int id) {
            if (SquareMode.getId() == id) {
                return SquareMode;
            } else if (ProfessionMode.getId() == id) {
                return ProfessionMode;
            }  else if (AutoMode.getId() == id) {
                return AutoMode;
            } else if (VideMode.getId() == id) {
                return VideMode;
            }  else if (PanoMode.getId() == id) {
                return PanoMode;
            } else {
                return AutoMode;
            }
        }
    }
    public interface MsgType {
        int MSG_ON_CAMERAOPEN = 0;
        int MSG_ON_PREVIEWSTART_FINISH = 1;
        int MSG_ON_PICTURETAKEN_FINISH = 2;
        int MSG_ON_VIDEO_SAVE = 3;
        int MSG_ON_UPDATE_THUMBNAIL = 4;
        int MSG_ON_CAPTURE_COMPLETE = 5;
        int MSG_ON_PREVIEWSTOP = 6;
        int MSG_ON_UPDATE_BYTE = 7;
        int MSG_ON_UPDATE_ZOOMTEXT = 8;
        int MSG_ON_UPDATE_ZOOMVIEW = 9;
        int MSG_ON_UPDATE_BITMAP =10;
        int MSG_ON_SHOWCOVER =11;
    }

    public interface CameraCommand {
        int OPENCAMERA = 0;
        int CLOSECAMERA = 1;
        int STARTPREVIEW = 2;
        int STOPPREVIEW = 3;
        int TAKEPICTURE = 4;
        int UPDATESIZE = 5;
    }
    public interface ModelWatersignType {
        String TYPE_OFF = "off";
        String TYPE_DEFAULT = "default";
        String TYPE_CUSTOM ="custom";
    }
    public interface  CameraRatio {
        int RATIO_4_TO_3 = 0;
        int RATIO_16_TO_9 = 1;
        int RATIO_18_TO_9 = 2;
        int RATIO_1_TO_1 = 3;
    }
    public interface  PictureRatioStr {
        String PICTURE_RATIO_STR_4_TO_3 = "4:3";
        String PICTURE_RATIO_STR_16_TO_9 = "16:9";
        String PICTURE_RATIO_STR_18_TO_9 = "18:9";
        String PICTURE_RATIO_STR_1_TO_1 = "1:1";
    }
    public interface  VideoRatioStr {
        String VIDEO_RATIO_STR_480P = "480P (4:3)";
        String VIDEO_RATIO_STR_720P = "720P (16:9)";
        String VIDEO_RATIO_STR_1080P = "1080P (16:9)";
        String VIDEO_RATIO_STR_4K = "4K (16:9)";
    }

    public interface CameraStatus {
        /**
         * Camera state: Showing camera preview.
         */
        int STATE_PREVIEW = 0;

        /**
         * Camera state: Waiting for the focus to be locked.
         */
        int STATE_WAITING_LOCK = 1;

        /**
         * Camera state: Waiting for the exposure to be precapture state.
         */
        int STATE_WAITING_PRECAPTURE = 2;

        /**
         * Camera state: Waiting for the exposure state to be something other than precapture.
         */
        int STATE_WAITING_NON_PRECAPTURE = 3;

        /**
         * Camera state: Picture was taken.
         */
        int STATE_PICTURE_TAKEN = 4;
    }

    public interface FlashMode {
        int FLASH_OFF = 0;
        int FLASH_ON = 1;
        int FLASH_AUTO = 2;
        int FLASH_TORCH = 3;
    }
    public static final int[] FLASHIMAGEARRAY = {R.mipmap.ic_flash_off_normal, R.mipmap.ic_flash_on_normal, R.mipmap.ic_flash_auto_normal};
    public static final int[] FLASHARRAY = {FlashMode.FLASH_OFF, FlashMode.FLASH_ON, FlashMode.FLASH_AUTO};
    public static class EventObject {
        public int mMsgType;
        public Object mMsgContent;

        public EventObject(int msgType, Object msg) {
            mMsgType = msgType;
            mMsgContent = msg;
        }
    }
    public static class MosaicJpeg {
        public MosaicJpeg(byte[] data, int width, int height) {
            this.data = data;
            this.width = width;
            this.height = height;
            this.isValid = true;
        }

        public MosaicJpeg() {
            this.data = null;
            this.width = 0;
            this.height = 0;
            this.isValid = false;
        }

        public final byte[] data;
        public final int width;
        public final int height;
        public final boolean isValid;
    }
    public interface VideoQuality {
        int Q480P = 0;
        int Q720P  = 1;
        int Q1080P = 2;
        int Q4K = 3;
    }

    public static class MediaData {
        public MediaData(long id, int orientation, long dateTake, Uri uri, boolean isPicture,
                     String path) {
            this.id = id;
            this.orientation = orientation;
            this.dateTaken = dateTake;
            this.uri = uri;
            this.isPicture = isPicture;
            this.path = path;
        }

        public final long id;
        public final int orientation;
        public final long dateTaken;
        public final Uri uri;
        public final boolean isPicture;
        public final String path;

    }

    public enum FilterType {
        Normal(0,R.string.filter_normal,"normal",0),
        Autumn(R.mipmap.autumn,R.string.filter_autumn,"lut",1),
        Blue(R.mipmap.blue,R.string.filter_blue,"lut",2),
        Film(R.mipmap.film,R.string.filter_film,"lut",3),
        Bleak(R.mipmap.bleak,R.string.filter_bleak,"lut",4),
        Green(R.mipmap.green,R.string.filter_green,"lut",5),
        GreenOrange(R.mipmap.greenorange,R.string.filter_green_orange,"lut",6),
        Amber(R.mipmap.amber,R.string.filter_amber,"lut",7),
        Sunset(R.mipmap.sunset,R.string.filter_sunset,"lut",8),
        White(R.mipmap.white,R.string.filter_white,"lut",9),
        Beauty(0,R.string.filter_beauty,"fb",10),
        YUV(0,R.string.filter_beauty,"yuv",11);
       // Beauty(0,"beauty",false,10);



        private int mIndex;
        private int mResoureId;
        private int mFilterName;
        private String mType;

        FilterType(int id, int name, String type,int index) {
            mResoureId = id;
            mFilterName = name;
            mType = type;
            mIndex = index;
        }
        public int getIndex() {
            return mIndex;
        }
        public int getId(){
            return mResoureId;
        }

        public int getName(){
            return mFilterName;
        }

        public boolean isLut() {
            return mType.equals("lut");
        }

        public String getType(){
            return mType;
        }

        public static FilterType convertFilterType(int id) {
            if (Normal.getIndex() == id) {
//                if(CameraData.getInstance().getCurrentModeType().equals(ModeType.AutoMode)){
//                    return YUV;
//                }
                return Normal;
            } else if (Autumn.getIndex() == id) {
               return Autumn;
            } else if (Blue.getIndex() == id) {
                return Blue;
            } else if (Film.getIndex() == id) {
                return Film;
            } else if (Bleak.getIndex() == id) {
                return Bleak;
            } else if (Green.getIndex() == id) {
                return Green;
            } else if (GreenOrange.getIndex() == id) {
                return GreenOrange;
            } else if (Amber.getIndex() == id) {
                return Amber;
            } else if (Sunset.getIndex() == id) {
                return Sunset;
            } else if (White.getIndex() == id) {
                return White;
            } else if (Beauty.getIndex() == id) {
                return Beauty;
            }else {
                return Normal;
            }
        }
    }
}
