/* 
 * File:   main.h
 * Author: Mark Furneaux
 * Copyright: 2014 Romaco Canada, Mark Furneaux
 *
 * Created on December 25, 2014, 5:15 PM
 * 
 * This file is part of PSUControl.
 */

#ifndef MAIN_H
#define	MAIN_H

#ifdef	__cplusplus
extern "C" {
#endif

    void setInteractive();
    void setNonblocking();
    void writeVersion();
    void printHelp();

#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_H */

