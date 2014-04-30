using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using org.umundo;
using org.umundo.s11n;

namespace umundo_s11n_stress
{
    using org.umundo.core;

    class StressReceiver : ITypedReceiver
    {
        public void ReceiveObject(object o, Message msg)
        {
            Test.AllTypes allTst = (Test.AllTypes)o;
            Console.WriteLine(allTst.doubleType);
        }
    }

    class StressGreeter : Greeter
    {
        public override void farewell(Publisher pub, SubscriberStub sub)
        {
            Console.WriteLine("\nLost Subscriber");
        }

        public override void welcome(Publisher pub, SubscriberStub sub)
        {
            Console.WriteLine("\nNew Subscriber");
        }

    }

    class Program
    {
        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        private static extern void SetDllDirectory(string lpPathName);

        static void Main(string[] args)
        {
            if (System.Environment.Is64BitProcess)
            {
                SetDllDirectory("C:\\Program Files (x86)\\uMundo\\share\\umundo\\bindings\\csharp64");
            }
            else
            {
                SetDllDirectory("C:\\Program Files (x86)\\uMundo\\share\\umundo\\bindings\\csharp");
            }

            while (true)
            {
                Node node1 = new Node();
                TypedPublisher pub1 = new TypedPublisher("s11nStress");
                TypedSubscriber sub1 = new TypedSubscriber("s11nStress", new StressReceiver());

                pub1.setGreeter(new StressGreeter());
                    
                Test.AllTypes protoTypeAll = new Test.AllTypes();
                sub1.RegisterType(protoTypeAll.GetType().Name, protoTypeAll.GetType());

                node1.addPublisher(pub1);
                node1.addSubscriber(sub1);

                Discovery disc = new Discovery(Discovery.DiscoveryType.MDNS);
                disc.add(node1);

                int i = 500;

                while (i-- > 0)
                {
                    Console.Write("o");
                    Test.AllTypes tstAll = new Test.AllTypes();
                    tstAll.boolType = true;
                    tstAll.doubleType = 1.23456;

                    pub1.SendObject(tstAll);
                    System.Threading.Thread.Sleep(5);
                }

                node1.removePublisher(pub1);
                node1.removeSubscriber(sub1);
                disc.remove(node1);
                System.GC.Collect();
            }
        }
    }
}
