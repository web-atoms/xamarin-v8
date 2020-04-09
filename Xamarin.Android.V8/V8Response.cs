using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace Xamarin.Android.V8
{
    [StructLayout(LayoutKind.Explicit)]
    internal struct V8Response
    {
        [FieldOffset(0), MarshalAs(UnmanagedType.U1)]
        public V8ResponseType type;

        [FieldOffset(4), MarshalAs(UnmanagedType.Struct)]
        public V8HandleContainer handle;

        [FieldOffset(4)]
        public V8Error error;

        [FieldOffset(4)]
        public Int64 longValue;

        [FieldOffset(4)]
        public IntPtr stringValue;

        [FieldOffset(4)]
        public bool booleanValue;

        [FieldOffset(4)]
        public int intValue;

    }
}