using System;
using System.Collections.Generic;
using System.Linq;
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
    internal static class DelegateHelper
    {

        public unsafe static string ToUtf16String(this IntPtr text, int length)
        {
            char* charArray = (char*)text;
            return new String(charArray, 0, length);
        }

        public static IntPtr ToIntPtr<T>(this T item) where T: Delegate
        {
            // var handle = GCHandle.Alloc(item);
            // return GCHandle.ToIntPtr(handle);
            IntPtr value = Marshal.GetFunctionPointerForDelegate((Delegate)item);
            return value;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct CLREnv
    {
        // [MarshalAs(UnmanagedType.FunctionPtr)]
        public IntPtr allocateMemory;
        // [MarshalAs(UnmanagedType.FunctionPtr)]
        public IntPtr freeMemory;

        // this is for wrapped objects
//         [MarshalAs(UnmanagedType.FunctionPtr)]
        public IntPtr freeHandle;
   //      [MarshalAs(UnmanagedType.FunctionPtr)]
        public IntPtr externalCall;
      //  [MarshalAs(UnmanagedType.FunctionPtr)]
        public IntPtr logger;
       // [MarshalAs(UnmanagedType.FunctionPtr)]
        public IntPtr WaitForDebugMessageFromProtocol;
      //  [MarshalAs(UnmanagedType.FunctionPtr)]
        public IntPtr SendDebugMessageToProtocol;
      //  [MarshalAs(UnmanagedType.FunctionPtr)]
        public IntPtr fatalErrorCallback;

       // [MarshalAs(UnmanagedType.FunctionPtr)]
        public IntPtr breakPauseOn;

    }

}