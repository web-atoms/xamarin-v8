using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;

namespace System
{
    public static class QueueExtensions
    {
        public static bool TryDequeue<T>(this Queue<T> queue, out T value)
        {
            if (queue.Count > 0)
            {
                value = queue.Dequeue();
                return true;
            }
            value = default;
            return false;
        }

    }

}

namespace WebAtoms.V8Sharp
{
    public class MainThread
    {
        public static object PostTimeout(Action action, long delay)
        {
            throw new NotImplementedException();
        }
        public static void InvokeOnMainThreadAsync(Action action)
        {
            throw new NotImplementedException();
        }
        public static void BeginInvokeOnMainThread(Action action)
        {
            throw new NotImplementedException();
        }
        public static bool IsMainThread => SynchronizationContext.Current != null;

    }
}
