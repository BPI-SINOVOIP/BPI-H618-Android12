package com.softwinner.agingdragonbox;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.Toast;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

import com.softwinner.agingdragonbox.engine.BaseCase;
import com.softwinner.agingdragonbox.engine.BaseConfiguration;
import com.softwinner.agingdragonbox.engine.DFEngine;
import com.softwinner.agingdragonbox.engine.Utils;
import com.softwinner.agingdragonbox.xml.Node;
import com.softwinner.agingdragonbox.xml.Parser;
import com.softwinner.agingdragonbox.xml.ParserException;
import com.softwinner.agingdragonbox.xml.parse.JaxpParser;

// ad by nana
public class Configuration extends Activity implements AdapterView.OnItemClickListener,
        View.OnClickListener {
    private static final String   TAG = "DragonAging-Configuration";
    private DraggableList         mListView;
    private DraggableArrayAdapter mAdapter    = null;
    private ArrayList<CaseHolder> mCaseList;
    private CheckBox              saveReportCheckbox;         // 报表设置CheckBox
    public static final String    SAVE_REPORT = "save_report"; // 是否保存报表属性名称

    private class CaseHolder {
        String   name;
        BaseCase mCase;
        boolean  selected;
    }

    protected Parser         mParser;

    private static final int MODE_EXTSD    = 0;
    private static final int MODE_USBHOST0 = 4;
    private static final int MODE_USBHOST1 = 5;
    private static final int MODE_USBHOST2 = 6;
    private static final int MODE_USBHOST3 = 7;

    private static final int MODE_DEFAULT  = 3;
    private int              mMode         = MODE_EXTSD;

    public interface OnActivityResult {
        void onActivityResult(int requestCode, int resultCode, Intent data);
    }

    private static final ArrayList<OnActivityResult> ACTIVITYRESULT
        = new ArrayList<OnActivityResult>();

    public static final void registerActivityResult(OnActivityResult callback) {
        if (!ACTIVITYRESULT.contains(callback)) {
            ACTIVITYRESULT.add(callback);
        }
    }

    public static final void unregisterActivityResult(OnActivityResult callback) {
        ACTIVITYRESULT.remove(callback);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (new File(DFEngine.EXTSD_CASE_DIR).exists()
                || new File(DFEngine.USBHOST0_CASE_DIR).exists()
                || new File(DFEngine.USBHOST1_CASE_DIR).exists()
                || new File(DFEngine.USBHOST2_CASE_DIR).exists()
                || new File(DFEngine.USBHOST3_CASE_DIR).exists()) {
            Log.d(TAG, "config file exists");
        } else {
            finish();
        }

        setContentView(R.layout.configuration);

        mListView = (DraggableList) findViewById(R.id.draggable_list);
        findViewById(R.id.save).setOnClickListener(this);
        // findViewById(R.id.reset).setOnClickListener(this);
        findViewById(R.id.exit).setOnClickListener(this);
        // findViewById(R.id.add_a).setOnClickListener(this);
        // findViewById(R.id.add_w).setOnClickListener(this);
        // findViewById(R.id.add_pause).setOnClickListener(this);
        findViewById(R.id.switch_mode).setOnClickListener(this);

        saveReportCheckbox = (CheckBox) findViewById(R.id.config_report_setting);

        mParser = new JaxpParser();

        if (new File(DFEngine.EXTSD_CASE_FILE_NAME).exists()) {
            mMode = MODE_EXTSD;
            Toast.makeText(this, getString(R.string.configuration_edit_extsd), Toast.LENGTH_SHORT)
                    .show();
        } else if (new File(DFEngine.USBHOST0_CASE_FILE_NAME).exists()) {
            mMode = MODE_USBHOST0;
            Toast.makeText(this, getString(R.string.configuration_edit_usbhost), Toast.LENGTH_SHORT)
                    .show();
        } else if (new File(DFEngine.USBHOST1_CASE_FILE_NAME).exists()) {
            mMode = MODE_USBHOST1;
            Toast.makeText(this, getString(R.string.configuration_edit_usbhost), Toast.LENGTH_SHORT)
                    .show();
        } else if (new File(DFEngine.USBHOST2_CASE_FILE_NAME).exists()) {
            mMode = MODE_USBHOST2;
            Toast.makeText(this, getString(R.string.configuration_edit_usbhost), Toast.LENGTH_SHORT)
                    .show();
        } else if (new File(DFEngine.USBHOST3_CASE_FILE_NAME).exists()) {
            mMode = MODE_USBHOST3;
            Toast.makeText(this, getString(R.string.configuration_edit_usbhost), Toast.LENGTH_SHORT)
                    .show();
        } else {
            Log.d(TAG, "nothing");
        }

        boolean openCustom = switchMode(mMode);
        mCaseList = openCustom ? mCaseList : getData();

        mAdapter = new DraggableArrayAdapter();
        mListView.setAdapter(mAdapter);
        mListView.setDropListener(onDrop);
        mListView.setOnItemClickListener(this);
    }

    boolean switchMode(int mode) {
        boolean openCustom = false;
        InputStream fis = null;

        try {
            switch (mode) {
            case MODE_EXTSD:
                fis = new FileInputStream(DFEngine.EXTSD_CASE_FILE_NAME);
                break;
            case MODE_USBHOST0:
                fis = new FileInputStream(DFEngine.USBHOST0_CASE_FILE_NAME);
                break;
            case MODE_USBHOST1:
                fis = new FileInputStream(DFEngine.USBHOST1_CASE_FILE_NAME);
                break;
            case MODE_USBHOST2:
                fis = new FileInputStream(DFEngine.USBHOST2_CASE_FILE_NAME);
                break;
            case MODE_USBHOST3:
                fis = new FileInputStream(DFEngine.USBHOST3_CASE_FILE_NAME);
                break;
            case MODE_DEFAULT:
                AssetManager assetManager = getResources().getAssets();
                fis = assetManager.open(DFEngine.DEFAULT_CASE_FILE_NAME);
            default:
                break;
            }
            mCaseList = getDataFromXML(fis);
            openCustom = true;

        } catch (FileNotFoundException e1) {
            e1.printStackTrace();
        } catch (ParserException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (fis != null) {
                try {
                    fis.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return openCustom;
    }

    private ArrayList<CaseHolder> getData() {
        // ArrayList<CaseHolder> list = new ArrayList<CaseHolder>();
        //
        // for (int i = 0; i < Utils.ALL_CASES.length; i++) {
        // CaseHolder holder = new CaseHolder();
        // holder._case = Utils.createCase(Utils.ALL_CASES[i]);
        // if (holder._case == null)
        // continue;
        // BaseConfiguration config = holder._case.getConfiguration();
        // config.initializeForConfig(this);
        // holder.name = config.getName();
        //
        // if(holder.name.equals(CaseBrowser.class.getSimpleName())
        // ||holder.name.equals(CaseBattery.class.getSimpleName())
        // ||holder.name.equals(CaseStorage.class.getSimpleName())
        // ||holder.name.equals(CaseBluetooth.class.getSimpleName())
        // ||holder.name.equals(CaseBacklight.class.getSimpleName())
        // ) {
        // holder.selected = false;
        // } else {
        // holder.selected = true;
        // }
        //
        // list.add(holder);
        // }
        // return list;
        ArrayList<CaseHolder> list = null;
        AssetManager assetManager = getResources().getAssets();
        InputStream is = null;
        try {
            is = assetManager.open(DFEngine.DEFAULT_CASE_FILE_NAME);
            list = getDataFromXML(is);
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        } catch (ParserException e) {
            e.printStackTrace();
        }
        return list;
    }

    private ArrayList<CaseHolder> getDataFromXML(InputStream is) throws ParserException {
        ArrayList<CaseHolder> list = new ArrayList<CaseHolder>();

        for (int i = 0; i < Utils.ALL_CASES.length; i++) {
            CaseHolder holder = new CaseHolder();
            holder.mCase = Utils.createCase(Utils.ALL_CASES[i]);
            if (holder.mCase == null) {
                continue;
            }
            BaseConfiguration config = holder.mCase.getConfiguration();
            config.initializeForConfig(this);
            holder.name = config.getName();
            holder.selected = false;
            list.add(holder);
        }

        Node node = null;
        node = mParser.parse(is);

        // 从xml读取报表设置
        if (node.getAttribute(SAVE_REPORT) != null) {
            saveReportCheckbox.setChecked(true);
        }

        int n = node.getNNodes();
        for (int i = 0; i < n; i++) {
            // create the case from config file;
            CaseHolder holder = new CaseHolder();
            Node child = node.getNode(i);
            String caseName = child.getName();
            holder.mCase = Utils.createCase(caseName);
            if (holder.mCase == null) {
                continue;
            }
            BaseConfiguration config = holder.mCase.getConfiguration();
            config.initializeForConfig(this);
            if (child.hasNodes() || child.hasAttributes()) {
                config.initializeForConfig(this, child);
            } else {
                config.initializeForConfig(this);
            }
            holder.name = config.getName();
            holder.selected = true;

            // replace to original list
            for (int j = 0; j < list.size(); j++) {
                CaseHolder h = list.get(j);
                String simpleName = holder.mCase.getClass().getSimpleName();
                if (h.mCase.getClass().getSimpleName().equals(simpleName) && !h.selected) {
                    list.remove(h);
                    // list.add(i, holder);
                    break;
                }
            }
            list.add(i, holder); // 将add从循环中一出来，即测试case不需要是内置case
        }
        return list;
    }

    @Override
    protected void onDestroy() {
        relaseCase();
        super.onDestroy();
    }

    private void relaseCase() {
        mCaseList.clear();
    }

    private DraggableList.DropListener onDrop = new DraggableList.DropListener() {
                                                  @Override
                                                  public void drop(int from, int to) {
                                                      CaseHolder item = mAdapter.getItem(from);
                                                      mAdapter.remove(item);
                                                      mAdapter.insert(item, to);
                                                      mAdapter.notifyDataSetChanged();
                                                  }

                                                  @Override
                                                  public void onCheckClicked(int index) {
                                                      onCheckChange(index);
                                                  }
                                              };

    private void onCheckChange(final int index) {
        CaseHolder holder = mCaseList.get(index);
        holder.selected = !holder.selected;
        mAdapter.notifyDataSetChanged();
    }

    class DraggableArrayAdapter extends ArrayAdapter<CaseHolder> {

        class ViewHolder {
            View     drag;
            TextView name;
            TextView attr;
            CheckBox checkbox;
        }

        DraggableArrayAdapter() {
            super(Configuration.this, R.layout.configuration_item, mCaseList);
        }

        public ArrayList<CaseHolder> getList() {
            return mCaseList;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                LayoutInflater inflater = getLayoutInflater();
                convertView = inflater.inflate(R.layout.configuration_item, parent, false);
                ViewHolder holder = new ViewHolder();
                holder.drag = convertView.findViewById(R.id.ic_drag);
                holder.name = (TextView) convertView.findViewById(R.id.case_name);
                holder.attr = (TextView) convertView.findViewById(R.id.case_attr);
                holder.checkbox = (CheckBox) convertView.findViewById(R.id.check_box);
                convertView.setTag(holder);
            }

            ViewHolder holder = (ViewHolder) convertView.getTag();
            CaseHolder caseHolder = mCaseList.get(position);
            holder.name.setText("" + position + "." + caseHolder.name);
            String configStr = caseHolder.mCase.getConfiguration().getConfigInfo();
            if (configStr != null) {
                holder.attr.setText(configStr);
            } else {
                holder.attr.setText("");
            }
            holder.checkbox.setChecked(caseHolder.selected);
            ViewGroup.LayoutParams params = convertView.getLayoutParams();
            params.height = Configuration.this.getResources().getDimensionPixelSize(
                    R.dimen.configuration_item_height);
            convertView.setLayoutParams(params);
            return (convertView);
        }
    }

    @Override
    public void onItemClick(AdapterView<?> adapterView, View view, int position, long id) {
        final CaseHolder holder = mCaseList.get(position);
        showConfigDlg(holder);
    }

    private void showConfigDlg(final CaseHolder holder) {
        if (holder.mCase.getConfiguration().configurable()) {
            String title = holder.name + " " + getString(R.string.configuration_suffix);
            AlertDialog.Builder builder = new AlertDialog.Builder(this)
                    .setTitle(title)
                    .setView(holder.mCase.getConfiguration().getConfigView())
                    .setCancelable(true)
                    .setPositiveButton(R.string.configutation_ok,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    dialog.dismiss();
                                    holder.mCase.getConfiguration().saveConfig();
                                    holder.name = holder.mCase.getConfiguration().getName();
                                    mAdapter.notifyDataSetChanged();
                                }
                            });
            // if (holder._case instanceof CasePause) {
            // builder.setNegativeButton(R.string.configutation_delete, new
            // DialogInterface.OnClickListener() {
            // @Override
            // public void onClick(DialogInterface dialog, int which) {
            // dialog.dismiss();
            // mCaseList.remove(holder);
            // mAdapter.notifyDataSetChanged();
            // }
            // });
            // }
            AlertDialog dialog = builder.create();
            dialog.setOnDismissListener(new DialogInterface.OnDismissListener() {

                @Override
                public void onDismiss(DialogInterface arg0) {
                    ViewGroup vg = (ViewGroup) holder.mCase.getConfiguration().getConfigView()
                            .getParent();
                    if (vg != null) {
                        vg.removeAllViews();
                    }
                }
            });
            dialog.show();

            // .setNegativeButton(R.string.configutation_cancel,
            // new DialogInterface.OnClickListener() {
            //
            // @Override
            // public void onClick(DialogInterface dialog,
            // int which) {
            // dialog.dismiss();
            // ViewGroup vg = (ViewGroup) holder._case
            // .getConfiguration().getConfigView()
            // .getParent();
            // if (vg != null) {
            // vg.removeAllViews();
            // }
            // }
            // }).show();
        }
    }

    private void save() {
        Node node = new Node();
        node.setName(Utils.CONFIGURATION_NODE);

        // 保存报表设置
        if (saveReportCheckbox.isChecked()) {
            node.setAttribute(SAVE_REPORT, "1");
        }

        for (CaseHolder holder : mCaseList) {
            if (holder.selected) {
                node.addNode(holder.mCase.getConfiguration().getConfigNode());
            }
        }
        boolean write = false;
        try {
            String caseFileName = DFEngine.EXTSD_CASE_FILE_NAME;

            // if (mMode == MODE_EXTSD) {
            // File localDir = new File(DFEngine.DRAGONFIRE_DIRECTORY);
            // if (!localDir.exists()) {
            // localDir.mkdirs();
            // }
            // caseFileName = DFEngine.EXTSD_CASE_FILE_NAME;
            // } else if (mMode == MODE_USBHOST) {
            // caseFileName = DFEngine.EXT_CASE_FILE_NAME;
            // }

            if (mMode == MODE_EXTSD) {
                caseFileName = DFEngine.EXTSD_CASE_FILE_NAME;
            } else if (mMode == MODE_USBHOST0) {
                caseFileName = DFEngine.USBHOST0_CASE_FILE_NAME;
            } else if (mMode == MODE_USBHOST1) {
                caseFileName = DFEngine.USBHOST1_CASE_FILE_NAME;
            } else if (mMode == MODE_USBHOST2) {
                caseFileName = DFEngine.USBHOST2_CASE_FILE_NAME;
            } else if (mMode == MODE_USBHOST3) {
                caseFileName = DFEngine.USBHOST3_CASE_FILE_NAME;
            }

            File f = new File(caseFileName);
            if (!f.getParentFile().exists()) {
                f.getParentFile().mkdirs();
            }
            if (!f.exists()) {
                f.createNewFile();
                // Runtime.getRuntime().exec("touch " + caseFileName);
            }

            byte[] content = node.toString().getBytes();
            ByteArrayInputStream contentIn = new ByteArrayInputStream(content);
            FileOutputStream fos = new FileOutputStream(caseFileName);
            byte[] writeBuf = new byte[1024 * 512];
            int length = 0;
            while ((length = contentIn.read(writeBuf)) > 0) {
                fos.write(writeBuf, 0, length);
            }
            contentIn.close();
            fos.flush();
            fos.close();
            write = true;
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        if (write) {
            Toast.makeText(this, R.string.configuration_write_succeful, Toast.LENGTH_SHORT).show();
        } else {
            Toast.makeText(this, R.string.configuration_write_failed, Toast.LENGTH_SHORT).show();
        }
        Log.d(TAG, "save:/n" + node.toString());
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        for (OnActivityResult ar : ACTIVITYRESULT) {
            ar.onActivityResult(requestCode, resultCode, data);
        }
    }

    @Override
    public void onConfigurationChanged(android.content.res.Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    @Override
    public void onClick(View v) {
        int id = v.getId();
        switch (id) {
        case R.id.save:
            save();
            break;
        case R.id.exit:
            finish();
            break;
        case R.id.switch_mode:
            boolean openCustom = switchMode(mMode);
            mCaseList = openCustom ? mCaseList : getData();
            mAdapter = new DraggableArrayAdapter();
            mListView.setAdapter(mAdapter);
            mAdapter.notifyDataSetChanged();
            break;
        default:
            break;
        }
    }

}
