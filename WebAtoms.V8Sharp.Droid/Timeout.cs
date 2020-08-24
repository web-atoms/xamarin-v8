using Android.OS;
using System;
using System.Linq;

namespace WebAtoms.V8Sharp
{
    internal class Timeout : Java.Lang.Object, Java.Lang.IRunnable, IDisposable
    {
        private readonly Action action;
        private Handler handler;

        public Timeout(Handler handler, Action action, long milliSeconds)
        {
            this.action = action;
            handler.PostDelayed(this, milliSeconds);
        }

        public void Run()
        {
            action();
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                handler?.RemoveCallbacks(this);
                handler = null;
            }
            base.Dispose(disposing);
        }
    }
}