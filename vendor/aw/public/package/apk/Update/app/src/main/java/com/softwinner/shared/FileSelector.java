package com.softwinner.shared;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.softwinner.update.PropertiesHelper;
import com.softwinner.update.Configs;
import com.softwinner.update.ui.AbSelectPackage;
import com.softwinner.update.ui.AbUpdate;
import com.softwinner.update.R;

import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import android.util.Log;
public class FileSelector extends Activity implements OnItemClickListener {

    private File mCurrentDirectory;

    private LayoutInflater mInflater;

    private FileAdapter mAdapter = new FileAdapter();

    private ListView mListView;
    private RelativeLayout backLayout;

    private TextView titleTxt;


    private File[] mRoot;

    private boolean abUpdate = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mInflater = LayoutInflater.from(this);
        setContentView(R.layout.select_file_list);
        mListView = (ListView) findViewById(R.id.file_list);
        mListView.setAdapter(mAdapter);
        mListView.setOnItemClickListener(this);
        backLayout = (RelativeLayout) findViewById(R.id.back_btn);
        titleTxt = (TextView) findViewById(R.id.title_text);
        backLayout.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                onBack();
            }
        });
        requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 0);
        initRoot();
        mAdapter.setCurrentList(mRoot);

        if (getIntent() != null) {
            Bundle bundle = getIntent().getExtras();
            if (bundle != null && "true".equals(bundle.getString(Configs.ABUPDATE))) {
                abUpdate = true;
            }
        }
    }

    private void initRoot() {
        List<File> list = new ArrayList<>();
        StorageManager sm = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
        try {
            Method getStorageVolumes = StorageManager.class.getMethod("getStorageVolumes");
            List<StorageVolume> vs = (List<StorageVolume>) getStorageVolumes.invoke(sm);
            for (StorageVolume v : vs) {
                if (Environment.MEDIA_MOUNTED.equals(v.getState())) {
                    File f = getPathFile(v);
                    if (f != null)
                        list.add(f);
                }
            }
        } catch (NoSuchMethodException | InvocationTargetException | IllegalAccessException e) {
            e.printStackTrace();
        }
        if (list.isEmpty())
            list.add(Environment.getExternalStorageDirectory());
        mRoot = list.toArray(new File[0]);
    }

    private File getPathFile(StorageVolume volume) {
        try {
            Method getPathFile = StorageVolume.class.getMethod("getPathFile");
            return (File) getPathFile.invoke(volume);
        } catch (NoSuchMethodException | InvocationTargetException | IllegalAccessException e) {
            e.printStackTrace();
        }
        return null;
    }

    private boolean isRoot(File file) {
        for (File f : mRoot) {
            if (f.compareTo(file) == 0)
                return true;
        }
        return false;
    }

    private void onBack() {
        if (mCurrentDirectory == null) {
            super.onBackPressed();
        } else if (isRoot(mCurrentDirectory)) {
            mCurrentDirectory = null;
            mAdapter.setCurrentList(mRoot);
            titleTxt.setText(R.string.select_file_title);
        } else {
            mCurrentDirectory = mCurrentDirectory.getParentFile();
            mAdapter.setCurrentList(mCurrentDirectory.listFiles());
            titleTxt.setText(mCurrentDirectory.getPath());
        }
    }

    @Override
    public void onBackPressed() {
        onBack();
    }

    @Override
    public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
        File selectFile = (File) adapterView.getItemAtPosition(position);
        if (selectFile.isDirectory()) {
            mCurrentDirectory = selectFile;
            FileAdapter adapter = (FileAdapter) adapterView.getAdapter();
            adapter.setCurrentList(selectFile.listFiles());
            titleTxt.setText(selectFile.getPath());
        } else if (selectFile.isFile()) {
            if (!abUpdate) {
                Intent intent = new Intent(FileSelector.this, LocalVerifyActivity.class);
                intent.putExtra(Configs.UPDATEPATH, selectFile.getPath());
                FileSelector.this.startActivity(intent);
            } else {
                Intent intent = new Intent();
                intent.putExtra(AbSelectPackage.FILEPATH, selectFile.getPath());
                setResult(RESULT_OK, intent);
                finish();
            }
        }
    }

    private class FileAdapter extends BaseAdapter {
        private String[] imgTypes = {"jpg", "png", "bmp"};
        private String[] videoTypes = {"mp4", "avi", "3gp", "mkv", "rmvb", "wmv"};
        private String musicType = "mp3";
        private String zipType = "zip";
        private File mFiles[];

        public void setCurrentList(File[] directories) {
            mFiles = directories;
            notifyDataSetChanged();
        }

        @Override
        public int getCount() {
            return mFiles == null ? 0 : mFiles.length;
        }

        @Override
        public File getItem(int position) {
            File file = mFiles == null ? null : mFiles[position];
            return file;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.file_item, null);
            }
            final ImageView fileImg, nextImg;
            final TextView fileName;
            fileImg = (ImageView) convertView.findViewById(R.id.file_type_img);
            nextImg = (ImageView) convertView.findViewById(R.id.next_img);
            nextImg.setVisibility(View.INVISIBLE);
            fileName = (TextView) convertView.findViewById(R.id.file_name);
            File file = mFiles[position];
            String name = file.getName();
            fileName.setText(name);
            if (file.isDirectory()) {
                fileImg.setBackgroundResource(R.drawable.ic_folder);
            } else {
                boolean isMatch = false;
                int i = 0;
                for (i = 0; i < imgTypes.length && !isMatch; i++) {
                    if (file.getName().endsWith(imgTypes[i])) {
                        fileImg.setBackgroundResource(R.drawable.ic_picture);
                        isMatch = true;
                        break;
                    }
                }
                for (i = 0; i < videoTypes.length && !isMatch; i++) {
                    if (file.getName().endsWith(videoTypes[i])) {
                        fileImg.setBackgroundResource(R.drawable.ic_viedo);
                        isMatch = true;
                        break;
                    }
                }
                if (!isMatch && file.getName().endsWith(musicType)) {
                    fileImg.setBackgroundResource(R.drawable.ic_music);
                    isMatch = true;
                }
                if (!isMatch && file.getName().endsWith(zipType)) {
                    fileImg.setBackgroundResource(R.drawable.ic_compressed_file);
                    isMatch = true;
                    nextImg.setVisibility(View.VISIBLE);
                }
                if (!isMatch) {
                    fileImg.setBackgroundResource(R.drawable.ic_file);
                }
            }
            return convertView;
        }

    }
}
