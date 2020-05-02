using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace Xamarin.Android.V8
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct V8HandleContainer
    {
        [MarshalAs(UnmanagedType.I4)]
        public V8HandleType handleType;

        public V8Value value;

        public IntPtr handle;
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct V8Value
    {
        [FieldOffset(0)]
        public bool boolValue;

        [FieldOffset(0)]
        public int intValue;

        [FieldOffset(0)]
        public Int64 longValue;

        [FieldOffset(0)]
        public double doubleValue;

        [FieldOffset(0)]
        public IntPtr refValue;

    }
}