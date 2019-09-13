using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO.Ports;

public class Relativ_setup {

	public string portName;

	public int baudRate;

	public int ReadTimeout;

	public string getPort() {
		SerialPort tmp_serialPort;
		foreach (string port in SerialPort.GetPortNames()) {
			tmp_serialPort = new SerialPort(port);
			tmp_serialPort.BaudRate = getBaudRate();
			tmp_serialPort.ReadTimeout = getTimeout();

            if (tmp_serialPort.IsOpen == false) {
			    try {
				    //open serial port
				    tmp_serialPort.Open ();
				    string dataComingFromRelativ = tmp_serialPort.ReadLine();

				    if (dataComingFromRelativ != "") {
					    this.portName = port;
					    tmp_serialPort.Close();
				    } else {
					    tmp_serialPort.Close ();
				    }
			    }
                catch(System.Exception e) {
			    }
			}
		}
		return this.portName;
	}

	public int getBaudRate() {
		return 250000;
	}

	public int getTimeout() {
		return 500;
	}
}
