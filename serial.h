/* 
 * File:   serial.h
 * Author: Mark Furneaux
 * Copyright: 2014 Romaco Canada, Mark Furneaux
 *
 * Created on December 27, 2014, 10:06 PM
 * 
 * This file is part of PSUControl.
 */

#ifndef SERIAL_H
#define	SERIAL_H

#ifdef	__cplusplus
extern "C" {
#endif

    extern double maxVoltage;
    extern double maxCurrent;
    extern bool outputOn;
    extern double currVoltage;
    extern double currCurrent;
    extern double currentDivider;

    bool getSerialPort(char* portAddr);
    int set_interface_attribs(int fd, speed_t speed, int parity);
    bool isOK(const char* recvBuff);
    std::string readData(int fd);
    void getSettings(int fd);
    int sendData(int fd, const char* data);
    void getCurrent(int fd);
    bool getOutputStatus(int fd);
    void setVoltage(int fd, double voltage);
    void setCurrent(int fd, double current);
    void toggleRemote(int fd);
    void setOutput(int fd, bool on);
    void getDeviceCapabilities(int fd);

#ifdef	__cplusplus
}
#endif

#endif	/* SERIAL_H */

