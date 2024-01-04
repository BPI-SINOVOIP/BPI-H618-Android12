package com.softwinner.dragonbox.testcase;

import org.xmlpull.v1.XmlPullParser;

import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.media.AudioManager;
import android.widget.TextView;

import com.softwinner.dragonbox.utils.AudioChannelUtil;
import com.softwinner.dragonbox.utils.MediaPlayerUtil;
import com.softwinner.dragonbox.R;
import com.softwinner.dragonbox.utils.MusicerUtil;
import com.softwinner.dragonbox.platform.AudioManagerProxy;

public class CaseSpeaker extends IBaseCase {

    TextView mSpeakerStatus;
    AudioManager audioManager;
    private Context mContext;
    private MediaPlayerUtil mMediaPlayerUtil;

    public CaseSpeaker(Context context) {
        super(context, R.string.case_speaker_name, R.layout.case_speaker_max,
                R.layout.case_speaker_min, TYPE_MODE_MANUAL);
        mSpeakerStatus = (TextView) mMinView.findViewById(R.id.case_speaker_status);
        mMediaPlayerUtil = MediaPlayerUtil.getInstance(context);
        audioManager = ((AudioManager)context.getSystemService(Context.AUDIO_SERVICE));
        mContext = context;
    }

    public CaseSpeaker(Context context, XmlPullParser xmlParser) {
        this(context);
    }

    @Override
    public void onStartCase() {
        //audioManager.setMode(AudioManager.MODE_RINGTONE);
        AudioChannelUtil.setOuputChannels(mContext, true ,AudioManagerProxy.AUDIO_NAME_CODEC);
        mMediaPlayerUtil.playMusic(MediaPlayerUtil.PLAY_SPEAKER);
    }

    @Override
    public void onStopCase() {
        mMediaPlayerUtil.stop();
        mMediaPlayerUtil.release();
        //audioManager.setMode(AudioManager.MODE_NORMAL);
        AudioChannelUtil.setOuputChannels(mContext, false ,"AUDIO_SPEAKER");
        mSpeakerStatus.setText(getCaseResult() ? R.string.case_speaker_status_success_text
                        : R.string.case_speaker_status_fail_text);
    }

    @Override
    public void reset() {
        super.reset();
        mSpeakerStatus.setText(R.string.case_speaker_status_text);
    }

}
