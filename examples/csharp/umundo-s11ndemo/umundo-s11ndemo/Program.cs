using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using org.umundo;
using org.umundo.s11n;

namespace org.umundo.s11n.demo
{
    using org.umundo.core;

    class TestTypedReceiver : ITypedReceiver
    {
        public void receiveObject(object o, Message msg)
        {
            AMessage rcvmsg = (AMessage)o;
            Console.WriteLine("r: " + rcvmsg.a + ", " + rcvmsg.b);
        }
    }

    class Program
    {
        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        private static extern void SetDllDirectory(string lpPathName);

        static void Main(string[] args)
        {
            /*
             * Make sure this path contains the umundoNativeCSharp.dll!
             */
            SetDllDirectory("C:\\Program Files\\uMundo\\lib");
            org.umundo.core.Node node = new org.umundo.core.Node();
            TypedPublisher pub = new TypedPublisher("s11ndemo");
            node.addPublisher(pub);
            TestTypedReceiver ttr = new TestTypedReceiver();
            TypedSubscriber sub = new TypedSubscriber("s11ndemo", ttr);
            node.addSubscriber(sub);
            // Currently not working in C#
            //Console.WriteLine("Waiting for subsrcibers...");
            //int subs = pub.waitForSubscribers(2);
            //Console.WriteLine(subs + " subscribers");
            AMessage msg = new AMessage();
            msg.a = 42;
            msg.b = 43;
            Console.WriteLine(msg.GetType().FullName);
            sub.RegisterType(msg);
            while (true)
            {
                Console.WriteLine("s: " + msg.a + ", " + msg.b);
                pub.SendObject(msg);
                System.Threading.Thread.Sleep(1000);
            }
        }
    }
}
