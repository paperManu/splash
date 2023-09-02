Datapath driver - Header files - Version 7.21.0.60886
=====================================================

This directory contains the header files for the Datapath drivers. These headers files are necessary to compile support for Datapath capture card into any Linux software. They have been extracted from the Datapath drivers that can be found [on their website](https://www.datapath.co.uk/datapath-current-downloads/vision-capture-card-downloads/vision-drivers/vision-drivers-1).

Part of these headers include the Video4Linux2 header files, which is GPL. The licence for their whole driver package can be found in the `LICENCE` file in this very folder.

To justify a bit more that it is OK to include these headers into Splash repository, here is a mail exchange between me and Datapath support in this regard:

```
Hello,

We have been developing for a few years now an open source projection-mapping software for Linux, which supports (among other video input) Datapath capture cards. For reference, here is its website: https://splashmapper.xyz.

We use the Datapath SDK, and for now support is only activated whenever the user compiles the software him/herself. We would like to include it be default in the various pre-built packages (namely for Flatpak, and as a Ubuntu package), but for this we must include part of your SDK with the package sources.

I read the documentation but it is always hard to know exactly which part is covered by which license. As the SDK relies on Video4Linux, which is GPL, at least part of it should be GPL. Could you confirm that it is OK to include the following source files inside our code repository:

* rgb133control.h
* rgb133v4l2.h

Regards
```

And their answer:

```
Hi Emmanuel, 
 
Our development team says that you are spot on using them in the manner they were intended. We can’t see anything wrong with these being made available under GPL, as they are intended for use with user space source code and not our proprietary Linux driver source code.
Thank you!
 
All the best,
Peter Pritz
Datapath Support
```
