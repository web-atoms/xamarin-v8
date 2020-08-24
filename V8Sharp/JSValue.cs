using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using WebAtoms;
using V8Handle = System.IntPtr;
using WebAtoms.V8Sharp;

namespace WebAtoms.V8Sharp
{
    public class JSValue: IJSValue
    {
        readonly JSContext jsContext;
        readonly V8ContextHandle context;
        internal V8Response handle;
        private string cachedString;
        internal JSValue(JSContext context, V8Response r)
        {
            this.jsContext = context;
            this.context = context.context;
            r.ThrowError();
            this.handle = r;
        }

        public IJSValue CreateNewInstance(params IJSValue[] args) {
            var r = JSContext.V8Context_NewInstance(context, handle.address, args.Length, args.ToHandles(jsContext));
            return new JSValue(jsContext, r);
        }

        public IJSContext Context => jsContext;

        public bool IsValueNull => this.handle.Type == V8HandleType.Null;

        public bool IsUndefined => this.handle.Type == V8HandleType.Undefined;

        public bool IsString => this.handle.Type == V8HandleType.String;

        public bool IsObject => (handle.Type & V8HandleType.Object) > 0;

        public bool IsFunction => handle.Type == V8HandleType.Function;

        public bool IsDate => handle.Type == V8HandleType.Date;

        public bool IsNumber => handle.Type == V8HandleType.Number 
            || handle.Type == V8HandleType.NotANumber
            || handle.Type == V8HandleType.Integer
            || handle.Type == V8HandleType.BigInt;

        public bool IsBoolean => handle.Type == V8HandleType.Boolean;

        public bool IsArray => handle.Type == V8HandleType.Array;

        public bool IsWrapped => 
            handle.Type == V8HandleType.Wrapped
            || ((handle.Type & V8HandleType.Object) > 0 && Has(jsContext.WrappedSymbol));

        public bool IsSymbol => handle.Type == V8HandleType.TypeSymbol;

        public IJSValue this[string name]
        {
            get
            {
                return new JSValue(jsContext, JSContext.V8Context_GetProperty(context, handle.address, name));
            }
            set
            {
                JSContext.V8Context_SetProperty(context, handle.address, name, value.ToHandle(jsContext));
            }
        }

        public IJSValue this[IJSValue name]
        {
            get
            {
                return new JSValue(jsContext, JSContext.V8Context_Get(context, handle.address, name.ToHandle(jsContext)));
            }
            set
            {
                JSContext.V8Context_Set(context, handle.address, name.ToHandle(jsContext), value.ToHandle(jsContext));
            }
        }



        public IJSValue this[int index]
        {
            get
            {
                return new JSValue(jsContext, JSContext.V8Context_GetPropertyAt(context, handle.address, index));
            }
            set
            {
                JSContext.V8Context_SetPropertyAt(context, handle.address, index, value.ToHandle(jsContext));
            }
        }

        public bool BooleanValue => 
            this.handle.Type == V8HandleType.Boolean
            ? this.handle.result.booleanValue
            : (
                this.IntValue > 0 
            );

        public int IntValue =>
            this.handle.Type == V8HandleType.Integer
            ? this.handle.result.intValue
            : (this.handle.Type == V8HandleType.BigInt
                ? (int) this.handle.result.longValue
                : (int) this.handle.result.doubleValue);

        public double DoubleValue =>
            this.handle.Type == V8HandleType.Number
            ? this.handle.result.doubleValue :
            (
                this.handle.Type == V8HandleType.BigInt
                ? (double) this.handle.result.longValue
                : (double) this.handle.result.intValue
            );

        public float FloatValue => (float)this.DoubleValue;

        public DateTime DateValue
        {
            get
            {
                if (this.handle.Type == V8HandleType.Number)
                {
                    var n = (long)this.handle.result.doubleValue;
                    return n.FromJSTime();
                }
                if (this.handle.Type == V8HandleType.BigInt)
                {
                    return this.handle.result.longValue.FromJSTime();
                }
                return DateTime.MinValue;
            }
        }

        /// <summary>
        /// Since number of elements can change, we need to retrive value from v8
        /// </summary>
        public int Length {
            get => this.IsArray ? JSContext.V8Context_GetArrayLength(context, handle.address).GetIntegerValue() : 0;
            set => this["length"] = jsContext.CreateNumber(value);
        }

        public long LongValue => this.handle.result.longValue;

        public string DebugView => this.ToString();

        public IEnumerable<JSProperty> Entries
        {
            get
            {
                var keys = (JSValue)this.jsContext["Object"].InvokeMethod("keys", this);
                int len = keys.Length;
                for (int i = 0; i < len; i++)
                {
                    var key = keys[i];
                    yield return new JSProperty(key.ToString(), this[key]);
                }
            }
        }

        public bool HasProperty(string name)
        {
            return JSContext.V8Context_HasProperty(context, handle.address, name).GetBooleanValue();
        }

        public bool Has(IJSValue value)
        {
            return JSContext.V8Context_Has(context, handle.address, value.ToHandle(jsContext)).GetBooleanValue();
        }

        public bool DeleteProperty(string name)
        {
            return JSContext.V8Context_DeleteProperty(context, handle.address, name).GetBooleanValue();
        }

