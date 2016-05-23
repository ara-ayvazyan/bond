﻿// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace UnitTest.SimpleInMem
{
    using System;
    using System.Threading.Tasks;
    using Bond.Comm;
    using Bond.Comm.SimpleInMem;
    using NUnit.Framework;
    using UnitTest.Comm;

    [TestFixture]
    public class SimpleInMemTransportTest
    {
        private readonly string m_address = "SimpleInMemTakesAnyRandomConnectionString";
        private SimpleInMemTransport m_transport;

        [SetUp]
        public void Init()
        {
            m_transport = new SimpleInMemTransportBuilder()
                .SetUnhandledExceptionHandler(Transport.ToErrorExceptionHandler)
                .Construct();
        }

        [TearDown]
        public void Cleanup()
        {
            m_transport.StopAsync();
            m_transport = null;
        }

        [Test]
        public void Builder_SetUnhandledExceptionHandler_Null_Throws()
        {
            Assert.Throws<ArgumentNullException>(() => new SimpleInMemTransportBuilder().SetUnhandledExceptionHandler(null));
        }

        [Test]
        public void Builder_Construct_DidntSetUnhandledExceptionHandler_Throws()
        {
            var builder = new SimpleInMemTransportBuilder();
            Assert.Throws<InvalidOperationException>(() => builder.Construct());
        }

        [Test]
        public void Construct_InvalidArgs_Throws()
        {
            Assert.Throws<ArgumentNullException>(() => new SimpleInMemTransport(null, null));

            LayerStack<Dummy> layerStack = new LayerStack<Dummy>(null, new TestLayer_DoesNothing());
            Assert.Throws<ArgumentNullException>(() => new SimpleInMemTransport(null, layerStack));
        }

        [Test]
        public void StopAsync()
        {
            Listener newListener = m_transport.MakeListener(m_address);
            Assert.True(newListener == m_transport.GetListener(m_address));
            m_transport.StopAsync();
            Assert.False(m_transport.ListenerExists(m_address));
            m_transport.MakeListener(m_address);
            Assert.True(m_transport.ListenerExists(m_address));
        }

        [Test]
        public void ConnectToAsync_NoListenerRunning()
        {
            Assert.Throws<ArgumentException>(async () => await m_transport.ConnectToAsync(m_address, new System.Threading.CancellationToken()));
        }

        [Test]
        public void MakeListener()
        {
            bool listenerExists = m_transport.ListenerExists(m_address);
            Assert.False(listenerExists);
            var listener = m_transport.MakeListener(m_address);
            listenerExists = m_transport.ListenerExists(m_address);
            Assert.True(listenerExists);
            m_transport.RemoveListener(m_address);
            Assert.Null(m_transport.GetListener(m_address));
        }

        [Test]
        public async Task ConnectToAsync()
        {
            Listener l = m_transport.MakeListener(m_address);
            Connection conn = await m_transport.ConnectToAsync(m_address, new System.Threading.CancellationToken());
            Assert.NotNull(conn);
            Assert.True(conn is SimpleInMemConnection);
            SimpleInMemConnection simpleConn = (SimpleInMemConnection)conn;
            Assert.True(simpleConn.ConnectionType == ConnectionType.Client);
            m_transport.RemoveListener(m_address);
            Assert.Null(m_transport.GetListener(m_address));
        }

        private class TestLayer_DoesNothing : ILayer<Dummy>
        {
            public Error OnSend(MessageType messageType, SendContext context, Dummy layerData)
            {
                return null;
            }

            public Error OnReceive(MessageType messageType, ReceiveContext context, Dummy layerData)
            {
                return null;
            }
        }
    }
}
