# import os
# import sys

# from PIL import Image

# if len(sys.argv) != 2:
#     print("Wrong number of args")
#     exit()

# dir = sys.argv[1]

# png_files = [f for f in os.listdir(dir) if os.path.isfile(os.path.join(dir, f)) and f.endswith(".png")]

# for f in png_files:
#     png_name = os.path.join(dir, f)
#     bmp_name = png_name[:-4] + ".bmp"
#     Image.open(png_name).save(bmp_name)

import os
import sys

import cv2 as cv


if len(sys.argv) != 2:
    print("Wrong number of args")
    exit()
dir = sys.argv[1]

png_files = [f for f in os.listdir(dir) if os.path.isfile(os.path.join(dir, f)) and f.endswith(".png")]

for f in png_files:
    png_name = os.path.join(dir, f)
    bmp_name = png_name[:-4] + ".bmp"
    img = cv.imread(png_name, cv.IMREAD_UNCHANGED)
    cv.imwrite(bmp_name, img)