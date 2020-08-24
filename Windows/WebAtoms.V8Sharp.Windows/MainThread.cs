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
    public interface IMainThread
    {
        bool IsMainThread { get; }
        IDisposable PostTimeout(Action action, long delay);
        void InvokeOnMainThreadAsync(Action action);
        void BeginInvokeOnMainThread(Action action);

    }

    public static class DictionaryExtensions
    {
        public static bool Remove<TKey, TValue>(this Dictionary<TKey, TValue> d, TKey key, out TValue value)
        {
            if (d.TryGetValue(key, out value))
            {
                return d.Remove(key);
            }
            return false;
        }
    }

    public class MainThread
    {

        public static IMainThread Instance;

        public static IDisposable PostTimeout(Action action, long delay)
        {
            return Instance.PostTimeout(action, delay);
        }
        public static void InvokeOnMainThreadAsync(Action action)
        {
            Instance.InvokeOnMainThreadAsync(action);
        }
        public static void BeginInvokeOnMainThread(Action action)
        {
            Instance.BeginInvokeOnMainThread(action);
        }
        public static bool IsMainThread => Instance.IsMainThread;

    }
}
