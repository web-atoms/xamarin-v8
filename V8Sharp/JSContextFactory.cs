using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

//using Android.App;
//using Android.Content;
//using Android.OS;
//using Android.Runtime;
//using Android.Views;
//using Android.Widget;
using WebAtoms;

namespace Xamarin.Android.V8
{
    public class JSContextFactory : IJSContextFactory
    {
        public IJSContext Create()
        {
            return new JSContext(true);
        }

        public IJSContext Create(Uri inverseWebSocketUri)
        {
            return new JSContext(inverseWebSocketUri);
        }

        //public JSName CreateName(string name)
        //{
        //    // var key = JSContext.CreateGlobalString(name);
        //    return new JSName(new JSNameKey(0),name);
        //}
    }
}