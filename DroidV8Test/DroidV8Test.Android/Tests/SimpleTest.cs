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
    public class SimpleTest : BaseTest
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


        [Test]
        public void SymbolTest()
        {
            var s = context.CreateSymbol("a");
            var a = (JSValue)context.CreateObject();
            a[s] = context.CreateString("Akash");
            context["a"] = a;
            context["s"] = s;
            var r = context.Evaluate("a[s]");
            Assert.Equal("Akash", r.ToString());
        }

        [Test]
        public void DefineDeleteTest()
        {
            var a = context.CreateObject();
            context["a"] = a;
            a.DefineProperty("id", new WebAtoms.JSPropertyDescriptor {
                Enumerable = true,
                Configurable = true,
                Value = context.CreateNumber(5)
            });

            var n = context.Evaluate("a.id");
            Assert.Equal(5, n.IntValue);

            if(!a.DeleteProperty("id"))
            {
                Assert.Throw("Not possible");
            }

            if(a.HasProperty("id"))
            {
                Assert.Throw("Not possible");
            }

        }
    }
}