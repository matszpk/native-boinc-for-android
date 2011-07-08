/*
 * Copyright (c) 2010, Hal Blackburn
 * 
 * Permission to use, copy, modify, and/or distribute this software for any 
 * purpose with or without fee is hereby granted, provided that the above 
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES 
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR 
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
package hal.android.workarounds;

import android.R;
import android.app.ProgressDialog;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.widget.ProgressBar;

/**
 * A subclass of ProgressDialog which fixes 
 * http://code.google.com/p/android/issues/detail?id=4266 (Animation in 
 * indeterminate ProgressDialog freezes in Donut).
 * <p>
 * Usage:
 * <pre>
 * // in an Activity subclass for example
 * protected Dialog onCreateDialog(int id)
 * {
 *     ....
 *     // Create a FixedProgressDialog wherever you would create a 
 *     // ProgressDialog then forget that it isn't a standard ProgressDialog.
 *     ProgressDialog d = new FixedProgressDialog(this);
 *     ....
 *     return d;
 * }
 * </pre>
 * 
 * @author Hal Blackburn http://helios.hud.ac.uk/u0661162
 */
public class FixedProgressDialog extends ProgressDialog
{
        /** The progress bar hosted by the dialog we're subclassing. */
        private ProgressBar mProgress;
        
        /** See {@link ProgressDialog#ProgressDialog(Context, int)}. */
        public FixedProgressDialog(Context context, int theme)
        { super(context, theme); }

        /** See {@link ProgressDialog#ProgressDialog(Context)}. */
        public FixedProgressDialog(Context context)
        { super(context); }
        
        @Override
        protected void onCreate(Bundle savedInstanceState)
        {
                super.onCreate(savedInstanceState);
                
                // Get a handle on our subclassed ProgressDialog's ProgressBar
                View progressBar = findViewById(R.id.progress);
                if(progressBar instanceof ProgressBar) // this also checks for null
                        mProgress = (ProgressBar)progressBar;
        }
        
        // Called whenever the dialog is shown
        @Override public void onStart()
        {
                super.onStart();
                
                if(isIndeterminate() && mProgress != null)
                {
                        // Remove the spinner from the layout and then re add it by 
                        // setting it's visibility to gone then making it visible again.
                        // This makes the progress bar animate again.
                        mProgress.setVisibility(View.GONE);
                        mProgress.setVisibility(View.VISIBLE);
                }
        }
}
