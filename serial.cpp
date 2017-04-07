/* 
 * File:   serial.cpp
 * Author: Mark Furneaux
 * Copyright: 2014 Romaco Canada, Mark Furneaux
 *
 * Created on December 27, 2014, 10:05 PM
 * 
 * This file is part of PSUControl.
 */

#include <cstdlib>
#include <ncurses.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <string>
#include "serial.h"

/*
 * Sets the output of the PSU on or off. Note that the protocol is inverted.
 */
void setOutput(int fd, bool on) {
    std::string cmd = "SOUT";

    cmd.append((on) ? "0" : "1");
    cmd.append("\r");

    sendData(fd, cmd.c_str());
    readData(fd);
}

/*
 * Sets the desired voltage of the PSU.
 */
void setVoltage(int fd, double voltage) {
    std::string cmd = "VOLT";
    char* param = (char*) malloc(4 * sizeof (char));

    //do nothing for invalid voltage setting
    if (voltage > maxVoltage || voltage < 0) {
        return;
    }

    voltage *= 10;

    sprintf(param, "%03d", (int) voltage);

    cmd.append(param);
    cmd.append("\r");

    sendData(fd, cmd.c_str());
    readData(fd);
}

/*
 * Sets the desired current of the PSU.
 */
void setCurrent(int fd, double current) {
    std::string cmd = "CURR";
    char* param = (char*) malloc(4 * sizeof (char));

    //do nothing for invalid current setting
    if (current > maxCurrent || current < 0) {
        return;
    }

    current *= 10;

    sprintf(param, "%03d", (int) current);

    cmd.append(param);
    cmd.append("\r");

    sendData(fd, cmd.c_str());
    readData(fd);
}

/*
 * Prompts the user for the serial port location and retrieves the input.
 */
bool getSerialPort(char* portAddr) {
    printw("Enter the serial port to use [or press Enter for /dev/ttyUSB0]: ");
    if (scanw("%s", portAddr) == 1) {
        if (strlen(portAddr) != 0) {
            //TODO check if exists
            return true;
        }
    } else {
        strcpy(portAddr, "/dev/ttyUSB0");
        return true;
    }

    return false;
}

/*
 * Sets up the serial port for the PSU.
 * TODO none of the errors are visible because endwin() is called after
 */
int set_interface_attribs(int fd, speed_t speed, int parity) {
    struct termios config;
    if (!isatty(fd)) {
        printw("Error: Not a TTY!");
        refresh();
        return -1;
    }
    if (tcgetattr(fd, &config) < 0) {
        printw("Error: Could not get port attributes!");
        refresh();
        return -2;
    }

    //input flags - Turn off input processing
    //convert break to null byte,
    //no NL to CR translation, don't mark parity errors or breaks
    //no input parity check, don't strip high bit off,
    //no XON/XOFF software flow control
    config.c_iflag &= ~(IGNBRK | BRKINT |
            INLCR | PARMRK | INPCK | ISTRIP | IXON);
    //convert CR to NL as the PSU only sends CR
    config.c_iflag |= ICRNL;

    //output flags - Turn off output processing
    //no CR to NL translation, no NL to CR-NL translation,
    //no NL to CR translation, no column 0 CR suppression,
    //no Ctrl-D suppression, no fill characters, no case mapping,
    //no local output processing
    //
    // config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
    //                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
    config.c_oflag = 0;

    //no line processing:
    //echo off, echo newline off,
    //extended input processing off, signal chars off
    config.c_lflag &= ~(ECHO | ECHONL | IEXTEN | ISIG);
    //canonical mode on
    config.c_lflag |= ICANON;

    //turn off character processing
    //clear current char size mask, no parity checking,
    //no output processing, force 8 bit input
    config.c_cflag &= ~(CSIZE | PARENB | CLOCAL | CRTSCTS);
    config.c_cflag |= CS8;

    //no blocking on read
    //inter-character timer to 10s
    config.c_cc[VMIN] = 0;
    config.c_cc[VTIME] = 100;

    //set communication speed
    if (cfsetispeed(&config, speed) < 0 || cfsetospeed(&config, speed) < 0) {
        printw("Error: Could not set port speed!");
        refresh();
        return -3;
    }

    //apply the configuration
    if (tcsetattr(fd, TCSANOW, &config) < 0) {
        printw("Error: Could not set attributes!");
        refresh();
        return -4;
    }

    return 0;
}

/*
 * TODO ?????????????????????????????????????????????????????????
 */
bool isOK(const char* recvBuff) {
    std::string buf = recvBuff;

    if (buf.find("OK") == std::string::npos) {
        return false;
    }
    return true;
}

/*
 * Read a buffer (line) from the serial port.
 */
std::string readData(int fd) {
    int bytesRead = 0;
    char recvBuff[100]{};
    bytesRead = read(fd, recvBuff, sizeof recvBuff);
    std::string buf = recvBuff;

    //DEBUG to be removed
    //    move(LINES, 0);
    //    clrtoeol();
    //    printw("%d bytes read from SP", bytesRead);
    //    refresh();

    //    if (!isOK(recvBuff)) {
    //        move(14, 0);
    //        printw("ERROR: PSU returned a NOT OK on the last command!");
    //        move(15, 0);
    //        printw("Buffer returned was: %s", recvBuff);
    //        refresh();
    //    }

    //    if (buf.find("\rOK") != std::string::npos) {
    //        buf = buf.substr(0, buf.length() - 3);
    //    } else if (buf.find("OK") != std::string::npos) {
    //        buf = buf.substr(0, buf.length() - 2);
    //    }

    return buf;
}

