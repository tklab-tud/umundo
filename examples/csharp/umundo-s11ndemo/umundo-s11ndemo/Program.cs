using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using org.umundo;
using org.umundo.s11n;

namespace umundo_s11ndemo
{
    using org.umundo.core;

    class TestTypedReceiver : ITypedReceiver
    {
        public void receiveObject(object o, Message msg)
        {
            Console.WriteLine("r: " + o);
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
            SetDllDirectory("C:\\Users\\Piri\\Documents\\Entwicklung\\tu-darmstadt\\umundo\\build\\lib");
            org.umundo.core.Node node = new org.umundo.core.Node();
            TypedPublisher pub = new TypedPublisher("s11ndemo");
            node.addPublisher(pub);
            TestTypedReceiver ttr = new TestTypedReceiver();
            TypedSubscriber sub = new TypedSubscriber("s11ndemo", ttr);
            node.addSubscriber(sub);
            AMessage msg = new AMessage();
            Console.WriteLine("s: " + msg);
            pub.sendObject("AMessage", msg);
            System.Threading.Thread.Sleep(1000);
        }
    }
}
