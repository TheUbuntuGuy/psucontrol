/* 
 * File:   main.cpp
 * Author: Mark Furneaux
 * Copyright: 2014 Romaco Canada, Mark Furneaux
 *
 * Created on December 25, 2014, 5:08 PM
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
#include "main.h"
#include "serial.h"


using namespace std;

bool outputOn = false;
double maxVoltage = 0;
double maxCurrent = 0;
double currVoltage = 0;
double currCurrent = 0;

int main(int argc, char** argv) {

    int ch;
    char* portAddr = (char*) malloc(255 * sizeof (char));
    double userFloatInput = 0;

    //start curses
    initscr();
    //disable line buffering
    cbreak();
    //capture function/arrow keys
    keypad(stdscr, TRUE);
    //wait for the user to select a serial port
    setInteractive();

    if (!getSerialPort(portAddr)) {
        endwin();
        printf("Error: Invalid Serial Port\n");
        exit(-1);
    } else {
        erase();
        move(0, 0);
        printw("Device is at: %s", portAddr);
        refresh();
    }

    //nothing to wait on anymore
    setNonblocking();

    //open the port
    int fd = open(portAddr, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        endwin();
        printf("Error: Could not open serial port\n");
        return -2;
    }

    //set port to 9600 baud, 8N1
    if (set_interface_attribs(fd, B9600, 0) < 0) {
        endwin();
        printf("Error: Could not set port attributes\n");
        return -3;
    }

    //lock the front controls
    //FIXME: the device unlocks automatically after so many seconds of inactivity,
    //so when the user inputs a new setting, the lock is defeated
    //toggleRemote(fd);

    //set control limits
    getDeviceCapabilities(fd);

    //write version info
    writeVersion();

    //print command reference
    printHelp();

    do {
        //get all the live parameters
        getSettings(fd);
        getCurrent(fd);

        //0.5 seconds
        usleep(500000);

        //get any input, if available
        ch = getch();

        //set voltage mode
        if (ch == 'v') {
            //prompt and wait
            move((LINES / 2) + 7, (COLS / 2) - 10);
            setInteractive();
            printw("Set New Voltage: ");
            refresh();

            //get the new voltage
            if (scanw("%lf", &userFloatInput) == 1) {
                setVoltage(fd, userFloatInput);
            }

            //remove the prompt and unblock all input
            move((LINES / 2) + 7, (COLS / 2) - 10);
            clrtoeol();
            setNonblocking();
        } else if (ch == 'c') {
            //prompt and wait
            move((LINES / 2) + 7, (COLS / 2) - 10);
            setInteractive();
            printw("Set New Current: ");
            refresh();

            //get the new current
            if (scanw("%lf", &userFloatInput) == 1) {
                setCurrent(fd, userFloatInput);
            }

            //remove the prompt and unblock all input
            move((LINES / 2) + 7, (COLS / 2) - 10);
            clrtoeol();
            setNonblocking();
        } else if (ch == 'o') {
            //toggle output
            outputOn = (outputOn) ? true : false;
            setOutput(fd, outputOn);
        } else if (ch == KEY_UP) {
            //increase voltage by one step
            currVoltage += 0.1;
            setVoltage(fd, currVoltage);
        } else if (ch == KEY_DOWN) {
            //decrease voltage by one step
            currVoltage -= 0.1;
            setVoltage(fd, currVoltage);
        }
        //exit on q pressed
    } while (ch != 'q');

    //DEBUG to be removed
    //    move(21, 0);
    //    printw("Exiting...");
    //
    //    //sleep(40);
    //    timeout(-1);
    //    move(22, 0);
    //    printw("Press any key to exit...");
    //    getch();

    //close the port
    close(fd);
    //exit curses
    endwin();

    return 0;
}

/*
 * Shows the cursor, echos all input, and blocks on input indefinitely.
 */
void setInteractive() {
    curs_set(1);
    echo();
    timeout(-1);
}

/*
 * Hides the cursor, hides all input, and times out on input in 500ms.
 */
void setNonblocking() {
    curs_set(0);
    noecho();
    timeout(500);
}

void writeVersion() {
    move(LINES - 1, COLS - 59);
    printw("Version 0.0.1   Copyright 2014 Romaco Canada, Mark Furneaux");
    refresh();
}

void printHelp() {
    move(LINES - 6, 0);
    printw("q - quit");
    move(LINES - 5, 0);
    printw("v - set voltage");
    move(LINES - 4, 0);
    printw("c - set current");
    move(LINES - 3, 0);
    printw("UP - increase voltage");
    move(LINES - 2, 0);
    printw("DOWN - decrease voltage");
    move(LINES - 1, 0);
    printw("o - toggle output on/off");

    refresh();
}