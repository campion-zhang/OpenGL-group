/******************************************************************************
*  Copyright(c) 2021-2023 Sietium
*
*  @file:    log.cpp
*  @brif:    categories display debugging information
*
*  @date:    05/10/2021
*  @Author:  cary.Zhang
*
******************************************************************************/
#include <cstdio>
#include <cstdarg>
#include <string>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include "Log.h"

using std::string;

const string Log::continuation_prefix("\x10");

static const string terminal_color_normal("\033[0m");
static const string terminal_color_red("\033[1;31m");
static const string terminal_color_cyan("\033[36m");
static const string terminal_color_yellow("\033[33m");

static void
print_prefixed_message(std::ostream &stream, const string &color, const string &prefix,
                       const string &fmt, va_list ap)
{
    va_list aq;

    /* Estimate message size */
    va_copy(aq, ap);
    int msg_size = vsnprintf(NULL, 0, fmt.c_str(), aq);
    va_end(aq);

    /* Create the buffer to hold the message */
    char *buf = new char[msg_size + 1];

    /* Store the message in the buffer */
    va_copy(aq, ap);
    vsnprintf(buf, msg_size + 1, fmt.c_str(), aq);
    va_end(aq);

    /*
     * Print the message lines prefixed with the supplied prefix.
     * If the target stream is a terminal make the prefix colored.
     */
    string linePrefix;
    if (!prefix.empty())
    {
        static const string colon(": ");
        string start_color;
        string end_color;
        if (!color.empty())
        {
            start_color = color;
            end_color = terminal_color_normal;
        }
        linePrefix = start_color + prefix + end_color + colon;
    }

    std::string line;
    std::stringstream ss(buf);

    while(std::getline(ss, line))
    {
        /*
         * If this line is a continuation of a previous log message
         * just print the line plainly.
         */
        if (line[0] == Log::continuation_prefix[0])
        {
            stream << line.c_str() + 1;
        }
        else
        {
            /* Normal line, emit the prefix. */
            stream << linePrefix << line;
        }

        /* Only emit a newline if the original message has it. */
        if (!(ss.rdstate() & std::stringstream::eofbit))
        {
            stream << std::endl;
        }
    }

    delete[] buf;
}


void Log::info(const char *fmt, ...)
{
    static const string infoprefix("Info");
    const string &prefix(infoprefix);
    va_list ap;
    va_start(ap, fmt);

    static const string &infocolor(terminal_color_cyan);
    const string &color(infocolor);
    print_prefixed_message(std::cout, color, prefix, fmt, ap);

    va_end(ap);
}

void Log::debug(const char *fmt, ...)
{
    static const string dbgprefix("Debug");
    va_list ap;
    va_start(ap, fmt);

    static const string &dbgcolor(terminal_color_yellow);
    print_prefixed_message(std::cout, dbgcolor, dbgprefix, fmt, ap);

    va_end(ap);
}

void Log::error(const char *fmt, ...)
{
    static const string errprefix("Error");
    va_list ap;
    va_start(ap, fmt);

    static const string &errcolor(terminal_color_red);
    print_prefixed_message(std::cerr, errcolor, errprefix, fmt, ap);

    va_end(ap);
}
