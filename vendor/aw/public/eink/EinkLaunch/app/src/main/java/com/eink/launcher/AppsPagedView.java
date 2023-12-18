package com.eink.launcher;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import com.softwinner.einklaunch.R;
import com.eink.launcher.Launcher;
import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.animation.AnimatorListenerAdapter;
import android.animation.ObjectAnimator;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

public class AppsPagedView extends PagedView {
    private LayoutInflater mInflater;
    private PackageManager mPackageManager;
    private ArrayList<ApplicationInfo> mApps;

    private HolographicOutlineHelper mHolographicOutlineHelper;

    private int mCellCountX, mCellCountY;
    private int mCellWidth, mCellHeight;
    private int mPageCount;

    private PagedLayout mPageLayout = null;

    public AppsPagedView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        // TODO Auto-generated constructor stub
        mInflater = LayoutInflater.from(context);
        mPackageManager = context.getPackageManager();
        mApps = new ArrayList<ApplicationInfo>();

        mHolographicOutlineHelper = new HolographicOutlineHelper();
        mPageLayout = new PagedLayout(context);
    }

    public AppsPagedView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
        // TODO Auto-generated constructor stub
    }

    public AppsPagedView(Context context) {
        this(context, null);
        // TODO Auto-generated constructor stub
    }

    public void setApps(ArrayList<ApplicationInfo> list) {
        mApps = list;
        Collections.sort(mApps, ContentModel.APP_NAME_COMPARATOR);
        updatePageCount();

        if (testDataReady()) requestLayout();
    }

    public void addApps(ArrayList<ApplicationInfo> list) {
        addAppsWithoutInvalidate(list);

        updatePageCount();
        invalidatePageData();
    }

    private int findAppByComponent(List<ApplicationInfo> list, ApplicationInfo item) {
        ComponentName removeComponent = item.intent.getComponent();
        int length = list.size();
        for (int i = 0; i < length; ++i) {
            ApplicationInfo info = list.get(i);
            if (info.intent.getComponent().equals(removeComponent)) {
                return i;
            }
        }
        return -1;
    }

    private void addAppsWithoutInvalidate(ArrayList<ApplicationInfo> list) {
        // We add it in place, in alphabetical order
        int count = list.size();
        for (int i = 0; i < count; ++i) {
            ApplicationInfo info = list.get(i);
            int index = Collections.binarySearch(mApps, info, ContentModel.APP_NAME_COMPARATOR);
            if (index < 0) {
                mApps.add(-(index + 1), info);
            }
        }
    }

    private void removeAppsWithoutInvalidate(ArrayList<ApplicationInfo> list) {
        int length = list.size();
        for (int i = 0; i < length; ++i) {
            ApplicationInfo info = list.get(i);
            int removeIndex = findAppByComponent(mApps, info);
            if (removeIndex > -1) {
                mApps.remove(removeIndex);
            }
        }
    }

    public void removeApps(ArrayList<ApplicationInfo> list) {
        removeAppsWithoutInvalidate(list);

        updatePageCount();
        invalidatePageData();
    }

    public void updateApps(ArrayList<ApplicationInfo> list) {
        removeAppsWithoutInvalidate(list);
        addAppsWithoutInvalidate(list);

        updatePageCount();
        invalidatePageData();
    }

    public void refreshApps() {
        updatePageCount();
        invalidatePageData();
    }

    private boolean testDataReady() {
        return !mApps.isEmpty();
    }

    private void updatePageCount() {
        mPageCount = (int) Math.ceil((float) mApps.size()
                / (mCellCountX * mCellCountY));
    }

    protected void onDataReady(int width, int height) {
        //boolean isLandscape = getResources().getConfiguration().orientation ==
        //       Configuration.ORIENTATION_LANDSCAPE;

        mPageLayout.setUnitGap(mPageLayoutWidthGap, mPageLayoutHeightGap);
        mPageLayout.setPadding(mPageLayoutPaddingLeft, mPageLayoutPaddingTop,
                mPageLayoutPaddingRight, mPageLayoutPaddingBottom);
        mPageLayout.calculateCellCount(width, height);
        mCellCountX = mPageLayout.getUnitCountX();
        mCellCountY = mPageLayout.getUnitCountY();

        updatePageCount();

        int widthSpec = MeasureSpec.makeMeasureSpec(getMeasuredWidth(), MeasureSpec.AT_MOST);
        int heightSpec = MeasureSpec.makeMeasureSpec(getMeasuredHeight(), MeasureSpec.AT_MOST);
        mPageLayout.measure(widthSpec, heightSpec);

        invalidatePageData(0, false);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        // TODO Auto-generated method stub
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);
        if (!isDataReady()) {
            if (testDataReady()) {
                setDataIsReady();
                setMeasuredDimension(width, height);
                onDataReady(width, height);

                //Log.d("debug","width : height = " + width + " " + height);
            }
        }

        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    private void setVisibilityOnChildren(ViewGroup layout, int visibility) {
        int childCount = layout.getChildCount();
        for (int i = 0; i < childCount; ++i) {
            layout.getChildAt(i).setVisibility(visibility);
        }
    }

    protected void animateClickFeedback(View v, final Runnable r) {
        // animate the view slightly to show click feedback running some logic after it is "pressed"
        ObjectAnimator anim = (ObjectAnimator) AnimatorInflater.
                loadAnimator(mContext, R.anim.paged_view_click_feedback);
        anim.setTarget(v);
        anim.addListener(new AnimatorListenerAdapter() {
            public void onAnimationRepeat(Animator animation) {
                r.run();
            }
        });
        anim.start();
    }

    protected void animateClickFeedback(final View v) {
        // animate the view slightly to show click feedback running some logic after it is "pressed"
        final ObjectAnimator anim = (ObjectAnimator) AnimatorInflater.
                loadAnimator(mContext, R.anim.paged_view_click_feedback);
        anim.setTarget(v);
        anim.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                v.setAlpha(1.0f);
            }
        });
        anim.start();
    }

    private OnClickListener mClickListener = new OnClickListener() {

        @Override
        public void onClick(View v) {
            // TODO Auto-generated method stub
            final ApplicationInfo info = (ApplicationInfo) v.getTag();
            // don't depend animation callback to click events
            /*animateClickFeedback(v, new Runnable() {
                @Override
                public void run() {
                    // TODO Auto-generated method stub
                    startActivitySafely(info.intent, info);
                }
            });*/
            animateClickFeedback(v);
            Launcher.startActivitySafely(v.getContext(), info.intent, info);
        }
    };

    private void syncAppsPageItems(int page, boolean immediate) {
        int numCells = mCellCountX * mCellCountY;
        int startIndex = page * numCells;
        int endIndex = Math.min(startIndex + numCells, mApps.size());

        PagedLayout layout = (PagedLayout) getChildAt(page);
        layout.removeAllViewsOnPage();
        for (int i = startIndex; i < endIndex; ++i) {
            ApplicationInfo info = mApps.get(i);
            PagedViewIcon icon = (PagedViewIcon) mInflater.inflate(
                    R.layout.apps_item, layout, false);
            icon.applyFromApplicationInfo(info, true, mHolographicOutlineHelper);
            icon.setOnClickListener(mClickListener);

            int index = i - startIndex;
            int x = index % mCellCountX;
            int y = index / mCellCountX;

            layout.addViewToLayout(icon, -1, i, new PagedLayout.LayoutParams(x, y, 1, 1));
        }
    }

    private void setupPage(PagedLayout layout) {
        layout.setUnitCount(mCellCountX, mCellCountY);
        layout.setUnitGap(mPageLayoutWidthGap, mPageLayoutHeightGap);
        layout.setPadding(mPageLayoutPaddingLeft, mPageLayoutPaddingTop,
                mPageLayoutPaddingRight, mPageLayoutPaddingBottom);

        layout.enableCenteredContent(true);

        setVisibilityOnChildren(layout, View.GONE);

        int widthSpec = MeasureSpec.makeMeasureSpec(getMeasuredWidth(), MeasureSpec.AT_MOST);
        int heightSpec = MeasureSpec.makeMeasureSpec(getMeasuredHeight(), MeasureSpec.AT_MOST);

        layout.measure(widthSpec, heightSpec);
        setVisibilityOnChildren(layout, View.VISIBLE);
    }

    @Override
    public void syncPages() {
        // TODO Auto-generated method stub
        removeAllViews();

        Context context = getContext();
        for (int i = 0; i < mPageCount; i++) {
            PagedLayout layout = new PagedLayout(context);
            setupPage(layout);
            addView(layout);
        }
    }

    @Override
    public void syncPageItems(int page, boolean immediate) {
        // TODO Auto-generated method stub
        if (page < mPageCount) {
            syncAppsPageItems(page, immediate);
        }
    }
}
