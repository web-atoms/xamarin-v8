using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace Xamarin.Android.V8
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct Utf16Value
    {
        [MarshalAs(UnmanagedType.LPWStr)]
        internal string Value;

        internal int Length;

        public static implicit operator Utf16Value(string value) => new Utf16Value { Value = value, Length = value?.Length ?? 0};
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct V8ResponseResult
    {
        [FieldOffset(0)]
        [MarshalAs(UnmanagedType.Struct)]
        public V8HandleContainer handle;

        [FieldOffset(0)]
        public V8Error error;

        [FieldOffset(0)]
        public Int64 longValue;

        //[FieldOffset(0)]
        //public IntPtr stringValue;

        [FieldOffset(0)]
        public bool booleanValue;

        [FieldOffset(0)]
        public int intValue;

        [FieldOffset(0)]
        public long AllValue;

    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct V8Response
    {
        [MarshalAs(UnmanagedType.SysInt)]
        public V8ResponseType type;

        [MarshalAs(UnmanagedType.LPWStr)]
        public string stringValue;

        public V8ResponseResult result;


    }
}