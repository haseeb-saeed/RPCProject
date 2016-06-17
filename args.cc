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

int arrayLen(int arg_type) {
    return arg_type & 0xFFFF;    
}

bool isEnd(int arg_type) {
    return arg_type == 0;    
}

}