/*
 * Get the current settings of the PSU.
 */
void getSettings(int fd) {
    sendData(fd, "GETS");

    std::string data = readData(fd);

    if (data.length() == 0) {
        return;
    }

    readData(fd);

    //    if (!isOK(readData(fd))) {
    //        move(14, 0);
    //        printw("ERROR: PSU returned a NOT OK on the last command!");
    //        move(15, 0);
    //        printw("Buffer returned was: %s", recvBuff);
    //        refresh();
    //    }

    //separate voltage and current values
    double voltage = std::stod(data.substr(0, 3), NULL);
    double current = std::stod(data.substr(3, 3), NULL);

    voltage /= 10;
    current /= 10;

    currVoltage = voltage;
    currCurrent = current;


    move((LINES / 2) - 2, (COLS / 2) - 23);
    clrtoeol();

    printw("Set Voltage:    ");
    attron(A_BOLD);
    printw("%2.1fV", voltage);
    attroff(A_BOLD);
    move((LINES / 2) - 2, (COLS / 2) + 2);
    clrtoeol();
    printw("Set Current:    ");
    attron(A_BOLD);
    printw("%2.1fA", current);
    attroff(A_BOLD);
    refresh();

}

/*
 * Get the current live values as read by the PSU.
 */
void getCurrent(int fd) {
    sendData(fd, "GETD");

    std::string data = readData(fd);

    if (data.length() == 0) {
        return;
    }

    readData(fd);

    //    if (!isOK(readData(fd))) {
    //        move(14, 0);
    //        printw("ERROR: PSU returned a NOT OK on the last command!");
    //        move(15, 0);
    //        printw("Buffer returned was: %s", recvBuff);
    //        refresh();
    //    }

    //separate the voltage, current, and mode
    double voltage = std::stod(data.substr(0, 4), NULL);
    double current = std::stod(data.substr(4, 4), NULL);
    bool mode = (bool)std::atoi(data.substr(8, 1).c_str());
    //get the output mode
    bool output = getOutputStatus(fd);

    voltage /= 100;
    current /= 100;

    move((LINES / 2) - 1, (COLS / 2) - 23);
    clrtoeol();
    printw("Actual Voltage: ");
    attron(A_BOLD);
    printw("%2.2fV", voltage);
    attroff(A_BOLD);
    move((LINES / 2) - 1, (COLS / 2) + 2);
    printw("Actual Current: ");
    attron(A_BOLD);
    printw("%2.2fA", current);
    attroff(A_BOLD);
    move((LINES / 2) + 1, (COLS / 2) - 23);
    clrtoeol();

    printw("Mode: ");
    attron(A_BOLD);
    if (mode) {
        printw("Constant Current");
    } else {
        printw("Constant Voltage");
    }
    attroff(A_BOLD);

    move((LINES / 2) + 1, (COLS / 2) + 2);
    printw("Output: ");

    //highlight if the output if off
    if (output) {
        attron(A_REVERSE | A_BOLD);
        printw("OFF");
        attroff(A_REVERSE | A_BOLD);
    } else {
        attron(A_BOLD);
        printw("ON");
        attroff(A_BOLD);
    }

    refresh();
}

/*
 * Send a string to the serial port.
 */
int sendData(int fd, const char* data) {
    std::string dataStr = data;
    dataStr.append("\r");

    return write(fd, dataStr.c_str(), dataStr.length());
}

/*
 * Get the state of the output, on or off.
 */
bool getOutputStatus(int fd) {
    sendData(fd, "GOUT");

    std::string data = readData(fd);
    readData(fd);

    outputOn = (bool)std::atoi(data.c_str());

    return outputOn;
}

/*
 * Toggle locking of the front panel knobs.
 * This is an undocumented command, so it is very flaky.
 */
void toggleRemote(int fd) {
    //can't combine these as it doesn't work...
    sendData(fd, "SESS\r");
    readData(fd);
    sendData(fd, "SESS\r");
    readData(fd);
}

/*
 * Get the max current and voltage of the PSU.
 */
void getDeviceCapabilities(int fd) {
    sendData(fd, "GMAX");

    std::string data = readData(fd);

    if (data.length() == 0) {
        return;
    }

    readData(fd);

    //    if (!isOK(readData(fd))) {
    //        move(14, 0);
    //        printw("ERROR: PSU returned a NOT OK on the last command!");
    //        move(15, 0);
    //        printw("Buffer returned was: %s", recvBuff);
    //        refresh();
    //    }

    //separate the voltage and current
    double voltage = std::stod(data.substr(0, 3), NULL);
    double current = std::stod(data.substr(3, 3), NULL);

    voltage /= 10;
    current /= 10;

    maxCurrent = current;
    maxVoltage = voltage;

    move(0, COLS - 38);
    clrtoeol();
    printw("Max Voltage: %2.1fV  Max Current: %2.1fA", maxVoltage, maxCurrent);
    refresh();
}
