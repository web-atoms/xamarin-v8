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
using WebAtoms.V8Sharp;

namespace DroidV8Test.Droid.Tests
{
    public class GCTest: BaseTest
    {

        [Test]
        public void GC1()
        {
            WeakReference r = null;
            using(var jc = new JSContext())
            {
                var a = new { };
                r = new WeakReference(a);
                jc["a"] = jc.Wrap(a);
                a = null;
            }

            System.GC.Collect();
            System.GC.WaitForPendingFinalizers();

            // Assert.False(r.IsAlive);
        }

    }
}