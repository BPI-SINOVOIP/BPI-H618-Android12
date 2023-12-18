package com.softwinner.shared;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.os.Handler;
import android.os.Message;
import android.os.Build;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.os.image.DynamicSystemClient;
import android.net.Uri;
import com.softwinner.update.PropertiesHelper;
import com.softwinner.update.R;
import java.io.File;
import android.support.v4.content.FileProvider;

import java.io.FileInputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.GZIPInputStream;

import static com.softwinner.shared.InstallPathDialog.*;

public class ImageSearch extends Activity implements OnItemClickListener, DynamicSystemClient.OnStatusChangedListener {

    private ListView mListView;
    private RelativeLayout backLayout;
    private LayoutInflater mInflater;
    private FileAdapter mAdapter = new FileAdapter();
    private TextView scanBtn;
    private File mFiles[];
    private TextView mTextView;
    private Uri uri;
    private int imageSize;
    private static final int READ_START = 0;
    private static final int READ_COMPLETE = 1;
    private DynamicSystemClient dSClient;


    public void onStatusChanged(int status, int cause, long progress, Throwable detail) {
    }

    private Handler handler = new Handler() {
        private int process = 0;

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case READ_START:
                    mTextView.setVisibility(View.VISIBLE);
                    mTextView.setText(getString(R.string.read_hint));//在主线程中更新UI界面
                    break;
                case READ_COMPLETE:
                    mTextView.setVisibility(View.INVISIBLE);
                    InstallPathDialog dialog = new Builder(
                            ImageSearch.this).setClickListener(new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            if (which == 0) {
                                try {
                                    dSClient.bind();
                                    dSClient.start(uri, imageSize);
                                } catch (SecurityException e) {

                                }
                                dialog.dismiss();
                            } else if (which == 1) {
                                dialog.dismiss();
                            }
                        }
                    }).create();
                    dialog.setCanceledOnTouchOutside(false);
                    dialog.show();
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.requestWindowFeature(Window.FEATURE_NO_TITLE);//去掉标题栏
        setContentView(R.layout.image_list);
        mInflater = LayoutInflater.from(this);
        mListView = (ListView) findViewById(R.id.file_list);
        mListView.setOnItemClickListener(this);
        backLayout = (RelativeLayout) findViewById(R.id.back_btn);
        backLayout.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                finish();
            }
        });
        mTextView = (TextView) findViewById(R.id.text);
        scanBtn = (TextView) findViewById(R.id.scan_btn);
        scanBtn.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                initList();
            }
        });
        requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 0);
        initList();
        dSClient = new DynamicSystemClient(this);
        dSClient.setOnStatusChangedListener(this);

    }
    @Override

    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

    }
    private void initList() {
        FileNameSelector zipSelector = new FileNameSelector("gz");
        StorageHelper sh = new StorageHelper(this);
        List<String> mps = sh.getMountPoints();
        int offset = 0;
        mFiles = null;
        for (String mp : mps) {
            File file = new File(mp);
            File[] files = file.listFiles(zipSelector);
            if (mFiles == null) {
                mFiles = files;
            } else {
                offset = mFiles.length;
                mFiles = Arrays.copyOf(mFiles, offset + files.length);
                System.arraycopy(files, 0, mFiles, offset, files.length);
            }
        }
        //File file = new File(Environment.getExternalStorageDirectory().getPath());
        //mFiles = file.listFiles(new FileNameSelector("zip"));
        mListView.setAdapter(mAdapter);
        mAdapter.setCurrentList(mFiles);
    }

    public int readGzip(File infile) throws IOException {
        GZIPInputStream gin = new GZIPInputStream(new FileInputStream(infile));
        try {
            byte[] buf = new byte[100000];
            int len;
            int size = 0;
            while ((len = gin.read(buf)) > 0) {
                size = size + len;
            }
            Log.e("ImageSearch", "len:" + size);
            return size;
        } finally {
            if (gin != null) {
                gin.close();
            }
        }
    }


    @Override
    public void onItemClick(AdapterView<?> arg0, View view, final int position, long id) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            final File file = new File(mFiles[position].getPath());

            new Thread() {
                @Override
                public void run() {
                    handler.sendEmptyMessage(READ_START);
                    try {
                        imageSize = readGzip(file);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    handler.sendEmptyMessage(READ_COMPLETE);
                }
            }.start();
            uri = FileProvider.getUriForFile(this, "com.softwinner.update.fileprovider", new File(mFiles[position].getPath()));
        } else {
            uri = Uri.parse("file://" + mFiles[position].getPath());
        }
    }


    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    public class FileNameSelector implements FilenameFilter {
        String extension = ".";

        public FileNameSelector(String fileExtensionNoDot) {
            extension += fileExtensionNoDot;
        }

        public boolean accept(File dir, String name) {
            return name.endsWith(extension);
        }
    }

    private class FileAdapter extends BaseAdapter {

        private String[] imgTypes = {"jpg", "png", "bmp"};
        private String[] videoTypes = {"mp4", "avi", "3gp", "mkv", "rmvb",
                "wmv"};
        private String musicType = "mp3";
        private String zipType = "gz";

        private File mFiles[];

        public void setCurrentList(File directory) {
            mFiles = directory.listFiles();
            notifyDataSetChanged();
        }

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

