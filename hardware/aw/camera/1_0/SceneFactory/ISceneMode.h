#ifndef __I_SCENE_MODE__
#define __I_SCENE_MODE__

namespace android {

// type of scene notify callback
typedef int (*SceneNotifyCb)(int cmd, void* data, int* ret,void* user);

enum scene_mode {
    SCENE_FACTORY_MODE_NONE            = 0,
    //SCENE_FACTORY_MODE_BACKLIGHT        = 1,
    //SCENE_FACTORY_MODE_BEACH_SNOW        = 2,
    //SCENE_FACTORY_MODE_CANDLE_LIGHT    = 3,
    //SCENE_FACTORY_MODE_DAWN_DUSK        = 4,
    //SCENE_FACTORY_MODE_FALL_COLORS    = 5,
    //SCENE_FACTORY_MODE_FIREWORKS        = 6,
    //SCENE_FACTORY_MODE_LANDSCAPE        = 7,
    SCENE_FACTORY_MODE_NIGHT        = 8,    //now we support the mode (8)
    //SCENE_FACTORY_MODE_PARTY_INDOOR    = 9,
    //SCENE_FACTORY_MODE_PORTRAIT        = 10,
    //SCENE_FACTORY_MODE_SPORTS        = 11,
    //CENE_FACTORY_MODE_SUNSET        = 12,
    //SCENE_FACTORY_MODE_TEXT        = 13,
    SCENE_FACTORY_MODE_AUTO            = 20,    //now we support the mode
    SCENE_FACTORY_MODE_HDR            = 21,    //now we support the mode (21)
};
enum scene_state {
    SCENE_CAPTURE_UNKNOW = 100,
    SCENE_CAPTURE_DONE = 101,
    SCENE_CAPTURE_FAIL = 102,
    SCENE_COMPUTE_DONE = 103,
    SCENE_COMPUTE_FAIL = 104,
};
enum SenceNotifyCMD{
    SCENE_NOTIFY_CMD_GET_AE_STATE,
    SCENE_NOTIFY_CMD_GET_HIST_STATE,
    SCENE_NOTIFY_CMD_SET_3A_LOCK,
    SCENE_NOTIFY_CMD_SET_HDR_SETTING,
    SCENE_NOTIFY_CMD_GET_HDR_FRAME_COUNT,
    //SCENE_NOTIFY_CMD_,
    //SCENE_NOTIFY_CMD_,
    //SCENE_NOTIFY_CMD_,
};

/*
** SceneMode public interface.
** All scene mode must extern it,and realize the virtual interfaces
** The user(client) just care and use the public virtual interfaces
*/
class ISceneMode {
public:
    ISceneMode():    \
        mSceneMode(SCENE_FACTORY_MODE_AUTO),    \
        mWidth(0),mHeight(0),    \
        mSceneNotifyCb(NULL),
        mUser(NULL){};    //do nothing
    virtual ~ISceneMode(){};

public:
    /* Do some software init,must be called after the SceneMode
    ** constured, eg: alloc buffer.... Can't do some operation
    ** about hardware here, eg: get AE state,set 3A lock ...
    ** width: capture frame width.
    ** height: capture frame height.
    */
    virtual int    InitSceneMode(int width,int height) = 0;

    /*TODO: It seems to need a hardhare init interface????(or not)~~*/

    /* Set notify callback,user(client) must realize the callback function and set it,
    ** SceneMode can get some capture information through this callback.
    ** scenenotifycb: callback function handle.
    ** user:          the user(client) this point.
    */
    virtual void    SetCallBack(SceneNotifyCb scenenotifycb,void* user) = 0;

    /* Start take sence picture,it must be called once when take sence picture
    ** shutter down, do some hardare or software setting,witch is differnt every time.
    */
    virtual int    StartScenePicture() = 0;

    /* It must be called when take the scene picture end. */
    virtual int    StopScenePicture() = 0;

    /* Get current frame source data, it must be called per frame in capture thread,
    ** SceneMode must distinguish the frame status weather the frame data needed or not.
    ** vaddr: capture frame data virtual addr
    ** return:when SenceMode get enough frame return SCENE_CAPTURE_DONE(101)
    */
    virtual int    GetCurrentFrameData(void* vaddr) = 0;

    /* Start to compute the scene picture,
    ** picture computed completly,copy the target scene picturn into vaddr,return SCENE_COMPUTE_DONE.
    ** picture computed uncompletly, do nothing about the vaddr,return SCENE_COMPUTE_FAIL.
    */
    virtual int    PostScenePicture(void* vaddr) = 0;

    /*Release*/
    virtual void    ReleaseSceneMode() = 0;

    /*Return current SceneMode state*/
    virtual int    GetScenePictureState() = 0;
    /*return current SceneMode mode*/
    virtual int    GetCurrentSceneMode() = 0;

protected:
    int mSceneMode;
    int mWidth;
    int mHeight;
    SceneNotifyCb mSceneNotifyCb;
    void* mUser;
};

};
#endif