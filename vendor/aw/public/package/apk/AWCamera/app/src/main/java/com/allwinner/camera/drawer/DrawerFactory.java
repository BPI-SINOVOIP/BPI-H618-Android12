package com.allwinner.camera.drawer;

import android.content.Context;
import android.util.Log;

import com.allwinner.camera.utils.EGLUtils;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

public class DrawerFactory {

    public final String NORMAL = "normal";
    public final String BIGEYE = "bigeye";
    public final String LUT = "LUT";
    public final String SMOOTH = "smooth";
    private Context mContext = null;

    public DrawerFactory(Context context){
        mContext = context;
    }


    HashMap<String,BaseDrawer> mDrawerMap = new HashMap<>();

    private final String TAG = "DrawerFactory";

    private synchronized void addDrawer(String name, BaseDrawer drawer) {
        if (drawer != null) {
            Log.d(TAG, "addDrawer drawername:" + name);
            mDrawerMap.put(name, drawer);
        }
    }

    private synchronized void removeDrawer(String name) {
        Log.d(TAG, "removeDrawer drawername:" + name);
        mDrawerMap.remove(name);
    }

    public synchronized BaseDrawer getDrawer(String name) {
        return mDrawerMap.get(name);
    }

    /**
     must be call by EGLThread
     * */
    public BaseDrawer getDrawer(String name , EGLUtils.TEXTURETYPE inputType, EGLUtils.TEXTURETYPE outputType){
        BaseDrawer drawer = getDrawer(name);
        if (drawer == null ) {
            switch (name){
                case NORMAL:
                    drawer = new BaseDrawer(inputType,outputType);
                    break;
                case BIGEYE:
                    drawer = new BigEyeDrawer(inputType,outputType);
                    break;
                case LUT:
                    drawer = new LutDrawer(inputType,outputType);
                    break;
                case SMOOTH:
                   drawer = new SmoothDrawer(inputType,outputType);
                   break;
                default:
                    drawer = new BaseDrawer(inputType,outputType);
                    break;
            }
            drawer.initEGL(mContext);
            addDrawer(name,drawer);
            return drawer;
        } else {
            drawer.setType(inputType,outputType);
            return drawer;
        }
    }

    public synchronized void freeDrawerMap() {
        Log.d(TAG, " freeDrawerMap");
        for (Iterator<Map.Entry<String, BaseDrawer>> it = mDrawerMap.entrySet().iterator(); it.hasNext(); ) {
            Map.Entry<String, BaseDrawer> item = it.next();
            //... todo with item
            String key = item.getKey();
            BaseDrawer drawer = item.getValue();
            if (drawer != null) {
                drawer.release();
                drawer = null;
            }
            it.remove();
        }
    }

    public void release(){
        freeDrawerMap();
    }
}
