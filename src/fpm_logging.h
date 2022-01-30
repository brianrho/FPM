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
#define FPM_LOG_LEVEL_INFO                  2
#define FPM_LOG_LEVEL_VERBOSE               3
#define FPM_LOG_LEVEL_V_VERBOSE             4

/* Set this macro to any one of the log levels above,
 * arranged in order of increasing verbosity
 *
 * Note: setting FPM_LOG_LEVEL_VERBOSE may cause sensor data transfers to fail, especially at high baud rates
 */
#define FPM_LOG_LEVEL                       FPM_LOG_LEVEL_INFO

#if (FPM_LOG_LEVEL != FPM_LOG_LEVEL_SILENT)

    #define FPM_LOG(level, fmt, ...) \
                do { if (level <= FPM_LOG_LEVEL) printf(fmt, ##__VA_ARGS__); } while (0)

    #define FPM_LOGLN(level, fmt, ...) \
                do { if (level <= FPM_LOG_LEVEL) printf("[+]" fmt "\r\n", ##__VA_ARGS__); } while (0)
                
    #define FPM_LOG_ERROR(fmt, ...)             FPM_LOG(FPM_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
    #define FPM_LOG_INFO(fmt, ...)              FPM_LOG(FPM_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
    #define FPM_LOG_VERBOSE(fmt, ...)           FPM_LOG(FPM_LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
    #define FPM_LOG_V_VERBOSE(fmt, ...)         FPM_LOG(FPM_LOG_LEVEL_V_VERBOSE, fmt, ##__VA_ARGS__)

    #define FPM_LOGLN_ERROR(fmt, ...)           FPM_LOGLN(FPM_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
    #define FPM_LOGLN_INFO(fmt, ...)            FPM_LOGLN(FPM_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
    #define FPM_LOGLN_VERBOSE(fmt, ...)         FPM_LOGLN(FPM_LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)
    #define FPM_LOGLN_V_VERBOSE(fmt, ...)       FPM_LOGLN(FPM_LOG_LEVEL_V_VERBOSE, fmt, ##__VA_ARGS__)

    #if defined(ARDUINO_ARCH_AVR)
    
    #include <Arduino.h>
                                             
    static int uart_putchar(char c, FILE *stream)
    {
        Serial.write(c);
        return 0;
    }
    
    #endif    
    
    void printf_begin(void)
    {
    #if defined(ARDUINO_ARCH_AVR)
        fdevopen(&uart_putchar, NULL);
    #endif
    }                                 

#else

    #define FPM_LOG_ERROR(fmt, ...)
    #define FPM_LOG_INFO(fmt, ...)
    #define FPM_LOG_VERBOSE(fmt, ...)
    #define FPM_LOG_V_VERBOSE(fmt, ...)

    #define FPM_LOGLN_ERROR(fmt, ...)
    #define FPM_LOGLN_INFO(fmt, ...)
    #define FPM_LOGLN_VERBOSE(fmt, ...)
    #define FPM_LOGLN_V_VERBOSE(fmt, ...)

#endif  /* (FPM_LOG_LEVEL != FPM_LOG_LEVEL_SILENT) */

#endif
