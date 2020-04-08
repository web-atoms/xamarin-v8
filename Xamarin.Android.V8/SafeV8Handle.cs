using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

using Android.App;
using Android.Content;
using Android.OS;
using Android.Runtime;
using Android.Views;
using Android.Widget;

namespace Xamarin.Android.V8
{
    public class SafeV8Handle : SafeHandle
    {

        private bool _isInvalid = false;

        public SafeV8Handle()
        {

        }

        public SafeV8Handle(IntPtr invalidHandleValue, bool ownsHandle) : base(invalidHandleValue, ownsHandle)
        {
            this._isInvalid = true;
        }

        public override bool IsInvalid => _isInvalid;

        protected override bool ReleaseHandle()
        {
            throw new NotImplementedException();
        }
    }
}