using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO.Ports;

public class Relativ_setup
{
    private int baudRate;

    private int readTimeout;

    public Relativ_setup(int baudRate, int readTimeout)
    {
        this.baudRate = baudRate;
        this.readTimeout = readTimeout;
    }

    public string findPort()
    {
        SerialPort tmpSerialPort;
        string portName = "";

        foreach (string port in SerialPort.GetPortNames())
        {

            tmpSerialPort = new SerialPort(port);
            tmpSerialPort.BaudRate = 250000;
            tmpSerialPort.ReadTimeout = 50;

            if (tmpSerialPort.IsOpen == false)
            {

                try
                {
                    //open serial port
                    tmpSerialPort.Open();
                    string dataComingFromRelativ = tmpSerialPort.ReadLine();

                    if (dataComingFromRelativ != "")
                    {
                        portName = port;
                    }

                }
                catch
                {
                    // ...
                }
                finally
                {
                    tmpSerialPort.Close();
                    tmpSerialPort = null;
                }
            }
        }
        return portName;
    }
}
