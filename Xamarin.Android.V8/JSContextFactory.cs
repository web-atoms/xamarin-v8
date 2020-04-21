using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Android.App;
using Android.Content;
using Android.OS;
using Android.Runtime;
using Android.Views;
using Android.Widget;
using WebAtoms;

namespace Xamarin.Android.V8
{
    public class JSContextFactory : IJSContextFactory
    {
        public IJSContext Create()
        {
            return new JSContext(true);
        }
    }
}