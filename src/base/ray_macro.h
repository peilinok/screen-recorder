#pragma once

#ifndef max
#define max(a,b) a > b ? a : b
#endif // !max

#ifndef min
#define min(a,b) a < b ? a : b
#endif // !min


// A macro to disallow the copy constructor and operator= functions 
// This should be used in the priavte:declarations for a class
#define    DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&) = delete;                \
    TypeName& operator=(const TypeName&) = delete


