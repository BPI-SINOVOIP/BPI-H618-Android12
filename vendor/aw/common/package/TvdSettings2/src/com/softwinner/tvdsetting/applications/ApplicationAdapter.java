package com.softwinner.tvdsetting.applications;

import java.util.ArrayList;
import java.util.Comparator;

import com.softwinner.tvdsetting.R;
import com.softwinner.tvdsetting.applications.ApplicationsState.AppEntry;

import android.content.Context;
import android.os.Environment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.BaseAdapter;
import android.widget.Filter;
import android.widget.Filterable;
import android.widget.ImageView;
import android.widget.TextView;

/*
 * Custom adapter implementation for the ListView
 * This adapter maintains a map for each displayed application and its properties
 * An index value on each AppInfo object indicates the correct position or index
 * in the list. If the list gets updated dynamically when the user is viewing the list of
 * applications, we need to return the correct index of position. This is done by mapping
 * the getId methods via the package name into the internal maps and indices.
 * The order of applications in the list is mirrored in mAppLocalList
 */
class ApplicationsAdapter extends BaseAdapter implements Filterable,
        ApplicationsState.Callbacks, AbsListView.RecyclerListener {
	
    private final ApplicationsState mState;
    private final ApplicationsState.Session mSession;
    private final Context mContext;
    private final ArrayList<View> mActive = new ArrayList<View>();
    private final int mFilterMode;
    private ArrayList<ApplicationsState.AppEntry> mBaseEntries;
    private ArrayList<ApplicationsState.AppEntry> mEntries;
    private boolean mResumed;
    private int mLastSortMode=-1;
    private boolean mWaitingForData;
    private int mWhichSize = SIZE_TOTAL;
    public static final int SIZE_TOTAL = 0;
    public static final int SIZE_INTERNAL = 1;
    public static final int SIZE_EXTERNAL = 2;
    private static final int MENU_OPTIONS_BASE = 0;
    // Filter options used for displayed list of applications
    public static final int FILTER_APPS_ALL = MENU_OPTIONS_BASE + 0;
    public static final int FILTER_APPS_THIRD_PARTY = MENU_OPTIONS_BASE + 1;
    public static final int FILTER_APPS_SDCARD = MENU_OPTIONS_BASE + 2;
    public static final int FILTER_APPS_DISABLED = MENU_OPTIONS_BASE + 3;

    public static final int SORT_ORDER_ALPHA = MENU_OPTIONS_BASE + 4;
    public static final int SORT_ORDER_SIZE = MENU_OPTIONS_BASE + 5;
    public static final int SHOW_RUNNING_SERVICES = MENU_OPTIONS_BASE + 6;
    public static final int SHOW_BACKGROUND_PROCESSES = MENU_OPTIONS_BASE + 7;
    public static final int RESET_APP_PREFERENCES = MENU_OPTIONS_BASE + 8;
	private static final String TAG = "ApplicationsAdapter";
	private static final boolean DEBUG = true;
    CharSequence mCurFilterPrefix;

    private Filter mFilter = new Filter() {
        @Override
        protected FilterResults performFiltering(CharSequence constraint) {
            ArrayList<ApplicationsState.AppEntry> entries
                    = applyPrefixFilter(constraint, mBaseEntries);
            FilterResults fr = new FilterResults();
            fr.values = entries;
            fr.count = entries.size();
            return fr;
        }

        @Override
        protected void publishResults(CharSequence constraint, FilterResults results) {
            mCurFilterPrefix = constraint;
            mEntries = (ArrayList<ApplicationsState.AppEntry>)results.values;
            notifyDataSetChanged();
        }
    };

    public ApplicationsAdapter(ApplicationsState state, Context ctx,int filterMode) {
        mState = state;
        mSession = state.newSession(this);
        mContext = ctx;
        mFilterMode = filterMode;
    }

    public void resume(int sort) {
        if (DEBUG) Log.i(TAG, "Resume!  mResumed=" + mResumed);
        if (!mResumed) {
            mResumed = true;
            mSession.resume();
            mLastSortMode = sort;
            rebuild(true);
        } else {
            rebuild(sort);
        }
    }

    public void pause() {
        if (mResumed) {
            mResumed = false;
            mSession.pause();
        }
    }

    public void rebuild(int sort) {
        if (sort == mLastSortMode) {
            return;
        }
        mLastSortMode = sort;
        rebuild(true);
    }
    
    public void rebuild(boolean eraseold) {
        if (DEBUG) Log.i(TAG, "Rebuilding app list...");
        ApplicationsState.AppFilter filterObj;
        Comparator<AppEntry> comparatorObj;
        boolean emulated = Environment.isExternalStorageEmulated();
        if (emulated) {
            mWhichSize = SIZE_TOTAL;
        } else {
            mWhichSize = SIZE_INTERNAL;
        }
        switch (mFilterMode) {
            case FILTER_APPS_THIRD_PARTY:
                filterObj = ApplicationsState.THIRD_PARTY_FILTER;
                break;
            case FILTER_APPS_SDCARD:
                filterObj = ApplicationsState.ON_SD_CARD_FILTER;
                if (!emulated) {
                    mWhichSize = SIZE_EXTERNAL;
                }
                break;
            case FILTER_APPS_DISABLED:
                filterObj = ApplicationsState.DISABLED_FILTER;
                break;
            default:
                filterObj = ApplicationsState.ALL_ENABLED_FILTER;
                break;
        }
        switch (mLastSortMode) {
            case SORT_ORDER_SIZE:
                switch (mWhichSize) {
                    case SIZE_INTERNAL:
                        comparatorObj = ApplicationsState.INTERNAL_SIZE_COMPARATOR;
                        break;
                    case SIZE_EXTERNAL:
                        comparatorObj = ApplicationsState.EXTERNAL_SIZE_COMPARATOR;
                        break;
                    default:
                        comparatorObj = ApplicationsState.SIZE_COMPARATOR;
                        break;
                }
                break;
            default:
                comparatorObj = ApplicationsState.ALPHA_COMPARATOR;
                break;
        }
        ArrayList<ApplicationsState.AppEntry> entries
                = mSession.rebuild(filterObj, comparatorObj);
        if (entries == null && !eraseold) {
            // Don't have new list yet, but can continue using the old one.
        	if (DEBUG) Log.i(TAG, "entries == null ,use the oldone");
            return;
        }
        mBaseEntries = entries;
        if (mBaseEntries != null) {
            mEntries = applyPrefixFilter(mCurFilterPrefix, mBaseEntries);
        } else {
            mEntries = null;
        }
        notifyDataSetChanged();
     

        if (entries == null) {
            mWaitingForData = true;

        } else {
    
        }
    }

    ArrayList<ApplicationsState.AppEntry> applyPrefixFilter(CharSequence prefix,
            ArrayList<ApplicationsState.AppEntry> origEntries) {
        if (prefix == null || prefix.length() == 0) {
            return origEntries;
        } else {
            String prefixStr = ApplicationsState.normalize(prefix.toString());
            final String spacePrefixStr = " " + prefixStr;
            ArrayList<ApplicationsState.AppEntry> newEntries
                    = new ArrayList<ApplicationsState.AppEntry>();
            for (int i=0; i<origEntries.size(); i++) {
                ApplicationsState.AppEntry entry = origEntries.get(i);
                String nlabel = entry.getNormalizedLabel();
                if (nlabel.startsWith(prefixStr) || nlabel.indexOf(spacePrefixStr) != -1) {
                    newEntries.add(entry);
                }
            }
            return newEntries;
        }
    }

    @Override
    public void onRunningStateChanged(boolean running) {
    }

    @Override
    public void onPackageListChanged() {
        rebuild(false);
    }

    @Override
    public void onPackageIconChanged() {
        // We ensure icons are loaded when their item is displayed, so
        // don't care about icons loaded in the background.
    }

    @Override
    public void onPackageSizeChanged(String packageName) {
      notifyDataSetChanged();
    }

    @Override
    public void onAllSizesComputed() {
        if (mLastSortMode == SORT_ORDER_SIZE) {
            rebuild(false);
        }
    }
    
    public int getCount() {
    	int retval = mEntries != null ? mEntries.size() : 0;
    	Log.d(TAG,"total count = " + retval);
        return retval;
    }
    
    public Object getItem(int position) {
        return mEntries.get(position);
    }
    
    public ApplicationsState.AppEntry getAppEntry(int position) {
        return mEntries.get(position);
    }

    public long getItemId(int position) {
        return mEntries.get(position).id;
    }
    
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        // A ViewHolder keeps references to children views to avoid unnecessary calls
        // to findViewById() on each row.
        // Bind the data efficiently with the holder
        ApplicationsState.AppEntry entry = mEntries.get(position);
        mState.requestSize(entry.info.packageName);
        convertView=LayoutInflater.from(mContext).inflate(R.layout.applist, null);
        ImageView appicon = (ImageView) convertView.findViewById(R.id.appicon);
		TextView appname = (TextView) convertView.findViewById(R.id.appname);
		TextView appsize = (TextView) convertView.findViewById(R.id.appsize);
        synchronized (entry) {
            if (entry.label != null) {
            	appname.setText(entry.label);
            }
            mState.ensureIcon(entry);
            if (entry.icon != null) {
            	appicon.setImageDrawable(entry.icon);
            }
            appsize.setText(entry.sizeStr);
        }    
        mActive.remove(convertView);
        mActive.add(convertView);
        return convertView;
    }

    @Override
    public Filter getFilter() {
        return mFilter;
    }

    @Override
    public void onMovedToScrapHeap(View view) {
        mActive.remove(view);
    }

	@Override
	public void onRebuildComplete(ArrayList<AppEntry> apps) {
		// TODO Auto-generated method stub
		
	}
}