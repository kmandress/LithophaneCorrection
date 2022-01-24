# Lithophane Correction
Correction for the exponential attenuation of light through a medium.

The model for attentuation is contained in the excel spreadsheet ExcelModel/transferFunction.xlsx.  It is based on an experiment in which I milled successively thinner sections (0.01 each time) and measured the light transmitted.  Note, the absolute values of the transmitted light is not important, but relative values are key.  I then fit an exponential curve to this data and produced the correction by inverting it.

Currrently, hard coded values for "Candlestone" lithophane material are currently used (both in the excel file and the C++ program), but constants are used to allow easy change for other materials.

The program also inverts the greyscales in prepation of using the dmap2gcode program from scorchworks.com.

This program reads an image file, applies the correction via a LUT and then outputs the resulting file.  Any 8-bit grey-scale image file types supported by opencv can be used.

usage: LithophaneCrrection infile.jpg outfile.jpg

# Dependencies

Install the following repositories at the same level as Lithophane Correction:
* opencv (opencv.org)
