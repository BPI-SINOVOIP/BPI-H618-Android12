package com.softwinner.agingdragonbox;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.io.File;

import com.softwinner.agingdragonbox.engine.SwFile;

public class FileSelector extends Activity implements OnItemClickListener {

    public static final String FILE     = "file";
    public static final String ROOT     = "root";

    private File               mCurrentDirectory;

    private LayoutInflater     mInflater;

    private FileAdapter        mAdapter = new FileAdapter();

    private ListView           mListView;

    private String             mRootPath;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mInflater = LayoutInflater.from(this);
        setContentView(R.layout.file_list);
        mListView = (ListView) findViewById(R.id.file_list);
        mListView.setAdapter(mAdapter);
        mListView.setOnItemClickListener(this);

        Intent intent = getIntent();
        String rootPath = intent.getStringExtra(ROOT);
        if (rootPath != null) {
            mAdapter.setCurrentList(new File(rootPath));
            mRootPath = rootPath;
        } else {
            setResult(RESULT_CANCELED);
        }
    }

    @Override
    public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
        File selectFile = (File) adapterView.getItemAtPosition(position);
        if (selectFile.isDirectory()) {
            mCurrentDirectory = selectFile;
            FileAdapter adapter = (FileAdapter) adapterView.getAdapter();
            adapter.setCurrentList(selectFile);
        } else if (selectFile.isFile()) {
            Intent intent = new Intent();
            intent.putExtra(FILE, selectFile.getPath());
            Log.v("addd", "" + intent);
            setResult(RESULT_OK, intent);
            finish();
        }
    }

    @Override
    public void onBackPressed() {
        if (mCurrentDirectory == null || mCurrentDirectory.getPath().equals(mRootPath)) {
            super.onBackPressed();
        } else {
            mCurrentDirectory = mCurrentDirectory.getParentFile();
            mAdapter.setCurrentList(mCurrentDirectory);
        }
    }

    private class FileAdapter extends BaseAdapter {

        private File[] mFiles;

        public void setCurrentList(File directory) {
            mFiles = directory.listFiles();
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
                convertView = mInflater.inflate(R.layout.large_text, null);
            }
            TextView tv = (TextView) convertView;
            File file = mFiles[position];
            String name = file.getName() + "         " + SwFile.byteToSize(file.length());
            tv.setText(name);
            return tv;
        }

    }
}
