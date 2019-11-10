import os
import sys

if __name__ == '__main__':
    if os.geteuid() != 0:
        print >> sys.stderr, 'Please run in root.'
        sys.exit(1)
    else:
    	cmd = './bin/main ' +" ".join(sys.argv[1:])
    	os.system(cmd)
