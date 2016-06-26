#include <string>

#include "args.h"
#include "rpc.h"
using namespace std;

namespace args {

bool isChar(int arg_type) {
    int mask = ARG_CHAR << 16;
    return (arg_type & mask) == mask;
}

bool isShort(int arg_type) {
    int mask = ARG_SHORT << 16;
    return (arg_type & mask) == mask;
}

bool isInt(int arg_type) {
    int mask = ARG_INT << 16;
    return (arg_type & mask) == mask;
}

bool isLong(int arg_type) {
    int mask = ARG_LONG << 16;
    return (arg_type & mask) == mask;
}

bool isDouble(int arg_type) {
    int mask = ARG_DOUBLE << 16;
    return (arg_type & mask) == mask;
}

bool isFloat(int arg_type) {
    int mask = ARG_FLOAT << 16;
    return (arg_type & mask) == mask;
}

bool isInput(int arg_type) {
    return arg_type & (1 << ARG_INPUT);
}

bool isOutput(int arg_type) {
    return arg_type & (1 << ARG_OUTPUT);
}

unsigned arrayLen(int arg_type) {
    return arg_type & 0xFFFF;    
}

int argSize(int arg_type) {
    int num = arrayLen(arg_type);
    num = max(num, 1);

    if (isChar(arg_type)) {
        return num * sizeof(char);
    } else if (isShort(arg_type)) {
        return num * sizeof(short);
    } else if (isInt(arg_type)) {
        return num * sizeof(int);
    } else if (isLong(arg_type)) {
        return num * sizeof(long);
    } else if (isFloat(arg_type)) {
        return num * sizeof(float);
    } else if (isDouble(arg_type)) {
        return num* sizeof(double);
    } else {
        // We should never hit this
        return -1;
    }
}

int numArgs(int* arg_types) {
    int count = 0;
    for (; arg_types[count] != 0; ++count);
    return count;
}

bool isEnd(int arg_type) {
    return arg_type == 0;    
}

string getSignature(const char* name, int* arg_types) {
    string signature(name);

    for (int i = 0; !isEnd(arg_types[i]); ++i) {
        int temp = arg_types[i];

        // Since array length is not part of the signature
        // set all arrays to have the same length
        if (arrayLen(temp) > 0) {
            temp |= 0xFFFF;
        }

        // Add the bytes of the arg to the signature
        signature += string((char*)&temp, sizeof(temp));
    }

    return signature;
}

}
