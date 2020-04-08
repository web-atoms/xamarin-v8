using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace Xamarin.Android.V8
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct V8HandleContainer
    {
        internal V8HandleType handleType;
        internal IntPtr handle;
        internal V8Value value;
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct V8Value
    {
        [FieldOffset(0)]
        internal bool boolValue;

        [FieldOffset(0)]
        internal int intValue;

        [FieldOffset(0)]
        internal long longValue;

        [FieldOffset(0)]
        internal double doubleValue;

    }
}