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
        public Int64 longValue;

        [FieldOffset(0)]
        public double doubleValue;

        [FieldOffset(0)]
        public bool booleanValue;

        [FieldOffset(0)]
        public int intValue;

        [FieldOffset(0)]
        public IntPtr refValue;

    }

    //[StructLayout(LayoutKind.Sequential)]
    //public struct ArrayValue
    //{
    //    [MarshalAs(UnmanagedType.I4)]
    //    public int Length;

    //    public IntPtr stringValue;

    //    public IntPtr arrayValue;

    //    internal unsafe JSValue[] ToJSValueArray(JSContext context)
    //    {
    //        V8Response* p = (V8Response*)arrayValue;
    //        JSValue[] result = new JSValue[Length];
    //        for (int i = 0; i < Length; i++)
    //        {
    //            var r = p[i];
    //            result[i] = new JSValue(context, r.GetContainer());
    //        }
    //        return result;
    //    }

    //    internal unsafe string String
    //    {
    //        get
    //        {
    //            char* c = (char*)stringValue;
    //            return new String(c, 0, Length);
    //        }
    //    }
    //}

    [StructLayout(LayoutKind.Sequential)]
    internal struct V8Response
    {
        private int type;

        public int length;

        public IntPtr address;

        public V8ResponseResult result;

        public V8HandleType Type
        {
            get => (V8HandleType)type;
            set => type = (int)value;
        }

        public unsafe string StringValue
        {
            get
            {
                char* c = (char*)address;
                return new String(c, 0, length);

            }
        }

        internal unsafe JSValue[] ToJSValueArray(JSContext context)
        {
            V8Response* p = (V8Response*)address;
            JSValue[] result = new JSValue[length];
            for (int i = 0; i < length; i++)
            {
                var r = p[i];
                result[i] = new JSValue(context, r);
            }
            return result;
        }
    }
}