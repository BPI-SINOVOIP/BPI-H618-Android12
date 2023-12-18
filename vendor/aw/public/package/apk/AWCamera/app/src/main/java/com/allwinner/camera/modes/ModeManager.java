package com.allwinner.camera.modes;

import android.content.Context;

import com.allwinner.camera.data.Contants;
import com.allwinner.camera.ui.UIManager;

import java.util.ArrayList;
import java.util.HashMap;

import static com.allwinner.camera.data.Contants.ModeType.AutoMode;
import static com.allwinner.camera.data.Contants.ModeType.PanoMode;
import static com.allwinner.camera.data.Contants.ModeType.ProfessionMode;
import static com.allwinner.camera.data.Contants.ModeType.SquareMode;
import static com.allwinner.camera.data.Contants.ModeType.VideMode;

public class ModeManager {

    private ArrayList<String> mModeTitle = new ArrayList<>();
    private HashMap<String, Object> mModeMap = new HashMap<>();
    private Context mContext;
    private UIManager mUiManager;

    public ModeManager(Context context, UIManager manager){
        mContext = context;
        mUiManager = manager;
    }

    public BaseModeFragment getFragment(int modeindex) {
        Contants.ModeType modeType = Contants.ModeType.convertModeType(modeindex);
        String modename = mContext.getString(modeType.getName());
        if (mModeMap.containsKey(modename)){
            return (BaseModeFragment) mModeMap.get(modename);
        } else {
            BaseModeFragment fragment = createFragment(modeindex);
            fragment.setUiManager(mUiManager);
            mModeMap.put(modename,fragment);
            return fragment;
        }
    }

    public int getModeCount(){
        return mModeTitle.size();
    }

    public ArrayList getModeTitle(){
        mModeTitle.add(mContext.getString(SquareMode.getName()));
       // mModeTitle.add(mContext.getString(ProfessionMode.getName()));
        mModeTitle.add(mContext.getString(AutoMode.getName()));
        mModeTitle.add(mContext.getString(VideMode.getName()));
    //    mModeTitle.add(mContext.getString(PanoMode.getName()));

        return mModeTitle;
    }

    public BaseModeFragment createFragment(int id) {
        if (SquareMode.getId() == id) {
            return new SquareModeFragment();
        } else if (ProfessionMode.getId() == id) {
            return new ProfessionModeFragment();
        }else if (AutoMode.getId() == id) {
            return new AutoModeFragment();
        } else if (VideMode.getId() == id) {
            return new VideoModeFragment();
        }  else if (PanoMode.getId() == id) {
            return new PanoModeFragment();
        } else {
            return new AutoModeFragment();
        }
    }
}
