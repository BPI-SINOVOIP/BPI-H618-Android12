package com.softwinner.agingdragonbox;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.ListView;

/**
 * draggable可拖动的列表,并且支持行删除功能
 *
 * @author admin
 *
 */
public class DraggableList extends ListView {

    private DropListener               mDropListener;

    private ImageView                  mDragView;
    private int                        mDragPos;                             // which
                                                                              // item
                                                                              // is
                                                                              // being
                                                                              // dragged
    private int                        mFirstDragPos;                        // where
                                                                              // was
                                                                              // the
                                                                              // dragged
                                                                              // item
                                                                              // originally
    private int                        mDragPoint;                           // at
                                                                              // what
                                                                              // offset
                                                                              // inside
                                                                              // the
                                                                              // item
                                                                              // did
                                                                              // the
                                                                              // user
                                                                              // grab
                                                                              // it
    private int                        mCoordOffset;                         // the
                                                                              // difference
                                                                              // between
                                                                              // screen
                                                                              // coordinates
                                                                              // and
                                                                              // coordinates
                                                                              // in
                                                                              // this
                                                                              // view

    private Rect                       mTempRect                = new Rect();
    private final int                  mTouchSlop;
    private int                        mHeight;
    private int                        mUpperBound;
    private int                        mLowerBound;
    private WindowManager              mWindowManager;
    private WindowManager.LayoutParams mWindowParams;
    private int                        dragndropBackgroundColor = 0x00000000;
    private Bitmap                     mDragBitmap;
    private int                        mItemHeightHalf          = 32;
    private int                        mItemHeightNormal        = 64;
    private int                        mItemHeightExpanded      = 128;

    // private int grabberId=-1;

