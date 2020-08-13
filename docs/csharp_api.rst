#######
C# API
#######

Overview
========
The OpenZen language bindings allow you to access sensor data from any C# program.
The function names are the same which are used by the OpenZen C API. In order to use
the C# language bindings, you need to add the C# files in the folder ``bindings/OpenZenCSharp``
to your project as well as the dynamic link libraries from the binary OpenZen folder.

You can find a complete OpenZen C# example for Visual Studio in this `folder <https://bitbucket.org/lpresearch/openzen/src/master/bindings/OpenZenCSharpBindingTest/>`_.

Initialize OpenZen in C#
========================

To create a new instance of OpenZen, you can use this code snippet:

.. code-block:: cpp

    ZenClientHandle_t zenHandle = new ZenClientHandle_t();
    OpenZen.ZenInit(zenHandle);

Receive Sensor data in C#
=========================

OpenZen events containing sensor data need to be read from the pointers returned
by the interface using the following method:

.. code-block:: cpp

    // read acceleration
    OpenZenFloatArray fa = OpenZenFloatArray.frompointer(zenEvent.data.imuData.a);
    // read euler angles
    OpenZenFloatArray fr = OpenZenFloatArray.frompointer(zenEvent.data.imuData.r);
    // read quaternion
    OpenZenFloatArray fq = OpenZenFloatArray.frompointer(zenEvent.data.imuData.q);

    Console.WriteLine("Sensor data\n -> Acceleration a = " + fa.getitem(0) + " " + fa.getitem(1) + " " + fa.getitem(2));
    Console.WriteLine(" -> Euler angles r = " + fr.getitem(0) + " " + fr.getitem(1) + " " + fr.getitem(2));
    Console.WriteLine(" -> Quaternion w = " + fq.getitem(0) + " x " + fq.getitem(1) + " y " + fq.getitem(2) + " z " + fq.getitem(3));
