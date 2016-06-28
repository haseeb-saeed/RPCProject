#include <string>

#include "args.h"
#include "rpc.h"
using namespace std;

namespace args {

static bool isType(int arg_type, int type) {
    return (arg_type & 0xFF0000) == (type << 16);
}

bool isChar(int arg_type) {
    return isType(arg_type, ARG_CHAR);
}

bool isShort(int arg_type) {
    return isType(arg_type, ARG_SHORT);
}

bool isInt(int arg_type) {
    return isType(arg_type, ARG_INT);
}

bool isLong(int arg_type) {
    return isType(arg_type, ARG_LONG);
}

bool isDouble(int arg_type) {
    return isType(arg_type, ARG_DOUBLE);
}

bool isFloat(int arg_type) {
    return isType(arg_type, ARG_FLOAT);
}

bool isInput(int arg_type) {
    return arg_type & (1 << ARG_INPUT);
}

bool isOutput(int arg_type) {
    return arg_type & (1 << ARG_OUTPUT);
}

int arrayLen(int arg_type) {
    return arg_type & 0xFFFF;
}

int argSize(int arg_type) {
    int num = max(arrayLen(arg_type), 1);

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

string getSignature(const char* name, int* arg_types) {
    string signature(name);

    for (int i = 0; arg_types[i] != 0; ++i) {
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

void copyArgTypes(int* dest, int* src) {
    // TODO: Copy arg types into dest
}

void copyArgs(void** dest, void** src, int* arg_types) {
    // TODO: Copy args into dest
}

}
