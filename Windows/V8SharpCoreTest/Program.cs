using System;
using System.Linq;
using System.Threading.Tasks;
using WebAtoms;
using WebAtoms.V8Sharp;

namespace V8SharpCoreTest
{
    class Program
    {
        async static Task Main(string[] args)
        {
            try
            {
                using (JSContext context = new JSContext(false, false))
                {
                    var name = context.EvaluateTemplate($"`${{{"Akash"}}} ${{{"Kava"}}}`");
                    Console.WriteLine(name.ToString());

                    context["add"] = context.CreateFunction(0, (c, a) => c.CreateString($"{a[0]} - {a[1]}"), "Add");

                    name = context.EvaluateTemplate($"add({"Akash"},{"Kava"})");
                    Console.WriteLine(name.ToString());

                    var r = await Task.Run(async () =>
                    {
                        await Task.Delay(10);
                        return context.EvaluateTemplate($"add({"Akash"},{"Kava"})");
                    }); ; ;

                    Console.WriteLine(r);

                    //var list = new string[][] {
                    //    new string [] { "a1", "b1"},
                    //    new string [] { "c1", "d1" }
                    //}.Select(async x =>
                    //{
                    //    await Task.Delay(100);
                    //    var a = context.CreateString(x[0]);
                    //    var b = context.CreateString(x[1]);
                    //    return context.EvaluateTemplate($"add({a},{b})").ToString();
                    //}).ToList();

                    //var result = await Task.WhenAll(list);
                    //foreach (var r in result)
                    //{
                    //    Console.WriteLine(r);
                    //}
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
            }

            Console.ReadLine();

        }
    }
}
