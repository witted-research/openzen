
#define ZEN_API extern

%include "carrays.i"
// generate the helper class to convert float c arrays (which are treated as float *)
// to proper arrays
%array_class(float, OpenZenFloatArray);

%include "stdint.i"

%module OpenZen
%{
#include "../include/ZenTypes.h"
#include "../include/OpenZenCAPI.h"
%}
%include "../include/ZenTypes.h"
%include "../include/OpenZenCAPI.h"
