/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
/*  Galileo support from spaniakos <spaniakos@gmail.com> */

/**
 * @file fpm_logging.h
 *
 * - Setup necessary to direct stdout to the Arduino Serial library, which
 *   enables 'printf'. 
 * - Select logging level.
 */

#ifndef FPM_LOGGING_H_
#define FPM_LOGGING_H_


#define FPM_LOG_LEVEL_SILENT                0
#define FPM_LOG_LEVEL_ERROR                 1
#define FPM_LOG_LEVEL_VERBOSE               2

/* Set this macro to any one of the log levels above,
 * arranged in order of increasing verbosity
 *
 * Note: setting FPM_LOG_LEVEL_VERBOSE may cause sensor data transfers to fail, especially at high baud rates
 */
#define FPM_LOG_LEVEL                       FPM_LOG_LEVEL_ERROR

#if (FPM_LOG_LEVEL != FPM_LOG_LEVEL_SILENT)

    #define FPM_LOG(level, fmt, ...) \
                do { if (level <= FPM_LOG_LEVEL) printf(fmt, ##__VA_ARGS__); } while (0)

    #define FPM_LOGLN(level, fmt, ...) \
                do { if (level <= FPM_LOG_LEVEL) printf(fmt"\r\n", ##__VA_ARGS__); } while (0)
                
    #define FPM_LOG_ERROR(fmt, ...)             FPM_LOG(FPM_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
    #define FPM_LOG_VERBOSE(fmt, ...)           FPM_LOG(FPM_LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)

    #define FPM_LOGLN_ERROR(fmt, ...)           FPM_LOGLN(FPM_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
    #define FPM_LOGLN_VERBOSE(fmt, ...)         FPM_LOGLN(FPM_LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)

    #if defined(ARDUINO_ARCH_AVR) || defined(__ARDUINO_X86__)

    int serial_putc(char c, FILE *)
    {
        Serial.write(c);
        return c;
    }
    #endif

    void printf_begin(void)
    {
        #if defined(ARDUINO_ARCH_AVR)
        fdevopen(&serial_putc, 0);

        #elif defined(__ARDUINO_X86__)
        // JESUS - For reddirect stdout to /dev/ttyGS0 (Serial Monitor port)
        stdout = freopen("/dev/ttyGS0", "w", stdout);
        delay(500);
        printf("Redirecting to Serial...");
        #endif // defined(__ARDUINO_X86__)
    }

#else

    #define FPM_LOG_ERROR(fmt, ...)
    #define FPM_LOG_VERBOSE(fmt, ...)

    #define FPM_LOGLN_ERROR(fmt, ...)
    #define FPM_LOGLN_VERBOSE(fmt, ...)

#endif  /* (FPM_LOG_LEVEL != FPM_LOG_LEVEL_SILENT) */

#endif // __PRINTF_H__
