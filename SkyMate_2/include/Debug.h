#ifndef DEBUG_H
#define DEBUG_H

#ifndef DEBUG_STREAM
#define DEBUG_STREAM Serial
#endif // DEBUG_STREAM

#ifdef DEBUG
// need to do some debugging...
#define DEBUG_PRINT(...)		DEBUG_STREAM.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...)		DEBUG_STREAM.println(__VA_ARGS__)
#endif

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif

#endif // DEBUG_H

