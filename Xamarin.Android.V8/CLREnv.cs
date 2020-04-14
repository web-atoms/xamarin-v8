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
    public struct CLREnv
    {

        public IntPtr freeMemory;
        public IntPtr externalCall;
        public IntPtr debug;

    }
}