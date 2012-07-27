using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.Management;
using System.IO.Ports;
using System.Threading;

namespace LoadTag
{
    class Program
    {
        static void Main(string[] args)
        {
            Byte[] buf=new Byte[5];
            SerialPort serial = new SerialPort("COM22",115200);
            serial.Open();
            
            for (;;) {
            try
            {
                ManagementObjectSearcher searcher = 
                    new ManagementObjectSearcher("root\\CIMV2", 
                    "SELECT Name, PercentProcessorTime FROM Win32_PerfFormattedData_Counters_ProcessorInformation WHERE NOT Name='_Total' AND NOT Name='0,_Total'");

                Console.WriteLine("-----------------------------------");
                buf[0] = 255;
                buf[1] = 0;
                buf[2] = 0;
                buf[3] = 0;
                buf[4] = 0;
                foreach (ManagementObject queryObj in searcher.Get())
                {
                    Console.WriteLine("PercentProcessorTime: {0} of {1}", queryObj["PercentProcessorTime"], queryObj["Name"]);
                    if (Convert.ToString(queryObj["Name"]) == "0,0") buf[1] = Convert.ToByte(queryObj["PercentProcessorTime"]);
                    if (Convert.ToString(queryObj["Name"]) == "0,1") buf[2] = Convert.ToByte(queryObj["PercentProcessorTime"]);
                    if (Convert.ToString(queryObj["Name"]) == "0,2") buf[3] = Convert.ToByte(queryObj["PercentProcessorTime"]);
                    if (Convert.ToString(queryObj["Name"]) == "0,3") buf[4] = Convert.ToByte(queryObj["PercentProcessorTime"]);
                }
            }
            catch (ManagementException e)
            {
                Console.WriteLine("An error occurred while querying for WMI data: " + e.Message);
            }

            Console.WriteLine("{0} {1} {2} {3} {4}", buf[0], buf[1], buf[2], buf[3], buf[4]);
            serial.Write(buf, 0, 5);
//            Console.ReadLine();
            Thread.Sleep(250);
        }
        }
    }
}
