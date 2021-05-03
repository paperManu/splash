![Icon](./images/icon.png)

[![GPLv3 license](https://img.shields.io/badge/License-GPLv3-blue.svg)](http://perso.crans.org/besson/LICENSE.html)
[![pipeline status](https://gitlab.com/sat-metalab/splash/badges/master/pipeline.svg)](https://gitlab.com/sat-metalab/splash/commits/develop)
[![coverage report](https://gitlab.com/sat-metalab/splash/badges/develop/coverage.svg)](https://gitlab.com/sat-metalab/splash/commits/develop)


## Introduction

**TL;DR**: go to [Installation](./Installation) to setup Splash, and then to [Walkthrough](./Walkthrough) to have an overview of how to use it.

## About
Splash is a free (as in GPL) modular mapping software. Provided that the user creates a 3D model with UV mapping of the projection surface, Splash will take care of calibrating the videoprojectors (intrinsic and extrinsic parameters, blending and color), and feed them with the input video sources. Splash can handle multiple inputs, mapped on multiple 3D models, and has been tested with up to eight outputs on two graphic cards. It currently runs on a single computer but support for multiple computers is planned.

![Splash in a 20 meters fulldome](./images/splash_sato.jpg)

Although Splash was primarily targeted toward fulldome mapping and has been extensively tested in this context, it can be used for virtually any surface provided that a 3D model of the geometry is available. Multiple fulldomes have been mapped, either by the authors of this software (two small dome (3m wide) with 4 projectors, a big one (20m wide) with 8 projectors) or by other teams. It has also been tested sucessfully as a more regular video-mapping software to project on buildings, or [onto moving objects](https://vimeo.com/268028595).

Regarding performances, our tests show that Splash can handle flawlessly a 3072x3072@60Hz live video input, or a 4096x4096@60Hz video while outputting to eight outputs (through two graphic cards) with a high end cpu and the [HapQ](http://vdmx.vidvox.net/blog/hap) video codec (on a SSD as this codec needs a very high bandwidth). Due to its architecture, higher resolutions are more likely to run smoothly when a single graphic card is used, although nothing higher than 4096x4096@60Hz has been tested yet (well, we tested 6144x6144@60Hz but the drive throughput was not sufficient to sustain the video bitrate).

Splash can read videos from various sources amoung which video files (most common format and Hap variations), video input (such as video cameras and capture cards), and Shmdata (a shared memory library used to make softwares from the SAT Metalab communicate between each others). An addon for Blender is included which allows for exporting draft configurations and update in real-time the meshes. Splash also handles automatically a few things:
- semi automatic geometric calibration of the video-projectors,
- projection warping,
- automatic calibration of the blending between projections,
- experimental automatic colorimetric calibration (with a gPhoto compatible camera)

## Community
Don't hesitate to report any issues with the [Gitlab issue tracker](https://gitlab.com/sat-metalab/splash/issues), and share your thoughts and success on IRC, [channel ##splash on Freenode](http://webchat.freenode.net/?randomnick=1&channels=%23%23splash&uio=d4)!

## Projet URL
This project can be found either on [its official website](https://sat-metalab.gitlab.io/splash), on the [SAT Metalab repository](https://gitlab.com/sat-metalab/splash) or on [Github](https://github.com/paperManu/splash).

### Licenses
This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

This program uses external libraries, some of them being bundled in the source code repository (directly or as submodules). They are located in `external`, and are not necessarily licensed under GPLv3. Please refer to the respective licenses for details.

## Sponsors
This project is made possible thanks to the [Society for Arts and Technologies](http://www.sat.qc.ca) (also known as SAT).
