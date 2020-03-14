using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace Xamarin.Android.V8
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct V8HandleContainer
    {
        internal IntPtr handle;
        internal V8HandleType handleType;
        internal IntPtr value;
    }
}