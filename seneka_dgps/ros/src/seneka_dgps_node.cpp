/*!
*****************************************************************
* seneka_dgps_node.cpp
*
* Copyright (c) 2013
* Fraunhofer Institute for Manufacturing Engineering
* and Automation (IPA)
*
*****************************************************************
*
* Repository name: seneka_sensor_node
*
* ROS package name: seneka_dgps
*
* Author: Ciby Mathew, E-Mail: Ciby.Mathew@ipa.fhg.de
* 
* Supervised by: Christophe Maufroy
*
* Date of creation: Jan 2013
* Modified 03/2014: David Bertram, E-Mail: davidbertram@gmx.de
* Modified 04/2014: Thorsten Kannacher, E-Mail: Thorsten.Andreas.Kannacher@ipa.fraunhofer.de
*
* Description:
*
* To-Do:
*
* - Generation and publishing of error messages
* - Extract all fields of a position record message (especially dynamic length of sat-channel_numbers and prns...)
* - Publish all gps values to ros topic (maybe need a new message if navsatFix cannot take all provided values...)
*
* - Monitor frequency/quality/... of incoming data packets... --> inform ROS about bad settings (publishing rate <-> receiving rate)
*
* - Rewrite function structure of interpretData and connected functions.. (still in dev state... double check for memory leaks etc...!!)
*
* - Extracting multi page messages from buffer...  (not needed for position records)
* - Clean up SerialIO files
* - Add more parameter handling (commandline, ...); document parameters and configuration
* - Testing!
*
*****************************************************************
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* - Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer. \n
* - Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution. \n
* - Neither the name of the Fraunhofer Institute for Manufacturing
* Engineering and Automation (IPA) nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission. \n
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License LGPL as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License LGPL for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License LGPL along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*
****************************************************************/

#include <seneka_dgps/seneka_dgps.h>
#include <seneka_dgps/Dgps.h>


/*****************************************************************/
/*************** main program seneka_dgps_node.cpp ***************/
/*****************************************************************/

int main(int argc, char** argv) {

    // ROS initialization; applying DGPS as node name;
    ros::init(argc, argv, "DGPS");

    SenekaDgps      cSenekaDgps;
    Dgps            cDgps;

    bool port_opened            = false;
    bool connection_is_ok       = false;
    bool success_getGpsData     = false;

    while (!port_opened) {

        ROS_INFO("Establishing connection to DGPS device... (Port: %s, Baud rate: %i)", cSenekaDgps.getPort().c_str(), cSenekaDgps.getBaud());
        cSenekaDgps.publishStatus("Establishing connection to DGPS device...", cSenekaDgps.OK);
        
        port_opened = cDgps.open(cSenekaDgps.getPort().c_str(), cSenekaDgps.getBaud());

        cSenekaDgps.extractDiagnostics(cDgps);

        if (!port_opened) {

            ROS_ERROR("Connection to DGPS device failed. Device is not available on given port %s. Retrying to establish connection every second...", cSenekaDgps.getPort().c_str());
            cSenekaDgps.publishStatus("Connection to DGPS failed. DGPS is not available on given port. Retrying to establish connection every second...", cSenekaDgps.ERROR);
        }

        else {

            ROS_INFO("Successfully connected to DPGS device.");
            cSenekaDgps.publishStatus("Successfully connected to DPGS device.", cSenekaDgps.OK);
        }

        // in case of success, wait for DPGS to get ready
        // in case of an error, wait before retry connection
        sleep(1);
    }

    ros::Rate loop_rate(cSenekaDgps.getRate()); // [] = Hz

    // testing the communications link by sending protocol request ENQ (05h) (see BD982 manual, page 65)
    connection_is_ok = cDgps.checkConnection();

    cSenekaDgps.extractDiagnostics(cDgps);

    if (!connection_is_ok) {

        ROS_ERROR("Testing the communications link failed (see Trimble BD982 GNSS Receiver manual page 65).");
        cSenekaDgps.publishStatus("Testing the communications link failed (see Trimble BD982 GNSS Receiver manual page 65).", cSenekaDgps.ERROR);
    }

    else {

        ROS_INFO("Successfully tested the communications link.");
        cSenekaDgps.publishStatus("Successfully tested the communications link.", cSenekaDgps.OK);

        ROS_INFO("Beginnig to obtain and publish DGPS data on topic %s...", cSenekaDgps.getPositionTopic().c_str());
        cSenekaDgps.publishStatus("Beginnig to obtain and publish DGPS data...", cSenekaDgps.OK);


        /*************************************************/
        /*************** main program loop ***************/
        /*************************************************/

        while (cSenekaDgps.nh.ok()) {

            /*  getGpsData call on dgps instance:
            *
            *       -> requests position record from receiver
            *       -> appends incoming data to ringbuffer
            *       -> tries to extract valid packets (incl. checksum verification)
            *       -> tries to read position record fields from valid packets
            *       -> writes position record data into struct of type gps_data
            *
            */
            success_getGpsData = cDgps.getGpsData();

            cSenekaDgps.extractDiagnostics(cDgps);

            // publish GPS data to ROS topic if getGpsData() was successfull, if not just publish status
            if (success_getGpsData) {

                ROS_DEBUG("Successfully obtained DGPS data.");
                ROS_DEBUG("Publishing DGPS position on topic %s...", cSenekaDgps.getPositionTopic().c_str());

                cSenekaDgps.publishStatus("Successfully obtained DGPS data.", cSenekaDgps.OK);
                cSenekaDgps.publishStatus("Publishing DGPS position...", cSenekaDgps.OK);

                cSenekaDgps.publishPosition(cDgps.getPosition());
            }

            else {

                ROS_WARN("Failed to obtain DGPS data. No DGPS data available.");
                cSenekaDgps.publishStatus("Failed to obtain DGPS data. No DGPS data available.", cSenekaDgps.WARN);
            }

            ros::spinOnce();
            loop_rate.sleep();
        }

        /*************************************************/
        /*************************************************/
        /*************************************************/

    }

    return 0;
}

