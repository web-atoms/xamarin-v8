using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.ConstrainedExecution;
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

    internal class V8ContextHandle
    {
        private IntPtr value;

        public void Clear()
        {
            this.value = IntPtr.Zero;
        }

        public bool IsDisposed => this.value == IntPtr.Zero;

        public static implicit operator V8ContextHandle(IntPtr v)
        {
            return new V8ContextHandle { value = v };
        }

        public static implicit operator IntPtr(V8ContextHandle v)
        {
            if (v.value == IntPtr.Zero)
            {
                throw new ObjectDisposedException("Context has been disposed");
            }
            return v.value;
        }

    }

    //public class SafeV8Handle : Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid
    //{
    //    /// <summary>Initializes a new instance of the <see cref="SafeV8Handle" /> class, specifying whether the handle is to be reliably released.</summary>
    //    /// <param name="ownsHandle">
    //    /// <see langword="true" /> to reliably release the handle during the finalization phase; <see langword="false" /> to prevent reliable release (not recommended).</param>
    //    [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
    //    protected SafeV8Handle(bool ownsHandle) : base(ownsHandle)
    //    {

    //    }

    //    protected override bool ReleaseHandle()
    //    {
    //        if (!this.IsInvalid)
    //        {

    //        }
    //        return true;
    //    }
    //}
}