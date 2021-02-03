using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
//using Android.App;
//using Android.Content;
//using Android.OS;
//using Android.Runtime;
//using Android.Views;
//using Android.Widget;

namespace Xamarin.Android.V8
{
    internal class AtomAsyncDispatcher : IDisposable
    {
        public static AtomAsyncDispatcher Instance = new AtomAsyncDispatcher();

        private BlockingCollection<Func<Task>> tasks = new BlockingCollection<Func<Task>>();

        public AtomAsyncDispatcher()
        {
            Task.Run(Run);
        }

        private async Task Run()
        {
            foreach (var item in tasks.GetConsumingEnumerable())
            {
                await item();
            }
        }

        public void EnqueueTask(Func<Task> task)
        {
            tasks.Add(task);
        }

        public void Dispose()
        {
            tasks.CompleteAdding();
        }
    }
}