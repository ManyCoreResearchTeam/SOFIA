#
# Copyright (c) 2005-2016 Imperas Software Ltd., www.imperas.com
#
# The contents of this file are provided under the Software License
# Agreement that you accepted before downloading this file.
#
# This source forms part of the Software and can be used for educational,
# training, and demonstration purposes but cannot be used for derivative
# works except in cases where the derivative works require OVP technology
# to run.
#
# For open source models released under licenses that you can use for
# derivative works, please visit www.OVPworld.org or www.imperas.com
# for the location of the open source models.
#

imodelnewsemihostlibrary \
    -name           cpuEstimator \
    -vendor         imperas.internal.com      \
    -library        intercept        \
    -version        1.0              \
    -imagefile      model            \

iadddocumentation -name Description \
    -text "Estimator Intercept Library. Implements interface and commands for Imperas Timing and Power Estimation Tools."

iadddocumentation -name Licensing \
    -text "Copyright (c) 2005-2016 Imperas Software Ltd. All Rights Reserved. Commercial License Required."

imodeladdsupportedprocessor -name arm \
    -processorvendor arm.ovpworld.org \
    -processorname arm \
    -processorlibrary processor \
    -processorversion 1.0

imodeladdsupportedprocessor -name armm \
    -processorvendor arm.ovpworld.org \
    -processorname armm \
    -processorlibrary processor \
    -processorversion 1.0

imodeladdsupportedprocessor -name v850 \
    -processorvendor renesas.ovpworld.org \
    -processorname v850 \
    -processorlibrary processor \
    -processorversion 1.0

imodeladdsupportedprocessor -name mips32 \
    -processorvendor imgtec.ovpworld.org \
    -processorname mips32 \
    -processorlibrary processor \
    -processorversion 1.0

#
# instTrace
#
imodeladdcommand -name instTrace -class "VMI_CT_MODE|VMI_CO_CPU|VMI_CA_CONTROL"
iadddocumentation -name Description -handle instTrace \
    -text "Enable cpu cycle counting based on instruction execution"

    imodeladdformalargument -name on -type flag -handle instTrace
        iadddocumentation -name Description -handle instTrace/on \
            -text "Turns on cpu cycle counting (default)"

    imodeladdformalargument -name stretch -type flag -handle instTrace
        iadddocumentation -name Description -handle instTrace/stretch \
            -text "Feed cpu cycle information back into simulator timing"

    imodeladdformalargument -name filename -type string -handle instTrace
        iadddocumentation -name Description -handle instTrace/filename \
            -text "Set filename to save estimated cpu cycle count"

#
# memorycycles
#
imodeladdcommand -name memorycycles -class "VMI_CT_MODE|VMI_CO_CPU|VMI_CA_CONTROL"
iadddocumentation -name Description -handle memorycycles \
    -text "Specify memory access time penalty for an address range (requires cpucycles command is turned on)"

    imodeladdformalargument -name low -type uns64 -handle memorycycles -mustbespecified
        iadddocumentation -name Description -handle memorycycles/low \
            -text "The low memory address for memory access penalty"

    imodeladdformalargument -name high -type uns64 -handle memorycycles -mustbespecified
        iadddocumentation -name Description -handle memorycycles/high \
            -text "The high memory address for memory access penalty"

    imodeladdformalargument -name penalty -type uns64 -handle memorycycles -mustbespecified
        iadddocumentation -name Description -handle memorycycles/penalty \
            -text "The additional cycles used by a load/store in this memory region"

#
# diagnostic
#
imodeladdcommand -name diagnostic  -class "VMI_CT_DEFAULT|VMI_CO_DIAG|VMI_CA_REPORT"
iadddocumentation -name Description -handle diagnostic \
    -text "Set how much additional information is reported for the library"

    imodeladdformalargument -name level -type integer -mustbespecified -handle diagnostic -class menu
        iadddocumentation -name Description -handle diagnostic/level \
            -text "Higher numbers print more diagnostic information
                                         0 = off (initial)
                                         1 = startup and shutdown
                                         2 = changes of major modes, infrequent commands
                                         3 = full noisy"
#
# Parameters
#
imodeladdformal -name stretchTimeGranularity -type uns32 -defaultValue 1 -min 1 -max 256
  iadddocumentation -name Description  \
      -text "Granularity of Time Stretch (instructions less than this number of cycles will not stretch simulator time)"
