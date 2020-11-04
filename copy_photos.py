import os
import sys
from shutil import copyfile

f = sys.argv[1]
direc = sys.argv[2]
num = int(sys.argv[3])

cdir = os.getcwd()
full_path = os.path.join(cdir, direc)
src_file = os.path.join(cdir, f)

if(not os.path.isdir(full_path)):
    os.mkdir(full_path)

for i in range(num):
    print(i)
    dst_name = os.path.join(full_path, "foto{}.jpg".format(i))
    copyfile(src_file, dst_name)


print("Done")
