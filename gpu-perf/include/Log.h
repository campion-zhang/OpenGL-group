/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    log.h
*  @brif:    categories display debugging information 
*
*  @date:    05/10/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#ifndef LOG_H
#define LOG_H
#include <string>
#include <iostream>

class Log
{
public:
    /**
     * @brif: Emit an informational message
     */
    static void info(const char *fmt, ...);

    /**
     * @brif: Emit a debugging message
     */
    static void debug(const char *fmt, ...);

    /**
     * @brif: Emit an error message
     */
    static void error(const char *fmt, ...);

    /**
     * @brif: A prefix constant that informs the logging infrastructure that the log
     * message is a continuation of a previous log message to be put on the same line.
     */
    static const std::string continuation_prefix;
};

#endif /* LOG_H_ */
