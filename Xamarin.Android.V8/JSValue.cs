using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using WebAtoms;
using V8Handle = System.IntPtr;

namespace Xamarin.Android.V8
{
    public class JSValue: IJSValue
    {
        readonly JSContext jsContext;
        readonly V8Handle context;
        internal V8HandleContainer handle;
        internal JSValue(JSContext context, V8HandleContainer handle)
        {
            this.jsContext = context;
            this.context = context.context;
            this.handle = handle;
        }

        public IJSValue CreateNewInstance(params IJSValue[] args) {
            var r = JSContext.V8Context_NewInstance(context, handle.handle, args.Length, args.ToHandles()).GetContainer();
            return new JSValue(jsContext, r);
        }

        public IJSContext Context => jsContext;

        public bool IsValueNull => this.handle.handleType == V8HandleType.Null;

        public bool IsUndefined => this.handle.handleType == V8HandleType.Undefined;

        public bool IsString => this.handle.handleType == V8HandleType.String;

        public bool IsObject => (handle.handleType & V8HandleType.Object) > 0;

        public bool IsFunction => handle.handleType == V8HandleType.Function;

        public bool IsDate => handle.handleType == V8HandleType.Date;

        public bool IsNumber => (handle.handleType & V8HandleType.Number) > 0 ;

        public bool IsBoolean => handle.handleType == V8HandleType.Boolean;

        public bool IsArray => handle.handleType == V8HandleType.Array;

        public bool IsWrapped => handle.handleType == V8HandleType.Object && Has(jsContext.WrappedSymbol);

        public bool IsSymbol => handle.handleType == V8HandleType.Symbol;

        public IJSValue this[string name]
        {
            get
            {
                return new JSValue(jsContext, JSContext.V8Context_GetProperty(context, handle.handle, name).GetContainer());
            }
            set
            {
                var v = value ?? (new JSValue(jsContext, JSContext.V8Context_CreateNull(context).GetContainer()));
                JSContext.V8Context_SetProperty(context, handle.handle, name, v.ToJSValue().handle.handle).GetContainer();
            }
        }

        public IJSValue this[IJSValue name]
        {
            get
            {
                return new JSValue(jsContext, JSContext.V8Context_Get(context, handle.handle, name.ToHandle()).GetContainer());
            }
            set
            {
                var v = value ?? (new JSValue(jsContext, JSContext.V8Context_CreateNull(context).GetContainer()));
                JSContext.V8Context_Set(context, handle.handle, name.ToHandle(), v.ToJSValue().handle.handle).GetContainer();
            }
        }



        public IJSValue this[int index]
        {
            get
            {
                return new JSValue(jsContext, JSContext.V8Context_GetPropertyAt(context, handle.handle, index).GetContainer());
            }
            set
            {
                var v = value ?? (new JSValue(jsContext, JSContext.V8Context_CreateNull(context).GetContainer()));
                JSContext.V8Context_SetPropertyAt(context, handle.handle, index, v.ToJSValue().handle.handle).GetContainer();
            }
        }

        public bool BooleanValue => 
            this.handle.handleType == V8HandleType.Boolean
            ? this.handle.value.boolValue
            : (
                this.IntValue > 0 
            );

        public int IntValue =>
            this.handle.handleType == V8HandleType.Integer
            ? this.handle.value.intValue
            : (this.handle.handleType == V8HandleType.BigInt
                ? (int) this.handle.value.longValue
                : (int) this.handle.value.doubleValue);

        public double DoubleValue =>
            this.handle.handleType == V8HandleType.Number
            ? this.handle.value.doubleValue :
            (
                this.handle.handleType == V8HandleType.BigInt
                ? (double) this.handle.value.longValue
                : (double) this.handle.value.intValue
            );

        public float FloatValue => (float)this.DoubleValue;

        public DateTime DateValue
        {
            get
            {
                long n = 0;
                if (this.handle.handleType == V8HandleType.Number)
                {
                    n = (long)this.handle.value.doubleValue;
                    return n.FromJSTime();
                }
                if (this.handle.handleType == V8HandleType.BigInt)
                {
                    return this.handle.value.longValue.FromJSTime();
                }
                return DateTime.MinValue;
            }
        }

        /// <summary>
        /// Since number of elements can change, we need to retrive value from v8
        /// </summary>
        public int Length =>
            this.IsArray ?
            JSContext.V8Context_GetArrayLength(context, handle.handle).GetIntegerValue()
            : 0;

        public long LongValue => this.handle.value.longValue;

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
                    yield return new JSProperty(key.ToString(), keys[i]);
                }
            }
        }

        public bool HasProperty(string name)
        {
            return JSContext.V8Context_HasProperty(context, handle.handle, name).GetBooleanValue();
        }

        public bool Has(IJSValue value)
        {
            return JSContext.V8Context_Has(context, handle.handle, value.ToHandle()).GetBooleanValue();
        }

        public bool DeleteProperty(string name)
        {
            return JSContext.V8Context_DeleteProperty(context, handle.handle, name).GetBooleanValue();
        }

        public T Unwrap<T>()
        {
            // we need to get wrapped instance..
            var w = this[jsContext.WrappedSymbol] as JSValue;
            var gc = GCHandle.FromIntPtr(w.handle.value.refValue);
            return (T)gc.Target;
        }

        public override string ToString()
        {
            if (this.IsUndefined) return "Undefined";
            if (this.IsUndefined) return "Undefined";
            if (this.IsObject)
            {
                return JSContext.V8Context_ToString(context, handle.handle).GetString();
            }
            if (this.IsBoolean) return this.BooleanValue.ToString();
            return base.ToString();
        }

        ~JSValue()
        {
            if (handle.handle != IntPtr.Zero)
            {
                JSContext.V8Context_ReleaseHandle(context, handle.handle).GetBooleanValue();
                handle.handle = IntPtr.Zero;
            }
        }
        internal IntPtr Detach()
        {
            var h = handle.handle;
            handle = new V8HandleContainer
            {
                handle = IntPtr.Zero
            };
            return h;
        }

        public IJSValue InvokeMethod(string name, params IJSValue[] args)
        {
            var r = JSContext.V8Context_InvokeMethod(context, handle.handle, name, args.Length, args.ToHandles()).GetContainer();
            return new JSValue(jsContext, r);
        }

        public IJSValue InvokeFunction(IJSValue thisValue, params IJSValue[] args)
        {
            V8Handle th = IntPtr.Zero;
            if (thisValue != null)
            {
                th = ((JSValue)thisValue).handle.handle;
            }
            var r = JSContext.V8Context_InvokeFunction(context, handle.handle, th, args.Length, args.ToHandles()).GetContainer();
            return new JSValue(jsContext, r);
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

            if(!JSContext.V8Context_DefineProperty(
                context,
                handle.handle,
                name,
                configurable,
                enumerable,
                writable,
                descriptor.Get.ToHandle(),
                descriptor.Set.ToHandle(),
                descriptor.Value.ToHandle()).GetBooleanValue())
            {
                throw new InvalidOperationException();
            }
        }

    }
}