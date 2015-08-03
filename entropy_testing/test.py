#!/usr/bin/env python


import numpy
import sys
input = open(sys.argv[1],"r")

data = numpy.fromfile(input, dtype=numpy.int16)


print "Min: %f" %(numpy.amin(data))
print "Max: %f" %(numpy.amax(data))

print "Med: %f" %(numpy.median(data))
print "Avg: %f" %(numpy.average(data))
print "Mea: %f" %(numpy.mean(data))

print "StD: %f" %(numpy.std(data))


input.close()
