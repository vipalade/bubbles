package com.example.vapa.bubbles;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.Drawable;
import android.text.TextPaint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

/**
 * TODO: document your custom view class.
 */
public class BubblesView extends View {
    private Paint mCirclePaint = new Paint();
    private boolean mHasTouch = false;
    private BubblesActivity activity;
    private int pointer_x = 0;
    private int pointer_y = 0;

    private int gui_pointer_x = 0;
    private int gui_pointer_y = 0;

    public BubblesView(Context context) {
        super(context);
        init(null, 0);
    }

    public BubblesView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs, 0);
    }

    public BubblesView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(attrs, defStyle);
    }

    public synchronized void setAutoPointerPos(int _x, int _y){
        if(!mHasTouch){
            pointer_x = _x;
            pointer_y = _y;
            activity.nativeMove(_x, _y);
        }
    }

    private synchronized void setPointerPos(int _x, int _y){
        if(mHasTouch){
            pointer_x = _x;
            pointer_y = _y;
        }
    }

    private synchronized void getPointerPos(){
        gui_pointer_x = pointer_x;
        gui_pointer_y = pointer_y;
    }

    public void setActivity(BubblesActivity _act){
        activity = _act;
    }

    private void init(AttributeSet attrs, int defStyle) {
        // Load attributes

    }

    private void invalidateTextPaintAndMeasurements() {

    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        getPointerPos();
        activity.nativePlotStart();
        int my_color = activity.nativePlotMyColor();

        canvas.translate(canvas.getWidth()/2, canvas.getHeight()/2);

        while(!activity.nativePlotEnd()) {
            int x = activity.nativePlotX();
            int y = activity.nativePlotY();
            int c = activity.nativePlotColor();

            drawCircle(canvas, x, y, c, 10);

            activity.nativePlotNext();
        }
        activity.nativePlotDone();
        drawCircle(canvas, gui_pointer_x, gui_pointer_y, my_color, 20);
    }

    void drawCircle(Canvas canvas, int x, int y, int c, int r){
        mCirclePaint.setColor(c);
        int nx = activity.nativeScaleX(x, this.getWidth());
        int ny = activity.nativeScaleY(y, this.getHeight());
        canvas.drawCircle(nx, ny, r, mCirclePaint);
    }

    @Override
    protected void onSizeChanged (
                        int w,
                        int h,
                        int oldw,
                        int oldh
    ){
        //activity.nativeSetFrame(w, h);
    }

    // BEGIN_INCLUDE(onTouchEvent)
    @Override
    public boolean onTouchEvent(MotionEvent event) {

        final int action = event.getAction();

        /*
         * Switch on the action. The action is extracted from the event by
         * applying the MotionEvent.ACTION_MASK. Alternatively a call to
         * event.getActionMasked() would yield in the action as well.
         */
        switch (action & MotionEvent.ACTION_MASK) {

            case MotionEvent.ACTION_DOWN: {
                // first pressed gesture has started
                int x = activity.nativeReverseScaleX((int)(event.getX(0) - this.getWidth()/2), this.getWidth());
                int y = activity.nativeReverseScaleY((int)(event.getY(0) - this.getHeight()/2), this.getHeight());
                activity.nativeMove(x, y);
                mHasTouch = true;
                setPointerPos(x, y);
                break;
            }

            case MotionEvent.ACTION_POINTER_DOWN: {
                /*
                 * A non-primary pointer has gone down, after an event for the
                 * primary pointer (ACTION_DOWN) has already been received.
                 */

                /*
                 * The MotionEvent object contains multiple pointers. Need to
                 * extract the index at which the data for this particular event
                 * is stored.
                 */

                break;
            }

            case MotionEvent.ACTION_UP: {
                /*
                 * Final pointer has gone up and has ended the last pressed
                 * gesture.
                 */

                /*
                 * Extract the pointer identifier for the only event stored in
                 * the MotionEvent object and remove it from the list of active
                 * touches.
                 */

                int x = activity.nativeReverseScaleX((int)(event.getX(0) - this.getWidth()/2), this.getWidth());
                int y = activity.nativeReverseScaleY((int)(event.getY(0) - this.getHeight()/2), this.getHeight());
                activity.nativeMove(x, y);
                setPointerPos(x, y);

                mHasTouch = false;

                break;
            }

            case MotionEvent.ACTION_POINTER_UP: {
                /*
                 * A non-primary pointer has gone up and other pointers are
                 * still active.
                 */

                /*
                 * The MotionEvent object contains multiple pointers. Need to
                 * extract the index at which the data for this particular event
                 * is stored.
                 */


                break;
            }

            case MotionEvent.ACTION_MOVE: {
                /*
                 * A change event happened during a pressed gesture. (Between
                 * ACTION_DOWN and ACTION_UP or ACTION_POINTER_DOWN and
                 * ACTION_POINTER_UP)
                 */

                /*
                 * Loop through all active pointers contained within this event.
                 * Data for each pointer is stored in a MotionEvent at an index
                 * (starting from 0 up to the number of active pointers). This
                 * loop goes through each of these active pointers, extracts its
                 * data (position and pressure) and updates its stored data. A
                 * pointer is identified by its pointer number which stays
                 * constant across touch events as long as it remains active.
                 * This identifier is used to keep track of a pointer across
                 * events.
                 */
                int x = activity.nativeReverseScaleX((int)(event.getX(0) - this.getWidth()/2), this.getWidth());
                int y = activity.nativeReverseScaleY((int)(event.getY(0) - this.getHeight()/2), this.getHeight());
                activity.nativeMove(x, y);
                setPointerPos(x, y);

                break;
            }
        }

        // trigger redraw on UI thread
        this.postInvalidate();

        return true;
    }
}
