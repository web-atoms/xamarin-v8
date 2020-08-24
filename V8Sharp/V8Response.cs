using System;
// using System.Buffers;
using System.Linq;
using System.Runtime.InteropServices;

namespace WebAtoms.V8Sharp
{


    [StructLayout(LayoutKind.Sequential)]
    internal struct Utf16IntPtr
    {
        internal IntPtr Value;

        internal int Length;

        internal IntPtr Handle;

        public static implicit operator Utf16IntPtr(string value) {
            // Value = Marshal.StringToHGlobalUni(value),
            // Length = value?.Length ?? 0

            if (string.IsNullOrEmpty(value))
            {
                return new Utf16IntPtr { Value = IntPtr.Zero, Handle = IntPtr.Zero };
            }
            var h = GCHandle.Alloc(value, GCHandleType.Pinned);
            var p = h.AddrOfPinnedObject();

            return new Utf16IntPtr {
                Value = p,
                Handle = GCHandle.ToIntPtr(h),
                Length = value?.Length ?? 0
            };
        }
    }


    [StructLayout(LayoutKind.Sequential)]
    internal struct Utf16Value
    {

        internal IntPtr Value;

        internal int Length;

        internal IntPtr Handle;

        internal int unusedValue;

        public unsafe static implicit operator Utf16Value(string value) {
            if (string.IsNullOrEmpty(value))
            {
                return new Utf16Value { Value = IntPtr.Zero, Handle = IntPtr.Zero };
            }
            var h = GCHandle.Alloc(value, GCHandleType.Pinned);
            var p = h.AddrOfPinnedObject();

            return new Utf16Value {
                Value = p,
                Handle = GCHandle.ToIntPtr(h),
                Length = value?.Length ?? 0
            };
        }
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct V8ResponseResult
    {

        [FieldOffset(0)]
        public Int64 longValue;

        [FieldOffset(0)]
        public double doubleValue;

        public bool booleanValue => intValue > 0;

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
                if (!(Type == V8HandleType.Error 
                    || Type == V8HandleType.ConstError 
                    || Type == V8HandleType.CharArray
                    || Type == V8HandleType.ConstCharArray))
                    return null;
                if (address == IntPtr.Zero)
                {
                    if (result.booleanValue)
                    {
                        return string.Empty;
                    }
                    return null;
                }
                var gc = GCHandle.FromIntPtr(address);
                string value = (string)gc.Target;
                // free only if it was not const...
                if (Type == V8HandleType.CharArray || Type == V8HandleType.Error)
                    gc.Free();
                return value;
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

        public static implicit operator V8Response(Exception ex)
        {
            string error = ex.ToString();
            IntPtr ptr = Marshal.StringToHGlobalUni(error);
            return new V8Response {
                Type = V8HandleType.Error,
                address = ptr,
                length = error.Length
            };
        }
    }
}