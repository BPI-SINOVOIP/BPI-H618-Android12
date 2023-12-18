package com.softwinner.tvfilemanager.presenter;

import android.support.v17.leanback.widget.ImageCardView;
import android.support.v17.leanback.widget.Presenter;
import android.view.View;

import com.softwinner.tvfilemanager.entity.IFile;

/**
 * Created by ningxiangzhong on 15-2-28.
 */
public class ViewHolder extends Presenter.ViewHolder {
    private IFile file;
    private ImageCardView mCardView;

    public ViewHolder (View view) {
        super(view);
        mCardView = (ImageCardView) view;
    }

    public void setFile(IFile file) {
        this.file = file;
    }

    public IFile getFile() {
        return file;
    }

    public ImageCardView getCardView() {
        return mCardView;
    }
}
