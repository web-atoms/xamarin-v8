﻿using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace Xamarin.Android.V8
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct V8Error
    {
        public IntPtr message;
        public IntPtr stack;
    }
}