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
    public class SimpleTest: BaseTest
    {

        [Test]
        public void Test1()
        {
            var a = context.Evaluate("4 + 5");
            Assert.True(a.IsNumber);
            Assert.Equal(a.IntValue, 9);
        }

        [Test]
        public void Test2GlobalValue()
        {
            context["n5"] = context.CreateNumber(5);
            var a = context.Evaluate("4 + n5");
            Assert.True(a.IsNumber);
            Assert.Equal(a.IntValue, 9);
        }

        [Test]
        public void Test2GlobalObjectValue()
        {
            context["n5"] = context.CreateNumber(5);
            var a = context.Evaluate("4 + global.n5");
            Assert.True(a.IsNumber);
            Assert.Equal(a.IntValue, 9);
        }

        [Test]
        public void StringTest()
        {
            context["firstName"] = context.CreateString("Akash");
            context["lastName"] = context.CreateString("Kava");
            var a = context.Evaluate("`${firstName} ${lastName}`");
            Assert.True(a.IsString);
            Assert.Equal("Akash Kava", a.ToString());
        }

    }
}