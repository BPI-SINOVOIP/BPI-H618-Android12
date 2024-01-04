package com.softwinner.tvfilemanager;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.v17.leanback.widget.ArrayObjectAdapter;
import android.support.v17.leanback.widget.OnItemViewClickedListener;
import android.support.v17.leanback.widget.OnItemViewSelectedListener;
import android.support.v17.leanback.widget.Presenter;
import android.support.v17.leanback.widget.Row;
import android.support.v17.leanback.widget.RowPresenter;
import android.support.v17.leanback.widget.VerticalGridPresenter;
import android.util.Log;

import com.softwinner.tvfilemanager.entity.LocalPicture;
import com.softwinner.tvfilemanager.presenter.PictureCardPresenter;

import java.io.File;
import java.util.List;

/**
 * Created by ningxiangzhong on 15-3-9.
 */
public class PictureGridFragment extends android.support.v17.leanback.app.VerticalGridFragment{

    private static final String TAG = "PictureGridFragment";

    private static final int NUM_COLUMNS = 5;

    private ArrayObjectAdapter mAdapter;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate");
        super.onCreate(savedInstanceState);

        setTitle(getString(R.string.picture_grid_title));

        setupFragment();
    }

    private void setupFragment() {
        VerticalGridPresenter gridPresenter = new VerticalGridPresenter();
        gridPresenter.setNumberOfColumns(NUM_COLUMNS);
        setGridPresenter(gridPresenter);

        mAdapter = new ArrayObjectAdapter(new PictureCardPresenter());

        long seed = System.nanoTime();

        List<LocalPicture> pictures = MyApplication.mLocalPicture;

        synchronized (MyApplication.mLocalPicture) {
            for (LocalPicture picture : pictures) {
                mAdapter.add(picture);
            }
        }
        setAdapter(mAdapter);

      /*  setOnSearchClickedListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(getActivity(), SearchActivity.class);
                startActivity(intent);
            }
        });*/

        setOnItemViewClickedListener(new ItemViewClickedListener());
        setOnItemViewSelectedListener(new ItemViewSelectedListener());
    }

    private final class ItemViewClickedListener implements OnItemViewClickedListener {
        @Override
        public void onItemClicked(Presenter.ViewHolder itemViewHolder, Object item,
                                  RowPresenter.ViewHolder rowViewHolder, Row row) {

            if (item instanceof LocalPicture) {
                LocalPicture picture = (LocalPicture) item;
                Log.d(TAG, "Item: " + item.toString());
                File file = new File(picture.getFile_path());
                Intent picIntent = new Intent();
                picIntent.setAction(android.content.Intent.ACTION_VIEW);
                picIntent.setDataAndType(Uri.fromFile(file), "image/*");
                try {
                    getActivity().startActivity(picIntent);
                } catch (ActivityNotFoundException e) {
                    e.printStackTrace();
                }
            }
        }
    }


    private final class ItemViewSelectedListener implements OnItemViewSelectedListener {
        @Override
        public void onItemSelected(Presenter.ViewHolder itemViewHolder, Object item,
                                   RowPresenter.ViewHolder rowViewHolder, Row row) {
        }
    }

}
