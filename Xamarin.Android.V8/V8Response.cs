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

        [FieldOffset(0)]
        public bool booleanValue;

        [FieldOffset(0)]
        public int intValue;

        [FieldOffset(0)]
        public ArrayValue array;

    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ArrayValue
    {
        public int Length;

        public IntPtr stringValue;

        public IntPtr arrayValue;

        internal unsafe JSValue[] ToJSValueArray(JSContext context)
        {
            uint* p = (uint*)arrayValue;
            JSValue[] result = new JSValue[Length];
            for (int i = 0; i < Length; i++)
            {
                IntPtr p1 = (IntPtr)p[i];
                var ri = Marshal.PtrToStructure<V8Response>(p1);
                p += IntPtr.Size;
                result[i] = new JSValue(context, ri.GetContainer());
            }
            return result;
        }

        internal unsafe string String
        {
            get
            {
                char* c = (char*)stringValue;
                return new String(c, 0, Length);
            }
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct V8Response
    {
        [MarshalAs(UnmanagedType.SysInt)]
        public V8ResponseType type;

        public V8ResponseResult result;

    }
}