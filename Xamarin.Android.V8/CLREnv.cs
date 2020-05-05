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
    [StructLayout(LayoutKind.Sequential)]
    internal struct CLREnv
    {
        // [MarshalAs(UnmanagedType.FunctionPtr)]
        public Delegate allocateMemory;
        // [MarshalAs(UnmanagedType.FunctionPtr)]
        public Delegate freeMemory;

        // this is for wrapped objects
//         [MarshalAs(UnmanagedType.FunctionPtr)]
        public Delegate freeHandle;
   //      [MarshalAs(UnmanagedType.FunctionPtr)]
        public Delegate externalCall;
      //  [MarshalAs(UnmanagedType.FunctionPtr)]
        public Delegate logger;
       // [MarshalAs(UnmanagedType.FunctionPtr)]
        public Delegate WaitForDebugMessageFromProtocol;
      //  [MarshalAs(UnmanagedType.FunctionPtr)]
        public Delegate SendDebugMessageToProtocol;
      //  [MarshalAs(UnmanagedType.FunctionPtr)]
        public Delegate fatalErrorCallback;

       // [MarshalAs(UnmanagedType.FunctionPtr)]
        public Delegate breakPauseOn;

    }

}