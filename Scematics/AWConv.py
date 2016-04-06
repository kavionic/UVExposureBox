#! python
import sys
import os

os.chdir("Artwork")
print "Convert Top"
#os.system("pcbconvert.py Top.bmp Top_mask.bmp Top_silk.bmp")
print "Convert Bottom"
#os.system("pcbconvert.py Bottom.bmp Bottom_mask.bmp Bottom_silk.bmp")

print "Tile Signal"
os.system("pcbtiler.py -A4 -x 1 -y 2 -m Top.tiff Bottom.tiff -o Signal.tiff")

print "Tile Mask"
os.system("pcbtiler.py -A4 -x 1 -y 2 -m Top_mask.tiff Bottom_mask.tiff -o Mask.tiff")

print "Tile Silk"
os.system("pcbtiler.py -A4 -x 1 -y 2 -m Top_silk.tiff Bottom_silk.tiff -o Silk.tiff") 
