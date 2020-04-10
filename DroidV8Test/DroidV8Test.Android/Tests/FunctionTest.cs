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

namespace DroidV8Test.Droid.Tests
{
    public class FunctionTest: BaseTest
    {

        [Test]
        public void Function()
        {
            context["clrFunction"] = context.CreateFunction(0, (c, a) => c.Convert("Test"), "T1");
            var a = context.Evaluate("clrFunction()");
            Assert.True(a.IsString);
            Assert.Equal("Test", a.ToString());
        }

    }
}