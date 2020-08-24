using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WebAtoms;
using WebAtoms.V8Sharp;

namespace V8SharpTest
{
    class Program
    {
        static void Main(string[] args)
        {

            try
            {
                using (JSContext context = new JSContext(false, true))
                {
                    var name = context.EvaluateTemplate($"`${{{"Akash"}}} ${{{"Kava"}}}`");
                    Console.WriteLine(name.ToString());

                    context["add"] = context.CreateFunction(0, (c, a) => c.CreateString($"{a[0]} - {a[1]}") , "Add");

                    name = context.EvaluateTemplate($"add({"Akash"},{"Kava"})");
                    Console.WriteLine(name.ToString());

                    var list = new string[][] {
                        new string [] { "a1", "b1"},
                        new string [] { "c1", "d1" }
                    }.Select(x =>
                    {
                        var a = x[0];
                        var b = x[1];
                        return Task.FromResult(context.EvaluateTemplate($"add({a},{b})").ToString());
                    }).ToList();

                    var result = Task.WhenAll(list).Result;
                    foreach(var r in result)
                    {
                        Console.WriteLine(r);
                    }
                }
            }catch(Exception ex) {
                Console.WriteLine(ex);
            }

            Console.ReadLine();

        }
    }
}
