package com.softwinner.tvfilemanager.presenter;

import android.content.Context;
import android.support.v17.leanback.widget.ImageCardView;
import android.support.v17.leanback.widget.Presenter;
import android.util.Log;
import android.view.ViewGroup;

import com.softwinner.tvfilemanager.ImageLoadManager;
import com.softwinner.tvfilemanager.R;
import com.softwinner.tvfilemanager.common.Constants;
import com.softwinner.tvfilemanager.entity.LocalVideo;

/**
 * Created by ningxiangzhong on 15-2-28.
 */
public class VideoCardPresenter extends Presenter {

    private final static String TAG = "VideoCardPresenter";
    private Context mContext;
    private ImageLoadManager imageLoadManager;

    @Override
    public ViewHolder onCreateViewHolder(ViewGroup viewGroup) {
//        Log.d(TAG, "onCreateViewHolder");
        mContext = viewGroup.getContext();
        imageLoadManager = ImageLoadManager.getInstance(mContext);

        ImageCardView cardView = new ImageCardView(mContext) {
            @Override
            public void setSelected(boolean selected) {
                int selectBackground = getResources().getColor(R.color.detail_background);
                int defaultBackground = getResources().getColor(R.color.default_background);
                int color = selected ? selectBackground : defaultBackground;
                findViewById(R.id.info_field).setBackgroundColor(color);
                super.setSelected(selected);
            }
        };

        cardView.setFocusable(true);
        cardView.setFocusableInTouchMode(true);
        cardView.setBackgroundColor(mContext.getResources().getColor(R.color.fastlane_background));
        return new com.softwinner.tvfilemanager.presenter.ViewHolder(cardView);
    }

    @Override
    public void onBindViewHolder(ViewHolder viewHolder, Object o) {
//        Log.d(TAG, "onBindViewHolder");

        LocalVideo video = (LocalVideo) o;

        ((com.softwinner.tvfilemanager.presenter.ViewHolder) viewHolder).setFile(video);

        ((com.softwinner.tvfilemanager.presenter.ViewHolder) viewHolder)
                .getCardView().setTitleText(video.getFile_name());

        ((com.softwinner.tvfilemanager.presenter.ViewHolder) viewHolder)
                .getCardView().setMainImageDimensions(Constants.CARD_WIDTH, Constants.CARD_HEIGHT);

        imageLoadManager.setImageView(ImageLoadManager.IMAGE_LOAD_TYPE.FILE_PATH,
                video.getFile_path(),
                ((com.softwinner.tvfilemanager.presenter.ViewHolder) viewHolder).getCardView());

    }

    @Override
    public void onUnbindViewHolder(ViewHolder viewHolder) {
        Log.d(TAG, "onUnbindViewHolder");
        //Remove references to images so that the garbage collector can free up memory;
        ((com.softwinner.tvfilemanager.presenter.ViewHolder) viewHolder).getCardView().setBadgeImage(null);
        ((com.softwinner.tvfilemanager.presenter.ViewHolder) viewHolder).getCardView().setMainImage(null);
    }
}
