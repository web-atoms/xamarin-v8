using System;
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

namespace WebAtoms.V8Sharp
{
    internal class AtomAsyncDispatcher : IDisposable
    {
        public static AtomAsyncDispatcher Instance = new AtomAsyncDispatcher();

        private Queue<Func<Task>> tasks = new Queue<Func<Task>>();

        private System.Threading.CancellationTokenSource cancellation;

        public AtomAsyncDispatcher()
        {
            Task.Run(() => Run());
        }

        private async Task Run()
        {
            while (true)
            {
                Func<Task> task = null;
                lock (tasks)
                {
                    if(tasks.TryDequeue(out task))
                    {
                        cancellation = null;
                    } else
                    {
                        cancellation = new System.Threading.CancellationTokenSource();
                    }
                }



                if (task != null)
                {
                    try
                    {
                        await task();
                    }catch (Exception ex)
                    {
                        System.Diagnostics.Debug.WriteLine(ex);
                    }
                    continue;
                }

                // setup wait...
                try
                {
                    await Task.Delay(1000, cancellation.Token);
                } catch (TaskCanceledException)
                {

                }

            }
        }

        public void EnqueueTask(Func<Task> task)
        {
            lock (tasks)
            {
                tasks.Enqueue(task);
                cancellation?.Cancel();
            }
        }

        public void Dispose()
        {

        }
    }
}