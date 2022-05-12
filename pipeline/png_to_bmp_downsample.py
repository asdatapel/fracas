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

png_files = [f for f in os.listdir(dir) if os.path.isfile(os.path.join(dir, f)) and f.endswith(".png") and not f.endswith('down.png')]

for f in png_files:
    png_name = os.path.join(dir, f)
    out_name = png_name[:-4] + "-down.png"
    img = cv.imread(png_name, cv.IMREAD_UNCHANGED)

    small_img = cv.resize(img, # original image
                        (0,0), # set fx and fy, not the final size
                        fx=0.25, 
                        fy=0.25, 
                        interpolation=cv.INTER_NEAREST)
    cv.imwrite(out_name, small_img)