        public T Unwrap<T>()
        {
            // we need to get wrapped instance..
            var w = this[jsContext.WrappedSymbol] as JSValue;
            if (w.IsUndefined)
                throw new InvalidCastException($"Cannot cast undefined to {typeof(T).FullName}");
            if (w.IsValueNull)
                return (T)(object)null;
            IntPtr v = w.handle.result.refValue;
            //if (v == IntPtr.Zero)
            //{
            //    return default;
            //}
            var gc = GCHandle.FromIntPtr(v);
            return (T)gc.Target;
        }

        public override int GetHashCode()
        {
            return (int)this.handle.Type;
        }

        public override bool Equals(object obj)
        {
            if (obj is JSValue jv)
            {
                switch (handle.Type)
                {
                    case V8HandleType.Null:
                        return jv.handle.Type == V8HandleType.Null;
                    case V8HandleType.Undefined:
                        return jv.handle.Type == V8HandleType.Undefined;
                    case V8HandleType.Boolean:
                        return handle.result.booleanValue == jv.handle.result.booleanValue;
                    case V8HandleType.Number:
                        return handle.result.doubleValue == jv.handle.result.doubleValue;                    
                }
                if (this.IsObject || this.IsArray)
                {
                    if (handle.address == jv.handle.address)
                    {
                        return true;
                    }
                    if (handle.result.refValue != IntPtr.Zero)
                    {
                        if (handle.result.refValue == jv.handle.result.refValue)
                        {
                            return true;
                        }
                    }
                }
                return JSContext.V8Context_Equals(context, handle.address, jv.handle.address).GetBooleanValue();
            }
            return base.Equals(obj);
        }


        public override string ToString()
        {
            switch (handle.Type)
            {
                case V8HandleType.Undefined: return "Undefined";
                case V8HandleType.Null: return "Null";
                case V8HandleType.Boolean: return this.handle.result.booleanValue.ToString();
                case V8HandleType.Integer: return this.handle.result.intValue.ToString();
                case V8HandleType.BigInt: return this.handle.result.longValue.ToString();
                case V8HandleType.Number: return this.handle.result.doubleValue.ToString();
                case V8HandleType.String:
                    // since JavaScript strings are immutable,
                    // we can cache the result..
                    if (cachedString == null)
                    {
                        var rs = JSContext.V8Context_ToString(context, handle.address);
                        rs.ThrowError();
                        cachedString = rs.StringValue;
                    }
                    return this.cachedString;
            }
            var r = JSContext.V8Context_ToString(context, handle.address);
            r.ThrowError();
            return r.StringValue;
        }

        ~JSValue()
        {
            // if context was disposed
            // the underlying handle was already deleted forcefully
            // so we can ignore this..
            if (context.IsDisposed || handle.address == IntPtr.Zero)
            {
                return;
            }
            IntPtr h = handle.address;
            handle.address = IntPtr.Zero;
            if (context.IsMultiThreaded || MainThread.IsMainThread)
            {
                JSContext.V8Context_ReleaseHandle(context, h).GetBooleanValue();
            }
            else
            {
                MainThread.BeginInvokeOnMainThread(() =>
                {
                    // if context was disposed
                    // the underlying handle was already deleted forcefully
                    // so we can ignore this..
                    if (context.IsDisposed || handle.address == IntPtr.Zero)
                    {
                        return;
                    }
                    JSContext.V8Context_ReleaseHandle(context, h).GetBooleanValue();
                });
            }
        }
        internal IntPtr GetHandle()
        {
            return handle.address;
        }

        public IJSValue InvokeMethod(string name, params IJSValue[] args)
        {
            var r = JSContext.V8Context_InvokeMethod(context, handle.address, name, args.Length, args.ToHandles(jsContext));
            return new JSValue(jsContext, r);
        }

        public IJSValue InvokeFunction(IJSValue thisValue, params IJSValue[] args)
        {
            V8Handle th = IntPtr.Zero;
            if (thisValue != null)
            {
                th = ((JSValue)thisValue).handle.address;
            }
            var r = JSContext.V8Context_InvokeFunction(context, handle.address, th, args.Length, args.ToHandles(jsContext));
            return new JSValue(jsContext, r);
        }

        public bool InstanceOf(IJSValue jsClass)
        {
            return JSContext.V8Context_IsInstanceOf(context, handle.address, jsClass.ToHandle(jsContext)).GetBooleanValue();
        }

        public IList<IJSValue> ToArray()
        {
            return new AtomEnumerable(this);
        }

        public void DefineProperty(string name, JSPropertyDescriptor descriptor)
        {
            NullableBool configurable = descriptor.Configurable.ToNullableBool();
            NullableBool enumerable = descriptor.Enumerable.ToNullableBool();
            NullableBool writable = descriptor.Writable.ToNullableBool();

            IntPtr value = descriptor.Value == null ? IntPtr.Zero : ((JSValue)descriptor.Value).handle.address;
            IntPtr get = descriptor.Get == null ? IntPtr.Zero : ((JSValue)descriptor.Get).handle.address;
            IntPtr set = descriptor.Set == null ? IntPtr.Zero : ((JSValue)descriptor.Set).handle.address;
            if (!JSContext.V8Context_DefineProperty(
                context,
                handle.address,
                name,
                configurable,
                enumerable,
                writable,
                get,
                set,
                value).GetBooleanValue())
            {
                throw new InvalidOperationException();
            }
        }

    }
}