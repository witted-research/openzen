%include "carrays.i"
// generate the helper class to convert float c arrays (which are treated as float *)
// to proper arrays

// run
// swig.exe -csharp -small -c++ -debug-typedef -DSWIGWORDSIZE64 -o OpenZenCSharp/OpenZen_wrap_csharp.cxx -outdir OpenZenCSharp OpenZen.i
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
