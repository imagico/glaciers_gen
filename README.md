
glaciers_gen glaciers generalization tool
=========================================

This is a variant of the [coastline_gen](https://github.com/imagico/coastline_gen)
tool for generalizing glacier polygons.  It is able to use coastline or other 
supplemental mask files to maintain connectivity or separation of the glaciers
with other features during generalization.

Warning
-------

Much of this program is highly experimental and not very robust.  It will 
crash or produce nonsense results when called with the wrong options.  Best 
to consider it just a demo for the moment.

Compiling the program
---------------------

glaciers_gen requires the [CImg](http://cimg.sourceforge.net/) image 
processing library to be built and should be compilable on all platforms 
supported by the library.  You need to download this library separately.  
Copying CImg.h to the source directory is sufficient.

Program options
---------------

There are two versions of the program - glaciers_gen and glaciers_gen2, the 
latter is using a digital elevation file to take into account the relief shape.

Both versions have the following command line options:

* `-i` glaciers mask image file (required)
* `-o` Output image file name for the generalized glaciers mask (required)
* `-f` Input image file containing a mask to restrict generalization.  Has to be the same size as main input (optional)

* `-sf` specifies how to interpret the fixed mask: 1: mask repels generalized features; -1: mask attracts generalized features.
* `-rf` influence radius of the fixed mask in pixels
* `-l` allows to set a bias in the outline position.  Larger values move the coastline to the land.  Default: `0.5`

* `-rs` selection radius.  Default: `4.0`
* `-rfs` fixed mask search radius.  Default: `5`
* `-rf` fixed mask buffer radius.  Default: `2.0`

* `-ro` Radius values for opening steps `erode:dilate:dilate:erode`. Default: `2.2:2.2:1.2:1.2`.
* `-rc` Radius values for closing steps `dilate:erode:erode:dilate`. Default: `2.2:2.2:1.2:1.2`.
* `-debug` Generate a large number of image files from intermediate steps in the current directory for debugging.  Default: off
* `-h` show available options

All image files are expected to be byte valued grayscale images with glacier pixel values > 0 and non-glacier pixel value 0.  You can 
use any file format supported by CImg.  When using GeoTiff files you will get some warnings that can be safely ignored.  Note though that
coordinate system information in any file is not transferred to the output.  If necessary you have to do that yourself.

The program operates purely on the raster image and is unaware of the geographic coordinate system the data is supplied in.  All parameters are specified 
in pixels as basic units.

glaciers_gen2 has additional parameters related to use of relief data.  Elevation data is expected in the same grid as the source data.

Using the program
-----------------

Like `coastline_gen` `glaciers_gen` operates on raster images and you will normally have to use the following steps:

1. Rasterizing the glacier polygons, for example using [gdal_rasterize](http://www.gdal.org/gdal_rasterize.html).
2. Running `glaciers_gen`.
3. Vectorizing the resulting image, for example using [potrace](http://potrace.sourceforge.net/).


Legal stuff
-----------

This program is licensed under the GNU GPL version 3.

Copyright 2013-2016 Christoph Hormann

