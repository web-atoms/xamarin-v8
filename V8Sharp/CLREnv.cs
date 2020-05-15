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
        public IntPtr allocateMemory;
        public IntPtr allocateString;
        public IntPtr freeMemory;

        // this is for wrapped objects
        public IntPtr freeHandle;
        public IntPtr externalCall;
        public IntPtr logger;
        public IntPtr WaitForDebugMessageFromProtocol;
        public IntPtr SendDebugMessageToProtocol;
        public IntPtr fatalErrorCallback;

        public IntPtr breakPauseOn;

    }

}