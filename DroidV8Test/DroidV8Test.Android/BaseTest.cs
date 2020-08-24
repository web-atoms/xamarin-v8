using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using Android.App;
using Android.Content;
using Android.OS;
using Android.Runtime;
using Android.Views;
using Android.Widget;
using WebAtoms.V8Sharp;

namespace DroidV8Test.Droid
{

    [AttributeUsage(AttributeTargets.Method)]
    public class TestAttribute: Attribute
    {
        public string Name { get; }
        public TestAttribute(string name = null)
        {
            this.Name = name;
        }
    }

    public class TestResult
    {
        public string TestName { get; set; }

        public bool Success { get; set; }

        public string Log { get; set; }
    }

    public class BaseTest: IDisposable
    {

        public static async Task RunAll(bool print = true)
        {
            try
            {
                var t = typeof(BaseTest);
                var allTests = AppDomain.CurrentDomain.GetAssemblies()
                    .Where(x => !x.IsDynamic)
                    .SelectMany(x => x.GetExportedTypes())
                    .Where(x => x != t && t.IsAssignableFrom(x))
                    .Select(x => (IEnumerable<(Type type, MethodInfo method, TestAttribute attribute)>)
                        x.GetRuntimeMethods()
                            .Select(m => 
                                (x, m, m.GetCustomAttribute<TestAttribute>()))
                    ).SelectMany(x => x)
                    .Where(x => x.attribute != null);

                var all = allTests.Select(m => Task.Run(() => Run(m.type, m.method, m.attribute))).ToList();

                var result = await Task.WhenAll(all);
                StringBuilder sb = new StringBuilder();
                foreach (var r in result.OrderBy(x => x.Success))
                {
                    if (r.Success)
                    {
                        sb.AppendLine($"{r.TestName} Success");
                    }
                    else
                    {
                        sb.AppendLine($"{r.TestName} Failed");
                        foreach (var line in r.Log.Split("\n"))
                        {
                            sb.AppendLine($"\t\t{line}");
                        }
                    }
                }

                string log = sb.ToString();
                TestResultModel.Model.Results = log;
                if (print)
                {
                    System.Diagnostics.Debug.WriteLine(log);
                }

            } catch(Exception ex)
            {
                System.Diagnostics.Debug.WriteLine(ex);
            }
        }

        private readonly static object[] Empty = new object[] { };

        private static async Task<TestResult> Run(Type type, MethodInfo method, TestAttribute attribute)
        {
            var testName = attribute.Name ?? method.Name;
            try
            {
                using (var t = Activator.CreateInstance(type) as IDisposable)
                {
                    if (method.ReturnType == typeof(void))
                    {
                        method.Invoke(t, Empty);
                    }
                    else
                    {
                        await (Task)method.Invoke(t, Empty);
                    }
                    return new TestResult
                    {
                        Success = true,
                        TestName = testName
                    };
                }
            } catch (Exception ex)
            {
                return new TestResult {
                    Success = false,
                    TestName = $"{type.Name}: {testName}",
                    Log = ex.ToString()
                };
            }
        }

        public void Dispose()
        {
            context.Dispose();
        }

        protected readonly JSContext context;

        public BaseTest()
        {
            context = new JSContext();
        }
                

    }
}