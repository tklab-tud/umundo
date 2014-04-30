using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using org.umundo;
using org.umundo.s11n;
namespace umundo_stress
{
    using org.umundo.core;

    class StressReceiver : Receiver
    {
        public override void receive(Message msg) {
            Console.Write("i");
            msg.Dispose(); // The garbage collector won't bother to call destructors
        }
    }

    class StressGreeter : Greeter
    {
        public override void farewell(Publisher pub, SubscriberStub sub) {
//            Console.WriteLine("\nLost Subscriber");          
        }

        public override void welcome(Publisher pub, SubscriberStub sub) {
//            Console.WriteLine("\nNew Subscriber");
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
                SetDllDirectory("C:\\Users\\sradomski\\Desktop\\build\\umundo\\lib");
                //                SetDllDirectory("C:\\Program Files (x86)\\uMundo\\share\\umundo\\bindings\\csharp64_d");
            }
            else
            {
                SetDllDirectory("C:\\Users\\sradomski\\Desktop\\build\\umundo\\lib");
                //                SetDllDirectory("C:\\Program Files (x86)\\uMundo\\share\\umundo\\bindings\\csharp64_d");
            }
            byte[] payload = new byte[4096 * 1024];
            for (int i = 0; i < 4096 * 1024; i++)
            {
                payload[i] = (byte)i;
            }

            Discovery disc = new Discovery(Discovery.DiscoveryType.MDNS);

            while (!Console.KeyAvailable)
            {
                Node node1 = new Node();
                Publisher pub1 = new Publisher("stress");
                pub1.setGreeter(new StressGreeter());
                Subscriber sub1 = new Subscriber("stress", new StressReceiver());

                node1.addPublisher(pub1);
                node1.addSubscriber(sub1);
                disc.add(node1);

                int i = 500;

                while (!Console.KeyAvailable && i-- > 0)
                {
                    Console.Write("o");
                    Message msg = new Message();
                    msg.setData(payload);
                    msg.putMeta("foo", "bar");
                    pub1.send(msg);
                    msg.Dispose();
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
