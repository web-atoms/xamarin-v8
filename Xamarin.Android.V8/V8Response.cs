using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace Xamarin.Android.V8
{
    [StructLayout(LayoutKind.Explicit)]
    internal struct V8Response
    {
        [FieldOffset(0)]
        internal V8ResponseType type;

        [FieldOffset(2)]
        internal V8HandleContainer handle;

        [FieldOffset(2)]
        internal V8Error error;

        [FieldOffset(2)]
        internal long longValue;

        [FieldOffset(2)]
        internal IntPtr stringValue;
    }
}