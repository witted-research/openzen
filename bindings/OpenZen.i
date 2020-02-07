%include "carrays.i"
// generate the helper class to convert float c arrays (which are treated as float *)
// to proper arrays

// run
// swigwin-4.0.0\swig.exe -csharp -outdir OpenZenCSharp OpenZen.i
// to export for Csharp
%array_class(float, OpenZenFloatArray);

%include "stdint.i"

%module OpenZen
%{
#include "../include/ZenTypes.h"
#include "../include/OpenZenCAPI.h"
%}
%include "../include/ZenTypes.h"
%include "../include/OpenZenCAPI.h"
