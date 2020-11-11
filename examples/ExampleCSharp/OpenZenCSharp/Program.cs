using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace OpenZenCSharpBindingTest
{
    public class ExThread
    {
        public ExThread(ZenClientHandle_t zenHandle)
        {
            mZenHandle = zenHandle;
        }

        // Non-static method 
        public void sensorEventThread()
        {
            try
            {
                while (true)
                {
                    ZenEvent zenEvent = new ZenEvent();
                    //Console.WriteLine(mZenHandle);
                    if (OpenZen.ZenWaitForNextEvent(mZenHandle, zenEvent))
                    {
                        if (zenEvent.component.handle == 0) {
                            // if the handle is on, its not a sensor event but a system wide 
                            // event
                            switch (zenEvent.eventType)
                            {
                                case ZenEventType.ZenEventType_SensorFound:
                                    Console.WriteLine("found sensor event event " + zenEvent.data.sensorFound.name);
                                    mFoundSensors.Add(zenEvent.data.sensorFound);
                                    break;
                                case ZenEventType.ZenEventType_SensorListingProgress:
                                    if (zenEvent.data.sensorListingProgress.progress == 1.0)
                                    {
                                        mSearchDone = true;
                                    }
                                    break;
                            }
                        } else {
                            switch (zenEvent.eventType)
                            {
                                case ZenEventType.ZenEventType_ImuData:
                                    mImuEventCount++;

                                    if (mImuEventCount % 100 == 0)
                                        continue;
                                    // read acceleration
                                    OpenZenFloatArray fa = OpenZenFloatArray.frompointer(zenEvent.data.imuData.a);
                                    // read angular velocity
                                    OpenZenFloatArray fg = OpenZenFloatArray.frompointer(zenEvent.data.imuData.g);
                                    // read euler angles
                                    OpenZenFloatArray fr = OpenZenFloatArray.frompointer(zenEvent.data.imuData.r);
                                    // read quaternion
                                    OpenZenFloatArray fq = OpenZenFloatArray.frompointer(zenEvent.data.imuData.q);

                                    Console.WriteLine("Sensor data\n -> Acceleration a = " + fa.getitem(0) + " " + fa.getitem(1) + " " + fa.getitem(2));
                                    Console.WriteLine(" -> Angular velocity g = " + fg.getitem(0) + " " + fg.getitem(1) + " " + fg.getitem(2));
                                    Console.WriteLine(" -> Euler angles r = " + fr.getitem(0) + " " + +fr.getitem(1) + " " + fr.getitem(2));
                                    Console.WriteLine(" -> Quaternion w = " + fq.getitem(0) + " x " + +fq.getitem(1) + " y " + +fq.getitem(2) + " z " + + fq.getitem(3));
                                    break;
                            }
                        }
                    }
                }
            }
            catch (ThreadAbortException)
            {
            }
        }

        public bool mSearchDone = false;
        public uint mImuEventCount = 0;
        public List<ZenSensorDesc> mFoundSensors = new List<ZenSensorDesc>();
        ZenClientHandle_t mZenHandle;
    }

    class Program
    {
        static void Main(string[] args)
        {
            ZenClientHandle_t zenHandle = new ZenClientHandle_t();
            OpenZen.ZenInit(zenHandle);

            ExThread obj = new ExThread(zenHandle);

            // Creating thread 
            // Using thread class 
            Thread thr = new Thread(new ThreadStart(obj.sensorEventThread));
            thr.Start();

            // start sensor listing, new sensors will be reported as Events in our event thread
            OpenZen.ZenListSensorsAsync(zenHandle);

            while (!obj.mSearchDone)
            {
                Console.WriteLine("Searching for sensors ...");
                Thread.Sleep(1000);
            }

            if (obj.mFoundSensors.Count == 0)
            {
                Console.WriteLine("No sensor found on the system");
                OpenZen.ZenShutdown(zenHandle);
                thr.Abort();
                return;
            }

            ZenSensorInitError sensorInitError = ZenSensorInitError.ZenSensorInitError_Max;
            // try three connection attempts
            for (int i = 0; i < 3; i++)
            {
                ZenSensorHandle_t sensorHandle = new ZenSensorHandle_t();
                sensorInitError = OpenZen.ZenObtainSensor(zenHandle, obj.mFoundSensors[0], sensorHandle);
                Console.WriteLine("Connecting to sensor " + obj.mFoundSensors[0].identifier + " on IO interface " + obj.mFoundSensors[0].ioType);
                if (sensorInitError == ZenSensorInitError.ZenSensorInitError_None)
                {
                    Console.WriteLine("Succesfully connected to sensor");
                    break;
                }
            }

            if (sensorInitError != ZenSensorInitError.ZenSensorInitError_None)
            {
                Console.WriteLine("Could not connect to sensor");
                System.Environment.Exit(1);
            }

            // stream some data
            Thread.Sleep(5000);

            OpenZen.ZenShutdown(zenHandle);
            thr.Abort();
        }
    }
}