/*****************************************************************/
/*****************************************************************/
/*****************************************************************/


#ifndef NDEBUG

// ##########################################################################
// ## dev-methods -> can be removed when not needed anymore                ##
// ##########################################################################

// context of this function needs to be created!!
//bool getFakePosition(double* latt) {
//    // set to true after extracting position values. method return value.
//    bool success = false;
//
//    int length;
//    unsigned char Buffer[1024] = {0};
//    int buffer_index = 0;
//    unsigned char data_buffer[1024] = {0};
//    int data_index = 0;
//    for (int i = 0; i < 1024; i++) Buffer[i] = '0';
//    char str[10];
//    char binary[10000] = {0};
//    int value[1000] = {0};
//    int open, y, bytesread, byteswrite, bin;
//
//    // see page 73 in BD982 user guide for packet specification
//    //  start tx,
//    //      status,
//    //          packet type,
//    //              length,
//    //                  type raw data,      [0x00: Real-Time Survey Data Record; 0x01: Position Record]
//    //                      flags,
//    //                          reserved,
//    //                              checksum,
//    //                                  end tx
//    unsigned char stx_ = 0x02;
//    unsigned char status_ = 0x00;
//    unsigned char packet_type_ = 0x56;
//    unsigned char length_ = 0x03;
//    unsigned char data_type_ = 0x01;
//    unsigned char etx_ = 0x03;
//
//    unsigned char checksum_ = status_ + packet_type_ + data_type_ + length_;
//    //(status_ + packet_type_ + data_type_ + 0 + 0 + length_)%256;
//
//
//
//    char message[] = {stx_, status_, packet_type_, length_, data_type_, 0x00, 0x00, checksum_, etx_}; // 56h command packet       // expects 57h reply packet (basic coding)
//
//    //        char message[]={ 0x05 };
//    length = sizeof (message) / sizeof (message[0]);
//
//    cout << "length of command: " << length << "\n";
//
//    //SerialIO dgps;
//    //open = dgps.open();
//    byteswrite = 9; //m_SerialIO.write(message, length);
//    printf("Total number of bytes written: %i\n", byteswrite);
//    std::cout << "command was: " << std::hex << message << "\n";
//    sleep(1);
//    bytesread = 118; //m_SerialIO.readNonBlocking((char*) Buffer, 1020);
//
//    string test_packet = " |02|  |20|  |57|  |0a|  |0c|  |11|  |00|  |00|  |00|  |4d|  |01|  |e1|  |01|  |e1|  |af|  |03|  |02|  |20|  |57|  |60|  |01|  |11|  |00|  |00|  |3f|  |d1|  |54|  |8a|  |b6|  |cf|  |c6|  |8d|  |3f|  |a9|  |e0|  |bd|  |3f|  |29|  |c8|  |f7|  |40|  |80|  |ae|  |2a|  |c9|  |7b|  |b7|  |11|  |c0|  |fd|  |d3|  |79|  |61|  |fb|  |23|  |99|  |c0|  |92|  |ca|  |3b|  |46|  |c7|  |05|  |15|  |3f|  |ff|  |9f|  |23|  |e0|  |00|  |00|  |00|  |be|  |44|  |16|  |1f|  |0d|  |84|  |d5|  |33|  |be|  |2c|  |8b|  |3b|  |bb|  |46|  |eb|  |85|  |3f|  |b5|  |ec|  |f0|  |c0|  |00|  |00|  |00|  |08|  |06|  |f0|  |d8|  |d4|  |07|  |0f|  |0d|  |13|  |01|  |0c|  |07|  |04|  |07|  |0d|  |08|  |09|  |0a|  |1a|  |1c|  |5c|  |03|";
//           test_packet = " |02|  |20|  |57|  |0a|  |0c|  |11|  |00|  |00|  |00|  |4d|  |01|  |e1|  |01|  |e1|  |af|  |03|  |02|  |20|  |57|  |60|  |01|  |11|  |00|  |00|  |3f|  |d1|  |54|  |8a|  |b6|  |cf|  |c6|  |8d|  |3f|  |a9|  |e0|  |bd|  |3f|  |29|  |c8|  |f7|  |40|  |80|  |ae|  |2a|  |c9|  |7b|  |b7|  |11|  |c0|  |fd|  |d3|  |79|  |61|  |fb|  |23|  |99|  |c0|  |92|  |ca|  |3b|  |46|  |c7|  |05|  |15|  |3f|  |ff|  |9f|  |23|  |e0|  |00|  |00|  |00|  |be|  |44|  |16|  |1f|  |0d|  |84|  |d5|  |33|  |be|  |2c|  |8b|  |3b|  |bb|  |46|  |eb|  |85|  |3f|  |b5|  |ec|  |f0|  |c0|  |00|  |00|  |00|  |08|  |06|  |f0|  |d8|  |d4|  |07|  |0f|  |0d|  |13|  |01|  |0c|  |07|  |04|  |07|  |0d|  |08|  |09|  |0a|  |1a|  |1c|  |5c|  |03|";
//
//    for (int i = 0; i < bytesread; i++) {
//        char hex_byte1 = test_packet[i * 6 + 2];
//        char hex_byte2 = test_packet[i * 6 + 3];
//
//        if (hex_byte1 > 96) hex_byte1 -= 87; // 96-9
//        else hex_byte1 -= 48;
//        if (hex_byte2 > 96) hex_byte2 -= 87; // 96-9
//        else hex_byte2 -= 48;
//
//        Buffer[i] = hex_byte1 * 16 + hex_byte2;
//        printf("%x%x-%i  ", hex_byte1, hex_byte2, Buffer[i]);
//
//    }
//    cout << "\n";
//
//    printf("\nTotal number of bytes read: %i\n", bytesread);
//    cout << "-----------\n";
//    for (int i = 0; i < bytesread; i++) {
//        printf(" |%.2x| ", Buffer[buffer_index + i]);
//
//    }
//    cout << std::dec << "\n";
//
//    cout << "-----------\n";
//
//    packet_data incoming_packet;
//    Dgps temp_gps_dev = Dgps();
//    temp_gps_dev.interpretData(Buffer, bytesread, incoming_packet);
//
//
//    // need to check if values were ok, right now just hardcoded true..
//    success = true;
//    return success;
//}

#endif // NDEBUG