package com.android.abupdater.view;

import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.LinearInterpolator;

import androidx.annotation.Nullable;

public class WaterWaveView extends View {

    private int width;
    private int height;
    private Paint paint;
    private Paint textPaint;
    private Paint circlePaint;
    private Path path;
    private int cycle = 160;
    private int waveHeight = 80;
    private int delayTime = 150;
    private Point startPoint;
    private int progress;
    private int translateX = 40;
    private boolean openAnimate = false;
    private boolean autoIncrement = true;

    public WaterWaveView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public WaterWaveView(Context context) {
        super(context);
        init(context);
    }

    private void init(Context context) {
        path = new Path();
        paint = new Paint();
        paint.setAntiAlias(true);
        paint.setStrokeWidth(dip2px(context, 5));
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.GREEN);

        circlePaint = new Paint();
        circlePaint.setStrokeWidth(dip2px(context, 5));
        circlePaint.setStyle(Paint.Style.STROKE);
        circlePaint.setAntiAlias(true);
        circlePaint.setColor(Color.WHITE);

        textPaint = new Paint();
        textPaint.setAntiAlias(true);
        textPaint.setTextSize(dip2px(context, 20));
        textPaint.setColor(Color.BLACK);
    }

    public void setColor(int color) {
        if (circlePaint != null) {
            circlePaint.setColor(color);
        }
    }

    public void setDelayTime(int delayTime) {
        this.delayTime = delayTime;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        Path circlePath = new Path();
        circlePath.addCircle(width / 2, height / 2, width / 2, Path.Direction.CW);
        canvas.clipPath(circlePath);
        canvas.drawPaint(circlePaint);
        canvas.drawCircle(width / 2, height / 2, width / 2, circlePaint);
        //根据进度改变起点坐标的y值
        startPoint.y = (int) (height - (progress / 100.0 * height));
        //起点
        path.moveTo(startPoint.x, startPoint.y);
        int j = 1;
        //循环绘制正弦曲线 循环一次半个周期
        for (int i = 1; i <= 8; i++) {
            if (i % 2 == 0) {
                //波峰
                path.quadTo(startPoint.x + (cycle * j), startPoint.y + waveHeight,
                startPoint.x + (cycle * 2) * i, startPoint.y);
            } else {
                //波谷
                path.quadTo(startPoint.x + (cycle * j), startPoint.y - waveHeight,
                startPoint.x + (cycle * 2) * i, startPoint.y);
            }
            j += 2;
        }
        //绘制封闭的曲线
        path.lineTo(width, height);
        path.lineTo(startPoint.x, height);
        path.lineTo(startPoint.x, startPoint.y);
        path.close();
        canvas.drawPath(path, paint);

        drawText(canvas, textPaint, progress + "%");
        if (startPoint.x + translateX >= 0) {
            startPoint.x = -cycle * 4;
        }

        startPoint.x += translateX;
        if (autoIncrement && progress < 100) {
            progress++;
        }

        if (progress >= 100) {
            openAnimate = true;
            canvas.drawCircle(width / 2, height / 2, width / 2, paint);
            drawText(canvas, textPaint, progress + "%");
        }

        path.reset();
        if (!openAnimate) {
            postInvalidateDelayed(delayTime);
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        width = getViewSize(400, widthMeasureSpec);
        height = getViewSize(400, heightMeasureSpec);
        startPoint = new Point(-cycle * 3, height / 2);
    }


    private int getViewSize(int defaultSize, int measureSpec) {
        int viewSize = defaultSize;
        int mode = MeasureSpec.getMode(measureSpec);
        int size = MeasureSpec.getSize(measureSpec);
        switch (mode) {
            case MeasureSpec.UNSPECIFIED:
            viewSize = defaultSize;
            break;
            case MeasureSpec.AT_MOST:
            viewSize = size;
            break;
            case MeasureSpec.EXACTLY:
            viewSize = size;
            break;
        }
        return viewSize;
    }

    private void drawText(Canvas canvas, Paint paint, String text) {
        Rect targetRect = new Rect(0, 0, width, height);
        Paint.FontMetricsInt fontMetrics = paint.getFontMetricsInt();
        int baseline = (targetRect.bottom + targetRect.top - fontMetrics.bottom - fontMetrics.top) / 2;
        paint.setTextAlign(Paint.Align.CENTER);
        canvas.drawText(text, targetRect.centerX(), baseline, paint);
    }

    public int dip2px(Context context, float dpValue) {
        final float scale = context.getResources().getDisplayMetrics().density;
        return (int) (dpValue * scale + 0.5f);
    }

    public void setWaveHeight(int waveHeight) {
        this.waveHeight = waveHeight;
        invalidate();
    }

    public void setCycle(int cycle) {
        this.cycle = cycle;
        invalidate();
    }

    public void setProgress(int progress) {
        if (progress > 100 || progress < 0){
            throw new RuntimeException(getClass().getName() + "请设置[0,100]之间的值");
        }
        this.progress = progress;
        autoIncrement = false;
        invalidate();
    }

    public void setTranslateX(int translateX) {
        this.translateX = translateX;
    }

    public void setProgress(final int progress, int duration) {
        if (progress > 100 || progress < 0){
            throw new RuntimeException(getClass().getName() + "请设置[0,100]之间的值");
        }
        autoIncrement = false;
        openAnimate = true;
        ValueAnimator progressAnimator = ValueAnimator.ofInt(this.progress, progress);
        progressAnimator.setDuration(duration);
        progressAnimator.setTarget(progress);
        progressAnimator.setInterpolator(new LinearInterpolator());
        progressAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                WaterWaveView.this.progress = (int) animation.getAnimatedValue();
                if (WaterWaveView.this.progress == progress){
                    openAnimate = false;
                }
                postInvalidateDelayed(delayTime);
            }
        });
        progressAnimator.start();
    }

    public int getProgress() {
        return progress;
    }
}
