#ifndef __ARGS_H__
#define __ARGS_H__

#include <string>

namespace args {

// Determines the type of the argument
bool isChar(int arg_type);
bool isShort(int arg_type);
bool isInt(int arg_type);
bool isLong(int arg_type);
bool isDouble(int arg_type);
bool isFloat(int arg_type);

// Determines whether the arg is input or output
bool isInput(int arg_type);
bool isOutput(int arg_type);

// Miscellaneous
int arrayLen(int arg_type);
int numArgs(int* arg_types);
int argSize(int arg_type);
std::string getSignature(const char* name, int* arg_types);
void copyArgTypes(int* dest, int* src);
void copyArgs(void** dest, void** src, int* arg_types);

}
#endif // __ARGS_H__
