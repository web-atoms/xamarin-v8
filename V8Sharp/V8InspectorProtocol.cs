using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using vtortola.WebSockets;
using vtortola.WebSockets.Rfc6455;

namespace Xamarin.Android.V8
{
    public abstract class V8InspectorProtocol: IDisposable
    {

        public abstract void Dispose();
        public abstract Task ConnectAsync(Action<string> onMessageReceived);

        public abstract void SendMessage(string message);

        public static V8InspectorProtocol CreateWebSocketServer(int port)
        {
            return new V8InspectorProtocolServer(port);
        }

        public static V8InspectorProtocol CreateInverseProxy(Uri uri)
        {
            return new V8InspectorProtocolProxy(uri);
        }

    }

    internal class V8InspectorProtocolProxy : V8InspectorProtocol
    {

        readonly Uri uri;
        System.Net.WebSockets.ClientWebSocket client;
        CancellationTokenSource cancellationTokenSource;
        Action<string> onMessageReceived;
        public V8InspectorProtocolProxy(Uri uri)
        {
            this.uri = uri;
            client = new System.Net.WebSockets.ClientWebSocket();

            cancellationTokenSource = new CancellationTokenSource();
        }

        public override async Task ConnectAsync(Action<string> onMessageReceived)
        {
            this.onMessageReceived = onMessageReceived;
            await client.ConnectAsync(uri, cancellationTokenSource.Token);

#pragma warning disable CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
            Task.Run(() => this.ReadMessagesAsync());
#pragma warning restore CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
        }

        private async Task ReadMessagesAsync()
        {
            try
            {
                var buffer = new ArraySegment<byte>(new byte[1024]);
                
                var sb = new StringBuilder();
                while (true)
                {
                    var r = await this.client.ReceiveAsync(buffer, CancellationToken.None);
                    if (r.MessageType != System.Net.WebSockets.WebSocketMessageType.Text)
                        break;
                    sb.Append(System.Text.Encoding.UTF8.GetString(buffer.Array, 0, r.Count));
                    if (r.EndOfMessage)
                    {
                        var text = sb.ToString();
                        onMessageReceived?.Invoke(text);
                        sb.Clear();
                    }
                }
            } catch (Exception ex)
            {

                System.Diagnostics.Debug.WriteLine(ex);
                // disconnect...
                client.Dispose();
                client = null;
            }
        }

        public override void Dispose()
        {
            client?.Dispose();
        }

        public override void SendMessage(string message)
        {
            AtomAsyncDispatcher.Instance.EnqueueTask(async () =>
            {
                var bytes = System.Text.Encoding.UTF8.GetBytes(message);
                var buffer = new ArraySegment<byte>(bytes);
                await client.SendAsync(buffer, System.Net.WebSockets.WebSocketMessageType.Text, true, cancellationTokenSource.Token);
            });
        }
    }

    internal class V8InspectorProtocolServer : V8InspectorProtocol
    {
        CancellationTokenSource cancellationTokenSource;
        WebSocketListener server;
        Action<string> onMessageReceived;
        List<WebSocket> clients = new List<WebSocket>();

        public V8InspectorProtocolServer(int port)
        {
            cancellationTokenSource = new CancellationTokenSource();

            var options = new WebSocketListenerOptions();
            options.Standards.RegisterRfc6455();
            server = new WebSocketListener(new IPEndPoint(IPAddress.Any, port), options);

        }

        public override async Task ConnectAsync(Action<string> onMessageReceived)
        {
            this.onMessageReceived = onMessageReceived;
            await server.StartAsync();
#pragma warning disable CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
            Task.Run(() => AcceptClientsAsync());
#pragma warning restore CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
        }

        private async Task AcceptClientsAsync()
        {
            while (true)
            {
                var client = await server.AcceptWebSocketAsync(cancellationTokenSource.Token);
                lock(clients)
                {
                    clients.Add(client);
                }

                // read clients...
#pragma warning disable CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed
                Task.Run(() => ReadMessagesAsync(client));
#pragma warning restore CS4014 // Because this call is not awaited, execution of the current method continues before the call is completed

            }
        }
        private async Task ReadMessagesAsync(WebSocket client)
        {
            while (true)
            {
                try
                {
                    var message = await client.ReadStringAsync(cancellationTokenSource.Token);
                    onMessageReceived?.Invoke(message);
                }catch (Exception ex)
                {
                    System.Diagnostics.Debug.WriteLine(ex);
                    lock(clients)
                    {
                        clients.Remove(client);
                    }
                    break;
                }

            }
        }

        public override void Dispose()
        {
            try
            {
                cancellationTokenSource.Cancel();

                server?.Dispose();

            }catch (Exception  ex)
            {
                System.Diagnostics.Debug.WriteLine(ex);
            }
        }

        public override void SendMessage(string message)
        {
            lock (clients) {
                foreach (var client in clients) {
                    AtomAsyncDispatcher.Instance.EnqueueTask(() => client.WriteStringAsync(message));
                }
            }
        }
    }
}