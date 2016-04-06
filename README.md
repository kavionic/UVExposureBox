# UVExposureBox
CAD and Firmware code for the UV exposure box described at http://kavionic.com/.

Please note that there is a bug in the schematic and PCB layout of the controller board.
The snubber diode D6 is wrongly connected between the compressor negative lead and GND.
This is not functional at all, and caused Q3 to go into avalanche and release the magic smoke.
I fixed this in my build by soldering D6 between the two pins of the compressor terminal instead
(where it should have been in the first place).