    public DraggableList(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public DraggableList(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mTouchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
        mItemHeightNormal = getResources().getDimensionPixelSize(R.dimen.configuration_item_height);
        mItemHeightHalf = mItemHeightNormal / 2;
        mItemHeightExpanded = mItemHeightNormal * 2;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if ((mDropListener != null) && mDragView != null) {
            int action = ev.getAction();
            switch (action) {
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                Rect r = mTempRect;
                mDragView.getDrawingRect(r);
                stopDragging();
                if (mDropListener != null && mDragPos >= 0 && mDragPos < getCount()) {
                    mDropListener.drop(mFirstDragPos, mDragPos);
                }
                unExpandViews(false);
                break;

            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_MOVE:
                int x = (int) ev.getX();
                int y = (int) ev.getY();
                dragView(x, y);

                int itemnum = getItemForPosition(y);
                if (itemnum >= 0) {
                    if (action == MotionEvent.ACTION_DOWN || itemnum != mDragPos) {

                        mDragPos = itemnum;
                        int firstPosition = getFirstVisiblePosition();
                        int lastPosition = getLastVisiblePosition();
                        int count = getAdapter().getCount();
                        if (mDragPos <= firstPosition && firstPosition > 0) {
                            smoothScrollToPosition(firstPosition - 1);
                        }
                        if (mDragPos >= lastPosition && lastPosition < count - 1) {
                            smoothScrollToPosition(lastPosition + 1);
                        }
                        doExpansion();
                    }
                }
                break;
            default:
                break;
            }
            return true;
        }
        return super.onTouchEvent(ev);

    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (mDropListener != null) {
            switch (ev.getAction()) {
            case MotionEvent.ACTION_DOWN:
                int x = (int) ev.getX();
                int y = (int) ev.getY();
                int itemnum = pointToPosition(x, y);
                if (itemnum == AdapterView.INVALID_POSITION) {
                    break;
                }
                ViewGroup item = (ViewGroup) getChildAt(itemnum - getFirstVisiblePosition());
                mDragPoint = y - item.getTop();
                mCoordOffset = ((int) ev.getRawY()) - y;
                // 拖动的ImageView
                View dragger = item.findViewById(R.id.ic_drag);
                Rect r = mTempRect;

                r.left = dragger.getLeft();
                r.right = dragger.getRight();
                r.top = dragger.getTop();
                r.bottom = dragger.getBottom();

                if ((r.left < x) && (x < r.right)) {
                    item.setDrawingCacheEnabled(true);
                    // Create a copy of the drawing cache so that it does not
                    // get recycled
                    // by the framework when the list tries to clean up memory
                    Bitmap bitmap = Bitmap.createBitmap(item.getDrawingCache());
                    startDragging(bitmap, y);
                    mDragPos = itemnum;
                    mFirstDragPos = mDragPos;
                    mHeight = getHeight();
                    int touchSlop = mTouchSlop;
                    mUpperBound = Math.min(y - touchSlop, mHeight / 3);
                    mLowerBound = Math.max(y + touchSlop, mHeight * 2 / 3);
                    return false;
                }
                // 删除的ImageView
                View delView = item.findViewById(R.id.check_box);
                r.left = delView.getLeft();
                r.right = delView.getRight();
                r.top = delView.getTop();
                r.bottom = delView.getBottom();
                if ((r.left < x) && (x < r.right)) {
                    mDropListener.onCheckClicked(itemnum);
                    return false;
                }
                mDragView = null;
                break;
            default:
                break;
            }
        }
        return super.onInterceptTouchEvent(ev);
    }

    private void startDragging(Bitmap bm, int y) {
        stopDragging();

        mWindowParams = new WindowManager.LayoutParams();
        mWindowParams.gravity = Gravity.TOP;
        mWindowParams.x = 0;
        mWindowParams.y = y - mDragPoint + mCoordOffset;

        mWindowParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
        mWindowParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
        mWindowParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
        mWindowParams.format = PixelFormat.TRANSLUCENT;
        mWindowParams.windowAnimations = 0;

        ImageView v = new ImageView(getContext());
        v.setBackgroundColor(dragndropBackgroundColor);
        v.setImageBitmap(bm);
        mDragBitmap = bm;

        mWindowManager = (WindowManager) getContext().getSystemService("window");
        mWindowManager.addView(v, mWindowParams);
        mDragView = v;
    }

    private void stopDragging() {
        if (mDragView != null) {
            WindowManager wm = (WindowManager) getContext().getSystemService("window");
            wm.removeView(mDragView);
            mDragView.setImageDrawable(null);
            mDragView = null;
        }
        if (mDragBitmap != null) {
            mDragBitmap.recycle();
            mDragBitmap = null;
        }
    }

    private void dragView(int x, int y) {
        float alpha = 1.0f;
        mWindowParams.alpha = alpha;
        // }
        mWindowParams.y = y - mDragPoint + mCoordOffset;
        mWindowManager.updateViewLayout(mDragView, mWindowParams);
    }

    private int getItemForPosition(int y) {
        int adjustedy = y - mDragPoint - mItemHeightHalf;
        int pos = myPointToPosition(0, adjustedy);
        if (pos >= 0) {
            if (pos <= mFirstDragPos) {
                pos += 1;
            }
        } else if (adjustedy < 0) {
            pos = 0;
        }
        return pos;
    }

    private void adjustScrollBounds(int y) {
        if (y >= mHeight / 3) {
            mUpperBound = mHeight / 3;
        }
        if (y <= mHeight * 2 / 3) {
            mLowerBound = mHeight * 2 / 3;
        }
    }

    /*
     * Restore size and visibility for all listitems
     */
    public void unExpandViews(boolean deletion) {
        for (int i = 0;; i++) {
            View v = getChildAt(i);
            if (v == null) {
                if (deletion) {
                    // HACK force update of mItemCount
                    int position = getFirstVisiblePosition();
                    int y = getChildAt(0).getTop();
                    setAdapter(getAdapter());
                    setSelectionFromTop(position, y);
                    // end hack
                }
                layoutChildren(); // force children to be recreated where needed
                v = getChildAt(i);
                if (v == null) {
                    break;
                }
            }
            ViewGroup.LayoutParams params = v.getLayoutParams();
            params.height = mItemHeightNormal;
            v.setLayoutParams(params);
            v.setVisibility(View.VISIBLE);
        }
    }

    /*
     * Adjust visibility and size to make it appear as though an item is being
     * dragged around and other items are making room for it: If dropping the
     * item would result in it still being in the same place, then make the
     * dragged listitem's size normal, but make the item invisible. Otherwise,
     * if the dragged listitem is still on screen, make it as small as possible
     * and expand the item below the insert point. If the dragged item is not on
     * screen, only expand the item below the current insertpoint.
     */
    private void doExpansion() {
        int childnum = mDragPos - getFirstVisiblePosition();
        if (mDragPos > mFirstDragPos) {
            childnum++;
        }

        View first = getChildAt(mFirstDragPos - getFirstVisiblePosition());

        for (int i = 0;; i++) {
            View vv = getChildAt(i);
            if (vv == null) {
                break;
            }
            int height = mItemHeightNormal;
            int visibility = View.VISIBLE;
            if (vv.equals(first)) {
                // processing the item that is being dragged
                if (mDragPos == mFirstDragPos) {
                    // hovering over the original location
                    visibility = View.INVISIBLE;
                } else {
                    // not hovering over it
                    height = 1;
                }
            } else if (i == childnum) {
                if (mDragPos < getCount() - 1) {
                    height = mItemHeightExpanded;
                }
            }
            ViewGroup.LayoutParams params = vv.getLayoutParams();
            params.height = height;
            vv.setLayoutParams(params);
            vv.setVisibility(visibility);
        }
    }

    /*
     * pointToPosition() doesn't consider invisible views, but we need to, so
     * implement a slightly different version.
     */
    private int myPointToPosition(int x, int y) {
        Rect frame = mTempRect;
        final int count = getChildCount();
        for (int i = count - 1; i >= 0; i--) {
            final View child = getChildAt(i);
            child.getHitRect(frame);
            if (frame.contains(x, y)) {
                return getFirstVisiblePosition() + i;
            }
        }
        return INVALID_POSITION;
    }

    public interface DropListener {
        void drop(int from, int to);

        void onCheckClicked(int index);
    }

    public void setDropListener(DropListener onDrop) {
        mDropListener = onDrop;
    }

}
